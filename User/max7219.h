#ifndef MAX7219_H_
#define MAX7219_H_

#include <stdbool.h>
#include <stdint.h>

#define LED_NUM 4
#define DEFAULT_INTENSIVITY 0x05

typedef enum {
  REG_DECODE_MODE = 0x09,
  REG_INTENSITY = 0x0A,
  REG_SCAN_LIMIT = 0x0B,
  REG_SHUTDOWN = 0x0C,
  REG_DISPLAY_TEST = 0x0F,
} MAX7219_REGISTERS;

void Max7219_Init(void);
void Max7219_SetIntensivity(uint8_t intensivity);
void Max7219_SendData(uint8_t addr, uint8_t data);
void Max7219_WriteByte(uint8_t addr, uint8_t data);

void Max7219_ShowTime(uint8_t hour, uint8_t minute, uint8_t second, uint32_t tick);
void Max7219_ShowDate(uint8_t month, uint8_t day,uint8_t week);

void Max7219_ShowNumbers(void);
#endif /* MAX7219_H_ */
