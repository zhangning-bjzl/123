#include "myrc522.h"
#include "delay.h"
#include "myiic.h"	




//��RC522�Ĵ���
//DZ:�Ĵ�����ַ
//���أ�������ֵ

u8 R_RC522(u8 DZ)
{						   
	u8 DAT=0;	
    DAT = RC522_RD_Reg(SLA_ADDR,  DZ);
	return DAT;          		//�����յ�������
}

//��    �ܣ�дRC522�Ĵ���
//����˵����DZ:�Ĵ�����ַ
//          DATA:д���ֵ

void W_RC522(u8 DZ,u8 DATA)
{
    RC522_WR_Reg(SLA_ADDR,  DZ, DATA);
}

//��    �ܣ���RC522�Ĵ���λ
//����˵����DZ:�Ĵ�����ַ
//          mask:��λֵ
void SetBitMask(u8 DZ,u8 mask)
{
    u8 tmp = 0x0;
    tmp = R_RC522(DZ);
	delay_ms(2);
    W_RC522(DZ,tmp | mask);	
}

//��    �ܣ���RC522�Ĵ���λ
//����˵����DZ:�Ĵ�����ַ
//          mask:��λֵ
void ClearBitMask(u8 DZ,u8 mask)  
{
    u8 tmp = 0x0;
    tmp = R_RC522(DZ);
	delay_ms(2);
	tmp = tmp&(~mask);
    W_RC522(DZ, tmp);  // clear bit mask
} 

//��    �ܣ���λRC522
// 
void PcdReset(void)
{
//    RC522_RST=0;
//    delay_ms(10);
//    RC522_RST=1;
//    delay_ms(10);  //0x01     / 0x0F //��λ
    W_RC522(CommandReg,PCD_RESETPHASE);//
    delay_ms(200);
                //0x11
    W_RC522(ModeReg,0x3D); //��Mifare��ͨѶ��CRC��ʼֵ0x6363
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

//�������� 
//off:1��������0���ر� 
//ÿ��������ر����շ���֮��Ӧ������1ms�ļ��
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
//����RC632�Ĺ�����ʽ 
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

//��MF522����CRC16����
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
				  // 0x05/�ж������־
        n = R_RC522(DivIrqReg);//��RC632�Ĵ���
        i--;
    }
    while ((i!=0) && !(n&0x04));
		
    pOutData[0] = R_RC522(CRCResultRegL);									
    pOutData[1] = R_RC522(CRCResultRegM);
}

//��    �ܣ�ͨ��RC522��ISO14443��ͨѶ
//����˵����Command[IN]:RC522������          
//          pInData[IN]:ͨ��RC522���͵���Ƭ������
//          InLenByte[IN]:�������ݵ��ֽڳ���        
//          pOutData[OUT]:���յ��Ŀ�Ƭ��������         
//          *pOutLenBit[OUT]:�������ݵ�λ����
//�ɹ���OKλ1Ϊ1������Ϊ0
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
       case PCD_AUTHENT://0x0E//��֤��Կ
          irqEn   = 0x12;
          waitFor = 0x10;
          break;
       case PCD_TRANSCEIVE://0x0C//���Ͳ���������
          irqEn   = 0x77;
          waitFor = 0x30;
          break;
       default:
         break;
    }
   //			0x02/�ж�����ʹ������
    W_RC522(ComIEnReg,irqEn|0x80);//д�Ĵ���
//				0x04/�ж������־
    ClearBitMask(ComIrqReg,0x80);
//				0x01				0x00//ȡ����ǰ����
    W_RC522(CommandReg,PCD_IDLE);
//				0x0Aָʾ�洢��FIFO�е��ֽ���
    SetBitMask(FIFOLevelReg,0x80);
   
    for (i=0; i<InLenByte; i++)
    {   
		W_RC522(FIFODataReg, pInData[i]);    }
//					0x01			//
		W_RC522(CommandReg, Command);
   
    //				0x0C//���Ͳ���������
    if (Command == PCD_TRANSCEIVE)
    {    //			 0x0D/����֡����
			SetBitMask(BitFramingReg,0x80);//���ã���������  
	 }
    
//    i = 600;//����ʱ��Ƶ�ʵ���������M1�����ȴ�ʱ��25ms
 i = 10000;
    do 
    {
		 
	//				0x04/�жϱ�־
         n = R_RC522(ComIrqReg);
         i--;
    }//								0x30
    while ((i!=0) && !(n&0x01) && !(n&waitFor));
//				0x0D/����֡����
    ClearBitMask(BitFramingReg,0x80);//����Ĵ���λ���رշ���
	      
    if (i!=0)
    {    		//	0x06/�����־
         if(!(R_RC522(ErrorReg)&0x1B))//���Ĵ������������־
         {
             cStatus = MI_OK;
					  
             if (n & irqEn & 0x01)
				//		(-1)	
             {   cStatus = MI_NOTAGERR;   }

             if (Command == PCD_TRANSCEIVE)//�ж����������Ƿ��ͽ���
             {//					0x0A/ָʾ�洢��FIFO�е��ֽ���
               	n = R_RC522(FIFOLevelReg);//��ȡFIFO������������ֽ���
					//							0x0C/�����Ƕ�ʱ������
              	lastBits = R_RC522(ControlReg) & 0x07;//��ʾ�����յ�
																		//���ֽ���Чλ������
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
                {   //					0X09/FIFO������
						pOutData[i] = R_RC522(FIFODataReg);//��ȡ����������
				}
            }
         }
         else///     
         {   cStatus = MI_ERR;   }
        
   }
   
