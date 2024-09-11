#ifndef LCD_H // 1.54 inch ST7789 240x240
#define LCD_H

#include <stdint.h>

#define LCD_MODE              1      // 0: GPIO, 1: SPI
#define LCD_ALIGN             0      // 0: TOP-DOWN, 1: BOTTOM-UP, 2: RIGHT-LEFT, 3: LEFT-RIGHT
#define LCD_WIDTH             240
#define LCD_HEIGHT            240

#define COLOR_WHITE           0xFFFF
#define COLOR_BLACK           0x0000      
#define COLOR_BLUE            0x001F  
#define COLOR_BRED            0xF81F
#define COLOR_GRED            0xFFE0
#define COLOR_GBLUE           0x07FF
#define COLOR_RED             0xF800
#define COLOR_MAGENTA         0xF81F
#define COLOR_GREEN           0x07E0
#define COLOR_CYAN            0x7FFF
#define COLOR_YELLOW          0xFFE0
#define COLOR_BROWN           0xBC40
#define COLOR_BRRED           0xFC07
#define COLOR_GRAY            0x8430
#define COLOR_DARKBLUE        0x01CF
#define COLOR_LIGHTBLUE       0x7D7C
#define COLOR_GRAYBLUE        0x5458
#define COLOR_LIGHTGREEN      0x841F
#define COLOR_LGRAY           0xC618
#define COLOR_LGRAYBLUE       0xA651
#define COLOR_LBBLUE          0x2B12

#define COLOR_FROMRGB(r, g, b) (((uint16_t)b * 31 / 255) | (((uint16_t)g * 63 / 255) << 5) | (((uint16_t)r * 31 / 255) << 11))

int lcd_init();
void lcd_fill(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color);
void lcd_blit(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint16_t* im);
void lcd_set_char(uint8_t row, uint8_t col, char c, uint16_t font_color, uint16_t bg_color);
void lcd_puts(uint8_t row, uint8_t col, const char* s, uint16_t font_color, uint16_t bg_color);

#endif // LCD_H
