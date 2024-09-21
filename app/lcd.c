#include "debug.h"
#include "lcd.h"
#include "font12x12_basic.h"
#include <stdlib.h>
#include <string.h>

fb_t LCD = { .width = LCD_WIDTH, .height = LCD_HEIGHT, .depth = DEPTH_16BPP, .flags = FB_DIRECT };

#ifndef _int16_t_swap
#define _int16_t_swap(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif // _int16_t_swap

#ifndef _int16_t_clamp
#define _int16_t_clamp(x, from, to)                                            \
  {                                                                            \
    if(x < from)                                                               \
        x = from;                                                              \
    else if(x > to)                                                            \
        x = to;                                                                \
  }
#endif // _int16_t_clamp

#define LCD_SCLK_CLR()  GPIO_ResetBits(GPIOB, GPIO_Pin_13)  // SCL=SCLK
#define LCD_SCLK_SET()  GPIO_SetBits(GPIOB, GPIO_Pin_13)

#define LCD_MOSI_CLR()  GPIO_ResetBits(GPIOB, GPIO_Pin_15)  // SDA=MOSI
#define LCD_MOSI_SET()  GPIO_SetBits(GPIOB, GPIO_Pin_15)

#define LCD_RES_CLR()   GPIO_ResetBits(GPIOB, GPIO_Pin_11)  // RES
#define LCD_RES_SET()   GPIO_SetBits(GPIOB, GPIO_Pin_11)

#define LCD_DC_CLR()    GPIO_ResetBits(GPIOB, GPIO_Pin_10)  // DC
#define LCD_DC_SET()    GPIO_SetBits(GPIOB, GPIO_Pin_10)

#define LCD_CS_CLR()    GPIO_ResetBits(GPIOB, GPIO_Pin_12)  // CS
#define LCD_CS_SET()    GPIO_SetBits(GPIOB, GPIO_Pin_12)

#define LCD_BLK_CLR()   GPIO_ResetBits(GPIOB, GPIO_Pin_9)   // BLK
#define LCD_BLK_SET()   GPIO_SetBits(GPIOB, GPIO_Pin_9)

static const uint16_t* fb_palette_p(const fb_t* fb)
{
    return (const uint16_t*)(fb->depth == DEPTH_16BPP ? 0 : (fb + 1));
}

static void* fb_data_p(const fb_t* fb)
{
uint16_t *p = (uint16_t*)fb_palette_p(fb);

    if(fb->flags & FB_DIRECT)
        return 0;

    switch(fb->depth) {
        case DEPTH_1BPP:
            p += 2;
            break;
        case DEPTH_2BPP:
            p += 4;
            break;
        case DEPTH_4BPP:
            p += 16;
            break;
        case DEPTH_8BPP:
            p += 256;
            break;
    }

    return p;
}

static int32_t fb_data_size(const fb_t* fb)
{
int32_t size = (int32_t)fb->width * fb->height;

    switch(fb->depth) {
        case DEPTH_1BPP:
            size >>= 3;
            break;
        case DEPTH_2BPP:
            size >>= 2;
            break;
        case DEPTH_4BPP:
            size >>= 1;
            break;
        case DEPTH_8BPP:
            break;
        default:
        case DEPTH_16BPP:
            size <<= 1;
            break;
    }
    return size;
}

static void LCD_write_data_raw(uint8_t data)
{
#if(LCD_MODE == 1)
    SPI_I2S_SendData(SPI2, data);
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
#else
    LCD_CS_CLR();
    for(uint8_t i = 0; i < 8; ++i) {
        LCD_SCLK_CLR();
        if(data & 0x80)
            LCD_MOSI_SET();
        else
            LCD_MOSI_CLR();
        LCD_SCLK_SET();
        data <<= 1;
    }
    LCD_CS_SET();    
#endif
}

static void LCD_write_data16_raw(uint16_t data)
{
    LCD_write_data_raw(data >> 8);
    LCD_write_data_raw(data);
}

static void LCD_write_data(uint8_t data)
{
#if(LCD_MODE == 1)
    LCD_CS_CLR();
#endif
    
    LCD_write_data_raw(data);

#if(LCD_MODE == 1)
    Delay_Us(1);
    LCD_CS_SET();
#endif
}

