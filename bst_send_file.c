#include "bst_api.h"
#include "ts_buffer.h"
#include "ds.h"

static TBLUETOP_DEV_HANDLE DevHandle;
BST_TRANSMIT_CFG_EX DevConfig;


static void procquit(void)
{
	//printf("quiting...\n");
	bst_stop_transmit(&DevHandle);
	
	if ( bst_fini(&DevHandle) < 0 ) 
		printf(" bst fini failed!\n");
	else
		printf(" bst fini successfully!\n");

}
static int isNITPack(unsigned char *p)
{
	struct timeval tv;
	static double nit_write_time_last;
	double nit_write_time;
	unsigned char section_header_flag=0;
	section_header_flag=*(unsigned char *)(p+1)  & 0x40;
	unsigned short pid=get_pid(p);
	
	
	if(pid==16 && (section_header_flag==0x40) &&(*(unsigned char *)(p+5) ==0x40) &&(*(unsigned char *)(p+0xb) ==0x00) )
	{
		printf("p+1:0x%x,and:0x%x\n",*(unsigned char *)(p+1),section_header_flag);
		gettimeofday (&tv, NULL);
		nit_write_time=tv.tv_sec + (double)tv.tv_usec / 1000000;
		printf("NIT seciton time:%f  ,interval:%f\n",nit_write_time,nit_write_time-nit_write_time_last);
		nit_write_time_last=nit_write_time;
		print_HEX(p,20);
		return 1;
	}
	return 0;
}
main(int argc,char **argv)
{
	int ret;
	
	DevConfig.version=3;
	DevConfig.rate=100000000;
	DevConfig.rate_ori=100000000;
	DevConfig.frame_length=FRAME_LEN;
	DevConfig.frame_length_ori=FRAME_LEN;
	//DevConfig.spi_asi_select=INTERFACE_ASI_SPI;
	DevConfig.spi_asi_select=INTERFACE_ASI;
	DevConfig.burst_enable=0;
	DevConfig.reference_in_enable=RFCLK_IN_INNER;
	DevConfig.reference_out_enable=RFCLK_OUT_DISABLE;
	DevConfig.raw_data=0;
	DevConfig.pcr_adj=0;
	
	
	printf("rate:%f \nrate_ori:%f\nburst_enable:%d\n", DevConfig.rate,DevConfig.rate_ori,DevConfig.burst_enable);

	ret=bst_start_transmitEx(&DevHandle, &DevConfig, 0);
	
	
	//init transmit
	bst_initial_dev_handle(&DevHandle, 0, 0);
	ret = bst_initial(&DevHandle, 0);
	if (ret < 0)
	{
		printf("bst_initial failed!\n");
		exit(0);
	}
	printf("bst_initial successfully!\n");
	
	ret = bst_get_device_num(&DevHandle);
	
	if (ret <=0) 
	{
		printf("bst_get_device_num failed!\n");
		exit(0);

	}
	printf("Found %d BST device(s)!\n",ret);
	
	
	ret=bst_start_transmitEx(&DevHandle, &DevConfig, 0);
	
	if (ret < 0) 
	{
		printf("bst_start_transmitEx failed!\n");
		exit(0);

	}
	printf("bst_start_transmitEx successfully\n");
	
	
	int fd;
	char buffer[188*1000];
	fd=open(argv[1],0);
	if(fd<0)
	{
		printf("FAILED TO OPEN!\n");
		exit(0);
	}
	int n;
	int num=0;
	while(1)
	{
		printf("start from the beginning!\n");
		
		lseek(fd,0,SEEK_SET);
		while(1)//one time of sending file
		{
			//memset(buffer,0,sizeof(buffer));
			n=read(fd,buffer,188);
			
			if(!n)
				break;
			num++;
			/*
			if(isNITPack(buffer))
				printf("num:%d\n",num);
			*/
			//my_nano_sleep(10000000);
			
			
			if(bst_transmit(&DevHandle, buffer ,188)<0)
			{
				printf("transmit error\n");

			}
			
			//isNITPack(buffer);
		}
	}
}

