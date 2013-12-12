
///transforing data in ts buffer with most hight speed till to the write postion////

#define MAX_PID_NUM 500

#include "bst_api.h"
#include "ts_buffer.h"
#include "ds.h"

static TBLUETOP_DEV_HANDLE DevHandle;
BST_TRANSMIT_CFG_EX DevConfig;
static char *p_map_ts;
static int logfd;
static char *mdname="TRSMIT";
static char logbuf[1024];
static int gstop=0;
static int galarm=0;

typedef struct
{
	unsigned short pid;
	unsigned char counter;
}TSCOUNTER;

TSCOUNTER tscourter[MAX_PID_NUM];

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
	proclog(logfd,mdname,"sigalarm\n");
}

static updater_counter(char *p)
{
	int i;
	unsigned short pid=0;
	pid=get_pid(p);
	for(i=0;i<MAX_PID_NUM;i++)
	{
		if( (tscourter[i].pid==0x1FF0) || (tscourter[i].pid==pid) )
		{
			if(tscourter[i].pid==0x1FF0) 
				tscourter[i].pid=pid;
			*(unsigned char *)(p+3)=(*(unsigned char *)(p+3)&0xF0)+tscourter[i].counter;
			if(tscourter[i].counter==0xF)
			{
				tscourter[i].counter=0;
			}
			else
			{
				tscourter[i].counter++;
			}
			
			break;
		}
	}
}
static void transmitting()
{
	//get null pkg
	char null_buffer[FRAME_LEN];
	gen_empty_pkg(null_buffer);
	
	unsigned int *p_rd_offs=(unsigned int *)(p_map_ts+TS_BUFFER_READER_OFFS);
	unsigned int *p_wr_offs=(unsigned int *)(p_map_ts+TS_BUFFER_WRITER_OFFS);

	//*p_rd_offs=*p_wr_offs;//read from the writer's position
	
	const unsigned int max_offs=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;

	sprintf(logbuf,"start from :TS_BUFFER_CTL_LEN+%d\n",*p_rd_offs);
	proclog(logfd,mdname,logbuf);
	
	unsigned char *p_ts=p_map_ts+TS_BUFFER_CTL_LEN;
	//unsigned char *p_ds_wflag=p_map_ts+TS_BUFFER_DS_WFLAG_OFFS;
	BUFFER_CAPABILITY bc;

	/*
	struct itimerval value;
	value.it_value.tv_sec=0;
	value.it_value.tv_usec=100000;
	*/
	char time[32];
	struct timeval tv;
	while(1)
	{
		if(gstop)
			exit(0);
		
		if(*p_rd_offs==*p_wr_offs)
		{
			sprintf(logbuf,"rear-end:*p_rd_offs==*p_wr_offs==%d",*p_rd_offs);
			proclog(logfd,mdname,logbuf);
			my_nano_sleep(5000000);

		}
		else
		{
			updater_counter(p_ts+*p_rd_offs);
			alarm(5);
			if(bst_transmit(&DevHandle, p_ts+*p_rd_offs ,FRAME_LEN)<0)
			{
				sprintf(logbuf,"transmit error at %d\n",*p_rd_offs);
				proclog(logfd,mdname,logbuf);
				print_HEX(p_ts+*p_rd_offs, FRAME_LEN);
			}
			alarm(0);
			*p_rd_offs =*p_rd_offs+FRAME_LEN;

			if(*p_rd_offs >= max_offs)
			{
				//sprintf(logbuf,"arrive to the max len:%d|%d\n",*p_rd_offs,max_offs);
				//proclog(logfd,mdname,logbuf);
				*p_rd_offs=0;
			}
		}
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

	int i;
	for(i=0;i<MAX_PID_NUM;i++)
	{
		tscourter[i].counter=0;
		tscourter[i].pid=0x1FF0;
	}
	//atexit
	if(atexit(&procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}
	
	//init mmap
	char tsmmap[128];
	read_config("ts_buffer",tsmmap);

	//create mmap
	p_map_ts=init_mmap0(tsmmap, TS_BUFFER_MAX_LEN);
	if(p_map_ts==NULL)
	{
		printf("%s init failed!\n",tsmmap);
		
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


	char rate[32]={0};
	char rate_ori[32]={0};
	char burst_enable[8];
	read_config("rate", rate);
	read_config("rate_ori", rate_ori);
	read_config("burst_enable",burst_enable);
	
	DevConfig.version=3;
	DevConfig.rate=atof(rate);
	DevConfig.rate_ori=atof(rate_ori);
	DevConfig.frame_length=FRAME_LEN;
	DevConfig.frame_length_ori=FRAME_LEN;
	//DevConfig.spi_asi_select=INTERFACE_ASI_SPI;
	DevConfig.spi_asi_select=INTERFACE_ASI;
	DevConfig.burst_enable=atoi(burst_enable);
	DevConfig.reference_in_enable=RFCLK_IN_INNER;
	DevConfig.reference_out_enable=RFCLK_OUT_DISABLE;
	DevConfig.raw_data=0;
	DevConfig.pcr_adj=0;


	printf("rate:%f \nrate_ori:%f\nburst_enable:%d\n", DevConfig.rate,DevConfig.rate_ori,DevConfig.burst_enable);

	ret=bst_start_transmitEx(&DevHandle, &DevConfig, 0);
	
	if (ret < 0) 
	{
		printf("bst_start_transmitEx failed!\n");
		exit(0);

	}
	printf("bst_start_transmitEx successfully\n");

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