static void LCD_write_data16(uint16_t data)
{
    LCD_write_data(data >> 8);
    LCD_write_data(data);
}

static void LCD_write_reg(uint8_t data)
{
    LCD_DC_CLR();
    LCD_write_data(data);
    LCD_DC_SET();
}

static void LCD_setaddr(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
#if(LCD_ALIGN == 0)
	LCD_write_reg(0x2a);
	LCD_write_data16(x1);
	LCD_write_data16(x2);
	LCD_write_reg(0x2b);
	LCD_write_data16(y1);
	LCD_write_data16(y2);
	LCD_write_reg(0x2c);
#elif(LCD_ALIGN == 1)
	LCD_write_reg(0x2a);
	LCD_write_data16(x1);
	LCD_write_data16(x2);
	LCD_write_reg(0x2b);
	LCD_write_data16(y1 + 80);
	LCD_write_data16(y2 + 80);
	LCD_write_reg(0x2c);
#elif(LCD_ALIGN == 2)
	LCD_write_reg(0x2a);
	LCD_write_data16(x1);
	LCD_write_data16(x2);
	LCD_write_reg(0x2b);
	LCD_write_data16(y1);
	LCD_write_data16(y2);
	LCD_write_reg(0x2c);
#else
	LCD_write_reg(0x2a);
	LCD_write_data16(x1 + 80);
	LCD_write_data16(x2 + 80);
	LCD_write_reg(0x2b);
	LCD_write_data16(y1);
	LCD_write_data16(y2);
	LCD_write_reg(0x2c);
#endif
}

void lcd_init()
{
GPIO_InitTypeDef GPIO_InitStructure = { 0 };

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

#if(LCD_MODE == 1)
    SPI_InitTypeDef SPI_InitStructure = { 0 };
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    // DC (PB10) & RESET (PB11) & CS (PB12)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    //GPIO_SetBits(GPIOA, GPIO_Pin_2);

    // SPI_CLK
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // SPI_MISO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // SPI_MOSI
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // APB1_CLK (72MHz)
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);

    SPI_Cmd(SPI2, ENABLE);
#else
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

    // Reset
    //LCD_RES_CLR();
    Delay_Ms(10);
    LCD_RES_SET();
    Delay_Ms(10);
    LCD_BLK_SET();
    Delay_Ms(10);
    
    // SLPOUT: Sleep Out
    LCD_write_reg(0x11);
    Delay_Ms(10);

    // MADCTL: Memory Data Access Control
    LCD_write_reg(0x36);

#if(LCD_ALIGN == 0)
    LCD_write_data(0x00);
#elif(LCD_ALIGN == 1)
    LCD_write_data(0xC0);
#elif(LCD_ALIGN == 2)
    LCD_write_data(0x70);
#else
    LCD_write_data(0xA0);
