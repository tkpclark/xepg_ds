
#include "bst_api.h"
#include "../ds/ts_buffer.h"
#include "lib.h"

static TBLUETOP_DEV_HANDLE DevHandle;
BST_TRANSMIT_CFG_EX DevConfig;
static char *p_map_ts;
static int logfd;
static char *mdname="TRSMIT";
static char logbuf[1024];
static int gstop=0;
static int galarm=0;

static void acquit(int signo)
{
	gstop=1;
	//proclog(logfd,"got quiting signal");
}
static void wakeup(int signo)
{
	;//proclog(logfd,"get data from heapfile");
}
static void acalarm(int signo)
{
	galarm=1;
	//proclog(logfd,mdname,"sigalarm\n");
}

static gen_empty_pkg(char *NULL_buf)
{
	memset(NULL_buf,0xFF,FRAME_LEN);
	NULL_buf[0]=0x47;
	NULL_buf[1]=0x1F;
	NULL_buf[2]=0xF0;
	NULL_buf[3]=0x10;
}
static void transmitting()
{
	char null_buffer[FRAME_LEN];
	gen_empty_pkg(null_buffer);
	unsigned int *p_rd_offs=(unsigned int *)(p_map_ts+TS_BUFFER_READER_OFFS);
	const unsigned int max_offs=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;
	if(*p_rd_offs>=max_offs)
	{
		*p_rd_offs=0;
	}
	unsigned char *p_start=p_map_ts+TS_BUFFER_CTL_LEN;

	sprintf(logbuf,"start from :TS_BUFFER_CTL_LEN+%d\n",*p_rd_offs);
	proclog(logfd,mdname,logbuf);

	
	/*
	double *p_rate=(double*)(p_map_ts+TS_BUFFER_ORI_RATE_OFFS);
	DevConfig.rate_ori=*p_rate;
	*/
	//DevConfig.rate_ori=10000000;
	bst_start_transmitEx(&DevHandle, &DevConfig, 0);
	
	sprintf(logbuf,"send rate:%f (bps)\n",DevConfig.rate_ori);
	proclog(logfd,mdname,logbuf);

	BUFFER_CAPABILITY bc;
	


	int ret;
	int counter;
	int counter1;
	int counter2;
	int data_num=0;
	
	struct itimerval value;
	value.it_value.tv_sec=0;
	value.it_value.tv_usec=100000;
	
	while(1)
	{
		if(gstop)
			exit(0);
		
		//
		/*
		if(*p_rate!=DevConfig.rate_ori)
		{
			
			bst_stop_transmit(&DevHandle);

			if ( bst_fini(&DevHandle) < 0 ) 
				proclog(logfd,mdname," bst fini failed!\n");
			else
				proclog(logfd,mdname," bst fini successfully!\n");



			sprintf(logbuf,"rate_ori changed! %f~%f(bps)\n",DevConfig.rate_ori,*p_rate);
			proclog(logfd,mdname,logbuf);
			DevConfig.rate_ori=*p_rate;

			ret = bst_initial(&DevHandle, 0);
			sprintf(logbuf,"bst_initial ret:%d\n",ret);
			proclog(logfd,mdname,logbuf);
			
			ret=bst_start_transmitEx(&DevHandle, &DevConfig, 3);
			sprintf(logbuf,"bst_start_transmitEx ret:%d\n",ret);
			proclog(logfd,mdname,logbuf);

		}
		*/
		
		setitimer(ITIMER_REAL, &value, NULL);
		counter=0;
		counter1=0;
		counter2=0;
		galarm=0;
		while(1)
		{
			
			//sprintf(logbuf,"counter:%d\n",counter);
			//proclog(logfd,mdname,logbuf);
			if(gstop)
				exit(0);
			if(*(p_start+*p_rd_offs)==0x47)
			{
				//proclog(logfd,mdname,"1\n");
				bst_transmit(&DevHandle, p_start+*p_rd_offs ,FRAME_LEN); 
				//proclog(logfd,mdname,"2\n");
				//memset(p_rd,0,FRAME_LEN);
				*(p_start+*p_rd_offs)=0;
				*p_rd_offs =*p_rd_offs+FRAME_LEN;

				if(*p_rd_offs >= max_offs)
				{
					sprintf(logbuf,"arrive to the max len:%d|%d\n",*p_rd_offs,max_offs);
					proclog(logfd,mdname,logbuf);
					*p_rd_offs=0;
				}
				counter1++;
			}
			else
			{
				//proclog(logfd,mdname,"3\n");
				bst_transmit(&DevHandle,null_buffer,FRAME_LEN);
				//proclog(logfd,mdname,"4\n");
				//sprintf(logbuf,"ts:%X\n",*(p_start+*p_rd_offs));
				//proclog(logfd,mdname,logbuf);
				//proclog(logfd,mdname,"no new data in the buffer\n");
				//nanosleep(&ts,NULL);
				counter2++;
			}
			
			
			counter++;
			if(counter >= (12000000/8/10/188)+1)
			{
				sprintf(logbuf,"task finished!1:%d 2:%d\n",counter1,counter2);
				proclog(logfd,mdname,logbuf);
				break;
			}
			
			bst_query(&DevHandle,BST_QUERY_DRV_DATA_SZ, &bc, sizeof(bc));
			//if(bc.DataBufSz-data_num <= 0)
			{
				sprintf(logbuf,"########data:%d,counter:%d \n",bc.DataBufSz,counter);
				proclog(logfd,mdname,logbuf);
			}
			
			data_num=bc.DataBufSz;
		}
		//proclog(logfd,mdname,"5\n");
		pause();
		//proclog(logfd,mdname,"6\n");
			
	}

}
static void procquit(void)
{
	//printf("quiting...\n");
	bst_stop_transmit(&DevHandle);
	
	if ( bst_fini(&DevHandle) < 0 ) 
		proclog(logfd,mdname," bst fini failed!\n");
	else
		proclog(logfd,mdname," bst fini successfully!\n");

}
main()
{
	int ret;
	//register signal
	struct sigaction signew;
	
	signew.sa_handler=acquit;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
  	sigaction(SIGINT,&signew,0);
	
	signew.sa_handler=acquit;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
	sigaction(SIGTERM,&signew,0);
	
	signew.sa_handler=wakeup;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
	sigaction(SIGUSR1,&signew,0);


	signew.sa_handler=acalarm;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
	sigaction(SIGALRM,&signew,0);
	
	//atexit
	if(atexit(&procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}
	
	//init mmap
	char tsmmap[128];
	read_config("ts_buffer",tsmmap);
	p_map_ts=init_mmap_use(tsmmap);
	if(p_map_ts==NULL)
	{
		printf("memory map failed!\n");
		exit(0);
	}

	//open log
	char logfile[128];
	read_config("logfile",logfile);
	logfd=open(logfile,O_CREAT|O_WRONLY|O_APPEND,0600);
	if(logfd==-1)
	{
		printf("open logfile error!\n");
		exit(0);
	}
	
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
	
	
	DevConfig.version=3;
	DevConfig.rate=20.0e+6;
	DevConfig.rate_ori=10.0e+6;
	DevConfig.frame_length=188;
	DevConfig.frame_length_ori=188;
	//DevConfig.spi_asi_select=INTERFACE_ASI_SPI;
	DevConfig.spi_asi_select=INTERFACE_ASI;
	DevConfig.burst_enable=1;
	DevConfig.reference_in_enable=RFCLK_IN_INNER;
	DevConfig.reference_out_enable=RFCLK_OUT_DISABLE;
	DevConfig.raw_data=0;
	DevConfig.pcr_adj=0;

	//bst_start_transmitEx(&DevHandle, &DevConfig, 0);

	/*
	{
		3,   //version
		20.0e+6,//rate
		20.0e+6, //rate_ori
		188, //frame_length
		188, //frame_length
		INTERFACE_ASI_SPI,//only asi or both asi and spi
		ASI_MODE_CONTINUE, //burst_enable  0:continous  1:burst
		RFCLK_IN_INNER, 
		RFCLK_OUT_DISABLE, 
		0,//ts format 
		0
	*/


	//transmitting
	transmitting();
	

	
}

