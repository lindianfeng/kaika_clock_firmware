/*
 * max72xx.h
 *
 *  Created on: Dec 13, 2020
 *      Author: lindi
 */

#ifndef INC_MAX72XX_H_
#define INC_MAX72XX_H_
#include <stdbool.h>
#include <stdint.h>

// Opcodes for the MAX7221 and MAX7219
// All OP_DIGITn are offsets from OP_DIGIT0
#define OP_NOOP       0 ///< MAX72xx opcode for NO OP
#define OP_DIGIT0     1 ///< MAX72xx opcode for DIGIT0
#define OP_DIGIT1     2 ///< MAX72xx opcode for DIGIT1
#define OP_DIGIT2     3 ///< MAX72xx opcode for DIGIT2
#define OP_DIGIT3     4 ///< MAX72xx opcode for DIGIT3
#define OP_DIGIT4     5 ///< MAX72xx opcode for DIGIT4
#define OP_DIGIT5     6 ///< MAX72xx opcode for DIGIT5
#define OP_DIGIT6     7 ///< MAX72xx opcode for DIGIT6
#define OP_DIGIT7     8 ///< MAX72xx opcode for DIGIT7
#define OP_DECODEMODE  9  ///< MAX72xx opcode for DECODE MODE
#define OP_INTENSITY   10 ///< MAX72xx opcode for SET INTENSITY
#define OP_SCANLIMIT   11 ///< MAX72xx opcode for SCAN LIMIT
#define OP_SHUTDOWN    12 ///< MAX72xx opcode for SHUT DOWN
#define OP_DISPLAYTEST 15 ///< MAX72xx opcode for DISPLAY TEST

//
#define ALL_CHANGED   0xff    ///< Mask for all rows changed in a buffer structure
#define ALL_CLEAR     0x00    ///< Mask for all rows clear in a buffer structure

//
#define ROW_SIZE  8   ///< The size in pixels of a row in the device LED matrix array
#define COL_SIZE  8   ///< The size in pixels of a column in the device LED matrix array

//
#define MAX_INTENSITY 0xf ///< The maximum intensity value that can be set for a LED array
#define MAX_SCANLIMIT 7   ///< The maximum scan limit value that can be set for the devices

//
#define MAX_DEVICES 4

#define FIRST_BUFFER 0                 ///< First buffer number
#define LAST_BUFFER  (MAX_DEVICES-1)   ///< Last buffer number

#define _hwDigRows 1
#define _hwRevCols 0
#define _hwRevRows 1


// Macros to map reversed ROW and COLUMN coordinates
#define HW_ROW(r) (_hwRevRows ? (ROW_SIZE - 1 - (r)) : (r)) ///< Pixel to hardware coordinate row mapping
#define HW_COL(c) (_hwRevCols ? (COL_SIZE - 1 - (c)) : (c)) ///< Pixel to hardware coordinate column mapping

//#define SPI_DATA_SIZE (sizeof(uint8_t)*MAX_DEVICES*2)   ///< Size of the SPI data buffers
//#define SPI_OFFSET(i,x) (((i)*2)+(x))     ///< SPI data offset for buffer i, digit x

#define SPI_DATA_SIZE (sizeof(uint8_t)*MAX_DEVICES*2)   ///< Size of the SPI data buffers
#define SPI_OFFSET(i,x) (((LAST_BUFFER-(i))*2)+(x))     ///< SPI data offset for buffer i, digit x

typedef enum {
  OFF = 0,  ///< General OFF status request
  ON = 1    ///< General ON status request
} controlValue_t;

typedef enum {
  SHUTDOWN = 0,   ///< Shut down the MAX7219. Requires ON/OFF value. Library default is OFF.
  SCANLIMIT = 1,  ///< Set the scan limit for the MAX7219. Requires numeric value [0..MAX_SCANLIMIT]. Library default is all on.
  INTENSITY = 2,  ///< Set the LED intensity for the MAX7219. Requires numeric value [0..MAX_INTENSITY]. LIbrary default is MAX_INTENSITY/2.
  TEST = 3,       ///< Set the MAX7219 in test mode. Requires ON/OFF value. Library default is OFF.
  DECODE = 4,     ///< Set the MAX7219 7 segment decode mode. Requires ON/OFF value. Library default is OFF.
  UPDATE = 10,    ///< Enable or disable auto updates of the devices from the library. Requires ON/OFF value. Library default is ON.
  WRAPAROUND = 11 ///< Enable or disable wraparound when shifting (circular buffer). Requires ON/OFF value. Library default is OFF.
} controlRequest_t;

typedef enum {
  TSL,  ///< Transform Shift Left one pixel element
  TSR,  ///< Transform Shift Right one pixel element
  TSU,  ///< Transform Shift Up one pixel element
  TSD,  ///< Transform Shift Down one pixel element
  TFLR, ///< Transform Flip Left to Right
  TFUD, ///< Transform Flip Up to Down
  TRC,  ///< Transform Rotate Clockwise 90 degrees
  TINV  ///< Transform INVert (pixels inverted)
} transformType_t;

void MAX72XX_Init(void);
void MAX72XX_DeInit(void);

uint8_t MAX72XX_GetDeviceCount(void);
uint8_t MAX72XX_GetColumnCount(void);

bool MAX72XX_ControlOne(uint8_t buf, controlRequest_t mode, int value);
bool MAX72XX_ControlBy(uint8_t startDev, uint8_t endDev, controlRequest_t mode, int value);
void MAX72XX_ControlAll(controlRequest_t mode, int value);


bool MAX72XX_ClearOne(uint8_t buf);
void MAX72XX_ClearBy(uint8_t startDev, uint8_t endDev);
void MAX72XX_ClearAll(void);

bool MAX72XX_SetBuffer(uint16_t col, uint8_t size, uint8_t *pd);
bool MAX72XX_GetBuffer(uint16_t col, uint8_t size, uint8_t *pd);

uint8_t MAX72XX_GetDevColumn(uint8_t buf, uint8_t c);
uint8_t MAX72XX_GetPixelColumn(uint8_t c);

bool MAX72XX_SetDevColumn(uint8_t buf, uint8_t c, uint8_t value);
bool MAX72XX_SetPixelColumn(uint16_t c, uint8_t value);

uint8_t MAX72XX_GetRow(uint8_t buf, uint8_t r);
bool MAX72XX_SetRowOne(uint8_t buf, uint8_t r, uint8_t value);
bool MAX72XX_SetRowBy(uint8_t startDev, uint8_t endDev, uint8_t r, uint8_t value);
bool MAX72XX_SetRowAll(uint8_t r, uint8_t value);

bool MAX72XX_GetPoint(uint8_t r, uint16_t c);
bool MAX72XX_SetPoint(uint8_t r, uint16_t c, bool state);

bool MAX72XX_TransformOne(uint8_t buf, transformType_t ttype);
bool MAX72XX_TransformBy(uint8_t startDev, uint8_t endDev, transformType_t ttype);
bool MAX72XX_TransformAll(transformType_t ttype);

void MAX72XX_UpdateAll(void);
void MAX72XX_UpdateOne(uint8_t buf);
void MAX72XX_UpdateMode(controlValue_t mode);

void MAX72XX_Wraparound(controlValue_t mode);

#endif /* INC_MAX72XX_H_ */
