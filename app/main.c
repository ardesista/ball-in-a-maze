/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*******************************************************************************/
#include "debug.h"
#include "lcd.h"
#include "gImage_test.h"

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
void GPIO_Toggle_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

int main(void)
{
    int i = 0;
    char str[] = "      @@@@@@@@      ";

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	Delay_Init();
	USART_Printf_Init(115200); // PA9 default
	// printf("SystemClk:%lu\r\n", SystemCoreClock);
	// printf("WCH ^V^\r\n");
    
    // // scanf("%d", &i);
    // // printf("%d\r\n", i);
    // char buf[33];
    // i = read(0, buf, 33);
    // buf[i] = 0;
    // puts(buf);


    GPIO_Toggle_init();



    lcd_init();

    // lcd_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLUE);
    // Delay_Ms(250);
    // lcd_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_RED);
    // Delay_Ms(250);
    // lcd_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    // Delay_Ms(250);
    lcd_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_WHITE);
    // Delay_Ms(250);
    // Delay_Ms(250);
    // Delay_Ms(250);

    lcd_blit(0, 0, gImage_test_WIDTH, gImage_test_HEIGHT, gImage_test);

    while(1) {
        char c = str[19];
        for(i = 19; i > 0; --i)
            str[i] = str[i - 1];
        str[0] = c;

        lcd_puts(17, 0, str, COLOR_BLACK, COLOR_WHITE);

        Delay_Ms(100);
    }

	while(1);
}
