#ifndef INC_SSD1306_H_
#define INC_SSD1306_H_

#include "stm32f4xx_hal.h"
#include "fonts.h"
#include "stdlib.h"
#include "string.h"

#define SSD1306_I2C_ADDR         0x78
#define SSD1306_WIDTH            128
#define SSD1306_HEIGHT           64
#define ssd1306_I2C_TIMEOUT      20000

typedef enum {
	SSD1306_COLOR_BLACK = 0x00,
	SSD1306_COLOR_WHITE = 0x01
} SSD1306_COLOR_t;

#define SSD1306_RIGHT_HORIZONTAL_SCROLL  0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL   0x27
#define SSD1306_DEACTIVATE_SCROLL        0x2E
#define SSD1306_ACTIVATE_SCROLL          0x2F

typedef enum {
	SSD1306_SCROLL_RIGHT = 0x00,
	SSD1306_SCROLL_LEFT  = 0x01
} SSD1306_SCROLL_DIR_t;

typedef struct {
    I2C_HandleTypeDef* hi2c;
    uint8_t address;
    uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
    uint16_t CurrentX;
    uint16_t CurrentY;
} SSD1306_t;

void ssd1306_I2C_Write(SSD1306_t* dev, uint8_t reg, uint8_t *data, uint16_t count);

uint8_t SSD1306_Init(SSD1306_t* dev, I2C_HandleTypeDef* hi2c, uint8_t address);
void SSD1306_UpdateScreen(SSD1306_t* dev);
void SSD1306_Fill(SSD1306_t* dev, SSD1306_COLOR_t Color);
void SSD1306_Clear(SSD1306_t* dev);
void SSD1306_SetCursor(SSD1306_t* dev, uint16_t x, uint16_t y);

HAL_StatusTypeDef SSD1306_DrawPixel(SSD1306_t* dev, uint16_t x, uint16_t y, SSD1306_COLOR_t color);
void SSD1306_Scroll(SSD1306_t* dev, SSD1306_SCROLL_DIR_t direction, uint8_t start_row, uint8_t end_row);
void SSD1306_Stopscroll(SSD1306_t* dev);
void SSD1306_Putc(SSD1306_t* dev, uint16_t x, uint16_t y, char ch, FontDef_t* Font);
HAL_StatusTypeDef SSD1306_Puts(SSD1306_t* dev, char* str, FontDef_t* Font);

#endif
