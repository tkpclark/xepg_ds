
#include "ts_buffer.h"
#include "ds.h"

static char *p_map_ts;
static int logfd;
static char *mdname="SPDTEST";
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

/*
int isNITPack(char *p)
{	if((*(unsigned char *)(p+1) & 0x40!=0x40))
		return 0;
	else if((*(unsigned char *)(p+5) ==0x40) && (*(unsigned char *)(p+0xb) ==0x00))
		return 1;
	else if((*(unsigned char *)(p+5) ==0x40) && (*(unsigned char *)(p+0xb) ==0x01))
		return 2;
}
*/
static isSpeSection(unsigned char *p)
{
	struct timeval tv;
	static double write_time_last;
	double write_time;
	unsigned char section_header_flag=0;
	section_header_flag=*(unsigned char *)(p+1)  & 0x40;
	
	
	if(
			p[0]==0x47 &&
			p[1]==0x5b &&
			p[2]==0x79 &&
			p[4]==0x00 &&
			p[5]==0x4e &&
			p[6]==0xf0 &&
		//	p[7]==0x0f &&
			p[8]==0x04 &&
			p[9]==0x91 
			/*
			p[10]==0xd9 &&
			p[11]==0x00 &&
			p[12]==0xf8 &&
			p[13]==0x00 &&
			p[14]==0x35 &&
			p[15]==0x03
			*/ 
		)
	//if((section_header_flag==0x40)   &&  (*(unsigned char *)(p+5) ==tableid)  &&  get_pid(p)==pid && (*(unsigned char *)(p+0xb) ==0)  )
	{
		//printf("p+1:0x%x,and:0x%x\n",*(unsigned char *)(p+1),section_header_flag);
		gettimeofday (&tv, NULL);
		write_time=tv.tv_sec + (double)tv.tv_usec / 1000000;
		if(write_time-write_time_last< 0.1)
		{
			printf("seciton time:%f  ,interval:%f\n",write_time,write_time-write_time_last);
			//print_HEX(p,40);
		}
		write_time_last=write_time;
	}

}
static void spdTest()
{
	
	//unsigned int *p_rd_offs=(unsigned int *)(p_map_ts+TS_BUFFER_READER_OFFS);
	unsigned int p_rd_offs=*(unsigned int *)(p_map_ts+TS_BUFFER_READER_OFFS);
	unsigned int *p_wr_offs=(unsigned int *)(p_map_ts+TS_BUFFER_WRITER_OFFS);

	//*p_rd_offs=*p_wr_offs;//read from the writer's position
	
	const unsigned int max_offs=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;
	
	unsigned char *p_ts=p_map_ts+TS_BUFFER_CTL_LEN;
	//unsigned char *p_ds_wflag=p_map_ts+TS_BUFFER_DS_WFLAG_OFFS;

	/*
	struct itimerval value;
	value.it_value.tv_sec=0;
	value.it_value.tv_usec=100000;
	*/

	while(1)
	{
		if(gstop)
			exit(0);
		
		if(p_rd_offs==*p_wr_offs)
		{
			;//my_nano_sleep(10000000);
		}
		else
		{
			isSpeSection(p_ts+p_rd_offs);

			p_rd_offs =p_rd_offs+FRAME_LEN;

			if(p_rd_offs >= max_offs)
			{
				//sprintf(logbuf,"arrive to the max len:%d|%d\n",*p_rd_offs,max_offs);
				//proclog(logfd,mdname,logbuf);
				p_rd_offs=0;
			}
		}
	}


}
static void procquit(void)
{

}
main(int argc,char **argv)
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

	if(atexit(&procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}
	
	//init mmap
	char tsmmap[128];
	read_config("ts_buffer",tsmmap);

	//create mmap
	p_map_ts=init_mmap_use(tsmmap);
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

	spdTest();
	

	
}

