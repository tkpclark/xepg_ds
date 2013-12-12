#include "ds.h"

static int logfd;
static char mdname[32];
static char logbuf[20480];
static char *p_map_pids;
static char *p_map_ts;
static char *p_map_flag;
static const char *pidfile="ds.pid";
static int gstop=0;
static int prtpid=-1;
static unsigned int *p_rd_offs;//=(unsigned int *)(p_map_ts+TS_BUFFER_READER_OFFS);
static unsigned int *p_wr_offs;//=(unsigned int *)(p_map_ts+TS_BUFFER_WRITER_OFFS);
static unsigned int max_offs;//=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;
static unsigned char *p_ts;//=p_map_ts+TS_BUFFER_CTL_LEN;
static int tasklockfd;
unsigned int frame_per_time;

static PID_GROUP *p_pid_group;


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
	proclog(logfd,mdname,"sigalarm");
}
static void procquit(void)
{
	if(getpid()==getfilepid(pidfile))
	{
 		unlink(pidfile);
  		proclog(logfd,mdname,"group quited!\n");
  		close(logfd);
	}
}


static init()
{
	sleep(2);
	strcpy(mdname,"DS");
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
	
	//register quit function
	if(atexit(&procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}
	
	//init pids mmap
	
	char pidmmap[128];
	read_config("pid_list",pidmmap);
	p_map_pids=init_mmap_use(pidmmap);
	if(p_map_pids==NULL)
	{
		printf("pids memory map failed!\n");
		exit(0);
	}

	//init ts_buffer mmap
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

	//init lockfd
	tasklockfd=open("task.lck",O_CREAT|O_WRONLY|O_TRUNC,S_IRWXU);
	if(tasklockfd==-1)
	{
		printf("open lock file error!\n");
		exit(0);
	}

	
}
static open_pid_file()
{
	if(p_pid_group->fd)
		close(p_pid_group->fd);

	
	char pid_files_dir[128];

	read_config("pid_files_dir", pid_files_dir);
	sprintf(p_pid_group->filename,"%s/%d.ts",pid_files_dir,p_pid_group->pid);

	p_pid_group->fd=open(p_pid_group->filename,O_CREAT|O_RDWR,0600);
	if(p_pid_group->fd<0)
	{
		sprintf(logbuf,"open %s failed:%s   pid:%d\n",p_pid_group->filename,strerror(errno),p_pid_group->pid);
		proclog(logfd,mdname,logbuf);
	
	}
	else
	{
		//p_pid_group->has_written_frame=0;
		;//p_pid_group->status=0;
	}

}

static write_to_mmap(char *p_ts,unsigned int *p_wr_offs,char *buffer,int send_this_time,int ts_buffer_len)
{
	int num=0;
	int i;
	for(i=0;i<5000;i++)
	{
		if(num==send_this_time)
			break;
		memcpy(p_ts+*p_wr_offs,buffer+num*FRAME_LEN,FRAME_LEN);
		*p_wr_offs=(*p_wr_offs+FRAME_LEN)%ts_buffer_len;
		num++;
	}
}
static update_pid_file()
{
	int fd;
	int i;
	struct timeval tv;
  	double time0=0,time1=0,time2=0,time3=0,time4=0;
	char tmp_pid_filename[128];
	char buffer[FRAME_LEN*100];
	char tmp_pid_files_dir[128];
	int n=0;
	int filesize=0;
	read_config("tmp_pid_files_dir", tmp_pid_files_dir);


	sprintf(tmp_pid_filename,"%s/%d.ts",tmp_pid_files_dir,p_pid_group->pid);
	fd=open(tmp_pid_filename,0);
	if(fd<0)
	{
		sprintf(logbuf,"open %s failed! %s",tmp_pid_filename,strerror(errno));
		proclog(logfd,mdname,logbuf);
		return;
	}
	alarm(1);
	gettimeofday (&tv, NULL);
      	 time1=tv.tv_sec + (double)tv.tv_usec / 1000000;
	if(myflock(fd, 1))
	{
		sprintf(logbuf,"flock %s failed! %s",tmp_pid_filename,strerror(errno));
		proclog(logfd,mdname,logbuf);
		alarm(0);
		return;
	}
	gettimeofday (&tv, NULL);
      	time2=tv.tv_sec + (double)tv.tv_usec / 1000000;
	if(time2-time1 > 0.00002)
	{
		sprintf(logbuf,"myflock waited too long! %f\n",time2-time1);
		proclog(logfd,mdname,logbuf);
	}
	alarm(0);
	filesize=get_file_size(fd);

	if(p_pid_group->fd <=0)
	{
		open_pid_file();
	}
	if(ftruncate(p_pid_group->fd,0))
	{
		sprintf(logbuf,"truncate %s failed! %s",p_pid_group->filename,strerror(errno));
		proclog(logfd,mdname,logbuf);
		return;
	}
	if(lseek(p_pid_group->fd,SEEK_SET,0))
	{
		sprintf(logbuf,"lseek failed! %s",strerror(errno));
		proclog(logfd,mdname,logbuf);
		return;
	}
	
	while(1)//copy file
	{
		memset(buffer,0,sizeof(buffer));
		n=read(fd,buffer,sizeof(buffer));
		if(n>0 && n%FRAME_LEN==0)
		{
			
			if(write(p_pid_group->fd,buffer,n)!=n)
			{
				sprintf(logbuf,"write %s failed! %s",tmp_pid_filename,strerror(errno));
				proclog(logfd,mdname,logbuf);
				break;
			}
		}
		else if(n==0)
		{
			break;
		}
		else
		{
			sprintf(logbuf,"read %s failed! return:%d,%s",tmp_pid_filename,n,strerror(errno));
			proclog(logfd,mdname,logbuf);
			break;
		}
		
	}
	n=get_file_size(p_pid_group->fd);
	if(n!=filesize)
	{
		sprintf(logbuf,"read %s from tmp dir failed!!!!!!!should:%d,real:%d\n",p_pid_group->filename,filesize,n);
		proclog(logfd,mdname,logbuf);
	}

	if(myflock(fd, 2))
	{
		sprintf(logbuf,"flock failed! %s",strerror(errno));
		proclog(logfd,mdname,logbuf);
	}

	close(fd);



	//update distance
	p_pid_group->distance=p_pid_group->pre_distance;//so it will be updated when being sent

	time3=tv.tv_sec + (double)tv.tv_usec / 1000000;
	if(time3-time1 > 0.000005)
	{
		sprintf(logbuf,"update %s too long! %f\n",p_pid_group->filename,time3-time1);
		proclog(logfd,mdname,logbuf);
	}

}
static send_one_frame()
{
	int n=0;
	char buffer[FRAME_LEN];
	int i;

	int read_zero=0;

	for(i=0;i<2;i++)//try i times,loop would be 1 time at normal
	{
		if(p_pid_group->fd <=0)
		{
			open_pid_file();
		}
		memset(buffer,0,sizeof(buffer));
		n=read(p_pid_group->fd ,buffer,FRAME_LEN);
		if(n==FRAME_LEN)
		{
			write_to_mmap(p_ts,p_wr_offs,buffer,1,max_offs);
			//p_pid_group->has_written_frame++;
			//print_HEX(buffer,FRAME_LEN);
			return;
		}
		else if(n==0)
		{
			
			if(p_pid_group->status==2)//pid file updated in tmp_dir
			{
				update_pid_file();
				p_pid_group->status=0;
			}
			

			close(p_pid_group->fd );
			p_pid_group->fd =0;

			/*
			if(p_pid_group->pid==0x12)
			{
				printf("has sent :%d\n",p_pid_group->has_written_frame);
			}
			*/

			
			read_zero++;
			if(read_zero ==2)
			{
				sprintf(logbuf,"read %s failed,read zero twice",p_pid_group->filename);
				proclog(logfd,mdname,logbuf);
			}
			
			
		}
		else
		{
			close(p_pid_group->fd );
			p_pid_group->fd =0;
			sprintf(logbuf,"read %s failed,%s",p_pid_group->filename,strerror(errno));
			proclog(logfd,mdname,logbuf);
		}
		
	}

		
}

int main(int argc,char **argv)
{
	int i;
	int ret=0;
	char buffer[FRAME_LEN*1000];

	char scan_interval[16]={0};
	char rate_ori[32]={0};

	read_config("rate_ori", rate_ori);
	read_config("scan_interval",scan_interval);
	PID_GROUP*p_mmapinfo=NULL;

	frame_per_time=atof(rate_ori)/((double)1000/atof(scan_interval))/FRAME_LEN/8;

	int frame_num=0;
	int null_num=0;
	char null_buffer[FRAME_LEN];
	gen_empty_pkg(null_buffer);
	int send_this_time=0;
	int pid_files_num=0;
	
		
	init();

	p_rd_offs=(unsigned int *)(p_map_ts+TS_BUFFER_READER_OFFS);
	p_wr_offs=(unsigned int *)(p_map_ts+TS_BUFFER_WRITER_OFFS);
	max_offs=TS_BUFFER_MAX_LEN-TS_BUFFER_CTL_LEN;
	p_ts=p_map_ts+TS_BUFFER_CTL_LEN;

	struct timeval tv;
  	double time0=0,time1=0,time2=0,time3=0,time4=0;
	double working_time=0;
	char filename[128];

	struct itimerval value;
	value.it_value.tv_sec=0;
	value.it_value.tv_usec=50;

	double sleep_over=0;
	double should_sleep=0;

	int rw_distance=0;

	int pos=0;
	int flag=0;

	int wait_num=0;

	double miss_distance=0;
	double max_miss_distance=0;
	PID_GROUP *p_max_miss_pid=NULL;

	if(argc==2)
		prtpid=atoi(argv[1]);
	


	//init distance
	p_pid_group=(PID_GROUP*)(p_map_pids+MMAP_PID_START_OFFS);
	for(i=0;i<MAX_PID_GROUP_NUM;i++)
	{
		if(p_pid_group->pid)
		{
			p_pid_group->last_position=0;
			p_pid_group->theory_position=0;
			p_pid_group->fd=0;
			p_pid_group->status=2;
			//p_pid_group->has_written_frame;

		}
		p_pid_group++;
	}
	
	//loop begin
	while(1)
	{
		
		gettimeofday (&tv, NULL);
      	 	time1=tv.tv_sec + (double)tv.tv_usec / 1000000;
		//pid_files_num=get_pid_file_info();

		null_num=0;
		frame_num=0;
		pos=0;
		
		
		
		while(pos<=frame_per_time)
		{
			//printf("--------------\n");
			if(gstop)
				exit(0);
			flag=0;//no real frame yet
			p_pid_group=(PID_GROUP*)(p_map_pids+MMAP_PID_START_OFFS);
			p_max_miss_pid=NULL;
			max_miss_distance=0;
			for(i=0;i<MAX_PID_GROUP_NUM;i++)
			{
				if(p_pid_group->pid && (pos >= p_pid_group->theory_position)  && p_pid_group->distance > 0.1)
				{
					miss_distance=(double)(pos-p_pid_group->theory_position);
					if(miss_distance > max_miss_distance)
					{
						max_miss_distance=miss_distance;
						p_max_miss_pid=p_pid_group;
					}
				}
				
				p_pid_group++;
			}
			if(p_max_miss_pid!=NULL)
			{
				p_pid_group=p_max_miss_pid;

				//time1=tv.tv_sec + (double)tv.tv_usec / 1000000;
				send_one_frame();
				
				p_pid_group->last_position=pos;
				p_pid_group->theory_position+=p_pid_group->distance;
				//printf("pid:%d ---> pos:%d\n",p_pid_group->pid,pos);
				frame_num++;
			}
			else//write null frame
			{
				write_to_mmap(p_ts, p_wr_offs, null_buffer, 1, max_offs);
				null_num++;
			}

			pos++;

			
			/*
			sprintf(logbuf,"pos:%d\n",pos);
			proclog(logfd,mdname,logbuf);
			*/
		}
		gettimeofday (&tv, NULL);
      	 	time2=tv.tv_sec + (double)tv.tv_usec / 1000000;

		//update last_position
		p_pid_group=(PID_GROUP*)(p_map_pids+MMAP_PID_START_OFFS);
		for(i=0;i<MAX_PID_GROUP_NUM;i++)
		{
			//if(p_pid_group->pid && p_pid_group->status==0)
			if( p_pid_group->pid)  
			{
				if(p_pid_group->distance > 0.1)
				{
					p_pid_group->last_position-=frame_per_time;
					p_pid_group->theory_position-=frame_per_time;
				}
				else if(p_pid_group->distance < 0.1 && p_pid_group->status==2)//pid file updated in tmp_dir)
				{
					update_pid_file();
					p_pid_group->status=0;
				}
			}
			p_pid_group++;
		}


		
		gettimeofday (&tv, NULL);
      	 	time3=tv.tv_sec + (double)tv.tv_usec / 1000000;

		working_time=(time3-time1)*1000;

			
		/********************proclog**************************/
		
		sprintf(logbuf, "should[%d]real[%d]null[%d]scantm[%s]worktm[%f]uplasttm[%f]rwdistance[%d]rate[%f]",
							frame_per_time,
							frame_num,
							null_num,
							scan_interval,
							working_time,
							(time3-time2)*1000,
							get_tsbuffer_rw_distance(*p_rd_offs, *p_wr_offs, max_offs),
							atof(rate_ori)
							);
		proclog(logfd,mdname,logbuf);
		
		wait_num=0;
		while(1)
		{
			if(gstop)
				exit(0);
			rw_distance=get_tsbuffer_rw_distance(*p_rd_offs, *p_wr_offs, max_offs);
			//printf("rw_distance:%d\n",rw_distance);
			if(rw_distance < frame_per_time)
			{
				if(rw_distance < frame_per_time/3 )
				{
					sprintf(logbuf,"rw_distance failed!rw:%d",rw_distance);
					proclog(logfd,mdname,logbuf);
				}
				break;
			}
			my_nano_sleep(50000000);
			wait_num++;
			if(wait_num > 300)
			{
				sprintf(logbuf,"wait num:%d\n",wait_num);
				proclog(logfd,mdname,logbuf);
			}
		}
			

	}

	
	
		

}


