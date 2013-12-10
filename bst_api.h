
/******************************************************************************
*  Copyright (C) 2000-2009, Bluetop Tech. Co., Ltd. 
*  All Rights Reserved 
* 
*  Bluetop Technology Co., Ltd. (BLUETOP) expressly claims any warranty 
*  for this software.  BLUETOP licenses this software under our specific 
*  terms and conditions.  Use of any of the software or derivatives thereof 
*  in any product without BLUETOP's OFFICAL LICENSE is strictly prohibited.
* 
*  THIS SOFTWARE IS PROVIDED FOR BLUETOP INTERNEL DEVELOPMENT ONLY. 
*
*  THE ENTIRE RISK OF ANY KIND ARISING FROM THE USE OF THIS SOFTWARE, 
*  EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, *  REMAINS WITH YOU. 
* 
*  Bluetop Technology Co., Ltd. 
*  No.10 HuaYuanDongLu, Haidian District, Beijing, P.R.C. 
*  7/F,Golder Plaza TEL:+86-10-82030550
*
* 
*  MODULE: 
*         API header
*    
* 
*  DESCRIPTION:
* 
*        API header
* 
* FUNCTION LIST: 
* 
*      Function Name and Description 
* 
*  HISTORY:  NULL
* 
*      <Author>             <Date>      <Version>  < Modification >
*      zhaogongchen    20090210         1.0        Initial version
* 
*****************************************************************************/


#ifndef _BST_API_H_
#define _BST_API_H_

//Debug if NDEBUG is undefined
#undef	NDEBUG

#ifndef	HANDLE
#define	HANDLE	int
#endif
#ifndef	INVALID_HANDLE_VALUE
#define	INVALID_HANDLE_VALUE	((HANDLE) -1)
#endif
#ifndef	UCHAR
#define	UCHAR	unsigned char
#endif
#ifndef	USHORT
#define	USHORT	unsigned short
#endif
#ifndef	ULONG
#define	ULONG	unsigned long
#endif
#ifndef	BOOL
#define	BOOL	unsigned char
#endif
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE 	0
#endif


#define	BST_DEVICE_MAX_NUM  8
#define	BST_DEVICE_MAX_CH   2

#define	DeviceId_IsValid(DevId)     ((DevId) >=0 && (DevId) < BST_DEVICE_MAX_NUM)
#define	DeviceId_IsInvalid(DevId)   ((DevId) < 0 || (DevId) >=BST_DEVICE_MAX_NUM)
#define	ChannelId_IsValid(ChId)     ((ChId ) >=0 && (ChId ) < BST_DEVICE_MAX_CH )
#define	ChannelId_IsInvalid(ChId)   ((ChId ) < 0 || (ChId ) >=BST_DEVICE_MAX_CH )

#define	FRAME_LEN_RAW   0
#define	FRAME_LEN_188   188
#define	FRAME_LEN_204   204

#define	INTERFACE_ASI_SPI   0
#define	INTERFACE_ASI   1
#define	INTERFACE_SPI   2

#define	ASI_MODE_CONTINUE   0
#define	ASI_MODE_PACKET_BURST	    1
#define	SPI_MODE_CONTINUE	    0

#define	RFCLK_IN_INNER  0
#define	RFCLK_IN_OUTER	    1

#define	RFCLK_OUT_DISABLE   0
#define	RFCLK_OUT_ENABLE    1

#define	PCR_RESTAMP_DISABLE 0
#define	PCR_RESTAMP_ENABLE  1

#define	BST_TRANSMIT_CFG_EX_V3  3

#define	BLUETOP_RET_OK	    0
#define	BLUETOP_ERR_InvalidParameter	    -1
#define	BLUETOP_ERR_UnknownDrvVersion   -2
#define	BLUETOP_ERR_UnknownHwVersion    -3
#define	BLUETOP_ERR_DeviceOpened    -4
#define	BLUETOP_ERR_NeedNewerDrv    -5
#define	BLUETOP_ERR_DevInitialFailure   -6
#define	BLUETOP_ERR_InvaildChannelId    -7
#define	BLUETOP_ERR_InvaildDeviceId     -8
#define	BLUETOP_ERR_OpenDeviceFailure   -10
#define	BLUETOP_ERR_Unsuccessful    -1000

#define	TX_CTRL_IDLE    0
#define	TX_CTRL_HOLD    1
#define	TX_CTRL_UNDEFINED   2
#define	TX_CTRL_SEND    3

#define	BST_QUERY_VERSION   4
#define 	BST_QUERY_HW_VERSION    4
#define 	BST_QUERY_DRV_DATA_SZ	    5
#define 	BST_QUERY_DRV_VERSION   6
#define 	BST_QUERY_EXT_COLOCK    7
#define 	BST_QUERY_DEV_LOCATION  8


typedef struct
{
	unsigned char  DeviceId;		// 发送卡设备编号,0开始
	unsigned char  ChannelId;		// 发送卡通道编号,0开始
	unsigned short Reserved;		// 保留
	HANDLE handle;				// 设备句柄
} TBLUETOP_DEV_HANDLE, *BLUETOP_DEV_HANDLE;

