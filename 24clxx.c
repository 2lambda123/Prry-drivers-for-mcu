/*
 * File      : 24clxx.c
 * 
 * This file is part of eeprom driver for 24clxx,such as at24c02/at24c16.
 * COPYRIGHT (C) 2016
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-10-22     Acuity      first version.
 * 2017-03-27     Acuity			Add 16bit address for eeprom,to test at24c64. 
 * 2017-05-09     Acuity			�޸�ҳд�㷨.
 *
 * Depend on:
 * i2c_core.c,i2c_hw.c
 *
 * Note:
 * 24c04 24c08 24c16 ��ַΪ8λ������0xff��ַ��ע��ҳ��ѡ���д
 * 24c32 �����ϵ�ַΪ16λ
 * EEPROM ҳ��д��Ҫ��Ӧ���ѯ����������ʱ�Եȴ�����д����ɣ�FM24CLXX����Ҫ
 */
 

#define USE_24CLXX_EN 		1
#define USE_24CLXX_DEBUG 	0


#if USE_24CLXX_EN
#include "i2c_core.h"
#include "i2c_hw.h"
#include "24clxx.h"	

//eeprom/fram ����
#define		EEPROM_TYPE							0				//0->EEPROM 1->FRAM
#define		EEPROM_MODEL						16			//EEPROM ���� 24c16
#define 	EE24CLXX_SLAVE_ADDR   	0x50		//ע���дλ��ʵ�ʵ�ַΪ0x50
#define 	EE24CLXX_PAGESIZE       16			//AT24C16ÿҳ��16���ֽ�   24C02->8

static void I2C_24CLXXWaitStandby(u8 slave_addr);

//ҳд��ʱ��FRAM����
static void I2C_24CLXXWaitStandby(u8 slave_addr) 
{
#if EEPROM_TYPE
	
#else
		u16 i;
		i = 0xFFFF;
		while (i--);
#endif
}

//д���ֽڣ���ȷ����ǰд��ַ+д�����ݳ��Ȳ��ܳ���EEPROM��һҳ
static void i2c_24clxx_write( u16 write_addr, char* pbuffer,u16 write_size)
{
		struct i2c_dev_message ee24_msg[2];
		u8	buf[2];
		u8  slave_addr;

		//if(write_addr+write_size > EE24CLXX_PAGESIZE)
		//		return;
		if(EEPROM_MODEL > 16)
		{//����2k�ֽ�ʱ����ַΪ16λ��24c16������
				slave_addr = EE24CLXX_SLAVE_ADDR;
				buf[0] = (write_addr >>8)& 0xff;
				buf[1] = write_addr & 0xff;
				ee24_msg[0].size  = 2;
		}
		else
		{//24c01����24c16
				slave_addr = EE24CLXX_SLAVE_ADDR | (write_addr>>8);
				buf[0] = write_addr & 0xff;
				ee24_msg[0].size  = 1;
		}
		ee24_msg[0].addr = slave_addr;
		ee24_msg[0].flags = I2C_BUS_WR;
		ee24_msg[0].buff  = buf;
		ee24_msg[1].addr = slave_addr;
		ee24_msg[1].flags = I2C_BUS_WR  | I2C_BUS_NO_START;	//
		ee24_msg[1].buff  = (u8*)pbuffer;
		ee24_msg[1].size  = write_size;
		i2c_bus_xfer(&i2c1_dev,ee24_msg,2);
}

/*******************************************************
**
**											 �ⲿ����
**
*******************************************************/

//дһ�ֽ�
char ee_24clxx_writebyte(u16 addr,u8 data)
{
		struct i2c_dev_message ee24_msg[2];
		u8	buf[3];
		u8  slave_addr;
	
		if(EEPROM_MODEL > 16)
		{//����2k�ֽ�ʱ����ַΪ16λ
				slave_addr = EE24CLXX_SLAVE_ADDR;
				buf[0] = (addr >>8)& 0xff;   //��λ��ַ��ǰ
				buf[1] = addr & 0xff;
				buf[2] = data;
				ee24_msg[0].size  = 3;
		}
		else
		{
				slave_addr = EE24CLXX_SLAVE_ADDR | (addr>>8);
				buf[0] = addr & 0xff;
				buf[1] = data;
				ee24_msg[0].size  = 2;
		}
		ee24_msg[0].addr = slave_addr;
		ee24_msg[0].flags = I2C_BUS_WR;
		ee24_msg[0].buff  = buf;
		i2c_bus_xfer(&i2c1_dev,ee24_msg,1);
		
		return 0;
}

