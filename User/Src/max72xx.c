/*
 * max72xx.c
 *
 *  Created on: Dec 13, 2020
 *      Author: lindi
 */

#include "max72xx.h"
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "utils.h"

#define CS_SET()  HAL_GPIO_WritePin(CS_MAX7219_GPIO_Port, CS_MAX7219_Pin, GPIO_PIN_RESET)
#define CS_RESET()  HAL_GPIO_WritePin(CS_MAX7219_GPIO_Port, CS_MAX7219_Pin, GPIO_PIN_SET)

extern SPI_HandleTypeDef hspi1;

typedef enum {
  REG_DECODE_MODE = 0x09,
  REG_INTENSITY = 0x0A,
  REG_SCAN_LIMIT = 0x0B,
  REG_SHUTDOWN = 0x0C,
  REG_DISPLAY_TEST = 0x0F,
} MAX7219_REGISTERS;

typedef struct {
  uint8_t dig[ROW_SIZE];  // data for each digit of the MAX72xx (DIG0-DIG7)
  uint8_t changed;        // one bit for each digit changed ('dirty bit')
} deviceInfo_t;

// Control data for the library
static bool _updateEnabled = true; // update the display when this is true, suspend otherwise
static bool _wrapAround = true;    // when shifting, wrap left to right and vice versa (circular buffer)

static deviceInfo_t *_matrix = NULL; // the current status of the LED matrix (buffers)
static uint8_t *_spiData = NULL;   // data buffer for writing to SPI interface

void MAX72XX_Init() {
  _matrix = (deviceInfo_t*) malloc(sizeof(deviceInfo_t) * MAX_DEVICES);
  _spiData = (uint8_t*) malloc(SPI_DATA_SIZE);

  MAX72XX_ControlAll(TEST, OFF);                   // no test
  MAX72XX_ControlAll(SCANLIMIT, ROW_SIZE - 1);       // scan limit is set to max on startup
  MAX72XX_ControlAll(INTENSITY, MAX_INTENSITY / 2);  // set intensity to a reasonable value
  MAX72XX_ControlAll(DECODE, OFF);                 // ensure no decoding (warm boot potential issue)
  MAX72XX_ClearAll();
  MAX72XX_ControlAll(SHUTDOWN, OFF);               // take the modules out of shutdown mode
}

void MAX72XX_DeInit() {
  free(_matrix);
  free(_spiData);
}

uint8_t MAX72XX_GetDeviceCount(void) {
  return (MAX_DEVICES);
}

uint8_t MAX72XX_GetColumnCount(void) {
  return (MAX_DEVICES * COL_SIZE);
}

static void MAX72XX_SpiSend() {
  CS_SET();
  HAL_SPI_Transmit(&hspi1, _spiData, SPI_DATA_SIZE, HAL_MAX_DELAY);
  CS_RESET();
}

static void MAX72XX_SpiClearBuffer(void) {
  memset(_spiData, 0, SPI_DATA_SIZE);
}

static void MAX72XX_FlushBufferAll() {
  // Only one data byte is sent to a device, so if there are many changes, it is more
  // efficient to send a data byte all devices at the same time, substantially cutting
  // the number of communication messages required.
  for (uint8_t i = 0; i < ROW_SIZE; i++) {
    // all data rows
    bool bChange = false; // set to true if we detected a change

    MAX72XX_SpiClearBuffer();

    for (uint8_t dev = FIRST_BUFFER; dev <= LAST_BUFFER; dev++) // all devices
        {
      if (bitRead(_matrix[dev].changed, i)) {
        // put our device data into the buffer
        _spiData[SPI_OFFSET(dev, 0)] = OP_DIGIT0 + i;
        _spiData[SPI_OFFSET(dev, 1)] = _matrix[dev].dig[i];
        bChange = true;
      }
    }

    if (bChange) {
      MAX72XX_SpiSend();
    }
  }

  // mark everything as cleared
  for (uint8_t dev = FIRST_BUFFER; dev <= LAST_BUFFER; dev++) {
    _matrix[dev].changed = ALL_CLEAR;
  }
}