#ifndef	HAVE_BUFFER_CAPABILITY
typedef struct _BUFFER_CAPABILITY
{
	unsigned long DataBufSz;				/* 缓冲的数据量    */
	unsigned long TotalBufSz;				/* 数据缓冲区的大小*/
} BUFFER_CAPABILITY,*PBUFFER_CAPABILITY;
#define	HAVE_BUFFER_CAPABILITY
#endif

// Device Location Structure
#ifndef	HAVE_DEVICE_LOCATION
typedef struct _DEVICE_LOCATION
{
	unsigned char  BusNumber;
	unsigned char  DeviceNumber;		// 5 bits
	unsigned char  FunctionNumber;		// 3 bits
	unsigned char  SlotNumber;
	unsigned short VendorID;
	unsigned short DeviceID;
	unsigned char  RevisionID;
	unsigned char  Reserved;
	unsigned short SubVendorID;		// Subsystem Vendor ID
	unsigned short SubDeviceID;			// Subsystem Device ID
	unsigned char  SerialNumber[16];
} DEVICE_LOCATION, *PDEVICE_LOCATION;
#define	HAVE_DEVICE_LOCATION
#endif


typedef struct _BST_TRANSMIT_CFG_EX
{
	int version;
	double  rate;						//发送卡输出信号的速率,bps为单位			
	double  rate_ori;						//TS流的原始码率
	unsigned	frame_length    :8;			//发送卡输出TS流的包长度,188或204
	unsigned	frame_length_ori    :8;			//TS流的原始包长度,188或204
	unsigned	spi_asi_select  :2;			//发送卡输出接口选择
											// 	0表示SPI和ASI接口同时输出,1表示只有ASI接口输出,2表示只有SPI接口输出
	unsigned	burst_enable    :1;			//ASI 包突发模式使能（只有在ASI接口输出模式下才可以使能该位）
											//  0表示使用ASI连续模式, 1表示ASI包突发模式
	unsigned	reference_in_enable :1;			//外部时钟基准输入使能位
											//  0表示使用内部时钟基准作为码流发送时钟参考,可任意设定码流速率
											//  1表示使用外部输入时钟作为码流发送字节时钟,不能任意调整码流速率,即该情况下rate设置无效
	unsigned	reference_out_enable    :1;			//时钟输出使能位
											//  0表示禁止时钟输出, 1表示允许码流字节时钟输出
	unsigned	raw_data    :1;			//原始数据输出
											//  0表示输出TS流 , 1表示输出原始格式数据
	unsigned	pcr_adj :1;			//pcr 调整允许位
											//  0表示关闭PCR调整 , 1表示启动PCR调整
} BST_TRANSMIT_CFG_EX, *PBST_TRANSMIT_CFG_EX;	
// Error code definition

typedef struct _BST_TRANSMIT_CFG
{
	float   rate;						//发送卡输出信号的速率,bps为单位			
	int frame_length;					//发送卡输出TS流的包长度,188或204
	int spi_asi_select;				//0表示SPI和ASI接口同时输出,1表示只有ASI接口输出,2表示只有SPI接口输出
	int burst_enable;					//ASI 包突发模式使能（只有在ASI接口输出模式下才可以使能该位）
										//  0表示使用ASI连续模式, 1表示ASI包突发模式
	int reference_in_enable;			//	外部时钟基准输入使能位
										//  0表示使用内部时钟基准作为码流发送时钟参考,可任意设定码流速率
										//  1表示使用外部输入时钟作为码流发送字节时钟,不能任意调整码流速率,即该情况下rate设置无效
	int reference_out_enable;			//	时钟输出使能位
										//  0表示禁止时钟输出, 1表示允许码流字节时钟输出
} BST_TRANSMIT_CFG,*PBST_TRANSMIT_CFG;	

int bst_initial(BLUETOP_DEV_HANDLE DevHandle,int downld);

int bst_fini(BLUETOP_DEV_HANDLE DevHandle);

int bst_flush(BLUETOP_DEV_HANDLE DevHandle);

int bst_query(BLUETOP_DEV_HANDLE DevHandle, int nQueryId,void* pQueryBuf,int nBufLen);

int bst_start_transmitEx(BLUETOP_DEV_HANDLE   DevHandle,PBST_TRANSMIT_CFG_EX pConfig,int version );

int bst_initial_dev_handle(BLUETOP_DEV_HANDLE DevHandle, int DeviceId, int ChannelId);

int bst_transmit(BLUETOP_DEV_HANDLE DevHandle, void* pBuffer, int nLength);

int bst_stop_transmit(BLUETOP_DEV_HANDLE DevHandle);

int bst_get_device_num(BLUETOP_DEV_HANDLE DevHandle);

int bst_get_device_channel_num(BLUETOP_DEV_HANDLE DevHandle);

unsigned long bst_get_api_version();

#endif
