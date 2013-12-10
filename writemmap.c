#include "ds.h"

static void acalarm(int signo)
{
	;//printf("sigalarm\n");
}


main(int argc,char **argv)
{

	struct sigaction signew;

	signew.sa_handler=acalarm;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
	sigaction(SIGALRM,&signew,0);
	
	char *p_map=NULL;
	char mmapfile[128];
	read_config("ts_buffer",mmapfile);
	p_map=init_mmap_use(mmapfile);
	if(p_map==NULL)
	{
		printf("memory map failed!\n");
		exit(0);
	}
	char buf1[FRAME_LEN];
	buf1[0]=0x47;
	buf1[1]=0x51;
	buf1[2]=0xF0;
	buf1[3]=0x10;
	char buf2[FRAME_LEN];
	buf2[0]=0x47;
	buf2[1]=0x52;
	buf2[2]=0xF0;
	buf2[3]=0x10;
	char buf3[FRAME_LEN];
	buf3[0]=0x47;
	buf3[1]=0x53;
	buf3[2]=0xF0;
	buf3[3]=0x10;
	char buf4[FRAME_LEN];
	buf4[0]=0x47;
	buf4[1]=0x54;
	buf4[2]=0xF0;
	buf4[3]=0x10;

	char buffer[FRAME_LEN*5];
	int i;

		memcpy(buffer,buf1,FRAME_LEN);
		memcpy(buffer+FRAME_LEN,buf2,FRAME_LEN);
		memcpy(buffer+FRAME_LEN*2,buf3,FRAME_LEN);
		memcpy(buffer+FRAME_LEN*3,buf4,FRAME_LEN);
		memcpy(buffer+FRAME_LEN*4,buf4,FRAME_LEN);

	
		
	int offs=0;
	unsigned int *p_wr=(unsigned int*)(p_map+TS_BUFFER_WRITER_OFFS);
	unsigned int *p_wr_av=(unsigned int*)(p_map+TS_BUFFER_WRITER_AV_OFFS);
	unsigned int max=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;
	
	struct itimerval value;
	value.it_value.tv_sec=0;
	value.it_value.tv_usec=200000;//1000000;
	
	
	while(1)
	{
		setitimer(ITIMER_REAL, &value, NULL);
		for(i=0;i<6640*2;i++)
		{
			
			memcpy(p_map+TS_BUFFER_CTL_LEN+*p_wr,buffer,FRAME_LEN);
			*p_wr=*p_wr+FRAME_LEN;
			//*p_wr_av=*p_wr_av+FRAME_LEN;
			if(*p_wr>=max)
			{
				printf("max:%d\n",*p_wr);
				*p_wr=0;
			}
		}
		//printf("sleeping..\n");
		sleep(10);
			
	//	printf("i:%d,j:%d,offs:%d\n",i,j,offs);
	}
	
}

