/*
 * stm32_ds3231.c
 *
 *  Created on: 2019. 3. 17.
 *      Author: kiki
 */
#include "ds3231.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c1;

#define DS3231_ADDR (0x68 << 1)

static uint16_t B2D(uint16_t bcd);
static uint16_t D2B(uint16_t decimal);

RTC_Data rtc = { .Year = 2020, .Month = 12, .Day = 11, .DaysOfWeek = FRIDAY, .Hour = 21, .Min = 21, .Sec = 20 };

void DS3231_Init() {
}

bool DS3231_GetTime(RTC_Data *rtc) {
  uint8_t startAddr = DS3231_REG_TIME;
  uint8_t buffer[7] = { 0 };

  if (HAL_I2C_Master_Transmit(&hi2c1, DS3231_ADDR, &startAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
    return false;
  }

  if (HAL_I2C_Master_Receive(&hi2c1, DS3231_ADDR, buffer, sizeof(buffer), HAL_MAX_DELAY) != HAL_OK) {
    return false;
  }

  rtc->Sec = B2D(buffer[0] & 0x7F);
  rtc->Min = B2D(buffer[1] & 0x7F);
  rtc->Hour = B2D(buffer[2] & 0x3F);
  rtc->DaysOfWeek = buffer[3] & 0x07;
  rtc->Day = B2D(buffer[4] & 0x3F);
  rtc->Month = B2D(buffer[5] & 0x1F);
  rtc->Year = B2D(buffer[6]);
  return true;
}

bool DS3231_SetTime(RTC_Data *rtc) {
  uint8_t startAddr = DS3231_REG_TIME;
  uint8_t buffer[8] = { startAddr, D2B(rtc->Sec), D2B(rtc->Min), D2B(rtc->Hour), rtc->DaysOfWeek, D2B(rtc->Day), D2B(rtc->Month), D2B(rtc->Year) };
  return HAL_I2C_Master_Transmit(&hi2c1, DS3231_ADDR, buffer, sizeof(buffer), HAL_MAX_DELAY) == HAL_OK;
}

bool DS3231_ReadTemperature(float *temp) {
  uint8_t startAddr = DS3231_REG_TEMP;
  uint8_t buffer[2] = { 0 };

  if (HAL_I2C_Master_Transmit(&hi2c1, DS3231_ADDR, &startAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
    return false;
  }

  if (HAL_I2C_Master_Receive(&hi2c1, DS3231_ADDR, buffer, sizeof(buffer), HAL_MAX_DELAY) != HAL_OK) {
    return false;
  }

  int16_t value = (buffer[0] << 8) | (buffer[1]);
  value = (value >> 6);

  *temp = value / 4.0f;
  return true;
}

bool DS3231_SetAlarm(uint8_t mode, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec) {
  uint8_t alarmSecond = D2B(sec);
  uint8_t alarmMinute = D2B(min);
  uint8_t alarmHour = D2B(hour);
  uint8_t alarmDate = D2B(date);

  switch (mode) {
    case ALARM_MODE_ALL_MATCHED:
      break;
    case ALARM_MODE_HOUR_MIN_SEC_MATCHED:
      alarmDate |= 0x80;
      break;
    case ALARM_MODE_MIN_SEC_MATCHED:
      alarmDate |= 0x80;
      alarmHour |= 0x80;
      break;
    case ALARM_MODE_SEC_MATCHED:
      alarmDate |= 0x80;
      alarmHour |= 0x80;
      alarmMinute |= 0x80;
      break;
    case ALARM_MODE_ONCE_PER_SECOND:
      alarmDate |= 0x80;
      alarmHour |= 0x80;
      alarmMinute |= 0x80;
      alarmSecond |= 0x80;
      break;
    default:
      break;
  }

  /* Write Alarm Registers */
  const uint8_t startAddr = DS3231_REG_ALARM1;
  uint8_t buffer[5] = { startAddr, alarmSecond, alarmMinute, alarmHour, alarmDate };

  if (HAL_I2C_Master_Transmit(&hi2c1, DS3231_ADDR, buffer, sizeof(buffer), HAL_MAX_DELAY) != HAL_OK) {
    return false;
  }

  /* Enable Alarm1 at Control Register */
  uint8_t ctrlReg = 0x00;
  ReadRegister(DS3231_REG_CONTROL, &ctrlReg);
  ctrlReg |= DS3231_CON_A1IE;
  ctrlReg |= DS3231_CON_INTCN;
  WriteRegister(DS3231_REG_CONTROL, ctrlReg);

  return true;
}

bool DS3231_ClearAlarm(void) {
  uint8_t ctrlReg;
  uint8_t statusReg;

  /* Clear Control Register */
  ReadRegister(DS3231_REG_CONTROL, &ctrlReg);
  ctrlReg &= ~DS3231_CON_A1IE;
  WriteRegister(DS3231_REG_CONTROL, ctrlReg);

  /* Clear Status Register */
  ReadRegister(DS3231_REG_STATUS, &statusReg);
  statusReg &= ~DS3231_STA_A1F;
  WriteRegister(DS3231_REG_STATUS, statusReg);

  return true;
}

bool ReadRegister(uint8_t regAddr, uint8_t *value) {
  return HAL_I2C_Master_Transmit(&hi2c1, DS3231_ADDR, &regAddr, 1, HAL_MAX_DELAY) == HAL_OK && HAL_I2C_Master_Receive(&hi2c1, DS3231_ADDR, value, 1, HAL_MAX_DELAY) == HAL_OK;
}

bool WriteRegister(uint8_t regAddr, uint8_t value) {
  uint8_t buffer[2] = { regAddr, value };
  return HAL_I2C_Master_Transmit(&hi2c1, DS3231_ADDR, buffer, sizeof(buffer), HAL_MAX_DELAY) == HAL_OK;
}

static uint16_t B2D(uint16_t bcd) {
  return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint16_t D2B(uint16_t decimal) {
  return (((decimal / 10) << 4) | (decimal % 10));
}
