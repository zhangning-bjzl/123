#include "myrc522.h"
#include "delay.h"
#include "myiic.h"	




//读RC522寄存器
//DZ:寄存器地址
//返回：读出的值

u8 R_RC522(u8 DZ)
{						   
	u8 DAT=0;	
    DAT = RC522_RD_Reg(SLA_ADDR,  DZ);
	return DAT;          		//返回收到的数据
}

//功    能：写RC522寄存器
//参数说明：DZ:寄存器地址
//          DATA:写入的值

void W_RC522(u8 DZ,u8 DATA)
{
    RC522_WR_Reg(SLA_ADDR,  DZ, DATA);
}

//功    能：置RC522寄存器位
//参数说明：DZ:寄存器地址
//          mask:置位值
void SetBitMask(u8 DZ,u8 mask)
{
    u8 tmp = 0x0;
    tmp = R_RC522(DZ);
	delay_ms(2);
    W_RC522(DZ,tmp | mask);	
}

//功    能：清RC522寄存器位
//参数说明：DZ:寄存器地址
//          mask:清位值
void ClearBitMask(u8 DZ,u8 mask)  
{
    u8 tmp = 0x0;
    tmp = R_RC522(DZ);
	delay_ms(2);
	tmp = tmp&(~mask);
    W_RC522(DZ, tmp);  // clear bit mask
} 

//功    能：复位RC522
// 
void PcdReset(void)
{
//    RC522_RST=0;
//    delay_ms(10);
//    RC522_RST=1;
//    delay_ms(10);  //0x01     / 0x0F //复位
    W_RC522(CommandReg,PCD_RESETPHASE);//
    delay_ms(200);
                //0x11
    W_RC522(ModeReg,0x3D); //和Mifare卡通讯，CRC初始值0x6363
	delay_ms(5);
					//0x2D
    W_RC522(TReloadRegL,30);
	delay_ms(5);           
					//0x2C
    W_RC522(TReloadRegH,0);
	delay_ms(5);
					// 0x2A
    W_RC522(TModeReg,0x8D);
	delay_ms(5);
					// 0x2B
    W_RC522(TPrescalerReg,0x3E);
	delay_ms(5);
					//0x15
    W_RC522(TxAutoReg,0x40);     
    
}

//开启天线 
//off:1、开启。0、关闭 
//每次启动或关闭天险发射之间应至少有1ms的间隔
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn(u8 off)
{
    u8 i;
	if(off&1)
	{
	    i = R_RC522(TxControlReg);
	    if (!(i & 0x03))
	    {
	        SetBitMask(TxControlReg, 0x03);
	    }
	}
	else {ClearBitMask(TxControlReg, 0x03);}
}

