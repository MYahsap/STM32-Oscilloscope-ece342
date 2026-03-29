#include "ssd1306.h"

static uint8_t SSD1306_Init_Config[] = {
    0xAE,
    0x20, 0x02,
    0xB0,
    0xC8,
    0x00,
    0x10,
    0x40,
    0x81, 0x7F,
    0xA1,
    0xA6,
    0xA8, 0x3F,
    0xA4,
    0xD3, 0x00,
    0xD5, 0x80,
    0xD9, 0xF1,
    0xDA, 0x12,
    0xDB, 0x20,
    0x8D, 0x14,
    0xAF
};

void ssd1306_I2C_Write(SSD1306_t* dev, uint8_t reg, uint8_t* data, uint16_t count) {
    uint8_t packet[129];

    packet[0] = reg;
    memcpy(&packet[1], data, count);

    HAL_I2C_Master_Transmit(dev->hi2c, dev->address, packet, count + 1, ssd1306_I2C_TIMEOUT);
}

uint8_t SSD1306_Init(SSD1306_t* dev, I2C_HandleTypeDef* hi2c, uint8_t address) {
    dev->hi2c = hi2c;
    dev->address = address;
    dev->CurrentX = 0;
    dev->CurrentY = 0;

    if (HAL_I2C_IsDeviceReady(dev->hi2c, dev->address, 1, 20000) != HAL_OK) {
        return 1;
    }

    HAL_Delay(10);

    ssd1306_I2C_Write(dev, 0x00, SSD1306_Init_Config, sizeof(SSD1306_Init_Config));
    SSD1306_Fill(dev, SSD1306_COLOR_BLACK);
    SSD1306_UpdateScreen(dev);

    return 0;
}

void SSD1306_UpdateScreen(SSD1306_t* dev) {
    uint8_t page;
    uint8_t cmds[3];

    for (page = 0; page < 8; page++) {
        cmds[0] = 0xB0 + page;
        cmds[1] = 0x00;
        cmds[2] = 0x10;

        ssd1306_I2C_Write(dev, 0x00, cmds, 3);
        ssd1306_I2C_Write(dev, 0x40, &dev->buffer[SSD1306_WIDTH * page], SSD1306_WIDTH);
    }
}
void SSD1306_Fill(SSD1306_t* dev, SSD1306_COLOR_t color) {
    memset(dev->buffer, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF, sizeof(dev->buffer));
}

void SSD1306_Clear(SSD1306_t* dev) {
    memset(dev->buffer, 0x00, sizeof(dev->buffer));
    dev->CurrentX = 0;
    dev->CurrentY = 0;
}

void SSD1306_SetCursor(SSD1306_t* dev, uint16_t x, uint16_t y) {
    dev->CurrentX = x;
    dev->CurrentY = y;
}

HAL_StatusTypeDef SSD1306_DrawPixel(SSD1306_t* dev, uint16_t x, uint16_t y, SSD1306_COLOR_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return HAL_ERROR;
    }

    if (color == SSD1306_COLOR_WHITE) {
        dev->buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    } else {
        dev->buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }

    return HAL_OK;
}

void SSD1306_Scroll(SSD1306_t* dev, SSD1306_SCROLL_DIR_t direction, uint8_t start_row, uint8_t end_row) {
    uint8_t cmds[7];
    uint8_t activate = SSD1306_ACTIVATE_SCROLL;

    cmds[0] = (direction == SSD1306_SCROLL_RIGHT) ?
              SSD1306_RIGHT_HORIZONTAL_SCROLL :
              SSD1306_LEFT_HORIZONTAL_SCROLL;
    cmds[1] = 0x00;
    cmds[2] = start_row;
    cmds[3] = 0x00;
    cmds[4] = end_row;
    cmds[5] = 0x00;
    cmds[6] = 0xFF;

    ssd1306_I2C_Write(dev, 0x00, cmds, 7);
    ssd1306_I2C_Write(dev, 0x00, &activate, 1);
}

void SSD1306_Stopscroll(SSD1306_t* dev) {
    uint8_t cmd = SSD1306_DEACTIVATE_SCROLL;
    ssd1306_I2C_Write(dev, 0x00, &cmd, 1);
}

void SSD1306_Putc(SSD1306_t* dev, uint16_t x, uint16_t y, char ch, FontDef_t* Font) {
    uint32_t i, j;
    uint16_t b;

    if (ch < 32 || ch > 126) return;

    for (i = 0; i < Font->FontHeight; i++) {
        b = Font->data[(ch - 32) * Font->FontHeight + i];
        for (j = 0; j < Font->FontWidth; j++) {
            if ((b << j) & 0x8000) {
                SSD1306_DrawPixel(dev, x + j, y + i, SSD1306_COLOR_WHITE);
            } else {
                SSD1306_DrawPixel(dev, x + j, y + i, SSD1306_COLOR_BLACK);
            }
        }
    }
}

HAL_StatusTypeDef SSD1306_Puts(SSD1306_t* dev, char* str, FontDef_t* Font) {
    while (*str != '\0') {
        if (dev->CurrentX + Font->FontWidth > SSD1306_WIDTH) {
            dev->CurrentX = 0;
            dev->CurrentY += Font->FontHeight;
        }

        if (dev->CurrentY + Font->FontHeight > SSD1306_HEIGHT) {
            return HAL_ERROR;
        }

        SSD1306_Putc(dev, dev->CurrentX, dev->CurrentY, *str, Font);
        dev->CurrentX += Font->FontWidth;
        str++;
    }

    return HAL_OK;
}

void drawLine(SSD1306_t* dev, int x0, int y0, int x1, int y1)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? -(y1 - y0) : -(y0 - y1);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (1)
    {
        if (x0 >= 0 && x0 < SSD1306_WIDTH && y0 >= 0 && y0 < SSD1306_HEIGHT)
        {
            SSD1306_DrawPixel(dev, x0, y0, SSD1306_COLOR_WHITE);
        }

        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_Wave(SSD1306_t* dev, uint16_t waveBuffer[], uint32_t vdiv, int16_t voffset)
{
	uint16_t i;
	float vin_mV;
	float y_f;
	int16_t y;
	int16_t prev_y = 0;
	int16_t center = SSD1306_HEIGHT / 2;

	for (i = 0; i < 128; i++)
	{
		// ADC 0..4095 -> -5000mV..+5000mV
		vin_mV = ((float)waveBuffer[i] * 10000.0f / 4095.0f) - 5000.0f;

		// volts -> pixels
		y_f = center - ((vin_mV - (float)voffset) * 32.0f / (float)vdiv);
		y = (int16_t)(y_f + 0.5f);

		if (y < 0) y = 0;
		if (y >= SSD1306_HEIGHT) y = SSD1306_HEIGHT - 1;

		if (i > 0)
		{
			drawLine(dev, i - 1, prev_y, i, y);
		}
		else
		{
			SSD1306_DrawPixel(dev, i, y, SSD1306_COLOR_WHITE);
		}

		prev_y = y;
	}
}

void buffer_Set(SSD1306_t* dev, const uint8_t sourceBuffer[]){
	memcpy(dev->buffer, sourceBuffer, SSD1306_WIDTH * SSD1306_HEIGHT / 8);
}
