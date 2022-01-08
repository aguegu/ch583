#ifndef __LCD_INIT_H
#define __LCD_INIT_H

#include "CH58x_common.h"

#define USE_HORIZONTAL 1  //���ú�������������ʾ 0��1Ϊ���� 2��3Ϊ����


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 80
#define LCD_H 160

#else
#define LCD_W 160
#define LCD_H 80
#endif



//-----------------LCD�˿ڶ���---------------- 

#define LCD_SCLK_Clr() GPIOA_ResetBits(GPIO_Pin_13)//SCL=SCLK
#define LCD_SCLK_Set() GPIOA_SetBits(GPIO_Pin_13)

#define LCD_MOSI_Clr() GPIOA_ResetBits(GPIO_Pin_14)//SDA=MOSI
#define LCD_MOSI_Set() GPIOA_SetBits(GPIO_Pin_14)

#define LCD_RES_Clr()  GPIOA_ResetBits(GPIO_Pin_0)//RES
#define LCD_RES_Set()  GPIOA_SetBits(GPIO_Pin_0)

#define LCD_DC_Clr()   GPIOA_ResetBits(GPIO_Pin_6)//DC
#define LCD_DC_Set()   GPIOA_SetBits(GPIO_Pin_6)
 		     
#define LCD_CS_Clr()   GPIOA_ResetBits(GPIO_Pin_5)//CS
#define LCD_CS_Set()   GPIOA_SetBits(GPIO_Pin_5)

#define LCD_BLK_Clr()  GPIOA_ResetBits(GPIO_Pin_4)//BLK
#define LCD_BLK_Set()  GPIOA_SetBits(GPIO_Pin_4)




void LCD_GPIO_Init(void);//��ʼ��GPIO
void LCD_WR_buf(uint8_t *dat, uint16_t len);

void LCD_Writ_Bus(uint8_t dat);//ģ��SPIʱ��
void LCD_WR_DATA8(uint8_t dat);//д��һ���ֽ�
void LCD_WR_DATA(uint16_t dat);//д�������ֽ�
void LCD_WR_REG(uint8_t dat);//д��һ��ָ��
void LCD_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2);//�������꺯��
void LCD_Init(void);//LCD��ʼ��
#endif




