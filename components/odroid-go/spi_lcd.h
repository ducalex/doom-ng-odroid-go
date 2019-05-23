/* 
 * This file is part of odroid-go-std-lib.
 * Copyright (c) 2019 ducalex.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

typedef struct {
      uint8_t charCode;
      int adjYOffset;
      int width;
      int height;
      int xOffset;
      int xDelta;
      uint16_t dataPtr;
} propFont;

#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_MH 0x04
#define TFT_RGB_BGR 0x08
#define TFT_LED_DUTY_MAX 0x1fff
#define LCD_CMD 0
#define LCD_DATA 1

static const int16_t default_palette[256] = {0, 0xFFFF, 7 << 5, 7, 7 << 10};

short backlight_percentage_get(void);
void backlight_percentage_set(short level);

void spi_lcd_init();
void spi_lcd_cmd(uint8_t cmd);
void spi_lcd_data(uint8_t *data, int len);
void spi_lcd_wait_finish();
void spi_lcd_fb_flush();
void spi_lcd_fb_setPalette(const int16_t *palette);
void spi_lcd_fb_setptr(uint8_t *buffer);
void spi_lcd_fb_write(uint8_t *buffer);
void spi_lcd_fb_clear();
void spi_lcd_fb_drawPixel(int x, int y, uint16_t color);
void spi_lcd_fb_setFont(const uint8_t *font);
void spi_lcd_fb_setFontColor(uint16_t color);
void spi_lcd_fb_print(int x, int y, char *string);
void spi_lcd_fb_printf(int x, int y, char *string, ...);