#endif

    // COLMOD: Interface Pixel Format
    LCD_write_reg(0x3A);            
    LCD_write_data(0x05);

    // PORCTRL: Porch Setting
    LCD_write_reg(0xB2);            
    LCD_write_data(0x0C);
    LCD_write_data(0x0C); 
    LCD_write_data(0x00); 
    LCD_write_data(0x33); 
    LCD_write_data(0x33);             

    // GCTRL: Gate Control
    LCD_write_reg(0xB7);            
    LCD_write_data(0x35);

    // VCOMS: VCOM Setting
    LCD_write_reg(0xBB);            
    LCD_write_data(0x32); // Vcom=1.35V

    // VDVVRHEN: VDV and VRH Command Enable        
    LCD_write_reg(0xC2);
    LCD_write_data(0x01);

    // VRHS: VRH Set
    LCD_write_reg(0xC3);            
    LCD_write_data(0x15); // GVDD=4.8V

    // VDVS: VDV Set     
    LCD_write_reg(0xC4);            
    LCD_write_data(0x20); // VDV, 0x20:0v

    // FRCTRL2: Frame Rate Control in Normal Mode
    LCD_write_reg(0xC6);            
    LCD_write_data(0x0F); // 0x0F:60Hz            

    // PWCTRL1: Power Control 1
    LCD_write_reg(0xD0);
    LCD_write_data(0xA4);
    LCD_write_data(0xA1);

    // PVGAMCTRL: Positive Voltage Gamma Control
    LCD_write_reg(0xE0);
    LCD_write_data(0xD0);
    LCD_write_data(0x08);
    LCD_write_data(0x0E);
    LCD_write_data(0x09);
    LCD_write_data(0x09);
    LCD_write_data(0x05);
    LCD_write_data(0x31);
    LCD_write_data(0x33);
    LCD_write_data(0x48);
    LCD_write_data(0x17);
    LCD_write_data(0x14);
    LCD_write_data(0x15);
    LCD_write_data(0x31);
    LCD_write_data(0x34);

    // NVGAMCTRL: Negative Voltage Gamma Control
    LCD_write_reg(0xE1);
    LCD_write_data(0xD0);
    LCD_write_data(0x08);
    LCD_write_data(0x0E);
    LCD_write_data(0x09);
    LCD_write_data(0x09);
    LCD_write_data(0x15);
    LCD_write_data(0x31);
    LCD_write_data(0x33);
    LCD_write_data(0x48);
    LCD_write_data(0x17);
    LCD_write_data(0x14);
    LCD_write_data(0x15);
    LCD_write_data(0x31);
    LCD_write_data(0x34);
    LCD_write_reg(0x21);

    // DISPON: Display On
    LCD_write_reg(0x29);

    Delay_Ms(100);
}

void lcd_draw_fb(int16_t x, int16_t y, const fb_t* fb)
{
const uint16_t *palette = fb_palette_p(fb);
int16_t width = fb->width, height = fb->height;
int32_t n = (int32_t)width * height;
int32_t k;

    if(fb->flags & FB_DIRECT)
        return;

    LCD_setaddr(x, y, x + width - 1, y + height - 1);

#if(LCD_MODE == 1)
    LCD_CS_CLR();
#endif

    if(!palette) {
        const uint16_t *data = (const uint16_t*)fb_data_p(fb);
        for(k = 0; k < n; ++k)
		    LCD_write_data16_raw(*data++);
    }
    else {
        const uint8_t *data = (const uint8_t*)fb_data_p(fb);
        uint8_t b;
        switch(fb->depth) {
            case DEPTH_1BPP:
                n >>= 3;
                for(k = 0; k < n; ++k) {
                    b = *data++;
                    LCD_write_data16_raw(palette[b & 0x1]);
                    LCD_write_data16_raw(palette[(b >> 1) & 0x1]);
                    LCD_write_data16_raw(palette[(b >> 2) & 0x1]);
                    LCD_write_data16_raw(palette[(b >> 3) & 0x1]);
                    LCD_write_data16_raw(palette[(b >> 4) & 0x1]);
                    LCD_write_data16_raw(palette[(b >> 5) & 0x1]);
                    LCD_write_data16_raw(palette[(b >> 6) & 0x1]);
                    LCD_write_data16_raw(palette[(b >> 7) & 0x1]);
                }
                break;
            case DEPTH_2BPP:
                n >>= 2;
                for(k = 0; k < n; ++k) {
                    b = *data++;
                    LCD_write_data16_raw(palette[b & 0x3]);
                    LCD_write_data16_raw(palette[(b >> 2) & 0x3]);
                    LCD_write_data16_raw(palette[(b >> 4) & 0x3]);
                    LCD_write_data16_raw(palette[(b >> 6) & 0x3]);
                }
                break;
            case DEPTH_4BPP:
                n >>= 1;
                for(k = 0; k < (int32_t)width * height; ++k) {
                    b = *data++;
                    LCD_write_data16_raw(palette[b & 0xf]);
                    LCD_write_data16_raw(palette[(b >> 4) & 0xf]);
                }
                break;
            case DEPTH_8BPP:
                for(k = 0; k < n; ++k)
                    LCD_write_data16_raw(palette[*data++]);
                break;
        }
    }

#if(LCD_MODE == 1)
    Delay_Us(1);
    LCD_CS_SET();
#endif
}