//////////////////////////////////////////////////////////////////////
//设置RC632的工作方式 
//////////////////////////////////////////////////////////////////////
void M500PcdConfigISOType()
{
   ClearBitMask(Status2Reg,0x08);
   W_RC522(ModeReg,0x3D);//3F
   W_RC522(RxSelReg,0x86);//84
   W_RC522(RFCfgReg,0x7F);   //4F
   W_RC522(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
   W_RC522(TReloadRegH,0);
   W_RC522(TModeReg,0x8D);
   W_RC522(TPrescalerReg,0x3E);
   delay_ms(10);
//   PcdAntennaOn(1);
}

//用MF522计算CRC16函数
/////////////////////////////////////////////////////////////////////
void CalulateCRC(u8 *pIndata,u8 len,u8 *pOutData)
{
    u8 i,n;
			 
    ClearBitMask(DivIrqReg,0x04);
    W_RC522(CommandReg,PCD_IDLE);
    SetBitMask(FIFOLevelReg,0x80);
	
    for (i=0; i<len; i++)
    {  
	 	W_RC522(FIFODataReg, *(pIndata+i));
    }
 		W_RC522(CommandReg, PCD_CALCCRC);                          
  
		i = 50;
    do 
    {	
		delay_ms(1);
				  // 0x05/中断请求标志
        n = R_RC522(DivIrqReg);//读RC632寄存器
        i--;
    }
    while ((i!=0) && !(n&0x04));
		
    pOutData[0] = R_RC522(CRCResultRegL);									
    pOutData[1] = R_RC522(CRCResultRegM);
}

//功    能：通过RC522和ISO14443卡通讯
//参数说明：Command[IN]:RC522命令字          
//          pInData[IN]:通过RC522发送到卡片的数据
//          InLenByte[IN]:发送数据的字节长度        
//          pOutData[OUT]:接收到的卡片返回数据         
//          *pOutLenBit[OUT]:返回数据的位长度
//成功置OK位1为1，否则为0
/////////////////////////////////////////////////////////////////////
char PcdComMF522(u8 Command,u8 *pInData,u8 InLenByte,u8 *pOutData,u16  *pOutLenBit)
{
    u8 irqEn   = 0x00;
    u8 waitFor = 0x00;
    u8 lastBits;
    u8 n;
    u16 i;
	  char cStatus;
	
    switch (Command)
    {
       case PCD_AUTHENT://0x0E//验证密钥
          irqEn   = 0x12;
          waitFor = 0x10;
          break;
       case PCD_TRANSCEIVE://0x0C//发送并接收数据
          irqEn   = 0x77;
          waitFor = 0x30;
          break;
       default:
         break;
    }
   //			0x02/中断请求使、禁能
    W_RC522(ComIEnReg,irqEn|0x80);//写寄存器
//				0x04/中断请求标志
    ClearBitMask(ComIrqReg,0x80);
//				0x01				0x00//取消当前命令
    W_RC522(CommandReg,PCD_IDLE);
//				0x0A指示存储在FIFO中的字节数
    SetBitMask(FIFOLevelReg,0x80);
   
    for (i=0; i<InLenByte; i++)
    {   
		W_RC522(FIFODataReg, pInData[i]);    }
//					0x01			//
		W_RC522(CommandReg, Command);
   
    //				0x0C//发送并接收数据
    if (Command == PCD_TRANSCEIVE)
    {    //			 0x0D/面向帧调节
			SetBitMask(BitFramingReg,0x80);//作用：开启发送  
	 }
    
//    i = 600;//根据时钟频率调整，操作M1卡最大等待时间25ms
 i = 10000;
    do 
    {
		 
	//				0x04/中断标志
         n = R_RC522(ComIrqReg);
         i--;
    }//								0x30
    while ((i!=0) && !(n&0x01) && !(n&waitFor));
//				0x0D/面向帧调节
    ClearBitMask(BitFramingReg,0x80);//清除寄存器位：关闭发送
	      
    if (i!=0)
    {    		//	0x06/错误标志
         if(!(R_RC522(ErrorReg)&0x1B))//读寄存器：检测错误标志
         {
             cStatus = MI_OK;
					  
             if (n & irqEn & 0x01)
				//		(-1)	
             {   cStatus = MI_NOTAGERR;   }

             if (Command == PCD_TRANSCEIVE)//判断输入命令是否发送接收
             {//					0x0A/指示存储在FIFO中的字节数
               	n = R_RC522(FIFOLevelReg);//读取FIFO缓存区保存的字节数
					//							0x0C/可能是定时器控制
              	lastBits = R_RC522(ControlReg) & 0x07;//显示最后接收到
																		//的字节有效位个数。
                if (lastBits)
                {   *pOutLenBit = (n-1)*8 + lastBits;   }
                else
                {   *pOutLenBit = n*8;   }
                if (n == 0)
                {   n = 1;    }
//								18
                if (n > 18)
                {   n = 18;   }
                for (i=0; i<n; i++)
                {   //					0X09/FIFO缓存区
						pOutData[i] = R_RC522(FIFODataReg);//读取缓存区数据
				}
            }
         }
         else///     
         {   cStatus = MI_ERR;   }
        
   }
   
//				0x0C
   SetBitMask(ControlReg,0x80);           // stop timer now
//					0x01/启动、停止。0x00//取消当前命令
   W_RC522(CommandReg,PCD_IDLE);
   return cStatus;	 
}

 //功    能：寻卡
//参数说明: req_code[IN]:寻卡方式
//                0x52 = 寻感应区内所有符合14443A标准的卡
//                0x26 = 寻未进入休眠状态的卡
//          pTagType[OUT]：卡片类型代码
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//成功置OK位2为1，否则为0
/////////////////////////////////////////////////////////////////////
char PcdRequest(u8 req_code,u8 *pTagType)
{
	 char cStatus;
   u16 unLen;
   u8 ucComMF522Buf[18]; 

   ClearBitMask(Status2Reg,0x08);//清寄存器位 
   W_RC522(BitFramingReg,0x07);//写寄存器
   SetBitMask(TxControlReg,0x03);//置寄存器位 
 
   ucComMF522Buf[0] = req_code;

   cStatus = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);//通过RC522和ISO14443卡通讯
   if ((cStatus == MI_OK) && (unLen == 0x10))
   {    
       *pTagType     = ucComMF522Buf[0];
       *(pTagType+1) = ucComMF522Buf[1];
   }
   else
   {   cStatus = MI_ERR;   }
   
	 return cStatus;
}

//功    能：防冲撞
//参数说明: pSnr[OUT]:卡片序列号，4字节
//成功置OK位3为1，否则为0
/////////////////////////////////////////////////////////////////////  
char PcdAnticoll(u8 *pSnr)
{
    char cStatus;
    u8 i,snr_check=0;
    u16  unLen;
    u8 ucComMF522Buf[18]; 

    ClearBitMask(Status2Reg,0x08);
    W_RC522(BitFramingReg,0x00);
    ClearBitMask(CollReg,0x80);

//							  0x93 //防冲撞
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;
//						 0x0C//发送并接收数据
    cStatus = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);
    
    if (cStatus == MI_OK)
    {
    	 for (i=0; i<4; i++)
         {   
             *(pSnr+i)  = ucComMF522Buf[i];
             snr_check ^= ucComMF522Buf[i];
         }
		   if(snr_check != ucComMF522Buf[i])
			 {
					cStatus = MI_ERR;
			 }
    }
    
    SetBitMask(CollReg,0x80);
    return cStatus;
}