static void MAX72XX_ControlHardware(uint8_t dev, controlRequest_t mode, int value) {
  // control command is for the devices, translate internal request to device bytes
  // into the transmission buffer
  uint8_t opcode = OP_NOOP;
  uint8_t param = 0;

  // work out data to write
  switch (mode) {
  case SHUTDOWN:
    opcode = OP_SHUTDOWN;
    param = (value == OFF ? 1 : 0);
    break;

  case SCANLIMIT:
    opcode = OP_SCANLIMIT;
    param = (value > MAX_SCANLIMIT ? MAX_SCANLIMIT : value);
    break;

  case INTENSITY:
    opcode = OP_INTENSITY;
    param = (value > MAX_INTENSITY ? MAX_INTENSITY : value);
    break;

  case DECODE:
    opcode = OP_DECODEMODE;
    param = (value == OFF ? 0 : 0xff);
    break;

  case TEST:
    opcode = OP_DISPLAYTEST;
    param = (value == OFF ? 0 : 1);
    break;

  default:
    return;
  }

  // put our device data into the buffer
  _spiData[SPI_OFFSET(dev, 0)] = opcode;
  _spiData[SPI_OFFSET(dev, 1)] = param;
}

static void MAX72XXControlLibrary(controlRequest_t mode, int value) {
  // control command was internal, set required parameters
  switch (mode) {
  case UPDATE:
    _updateEnabled = (value == ON);
    if (_updateEnabled)
      MAX72XX_FlushBufferAll();
    break;

  case WRAPAROUND:
    _wrapAround = (value == ON);
    break;

  default:
    break;
  }
}

bool MAX72XX_ControlOne(uint8_t buf, controlRequest_t mode, int value) {
  // dev is zero based and needs adjustment if used
  if (buf > LAST_BUFFER) {
    return (false);
  }

  if (mode < UPDATE) {
    // device based control
    MAX72XX_SpiClearBuffer();
    MAX72XX_ControlHardware(buf, mode, value);
    MAX72XX_SpiSend();
  } else {
    // internal control function, doesn't relate to specific device
    MAX72XXControlLibrary(mode, value);
  }

  return (true);
}
bool MAX72XX_ControlBy(uint8_t startDev, uint8_t endDev, controlRequest_t mode, int value) {
  if (endDev < startDev) {
    return (false);
  }

  if (mode < UPDATE) {
    // device based control
    MAX72XX_SpiClearBuffer();
    for (uint8_t i = startDev; i <= endDev; i++) {
      MAX72XX_ControlHardware(i, mode, value);
    }

    MAX72XX_SpiSend();
  } else {
    // internal control function, doesn't relate to specific device
    MAX72XXControlLibrary(mode, value);
  }

  return (true);
}

void MAX72XX_ControlAll(controlRequest_t mode, int value) {
  MAX72XX_ControlBy(0, MAX72XX_GetDeviceCount() - 1, mode, value);
}

////////////////////////////
static void MAX72XX_FlushBuffer(uint8_t buf) {
  // Use this function when the changes are limited to one device only.
  // Address passed is a buffer address
  if (buf > LAST_BUFFER) {
    return;
  }

  for (uint8_t i = 0; i < ROW_SIZE; i++) {
    if (bitRead(_matrix[buf].changed, i)) {
      MAX72XX_SpiClearBuffer();
      // put our device data into the buffer
      _spiData[SPI_OFFSET(buf, 0)] = OP_DIGIT0 + i;
      _spiData[SPI_OFFSET(buf, 1)] = _matrix[buf].dig[i];

      MAX72XX_SpiSend();
    }
  }
  _matrix[buf].changed = ALL_CLEAR;
}

bool MAX72XX_ClearOne(uint8_t buf) {
  if (buf > LAST_BUFFER) {
    return (false);
  }

  memset(_matrix[buf].dig, 0, sizeof(_matrix[buf].dig));
  _matrix[buf].changed = ALL_CHANGED;

  if (_updateEnabled) {
    MAX72XX_FlushBuffer(buf);
  }

  return (true);
}