//д���ֽڣ���ҳд�㷨
char ee_24clxx_writebytes(u16 write_addr, char* pwrite_buff, u16 writebytes)
{//20170509
		u8   write_len,page_offset;
		char error = 0;
		u16  check_writes,check_addr;
		char *check_buf;
	
		check_writes= writebytes;
		check_addr  = write_addr;
		check_buf   = pwrite_buff;
		while(writebytes > 0)
		{
				page_offset = EE24CLXX_PAGESIZE - (write_addr & (EE24CLXX_PAGESIZE-1));//write_current_addr%EE24CLXX_PageSize
				write_len   = writebytes > page_offset ? page_offset : writebytes;
				i2c_24clxx_write(write_addr,pwrite_buff, write_len);
				writebytes   = writebytes - write_len;
				if(writebytes > 0)
				{
						pwrite_buff = pwrite_buff + write_len;
						write_addr  = write_addr + write_len;
						I2C_24CLXXWaitStandby(0);
				}
		}
		I2C_24CLXXWaitStandby(0);
		{//У������
				int i;
				char checkdata;
			
				for(i = 0;i < check_writes;i++)
				{
						ee_24clxx_readbytes(check_addr+i,&checkdata,1);	
						if(checkdata != check_buf[i])
						{
								error = 1;
								break;
						}
				}
		}
		return error;
}

//�����ֽڣ�������
void ee_24clxx_readbytes(u16 read_ddr, char* pbuffer, u16 read_size)
{  
		struct i2c_dev_message ee24_msg[2];
		u8	buf[2];
		u8  slave_addr;
	
		if(EEPROM_MODEL > 16)
		{//����2k�ֽ�ʱ����ַΪ16λ
				slave_addr = EE24CLXX_SLAVE_ADDR;
				buf[0] = (read_ddr >>8)& 0xff;
				buf[1] = read_ddr & 0xff;
				ee24_msg[0].size  = 2;
		}
		else
		{
				slave_addr = EE24CLXX_SLAVE_ADDR | (read_ddr>>8);
				buf[0] = read_ddr & 0xff;
				ee24_msg[0].size  = 1;
		}
		ee24_msg[0].buff  = buf;
		ee24_msg[0].addr  = slave_addr;
		ee24_msg[0].flags = I2C_BUS_WR;
		ee24_msg[1].addr  = slave_addr;
		ee24_msg[1].flags = I2C_BUS_RD;
		ee24_msg[1].buff  = (u8*)pbuffer;
		ee24_msg[1].size  = read_size;
		i2c_bus_xfer(&i2c1_dev,ee24_msg,2);
}

//����EEPROM
char ee_24clxx_erasebytes(u16 WriteAddr, char Erasedata, u16 NumByteToErase)
{
		char error = 0;
		u16 i;
		//char *buff;
		char buff[2048];	//�����ã�ʵ��ʹ���趯̬�����ڴ�
		
		//buff = (char*)malloc(2048);
		for(i = 0;i < NumByteToErase;i++)
		{
				buff[i] = Erasedata;
		}
		error = ee_24clxx_writebytes(WriteAddr,buff,NumByteToErase);				
		//free(buff);
		return error;
}

//eeprom test
#if USE_24CLXX_DEBUG
#define		WRSIZE							2048
#define 	EEP_Firstpage      	0
char I2c_Buf_Write[WRSIZE];
char I2c_Buf_Read[WRSIZE];
void EE24CLXX_Test(void)
{
		u16 i,error = 0;
			
		for ( i=0; i<WRSIZE; i++ ) //��仺��
		{   
				I2c_Buf_Write[i] = i;   
		}
		ee_24clxx_readbytes(EEP_Firstpage, I2c_Buf_Read, WRSIZE); 
		ee_24clxx_readbytes(EEP_Firstpage, I2c_Buf_Read, WRSIZE);
		error = ee_24clxx_writebytes(EEP_Firstpage, I2c_Buf_Write, WRSIZE);	 
		Delayms(5);
		ee_24clxx_readbytes(EEP_Firstpage, I2c_Buf_Read, WRSIZE); 

		for ( i=0; i<WRSIZE; i++ ) //��仺�� 
				I2c_Buf_Write[i] = 0;   
		error = ee_24clxx_writebytes(EEP_Firstpage, I2c_Buf_Write, WRSIZE);
		Delayms(5);		
		ee_24clxx_readbytes(EEP_Firstpage, I2c_Buf_Read,WRSIZE); 
		//while(error);
}
#endif

#endif