//功    能：选定卡片
//参数说明: pSnr[IN]:卡片序列号，4字节
//成功置OK位4为1，否则为0
/////////////////////////////////////////////////////////////////////
char PcdSelect(u8 *pSnr)
{

    u8 i;
    u16 unLen;
    u8 ucComMF522Buf[18];
	  char cStatus;
    //					  0x93//防冲撞
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
//
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);//用MF522计算CRC16函数

 
    ClearBitMask(Status2Reg,0x08);

    cStatus = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((cStatus == MI_OK) && (unLen == 0x18))
    {   
				cStatus = MI_OK;
		}
		else
		{
				cStatus = MI_ERR;
		}
		
		return cStatus;
 
}

//功    能：验证卡片密码
//参数说明: auth_mode[IN]: 密码验证模式
//                 0x60 = 验证A密钥
//                 0x61 = 验证B密钥 
//          addr[IN]：块地址
//          pKey[IN]：密码
//          pSnr[IN]：卡片序列号，4字节
//成功置OK位5为1，否则为0
/////////////////////////////////////////////////////////////////////               
char PcdAuthState(u8 auth_mode,u8 addr,u8 *pKey,u8 *pSnr)
{
    char cStatus;
    u16  unLen;
    u8 i,ucComMF522Buf[18]; 
	
    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+2] = *(pKey+i);   }
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
 
    
    cStatus = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
	
    if ((cStatus != MI_OK) || (!(R_RC522(Status2Reg) & 0x08)))
    {   cStatus = MI_ERR;   }
    
		return cStatus;
    
}

//功    能：读取M1卡一块数据
//参数说明: addr[IN]：块地址
//          pData[OUT]：读出的数据，16字节
//成功置OK位6为1，否则为0
///////////////////////////////////////////////////////////////////// 
char PcdRead(u8 addr,u8 *pData)
{

    u16  unLen;
    u8 i,ucComMF522Buf[18];
	  char cStatus; 

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;

    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);//用MF522计算CRC16函数
   
    cStatus = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((cStatus == MI_OK) && (unLen == 0x90))
    {		
        for (i=0; i<16; i++)
        {    *(pData+i) = ucComMF522Buf[i];   }
    }
		else
		cStatus = MI_ERR;
		
		return cStatus;
}

//功    能：写数据到M1卡一块
//参数说明: addr[IN]：块地址
//          pData[IN]：写入的数据，16字节
//成功置OK位7为1，否则为0
/////////////////////////////////////////////////////////////////////                  
char PcdWrite(u8 addr,u8 *pData)
{
    char cStatus;
    u16  unLen;
    u8 i,ucComMF522Buf[18]; 
    
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);//用MF522计算CRC16函数
 
    cStatus = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
        
    if((cStatus != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
		{
				cStatus = MI_ERR;
		}
		if(cStatus == MI_OK)
		{
			for(i=0;i<16;i++)
			{
					ucComMF522Buf[i] = *(pData+i);
			}
			CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);
			cStatus = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
			if((cStatus != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
			{
					cStatus = MI_ERR;
			}
		}
		return cStatus;
}



//写寄存器
//addr:寄存器地址
//val:要写入的值
//返回值:无
void RC522_WR_Reg(u8 RCsla,u8 addr,u8 val) 
{
	IIC_Start();  				 
	IIC_Send_Byte(RCsla);     	//发送写器件指令	 
	IIC_Wait_Ack();	   
    IIC_Send_Byte(addr);   			//发送寄存器地址
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(val);     		//发送值					   
	IIC_Wait_Ack();  		    	   
    IIC_Stop();						//产生一个停止条件 	   
}
//读寄存器
//addr:寄存器地址
//返回值:读到的值
u8 RC522_RD_Reg(u8 RCsla,u8 addr) 		
{
	u8 temp=0;		 
	IIC_Start();  				 
	IIC_Send_Byte(RCsla);	//发送写器件指令	 
	temp=IIC_Wait_Ack();	   
    IIC_Send_Byte(addr);   		//发送寄存器地址
	temp=IIC_Wait_Ack(); 	 										  		   
	IIC_Start();  	 	   		//重新启动
	IIC_Send_Byte(RCsla+1);	//发送读器件指令	 
	temp=IIC_Wait_Ack();	   
    temp=IIC_Read_Byte(0);		//读取一个字节,不继续再读,发送NAK 	    	   
    IIC_Stop();					//产生一个停止条件 	    
	return temp;				//返回读到的值
}  