void MAX72XX_ClearBy(uint8_t startDev, uint8_t endDev) {
  if (endDev < startDev) {
    return;
  }

  for (uint8_t buf = startDev; buf <= endDev; buf++) {
    memset(_matrix[buf].dig, 0, sizeof(_matrix[buf].dig));
    _matrix[buf].changed = ALL_CHANGED;
  }

  if (_updateEnabled) {
    MAX72XX_FlushBufferAll();
  }
}

void MAX72XX_ClearAll(void) {
  MAX72XX_ClearBy(0, MAX72XX_GetDeviceCount() - 1);
}

static uint8_t MAX72XX_BitReverse(uint8_t b) {
  // Reverse the order of bits within a byte.
  // Returns the reversed byte value.
  b = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
  b = ((b & 0xcc) >> 2) | ((b & 0x33) << 2);
  b = ((b & 0xaa) >> 1) | ((b & 0x55) << 1);
  return (b);
}

static bool MAX72XX_CopyC(uint8_t buf, uint8_t cSrc, uint8_t cDest) {
  // Src and Dest are in pixel coordinates.
  // if we are just copying rows there is no need to repackage any data
  uint8_t maskSrc = 1 << cSrc;  // which column/row of bits is the column data

  if ((buf > LAST_BUFFER) || (cSrc >= COL_SIZE) || (cDest >= COL_SIZE)) {
    return (false);
  }

  for (uint8_t i = 0; i < ROW_SIZE; i++) {
    if (_matrix[buf].dig[i] & maskSrc) {
      bitSet(_matrix[buf].dig[i], cDest);
    } else {
      bitClear(_matrix[buf].dig[i], cDest);
    }
  }

  _matrix[buf].changed = ALL_CHANGED;

  if (_updateEnabled) {
    MAX72XX_FlushBuffer(buf);
  }

  return (true);
}

static bool MAX72XX_CopyR(uint8_t buf, uint8_t rSrc, uint8_t rDest) {
  // Src and Dest are in pixel coordinates.
  // if we are just copying digits there is no need to repackage any data
  if ((buf > LAST_BUFFER) || (rSrc >= ROW_SIZE) || (rDest >= ROW_SIZE)) {
    return (false);
  }

  _matrix[buf].dig[rDest] = _matrix[buf].dig[rSrc];
  bitSet(_matrix[buf].changed, rDest);

  if (_updateEnabled) {
    MAX72XX_FlushBuffer(buf);
  }

  return (true);
}

bool MAX72XX_CopyColumn(uint8_t buf, uint8_t cSrc, uint8_t cDest) {
  return (MAX72XX_CopyC(buf, cSrc, cDest));
}

static bool MAX72XX_CopyRow(uint8_t buf, uint8_t rSrc, uint8_t rDest) {
  return (MAX72XX_CopyR(buf, rSrc, rDest));
}

static uint8_t MAX72XX_GetC(uint8_t buf, uint8_t c)
// c is in pixel coordinates and the return value must be in pixel coordinate order
{
  uint8_t mask = 1 << c;  // which column/row of bits is the column data
  uint8_t value = 0;        // assembles data to be returned to caller

  if ((buf > LAST_BUFFER) || (c >= COL_SIZE)) {
    return (0);
  }

  // for each digit data, pull out the column/row bit and place
  // it in value. The loop creates the data in pixel coordinate order as it goes.
  for (uint8_t i = 0; i < ROW_SIZE; i++) {
    if (_matrix[buf].dig[i] & mask) {
      bitSet(value, i);
    }
  }

  return (value);
}

static uint8_t MAX72XX_GetR(uint8_t buf, uint8_t r) {
  // r is in pixel coordinates for this buffer
  // returned value is in pixel coordinates
  if ((buf > LAST_BUFFER) || (r >= ROW_SIZE)) {
    return (0);
  }

  uint8_t value = _matrix[buf].dig[r];

  return (value);
}