void fb_fill(fb_t* fb, uint16_t color)
{
void *data = fb_data_p(fb);

    if(!data || fb->depth == DEPTH_16BPP)
        fb_fill_rect(fb, 0, 0, fb->width, fb->height, color);
    else {
        uint8_t b = color;

        switch(fb->depth) {
            case DEPTH_1BPP:
                b = (b ? 0xff : 0x0);
                break;
            case DEPTH_2BPP:
                b |= (b << 2) | (b << 4) | (b << 6);
                break;
            case DEPTH_4BPP:
                b |= b << 4;
                break;
        }
        memset(data, b, fb_data_size(fb));
    }
}

void fb_setpixel(fb_t* fb, int16_t x, int16_t y, uint16_t color)
{
int16_t width = fb->width, height = fb->height;
int32_t k = x + y * width;

    if(x < 0 || x >= width || y < 0 || y >= height)
        return;

    if(fb->flags & FB_DIRECT) {
        LCD_setaddr(x, y, x, y);

        #if(LCD_MODE == 1)
            LCD_CS_CLR();
        #endif

        LCD_write_data16_raw(color);

        #if(LCD_MODE == 1)
            Delay_Us(1);
            LCD_CS_SET();
        #endif
    }
    else if(fb->depth == DEPTH_16BPP) {
        *((uint16_t*)fb_data_p(fb) + k) = color;
    }
    else {
        uint8_t *data = (uint8_t*)fb_data_p(fb);
        uint8_t m, bo;

        switch(fb->depth) {
            case DEPTH_1BPP:
                bo = k & 0x7;
                k >>= 3;
                m = 0x1 << bo;
                if(color)
                    data[k] |= m;
                else
                    data[k] &= ~m;
                break;
            case DEPTH_2BPP:
                bo = (k & 0x3) << 1;
                k >>= 2;
                m = 0x3 << bo;
                data[k] = (data[k] & ~m) | (color << bo);
                break;
            case DEPTH_4BPP:
                bo = (k & 0x1) << 2;
                k >>= 1;
                m = 0xf << bo;
                data[k] = (data[k] & ~m) | (color << bo);
                break;
            case DEPTH_8BPP:
                data[k] = color;
                break;
        }
    }
}

void fb_draw_hline(fb_t* fb, int16_t x0, int16_t y0, int16_t length, uint16_t color)
{
int16_t x1 = x0 + length - 1;
int16_t width = fb->width, height = fb->height;
int16_t k;

    if(length <= 0 || y0 < 0 || y0 >= height || x0 >= width || x1 < 0)
        return;

    if(x0 < 0)
        x0 = 0;
    if(x1 >= width)
        x1 = width - 1;
    
    if(fb->flags & FB_DIRECT) {
        LCD_setaddr(x0, y0, x1, y0);

        #if(LCD_MODE == 1)
            LCD_CS_CLR();
        #endif

        for(k = 0; k < length; ++k)
            LCD_write_data16_raw(color);

        #if(LCD_MODE == 1)
            Delay_Us(1);
            LCD_CS_SET();
        #endif
    }
    else {
        while(x0 <= x1)
            fb_setpixel(fb, x0++, y0, color);
    }
}

void fb_draw_vline(fb_t* fb, int16_t x0, int16_t y0, int16_t length, uint16_t color)
{
int16_t y1 = y0 + length - 1;
int16_t width = fb->width, height = fb->height;
int16_t k;

    if(length <= 0 || x0 < 0 || x0 >= width || y0 >= height || y1 < 0)
        return;

    if(y0 < 0)
        y0 = 0;
    if(y1 >= height)
        y1 = height - 1;
    
    if(fb->flags & FB_DIRECT) {
        LCD_setaddr(x0, y0, x0, y1);

        #if(LCD_MODE == 1)
            LCD_CS_CLR();
        #endif

        for(k = 0; k < length; ++k)
            LCD_write_data16_raw(color);

        #if(LCD_MODE == 1)
            Delay_Us(1);
            LCD_CS_SET();
        #endif
    }
    else {
        while(y0 <= y1)
            fb_setpixel(fb, x0, y0++, color);
    }
}

