#include "debug.h"
#include "lcd.h"
#include "font12x12_basic.h"

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

void LCD_write_data_raw(uint8_t data)
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

void LCD_write_data16_raw(uint16_t data)
{
    LCD_write_data_raw(data >> 8);
    LCD_write_data_raw(data);
}

void LCD_write_data(uint8_t data)
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

void LCD_write_data16(uint16_t data)
{
    LCD_write_data(data >> 8);
    LCD_write_data(data);
}

void LCD_write_reg(uint8_t data)
{
    LCD_DC_CLR();
    LCD_write_data(data);
    LCD_DC_SET();
}

void LCD_setaddr(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
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

int lcd_init()
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

    //LCD_RES_CLR();
    Delay_Ms(100);

    LCD_RES_SET();
    Delay_Ms(100);
    
    LCD_BLK_SET();
    Delay_Ms(100);
    
    // Start Initial Sequence
    LCD_write_reg(0x11);
    Delay_Ms(120);

    //
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

    LCD_write_reg(0x3A);            
    LCD_write_data(0x05);

    LCD_write_reg(0xB2);            
    LCD_write_data(0x0C);
    LCD_write_data(0x0C); 
    LCD_write_data(0x00); 
    LCD_write_data(0x33); 
    LCD_write_data(0x33);             

    LCD_write_reg(0xB7);            
    LCD_write_data(0x35);

    LCD_write_reg(0xBB);            
    LCD_write_data(0x32); // Vcom=1.35V
                    
    LCD_write_reg(0xC2);
    LCD_write_data(0x01);

    LCD_write_reg(0xC3);            
    LCD_write_data(0x15); // GVDD=4.8V
                
    LCD_write_reg(0xC4);            
    LCD_write_data(0x20); // VDV, 0x20:0v

    LCD_write_reg(0xC6);            
    LCD_write_data(0x0F); // 0x0F:60Hz            

    LCD_write_reg(0xD0);
    LCD_write_data(0xA4);
    LCD_write_data(0xA1);

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

    LCD_write_reg(0x29);

    Delay_Ms(100);

    return 0;
}

void lcd_fill(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color)
{
uint16_t k;

    LCD_setaddr(x, y, (uint16_t)x + width - 1, (uint16_t)y + height - 1);

#if(LCD_MODE == 1)
    LCD_CS_CLR();
#endif

    for(k = 0; k < (uint16_t)width * height; ++k)
		LCD_write_data16_raw(color);

#if(LCD_MODE == 1)
    Delay_Us(1);
    LCD_CS_SET();
#endif
}

void lcd_blit(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint16_t* im)
{
uint16_t k;

    LCD_setaddr(x, y, (uint16_t)x + width - 1, (uint16_t)y + height - 1);

#if(LCD_MODE == 1)
    LCD_CS_CLR();
#endif

    for(k = 0; k < (uint16_t)width * height; ++k)
		LCD_write_data16_raw(*im++);

#if(LCD_MODE == 1)
    Delay_Us(1);
    LCD_CS_SET();
#endif
}

void lcd_set_char(uint8_t row, uint8_t col, char c, uint16_t font_color, uint16_t bg_color)
{
const uint8_t *b_row = font12x12_basic[(c > 32 && c < 127) ? ((uint8_t)c - 32) : 0];
uint8_t b, i, j;

    if(!c)
        return;
    
    LCD_setaddr(((uint16_t)col) * 12, ((uint16_t)row) * 12, (((uint16_t)col + 1) * 12) - 1, (((uint16_t)row + 1) * 12) - 1);

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

void lcd_puts(uint8_t row, uint8_t col, const char* s, uint16_t font_color, uint16_t bg_color)
{

    while(*s) {
        lcd_set_char(row, col++, *s++, font_color, bg_color);
    }
}
