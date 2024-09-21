#include "ch32v30x.h"
#include "adxl345.h"
#include "debug.h"
#include <stdint.h>

#define ADXL345_DEVICE   0x53   // Device Address for ADXL345
#define ADXL345_TO_READ  6      // Number of Bytes Read - Two Bytes Per Axis

static void _i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, reg);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1, val);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

static void _i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t count, uint8_t* buf)
{
uint8_t i;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, reg);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(i = 0; i < count; ++i) {
        while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
        buf[i] = I2C_ReceiveData(I2C1);
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
}

void adxl345_init()
{
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    I2C_InitTypeDef I2C_InitStructure = { 0 };

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Power on
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
    Delay_Ms(10);

    //AFIO->PCFR1 |= AFIO_PCFR1_I2C1_REMAP;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);

    // B8 I2C1 SCL
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // B9 I2C1 SDA
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 100KHz master
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9; // I2C_DutyCycle_16_9
    I2C_InitStructure.I2C_OwnAddress1 = 0;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    // Init
    _i2c_write_reg(ADXL345_DEVICE, ADXL345_POWER_CTL, 0);	// Wakeup     
	_i2c_write_reg(ADXL345_DEVICE, ADXL345_POWER_CTL, 16);	// Auto_Sleep
	_i2c_write_reg(ADXL345_DEVICE, ADXL345_POWER_CTL, 8);	// Measure
}

void adxl345_read_accel(int16_t* x, int16_t* y, int16_t* z)
{
    uint8_t buf[6];

	_i2c_read_reg(ADXL345_DEVICE, ADXL345_DATAX0, 6, buf);	// Read Accel Data from ADXL345
	
	// Each Axis @ All g Ranges: 10 Bit Resolution (2 Bytes)
	*x = (((int16_t)buf[1]) << 8) | buf[0];   
	*y = (((int16_t)buf[3]) << 8) | buf[2];
	*z = (((int16_t)buf[5]) << 8) | buf[4];
}