uint8_t MAX72XX_GetDevColumn(uint8_t buf, uint8_t c) {
  return (MAX72XX_GetC(buf, c));
}

uint8_t MAX72XX_GetPixelColumn(uint8_t c) {
  return MAX72XX_GetDevColumn((c / COL_SIZE), c % COL_SIZE);
}

uint8_t MAX72XX_GetRow(uint8_t buf, uint8_t r) {
  return (MAX72XX_GetR(buf, r));
}

static bool MAX72XX_SetC(uint8_t buf, uint8_t c, uint8_t value) {
  // c and value are in pixel coordinate order

  if ((buf > LAST_BUFFER) || (c >= COL_SIZE)) {
    return (false);
  }

  for (uint8_t i = 0; i < ROW_SIZE; i++) {
    if (value & (1 << i)) {
      // mask off next column/row value passed in and set it in the dig buffer
      bitSet(_matrix[buf].dig[i], c);
    } else {
      bitClear(_matrix[buf].dig[i], c);
    }
  }
  _matrix[buf].changed = ALL_CHANGED;

  if (_updateEnabled) {
    MAX72XX_FlushBuffer(buf);
  }

  return (true);
}

static bool MAX72XX_SetR(uint8_t buf, uint8_t r, uint8_t value) {
  // r and value are in pixel coordinates
  if ((buf > LAST_BUFFER) || (r >= ROW_SIZE)) {
    return (false);
  }

  _matrix[buf].dig[r] = value;
  bitSet(_matrix[buf].changed, r);

  if (_updateEnabled) {
    MAX72XX_FlushBuffer(buf);
  }

  return (true);
}

bool MAX72XX_SetDevColumn(uint8_t buf, uint8_t c, uint8_t value) {
  return (MAX72XX_SetC(buf, c, value));
}

bool MAX72XX_SetPixelColumn(uint16_t c, uint8_t value) {
  return MAX72XX_SetDevColumn((c / COL_SIZE), c % COL_SIZE, value);
}

bool MAX72XX_GetBuffer(uint16_t col, uint8_t size, uint8_t *pd) {
  if ((col >= MAX72XX_GetColumnCount()) || (pd == NULL))
    return (false);

  for (uint8_t i = 0; i < size; i++) {
    *pd++ = MAX72XX_GetPixelColumn(col--);
  }

  return (true);
}

bool MAX72XX_SetBuffer(uint16_t col, uint8_t size, uint8_t *pd) {
  bool b = _updateEnabled;

  if ((col >= MAX72XX_GetColumnCount()) || (pd == NULL)) {
    return (false);
  }

  _updateEnabled = false;

  for (uint8_t i = 0; i < size; i++) {
    MAX72XX_SetPixelColumn(col--, *pd++);
  }

  _updateEnabled = b;

  if (_updateEnabled) {
    MAX72XX_FlushBufferAll();
  }

  return (true);
}

bool MAX72XX_SetRowOne(uint8_t buf, uint8_t r, uint8_t value) {
  return (MAX72XX_SetR(buf, r, value));
}

bool MAX72XX_SetRowBy(uint8_t startDev, uint8_t endDev, uint8_t r, uint8_t value) {
  bool b = _updateEnabled;

  if ((r >= ROW_SIZE) || (endDev < startDev)) {
    return (false);
  }

  _updateEnabled = false;

  for (uint8_t i = startDev; i <= endDev; i++) {
    MAX72XX_SetRowOne(i, r, value);
  }

  _updateEnabled = b;

  if (_updateEnabled) {
    MAX72XX_FlushBufferAll();
  }

  return (true);
}

bool MAX72XX_SetRowAll(uint8_t r, uint8_t value) {
  return MAX72XX_SetRowBy(0, MAX72XX_GetDeviceCount() - 1, r, value);
}

