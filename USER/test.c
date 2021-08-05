#include "sys.h"
#include "usart.h"		
#include "delay.h"
#include "led.h"
#include "lcd.h"
#include "myiic.h"
#include "myrc522.h"
# include <stdio.h>

//////////////////////////////////////////////////////////
//M1���֞�16���ȅ^��ÿ���ȅ^��4�K���K0���K1���K2���K3���M��
//�҂�Ҳ��16���ȅ^��64���K���^����ַ��̖0~63
//��0�ȅ^�ĉK0�����^����ַ0�K��������춴�ŏS�̴��a���ѽ��̻������ɸ���
//ÿ���ȅ^�ĉK0���K1���K2�锵���K������춴�Ŕ���
//ÿ���ȅ^�ĉK3����ƉK���^����ַ�K3��7��11....�����������ܴaA����ȡ���ơ��ܴaB��








/*******************************
*����˵����
*IRQ<------>����
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
    RCC->APB2ENR|=1<<3;//��ʹ������IO PORTBʱ�� 	
    
	GPIOB->CRL&=0XFF0FFF0F;//PB1��5 �������        //PB1 == rc522��λ
	GPIOB->CRL|=0X00300030;	   

    GPIOB->ODR|=1<<1; 
    GPIOB->ODR|=1<<5; 
    
	GPIOB->CRH&=0XFFFFFFF0;//PB8 �������
	GPIOB->CRH|=0X00000003;	   
	GPIOB->ODR|=0<<8;     //PB8 �����
}




int main()
{
	char cStatus;
  u8 g_ucTempbuf[20];
	u8 DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	u8 ucArray_ID[4];
	char cStr [ 30 ];
//Stm32_Clock_Init(9);	//ϵͳʱ������
  uart_init(9600);	 	//���ڳ�ʼ��Ϊ9600
	delay_init();	   	 	//��ʱ��ʼ��

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
