#include "ds.h"

main(int argc,char **argv)
{
	int i;

	
	char *p_map=NULL;
	char mmapfile[128];
	read_config("ts_buffer",mmapfile);
	p_map=init_mmap_use(mmapfile);
	if(p_map==NULL)
	{
		printf("memory map failed!\n");
		exit(0);
	}
	//print_HEX(p_map+TS_BUFFER_CTL_LEN, 100);
	printf("writer:%d\n",*(unsigned int*)(p_map+TS_BUFFER_WRITER_OFFS));
	printf("reader:%d\n",*(unsigned int*)(p_map+TS_BUFFER_READER_OFFS));

	if((argc==4)&&(!strcmp(argv[1],"1")))
	{
		int offs=atoi(argv[2]);
		
		for(i=0;i<atoi(argv[3]);i++)
		{
			printf("%d: ",offs);
			print_HEX(p_map+TS_BUFFER_CTL_LEN+offs, FRAME_LEN);
			offs+=FRAME_LEN;
		}
	}
	if((argc==2)&&(!strcmp(argv[1],"5")))
	{
		int offs=0;
		while(1)
		{
			if(*(p_map+TS_BUFFER_CTL_LEN+offs)!=0x47)
			{
				printf("%d: ",offs);
				print_HEX(p_map+TS_BUFFER_CTL_LEN+offs, FRAME_LEN);
			}
			offs+=FRAME_LEN;
			if(offs==188000000)
				exit(0);
		}
	}
	if((argc==3)&&(!strcmp(argv[1],"2")))
	{
		
		unsigned int *p_wr=(unsigned int*)(p_map+TS_BUFFER_WRITER_OFFS);
		unsigned int *p_rd=(unsigned int*)(p_map+TS_BUFFER_READER_OFFS);
		unsigned int *p_wr_av=(unsigned int*)(p_map+TS_BUFFER_WRITER_AV_OFFS);
		const unsigned int max_offs=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;
		unsigned char *p_ts=p_map+TS_BUFFER_CTL_LEN;
		unsigned int oldwr=*p_wr;
		unsigned int oldrd=*p_rd;
		unsigned int newwr;
		unsigned int newrd;
		unsigned int oldwrav=*p_wr_av;
		int i;
		short pid=atoi(argv[2]);
		short tmp_pid=0;

		double rate_rd,rate_wr,rate_wr_av;
		char tmp[12];

		struct timeval tv;
       		double time0,time1,time2,time3,time4;

		int distance=0;
		int pid_num=0;

		struct itimerval value;
		value.it_value.tv_sec=1;
		value.it_value.tv_usec=0;
		
		while(1)
		{
			//setitimer(ITIMER_REAL, &value, NULL);
			gettimeofday (&tv, NULL);
       			time1=tv.tv_sec + (double)tv.tv_usec / 1000000;
			newrd=*p_rd;
			newwr=*p_wr;
			pid_num=0;

			rate_wr=8*(double)(get_tsbuffer_rw_distance(oldwr, newwr, max_offs)*FRAME_LEN)/1000000;
			rate_rd=8*(double)(get_tsbuffer_rw_distance(oldrd, newrd, max_offs))*FRAME_LEN/1000000;

			
			for(i=0;i<get_tsbuffer_rw_distance(oldrd, newrd, max_offs);i++)
			{
				tmp_pid=get_pid(p_ts+((oldrd+i*FRAME_LEN)%max_offs));
				//printf("tmppid:%d\n",tmp_pid);
				if(tmp_pid==pid)
					pid_num++;
			}
		
			
			my_nano_sleep(200999999);
			gettimeofday (&tv, NULL);
       			time2=tv.tv_sec + (double)tv.tv_usec / 1000000;
			printf("wr:%10fMbps\trd:%10fMbps\t pid:%d num:%d\trate:%f:time:%f worte:%d\n",rate_wr,rate_rd,pid,pid_num,(double)pid_num*FRAME_LEN*8/(time2-time1),time2-time1,get_tsbuffer_rw_distance(oldwr, newwr, max_offs));

			oldrd=newrd;
			oldwr=newwr;
			oldwrav=*p_wr_av;
		}
	}
	if((argc==3)&&(!strcmp(argv[1],"3")))
	{
		*(double*)(p_map+TS_BUFFER_ORI_RATE_OFFS)=atof(argv[2]);
	}
	if((argc==2)&&(!strcmp(argv[1],"3")))
	{
		for(i=0;i<=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;i+=FRAME_LEN)
		{
			
			if(*(p_map+TS_BUFFER_CTL_LEN+i)!=0x47)
			{
				printf("%d \n",i/FRAME_LEN);
				print_HEX(p_map+TS_BUFFER_CTL_LEN+i, FRAME_LEN);
			}
		}
		
	}
	if((argc==2)&&(!strcmp(argv[1],"4")))
	{
		unsigned int *p_wr=(unsigned int*)(p_map+TS_BUFFER_WRITER_OFFS);
		unsigned int *p_rd=(unsigned int*)(p_map+TS_BUFFER_READER_OFFS);
		const unsigned int max_offs=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;
		while(1)
		{
			printf("rw distance:%d\n",get_tsbuffer_rw_distance(*p_rd, *p_wr,max_offs));
			sleep(1);
		}
		
	}

}