//				0x0C
   SetBitMask(ControlReg,0x80);           // stop timer now
//					0x01/������ֹͣ��0x00//ȡ����ǰ����
   W_RC522(CommandReg,PCD_IDLE);
   return cStatus;	 
}

 //��    �ܣ�Ѱ��
//����˵��: req_code[IN]:Ѱ����ʽ
//                0x52 = Ѱ��Ӧ�������з���14443A��׼�Ŀ�
//                0x26 = Ѱδ��������״̬�Ŀ�
//          pTagType[OUT]����Ƭ���ʹ���
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//�ɹ���OKλ2Ϊ1������Ϊ0
/////////////////////////////////////////////////////////////////////
char PcdRequest(u8 req_code,u8 *pTagType)
{
	 char cStatus;
   u16 unLen;
   u8 ucComMF522Buf[18]; 

   ClearBitMask(Status2Reg,0x08);//��Ĵ���λ 
   W_RC522(BitFramingReg,0x07);//д�Ĵ���
   SetBitMask(TxControlReg,0x03);//�üĴ���λ 
 
   ucComMF522Buf[0] = req_code;

   cStatus = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);//ͨ��RC522��ISO14443��ͨѶ
   if ((cStatus == MI_OK) && (unLen == 0x10))
   {    
       *pTagType     = ucComMF522Buf[0];
       *(pTagType+1) = ucComMF522Buf[1];
   }
   else
   {   cStatus = MI_ERR;   }
   
	 return cStatus;
}

//��    �ܣ�����ײ
//����˵��: pSnr[OUT]:��Ƭ���кţ�4�ֽ�
//�ɹ���OKλ3Ϊ1������Ϊ0
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

//							  0x93 //����ײ
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;
//						 0x0C//���Ͳ���������
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

//��    �ܣ�ѡ����Ƭ
//����˵��: pSnr[IN]:��Ƭ���кţ�4�ֽ�
//�ɹ���OKλ4Ϊ1������Ϊ0
/////////////////////////////////////////////////////////////////////
char PcdSelect(u8 *pSnr)
{

    u8 i;
    u16 unLen;
    u8 ucComMF522Buf[18];
	  char cStatus;
    //					  0x93//����ײ
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
//
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);//��MF522����CRC16����

 
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

//��    �ܣ���֤��Ƭ����
//����˵��: auth_mode[IN]: ������֤ģʽ
//                 0x60 = ��֤A��Կ
//                 0x61 = ��֤B��Կ 
//          addr[IN]�����ַ
//          pKey[IN]������
//          pSnr[IN]����Ƭ���кţ�4�ֽ�
//�ɹ���OKλ5Ϊ1������Ϊ0
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

//��    �ܣ���ȡM1��һ������
//����˵��: addr[IN]�����ַ
//          pData[OUT]�����������ݣ�16�ֽ�
//�ɹ���OKλ6Ϊ1������Ϊ0
///////////////////////////////////////////////////////////////////// 
char PcdRead(u8 addr,u8 *pData)
{

    u16  unLen;
    u8 i,ucComMF522Buf[18];
	  char cStatus; 

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;

    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);//��MF522����CRC16����
   
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

//��    �ܣ�д���ݵ�M1��һ��
//����˵��: addr[IN]�����ַ
//          pData[IN]��д������ݣ�16�ֽ�
//�ɹ���OKλ7Ϊ1������Ϊ0
/////////////////////////////////////////////////////////////////////                  
char PcdWrite(u8 addr,u8 *pData)
{
    char cStatus;
    u16  unLen;
    u8 i,ucComMF522Buf[18]; 
    
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);//��MF522����CRC16����
 
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



//д�Ĵ���
//addr:�Ĵ�����ַ
//val:Ҫд���ֵ
//����ֵ:��
void RC522_WR_Reg(u8 RCsla,u8 addr,u8 val) 
{
	IIC_Start();  				 
	IIC_Send_Byte(RCsla);     	//����д����ָ��	 
	IIC_Wait_Ack();	   
    IIC_Send_Byte(addr);   			//���ͼĴ�����ַ
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(val);     		//����ֵ					   
	IIC_Wait_Ack();  		    	   
    IIC_Stop();						//����һ��ֹͣ���� 	   
}
//���Ĵ���
//addr:�Ĵ�����ַ
//����ֵ:������ֵ
u8 RC522_RD_Reg(u8 RCsla,u8 addr) 		
{
	u8 temp=0;		 
	IIC_Start();  				 
	IIC_Send_Byte(RCsla);	//����д����ָ��	 
	temp=IIC_Wait_Ack();	   
    IIC_Send_Byte(addr);   		//���ͼĴ�����ַ
	temp=IIC_Wait_Ack(); 	 										  		   
	IIC_Start();  	 	   		//��������
	IIC_Send_Byte(RCsla+1);	//���Ͷ�����ָ��	 
	temp=IIC_Wait_Ack();	   
    temp=IIC_Read_Byte(0);		//��ȡһ���ֽ�,�������ٶ�,����NAK 	    	   
    IIC_Stop();					//����һ��ֹͣ���� 	    
	return temp;				//���ض�����ֵ
}  


