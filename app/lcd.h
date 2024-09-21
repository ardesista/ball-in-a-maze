#ifndef LCD_H // 1.54 inch ST7789 240x240
#define LCD_H

#include <stdint.h>

#define LCD_MODE              1      // 0: GPIO, 1: SPI
#define LCD_ALIGN             3      // 0: TOP-DOWN, 1: BOTTOM-UP, 2: RIGHT-LEFT, 3: LEFT-RIGHT
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

enum {
    DEPTH_1BPP,
    DEPTH_2BPP,
    DEPTH_4BPP,
    DEPTH_8BPP,
    DEPTH_16BPP
};

#define FB_DIRECT 0x80

#define lcd_setpixel(x, y, color)                    fb_setpixel(&LCD, x, y, color)
#define lcd_draw_hline(x0, y0, length, color)        fb_draw_hline(&LCD, x0, y0, length, color)
#define lcd_draw_vline(x0, y0, length, color)        fb_draw_vline(&LCD, x0, y0, length, color)
#define lcd_draw_line(x0, y0, x1, y1, color)         fb_draw_line(&LCD, x0, y0, x1, y1, color)
#define lcd_draw_circle(xc, yc, radius, color)       fb_draw_circle(&LCD, xc, yc, radius, color)
#define lcd_draw_image(x0, y0, width, height, im)    fb_draw_image(&LCD, x0, y0, width, height, im)
#define lcd_fill_rect(x0, y0, width, height, color)  fb_fill_rect(&LCD, x0, y0, width, height, color)
#define lcd_fill_circle(xc, yc, radius, color)       fb_fill_circle(&LCD, xc, yc, radius, color)
#define lcd_putchar(x, y, c, font_color, bg_color)   fb_putchar(&LCD, x, y, c, font_color, bg_color)
#define lcd_puts(x, y, s, font_color, bg_color)      fb_puts(&LCD, x, y, s, font_color, bg_color)

typedef struct {
    uint32_t width   : 10;
    uint32_t height  : 10;
    uint32_t depth   : 4;
    uint32_t flags   : 8;
} fb_t;

extern fb_t LCD;

void lcd_init();
void lcd_draw_fb(int16_t x, int16_t y, const fb_t* fb);

void fb_fill(fb_t* fb, uint16_t color);
void fb_setpixel(fb_t* fb, int16_t x, int16_t y, uint16_t color);
void fb_draw_hline(fb_t* fb, int16_t x0, int16_t y0, int16_t length, uint16_t color);
void fb_draw_vline(fb_t* fb, int16_t x0, int16_t y0, int16_t length, uint16_t color);
void fb_draw_line(fb_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void fb_draw_circle(fb_t* fb, int16_t xc, int16_t yc, int16_t radius, uint16_t color);
void fb_draw_image(fb_t* fb, int16_t x0, int16_t y0, int16_t width, int16_t height, const uint16_t* im);
void fb_fill_rect(fb_t* fb, int16_t x0, int16_t y0, int16_t width, int16_t height, uint16_t color);
void fb_fill_circle(fb_t* fb, int16_t xc, int16_t yc, int16_t radius, uint16_t color);
void fb_putchar(fb_t* fb, int16_t x, int16_t y, char c, uint16_t font_color, uint16_t bg_color);
void fb_puts(fb_t* fb, int16_t x, int16_t y, const char* s, uint16_t font_color, uint16_t bg_color);

#endif // LCD_H
