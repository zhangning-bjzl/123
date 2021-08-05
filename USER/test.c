#include "sys.h"
#include "usart.h"		
#include "delay.h"
#include "led.h"
#include "lcd.h"
#include "myiic.h"
#include "myrc522.h"
# include <stdio.h>

//////////////////////////////////////////////////////////
//M1卡分為16個扇區，每個扇區由4塊（塊0、塊1、塊2、塊3）組成
//我們也將16個扇區的64個塊按絕對地址編號0~63
//第0扇區的塊0（即絕對地址0塊），他用於存放廠商代碼，已經固化，不可更改
//每個扇區的塊0、塊1、塊2為數據塊，可用於存放數據
//每個扇區的塊3為控制塊（絕對地址塊3、7、11....），包括了密碼A，存取控制、密碼B。








/*******************************
*连线说明：
*IRQ<------>悬空
*SDA<----->PB11
*SCL<----->PB10
*RST <----->PB1
*GND <----->GND
*VCC <----->VCC
************************************/
#define LED0 PBout(5)
#define BEEP PBout(8)

void IO()
{
    RCC->APB2ENR|=1<<3;//先使能外设IO PORTB时钟 	
    
	GPIOB->CRL&=0XFF0FFF0F;//PB1、5 推挽输出        //PB1 == rc522复位
	GPIOB->CRL|=0X00300030;	   

    GPIOB->ODR|=1<<1; 
    GPIOB->ODR|=1<<5; 
    
	GPIOB->CRH&=0XFFFFFFF0;//PB8 推挽输出
	GPIOB->CRH|=0X00000003;	   
	GPIOB->ODR|=0<<8;     //PB8 输出低
}




int main()
{
	char cStatus;
  u8 g_ucTempbuf[20];
	u8 DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	u8 ucArray_ID[4];
	char cStr [ 30 ];
//Stm32_Clock_Init(9);	//系统时钟设置
  uart_init(9600);	 	//串口初始化为9600
	delay_init();	   	 	//延时初始化

  LED_Init();
	LCD_Init();
  POINT_COLOR=RED;
	LCD_Clear(WHITE);
	IIC_Init();
  IO();

	PcdReset();
  M500PcdConfigISOType();
	PcdAntennaOn(0);
	delay_ms(2);
	PcdAntennaOn(1);
	printf ("reset ok\r\n");

	while(1)
	{
			cStatus = PcdRequest(PICC_REQALL, ucArray_ID);
		  if(cStatus !=MI_OK)
			{
					cStatus = PcdRequest(PICC_REQALL, ucArray_ID);			  				
			}
			if(cStatus == MI_OK)
			{ 
				  sprintf ( cStr, "The Card Type is: %02X%02X", ucArray_ID [ 0 ], ucArray_ID [ 1 ] );
          printf ("%s\r\n",cStr ); 
					if ( PcdAnticoll ( ucArray_ID ) == MI_OK )
					{
							sprintf ( cStr, "The Card ID is: %02X%02X%02X%02X", ucArray_ID [ 0 ], ucArray_ID [ 1 ], ucArray_ID [ 2 ], ucArray_ID [ 3 ] );				
							printf ("%s\r\n",cStr ); 
					}

			}
	}	 
}