bool MAX72XX_GetPoint(uint8_t r, uint16_t c) {
  uint8_t buf = c / COL_SIZE;

  c %= COL_SIZE;

  if ((buf > LAST_BUFFER) || (r >= ROW_SIZE) || (c >= COL_SIZE)) {
    return (false);
  }

  return (bitRead(_matrix[buf].dig[r], c == 1));

}

bool MAX72XX_SetPoint(uint8_t r, uint16_t c, bool state) {
  uint8_t buf = c / COL_SIZE;
  c %= COL_SIZE;

  if ((buf > LAST_BUFFER) || (r >= ROW_SIZE) || (c >= COL_SIZE))
    return (false);

  if (state) {
    bitSet(_matrix[buf].dig[r], c);

  } else {
    bitClear(_matrix[buf].dig[r], c);
  }

  bitSet(_matrix[buf].changed, r);

  if (_updateEnabled) {
    MAX72XX_FlushBuffer(buf);
  }

  return (true);
}

bool MAX72XX_TransformBuffer(uint8_t buf, transformType_t ttype) {
  uint8_t t[ROW_SIZE];

  switch (ttype) {
//--------------
  case TSL: // Transform Shift Left one pixel element
    for (uint8_t i = 0; i < ROW_SIZE; i++) {
      _matrix[buf].dig[i] <<= 1;
    }
    break;

    //--------------
  case TSR: // Transform Shift Right one pixel element
    for (uint8_t i = 0; i < ROW_SIZE; i++) {
      _matrix[buf].dig[i] >>= 1;
    }
    break;

    //--------------
  case TSU: // Transform Shift Up one pixel element
    if (_wrapAround) {
      // save the first row or a zero row
      t[0] = MAX72XX_GetRow(buf, 0);
    } else {
      t[0] = 0;
    }

    for (uint8_t i = 0; i < ROW_SIZE - 1; i++) {
      MAX72XX_CopyRow(buf, i + 1, i);
    }

    MAX72XX_SetRowOne(buf, ROW_SIZE - 1, t[0]);
    break;

    //--------------
  case TSD: // Transform Shift Down one pixel element
    if (_wrapAround) {
      // save the last row or a zero row
      t[0] = MAX72XX_GetRow(buf, ROW_SIZE - 1);
    } else {
      t[0] = 0;
    }

    for (uint8_t i = ROW_SIZE; i > 0; --i) {
      MAX72XX_CopyRow(buf, i - 1, i);
    }

    MAX72XX_SetRowOne(buf, 0, t[0]);
    break;

    //--------------
  case TFLR: // Transform Flip Left to Right

    for (uint8_t i = 0; i < ROW_SIZE; i++)
      _matrix[buf].dig[i] = MAX72XX_BitReverse(_matrix[buf].dig[i]);

    break;

    //--------------
  case TFUD: // Transform Flip Up to Down

    for (uint8_t i = 0; i < ROW_SIZE / 2; i++) {
      uint8_t t = _matrix[buf].dig[i];
      _matrix[buf].dig[i] = _matrix[buf].dig[ROW_SIZE - i - 1];
      _matrix[buf].dig[ROW_SIZE - i - 1] = t;
    }

    break;

    //--------------
  case TRC: // Transform Rotate Clockwise
    for (uint8_t i = 0; i < ROW_SIZE; i++)
      t[i] = MAX72XX_GetDevColumn(buf, COL_SIZE - 1 - i);

    for (uint8_t i = 0; i < ROW_SIZE; i++)
      MAX72XX_SetRowOne(buf, i, t[i]);
    break;

    //--------------
  case TINV: // Transform INVert
    for (uint8_t i = 0; i < ROW_SIZE; i++)
      _matrix[buf].dig[i] = ~_matrix[buf].dig[i];
    break;

  default:
    return (false);
  }

  _matrix[buf].changed = ALL_CHANGED;
  return (true);
}