void fb_draw_line(fb_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _int16_t_swap(x0, y0);
    _int16_t_swap(x1, y1);
  }

  if (x0 > x1) {
    _int16_t_swap(x0, x1);
    _int16_t_swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      fb_setpixel(fb, y0, x0, color);
    } else {
      fb_setpixel(fb, x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void fb_draw_circle(fb_t* fb, int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  //startWrite();
  fb_setpixel(fb, x0, y0 + r, color);
  fb_setpixel(fb, x0, y0 - r, color);
  fb_setpixel(fb, x0 + r, y0, color);
  fb_setpixel(fb, x0 - r, y0, color);

  while(x < y) {
    if(f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    fb_setpixel(fb, x0 + x, y0 + y, color);
    fb_setpixel(fb, x0 - x, y0 + y, color);
    fb_setpixel(fb, x0 + x, y0 - y, color);
    fb_setpixel(fb, x0 - x, y0 - y, color);
    fb_setpixel(fb, x0 + y, y0 + x, color);
    fb_setpixel(fb, x0 - y, y0 + x, color);
    fb_setpixel(fb, x0 + y, y0 - x, color);
    fb_setpixel(fb, x0 - y, y0 - x, color);
  }
  //endWrite();
}

void fb_draw_image(fb_t* fb, int16_t x0, int16_t y0, int16_t width, int16_t height, const uint16_t* im)
{
int32_t k;

    if(width <= 0 || height <= 0)
        return;

    LCD_setaddr(x0, y0, x0 + width - 1, y0 + height - 1);

#if(LCD_MODE == 1)
    LCD_CS_CLR();
#endif

    for(k = 0; k < (int32_t)width * height; ++k)
		LCD_write_data16_raw(*im++);

#if(LCD_MODE == 1)
    Delay_Us(1);
    LCD_CS_SET();
#endif
}

void fb_fill_rect(fb_t* fb, int16_t x0, int16_t y0, int16_t width, int16_t height, uint16_t color)
{
int16_t x1 = x0 + width - 1;
int16_t y1 = y0 + height - 1;
int32_t k;

    if(width <= 0 || height <= 0 || x0 >= LCD_WIDTH || x1 < 0 || y0 >= LCD_HEIGHT || y1 < 0)
        return;

    LCD_setaddr(x0, y0, x1, y1);

#if(LCD_MODE == 1)
    LCD_CS_CLR();
#endif

    for(k = 0; k < (int32_t)width * height; ++k)
		LCD_write_data16_raw(color);

#if(LCD_MODE == 1)
    Delay_Us(1);
    LCD_CS_SET();
#endif
}

static void _fb_fill_circle_helper(fb_t* fb, int16_t x0, int16_t y0, int16_t r, uint8_t corners, uint16_t delta, uint16_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        fb_draw_vline(fb, x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        fb_draw_vline(fb, x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        fb_draw_vline(fb, x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        fb_draw_vline(fb, x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

void fb_fill_circle(fb_t* fb, int16_t xc, int16_t yc, int16_t r, uint16_t color)
{
  //startWrite();
  fb_draw_vline(fb, xc, yc - r, 2 * r + 1, color);
  _fb_fill_circle_helper(fb, xc, yc, r, 3, 0, color);
  //endWrite();
}

void fb_putchar(fb_t* fb, int16_t x, int16_t y, char c, uint16_t font_color, uint16_t bg_color)
{
const uint8_t *b_row = font12x12_basic[(c > 32 && c < 127) ? ((uint8_t)c - 32) : 0];
uint8_t b, i, j;

    if(!c)
        return;
    
    LCD_setaddr(x, y, x + 11, y + 11);

#if(LCD_MODE == 1)
    LCD_CS_CLR();
#endif

    for(i = 0; i < 18; ++i) {
        b = b_row[i];
        for(j = 0; j < 8; ++j)
		    LCD_write_data16_raw(((b >> j) & 0x1) ? font_color : bg_color);
    }

#if(LCD_MODE == 1)
    Delay_Us(1);
    LCD_CS_SET();
#endif
}

void fb_puts(fb_t* fb, int16_t x, int16_t y, const char* s, uint16_t font_color, uint16_t bg_color)
{
    while(*s) {
        fb_putchar(fb, x, y, *s++, font_color, bg_color);
        x += 12;
    }
}
