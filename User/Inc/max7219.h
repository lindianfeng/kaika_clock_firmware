#ifndef MAX7219_H_
#define MAX7219_H_

#include <stdbool.h>
#include <stdint.h>

#define LED_UNIT_NUM 4
#define FRAME_DATA_SIZE (8*LED_UNIT_NUM)
#define DEFAULT_INTENSIVITY 0x0a

typedef enum {
  REG_DECODE_MODE = 0x09,
  REG_INTENSITY = 0x0A,
  REG_SCAN_LIMIT = 0x0B,
  REG_SHUTDOWN = 0x0C,
  REG_DISPLAY_TEST = 0x0F,
} MAX7219_REGISTERS;

void Max7219_Init(void);
void Max7219_SetIntensivity(uint8_t intensivity);

void Max7219_SetData(const uint8_t *data, uint32_t len);
void Max7219_Render(void);
void Max7219_RenderData(const uint8_t *frame_data,uint32_t len);


#endif /* MAX7219_H_ */