bool MAX72XX_TransformOne(uint8_t buf, transformType_t ttype) {
  if (buf > LAST_BUFFER) {
    return (false);
  }

  if (!MAX72XX_TransformBuffer(buf, ttype)) {
    return (false);
  }

  if (_updateEnabled) {
    MAX72XX_FlushBuffer(buf);
  }

  return (true);
}

bool MAX72XX_TransformBy(uint8_t startDev, uint8_t endDev, transformType_t ttype) {
// uint8_t t[ROW_SIZE];
  uint8_t colData;
  bool b = _updateEnabled;

  if (endDev < startDev)
    return (false);

  _updateEnabled = false;

  switch (ttype) {
  case TSL: // Transform Shift Left one pixel element (with overflow)
    colData = 0;
    // if we can call the user function later then we don't need to do anything here
    // however, wraparound mode means we know the data so no need to request from the
    // callback at all - just save it for later
    if (_wrapAround)
      colData = MAX72XX_GetPixelColumn(((endDev + 1) * COL_SIZE) - 1);

    // shift all the buffers along
    for (int8_t buf = endDev; buf >= startDev; --buf) {
      MAX72XX_TransformBuffer(buf, ttype);
      // handle the boundary condition
      MAX72XX_SetDevColumn(buf, 0, MAX72XX_GetDevColumn(buf - 1, COL_SIZE - 1));
    }

    MAX72XX_SetPixelColumn((startDev * COL_SIZE), colData);
    break;

  case TSR: // Transform Shift Right one pixel element (with overflow)
    // if we can call the user function later then we don't need to do anything here
    // however, wraparound mode means we know the data so no need to request from the
    // callback at all - just save it for later.
    colData = 0;
    if (_wrapAround)
      colData = MAX72XX_GetPixelColumn(startDev * COL_SIZE);

    // shift all the buffers along
    for (uint8_t buf = startDev; buf <= endDev; buf++) {
      MAX72XX_TransformBuffer(buf, ttype);

      // handle the boundary condition
      MAX72XX_SetDevColumn(buf, COL_SIZE - 1, MAX72XX_GetDevColumn(buf + 1, 0));
    }

    MAX72XX_SetPixelColumn(((endDev + 1) * COL_SIZE) - 1, colData);
    break;

  case TFLR: // Transform Flip Left to Right (use the whole field)
    // first reverse the device buffers end for end
    for (uint8_t buf = 0; buf < (endDev - startDev + 1) / 2; buf++) {
      deviceInfo_t t;

      t = _matrix[startDev + buf];
      _matrix[startDev + buf] = _matrix[endDev - buf];
      _matrix[endDev - buf] = t;
    }

    // now reverse the columns in each device
    for (uint8_t buf = startDev; buf <= endDev; buf++)
      MAX72XX_TransformBuffer(buf, ttype);
    break;

    // These next transformations work the same just by doing the individual devices
  case TSU:   // Transform Shift Up one pixel element
  case TSD:   // Transform Shift Down one pixel element
  case TFUD:  // Transform Flip Up to Down
  case TRC:   // Transform Rotate Clockwise
  case TINV:  // Transform INVert
    for (uint8_t buf = startDev; buf <= endDev; buf++)
      MAX72XX_TransformBuffer(buf, ttype);
    break;

  default:
    return (false);
  }

  _updateEnabled = b;

  if (_updateEnabled) {
    MAX72XX_FlushBufferAll();
  }

  return (true);
}

bool MAX72XX_TransformAll(transformType_t ttype) {
  return MAX72XX_TransformBy(0, MAX72XX_GetDeviceCount() - 1, ttype);
}

void MAX72XX_UpdateAll(void) {
  MAX72XX_FlushBufferAll();
}
void MAX72XX_UpdateMode(controlValue_t mode) {
  MAX72XX_ControlAll(UPDATE, mode);
}

void MAX72XX_UpdateOne(uint8_t buf) {
  MAX72XX_FlushBuffer(buf);
}

void MAX72XX_Wraparound(controlValue_t mode) {
  MAX72XX_ControlAll(WRAPAROUND, mode);
}
;

///////////

