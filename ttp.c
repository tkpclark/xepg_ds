
#include "ds.h"

static int logfd;
static char mdname[32];
static char logbuf[20480];

static int gstop=0;
static int prtpid=-1;
static int tasklockfd;

static char *p_map_tasks;
static char *p_map_pids;

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
	;//printf("sigalarm\n");
}
static void procquit(void)
{
;
}

static locate_p_pid_group(unsigned short pid)
{
	p_pid_group=(PID_GROUP*)(p_map_pids+MMAP_PID_START_OFFS);
	int i;
	int flag=0;
	for(i=0;i<MAX_PID_GROUP_NUM;i++)
	{
		if(p_pid_group->pid==pid)
		{
			flag=1;
			break;
		}
		p_pid_group++;
	}

	if(!flag)
	{
		p_pid_group=(PID_GROUP*)(p_map_pids+MMAP_PID_START_OFFS);
		for(i=0;i<MAX_PID_GROUP_NUM;i++)
		{
			if(p_pid_group->pid==0)
			{
				memset(p_pid_group,0,sizeof(PID_GROUP));
				p_pid_group->pid=pid;
				break;
			}
			p_pid_group++;
		}
	}

	p_pid_group->pid=pid;
}
static init()
{
	sleep(2);
	strcpy(mdname,"TTP");
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
	
	//init tasks mmap
	
	char taskmmap[128];
	read_config("task_list",taskmmap);
	p_map_tasks=init_mmap_use(taskmmap);
	if(p_map_tasks==NULL)
	{
		printf("tasks memory map failed!\n");
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
static get_one_pid_group_info()
{
	int i,j,k;
	
	TASK_INFO *p_tasks=NULL;
	PID_GROUP *p_pids=NULL;
	
	p_tasks=(TASK_INFO*)(p_map_tasks+MMAP_TASK_START_OFFS);

	/************************get members of this group********************************/


	//init
	p_pid_group->member_number=0;
	p_pid_group->all_frame_number=0;
	p_pid_group->all_section_number=0;
	p_pid_group->filesize=0;
	p_pid_group->interval=0;
	if(p_pid_group->distance < 0.1)  //a task just started
	{
		p_pid_group->last_position=0;
	}
	//p_pid_group->distance=0;


	
	myflock(tasklockfd, 1);
	for(i=0;i<MAX_CHLD_NUM;i++)
	{
		
		if (p_tasks->pid==p_pid_group->pid) 
		{
			p_tasks->isupdated=0;
			if(p_tasks->state==1) 
			{
				p_pid_group->member[p_pid_group->member_number].p_task=p_tasks;
				p_pid_group->member_number++;
				
			}
			else if(p_tasks->state==3) 
			{
				memset(p_tasks,0,sizeof(TASK_INFO));
			}
			

		}

		p_tasks++;
			
	}
	myflock(tasklockfd, 2);

	if(p_pid_group->member_number)
		p_pid_group->status=0;
	else
		p_pid_group->status=-1;


	/*
	if(p_pid_group->member_number==0)
	{
		return ;//p_pid_group->member_number;
	}
	*/

	/************************count min_common_multiple of this group********************************/
	int flag=1;


	if(p_pid_group->member_number >0)
	{
		for(j=100;j<=1000*1000;j+=10)
		{
			flag=1;
			for(k=0;k<p_pid_group->member_number;k++)
			{
				if(j%p_pid_group->member[k].p_task->interval)
				{
					flag=0;
					break;
				}
			}
			if(flag)
			{
				p_pid_group->interval=j;
				//printf("group[%d].min_common_multiple=%d\n",i,j);
				break;
			}
		}
		if(!flag)
		{
			printf("group 's min_common_multiple too large ,please reset circle time\n",j);
		}
	}




	/**************init circle time,distance,section number,circle_buffer**********************/
	char *p_clt_ts_buffer=NULL;

	
	for(j=0;j<p_pid_group->member_number;j++)
	{
		//init circle time
		p_pid_group->member[j].circle_times=p_pid_group->interval/p_pid_group->member[j].p_task->interval;
		
		/////circle_buffer
		p_clt_ts_buffer=malloc(p_pid_group->member[j].p_task->ts_len);
		memset(p_clt_ts_buffer,0,p_pid_group->member[j].p_task->ts_len);
		if(get_clt_ts_data(p_clt_ts_buffer,p_pid_group->member[j].p_task)==-1)
		{
			sprintf(logbuf,"get_clt_ts_data failed! %s",strerror(errno));
			proclog(logfd,mdname,logbuf);
			free(p_clt_ts_buffer);
			continue;
		}
		p_pid_group->member[j].p_circle_buffer=malloc(p_pid_group->member[j].p_task->ts_len*p_pid_group->member[j].circle_times);
		memset(p_pid_group->member[j].p_circle_buffer,0,p_pid_group->member[j].p_task->ts_len*p_pid_group->member[j].circle_times);
		for(k=0;k<p_pid_group->member[j].circle_times;k++)
		{
			memcpy(p_pid_group->member[j].p_circle_buffer+k*p_pid_group->member[j].p_task->ts_len,p_clt_ts_buffer,p_pid_group->member[j].p_task->ts_len);
		}

		//init section number
		p_pid_group->member[j].section_number=get_section_number_from_buffer(p_clt_ts_buffer,p_pid_group->member[j].p_task->ts_len);
		p_pid_group->all_section_number+=p_pid_group->member[j].section_number*p_pid_group->member[j].circle_times;

		//frame number
		p_pid_group->all_frame_number+=(p_pid_group->member[j].p_task->ts_len/FRAME_LEN)*(p_pid_group->member[j].circle_times);

		p_pid_group->filesize+=p_pid_group->member[j].p_task->ts_len*p_pid_group->member[j].circle_times;




		//printf("freeing p_clt_ts_buffer\n");
		free(p_clt_ts_buffer);
		
	}




	//update group info
	p_pid_group->uptm=time(0);
	p_pid_group->status=0;




	char rate_ori[32]={0};
	read_config("rate_ori", rate_ori);
	if(p_pid_group->filesize &&p_pid_group->interval)
	{
		p_pid_group->pre_distance=(double)atof(rate_ori)/(p_pid_group->filesize*8*((double)1000/p_pid_group->interval));
	}
	else
	{
		p_pid_group->pre_distance=0;
	}


	/************************print  infomation of this group***********************************/
	sprintf(logbuf,"pid[%d]allsection[%d]frame[%d]member[%d]interval[%d]filesize[%d]pre_distance[%f]",
		p_pid_group->pid,
		p_pid_group->all_section_number,
		p_pid_group->all_frame_number,
		p_pid_group->member_number,
		p_pid_group->interval,
		p_pid_group->filesize,
		p_pid_group->pre_distance
		
		);
	proclog(logfd,mdname,logbuf);



	/************************print member infomation of this group***********************************/
	char timeStr[64];
	for(j=0;j<p_pid_group->member_number;j++)
	{
		//init distance
		//p_pid_group->member[j].section_distance=(double)p_pid_group->all_frame_number/(p_pid_group->member[j].section_number*p_pid_group->member[j].circle_times);
		p_pid_group->member[j].frame_distance=(double)p_pid_group->all_frame_number/((p_pid_group->member[j].p_task->ts_len/FRAME_LEN)*p_pid_group->member[j].circle_times);


		sprintf(logbuf," 		member[%d]cltid[%d]circle[%d]distance[%f]pid[%d]section[%d]interval[%d]ts_len[%d]nm[%s]",j,
			p_pid_group->member[j].p_task->cltid,
			p_pid_group->member[j].circle_times,
			p_pid_group->member[j].frame_distance,
			p_pid_group->member[j].p_task->pid,
			p_pid_group->member[j].section_number,
			p_pid_group->member[j].p_task->interval,
			p_pid_group->member[j].p_task->ts_len,
			p_pid_group->member[j].p_task->tname,
			timet_to_str(p_pid_group->uptm,timeStr)
			);
		proclog(logfd,mdname,logbuf);
	}
		
	//return p_pid_group->member_number;
	
}
static int is_member_finished(int member_id)
{
	if(p_pid_group->member[member_id].has_written_frame * FRAME_LEN>= p_pid_group->member[member_id].p_task->ts_len * p_pid_group->member[member_id].circle_times)
	{
		p_pid_group->member[member_id].is_finished=1;
		sprintf(logbuf,"pid[%d]member [%d] finished!",p_pid_group->pid,member_id);
		proclog(logfd,mdname,logbuf);
		return 1;
	}
	else
	{
		return 0;
	}
}
static int write_one_section(int fd,int member_id)
{
	int write_frame_number=0;

	//write the first 188
	write(fd,p_pid_group->member[member_id].p_circle_buffer+p_pid_group->member[member_id].has_written_frame*FRAME_LEN,FRAME_LEN);
	//p_pid_group->member[member_id].p_circle_buffer+=FRAME_LEN;
	p_pid_group->member[member_id].has_written_frame++;
	write_frame_number++;
	
	

	//wirte till the next first 188
	while(1)
	{
		if(is_member_finished(member_id))
			break;
		
		if(is_first_frame(p_pid_group->member[member_id].p_circle_buffer+p_pid_group->member[member_id].has_written_frame*FRAME_LEN))
			break;
		write(fd,p_pid_group->member[member_id].p_circle_buffer+p_pid_group->member[member_id].has_written_frame*FRAME_LEN,FRAME_LEN);
		//p_pid_group->member[member_id].p_circle_buffer+=FRAME_LEN;
		write_frame_number++;
		p_pid_group->member[member_id].has_written_frame++;
	}


	//update has_written_section
	p_pid_group->member[member_id].has_written_section++;

	//is_member_finished(member_id);

	

	
	return write_frame_number;
}
static gen_pid_file_of_one_group()
{
	int i,j;
	//open pidfile
	char tmp_pid_files_dir[128];
	char tmp[32];
	int pidfd=0;

	char tmp_pid_file_name[128];

	read_config("tmp_pid_files_dir", tmp_pid_files_dir);
	sprintf(tmp_pid_file_name,"%s/%d.ts",tmp_pid_files_dir,p_pid_group->pid);

	//printf("pid_file_name:%s\n",pid_file_name);
	pidfd=open(tmp_pid_file_name,O_CREAT|O_WRONLY,0600);
	if(pidfd==-1)
	{
		sprintf(logbuf,"open %s failed when making pid file!\n",tmp_pid_file_name);
		proclog(logfd,mdname,logbuf);
		return;
	}


	if(myflock(pidfd, 1))
	{
		sprintf(logbuf,"flock failed! %s",strerror(errno));
		proclog(logfd,mdname,logbuf);
	}

	if(ftruncate(pidfd,0))
	{
		sprintf(logbuf,"truncate failed! %s",strerror(errno));
		proclog(logfd,mdname,logbuf);
	}


	/*
	if(group_id!=12)
		return;
	*/
	//mix and write to pidfile
	int has_written_frames=0;
	int wrote_flag=0;
	int min_distance=1000000;
	double max_miss_distance=-1000;
	int max_miss_distance_member=-1;
	//printf("writing file of group...\n");

	//init
	for(j=0;j<p_pid_group->member_number;j++)
	{
		p_pid_group->member[j].miss_distance=0;
		p_pid_group->member[j].has_written_frame=0;
		p_pid_group->member[j].is_finished=0;
		
		
	}
	
	
	while (has_written_frames<p_pid_group->all_frame_number)
	{
		if(gstop)
			exit(0);

		max_miss_distance=-1000;
		for(j=0;j<p_pid_group->member_number;j++)
		{
			if(p_pid_group->member[j].is_finished)
				continue;


			p_pid_group->member[j].miss_distance=(double)has_written_frames -p_pid_group->member[j].frame_distance * p_pid_group->member[j].has_written_frame;
			

			if(p_pid_group->member[j].miss_distance>=max_miss_distance)
			{
				max_miss_distance=p_pid_group->member[j].miss_distance;
				max_miss_distance_member=j;
			}
		}


		if(max_miss_distance==-1000)
		{
			sprintf(logbuf,"section mix failed!,has_written_frames[%d] should[%d]",has_written_frames,p_pid_group->all_frame_number);
			proclog(logfd,mdname,logbuf);
			break;
		}
		has_written_frames+=write_one_section(pidfd,max_miss_distance_member);
		
		
	}


	if(myflock(pidfd, 2))
	{
		sprintf(logbuf,"unflock failed! %s",strerror(errno));
		proclog(logfd,mdname,logbuf);
	}
	//close pid file
	close(pidfd);

	p_pid_group->status=2;//pid file updated

	//printf("%s updated!\n",tmp_pid_file_name);

	///free pointer
	for(j=0;j<p_pid_group->member_number;j++)
	{
		//printf("freeing member[%d]'s p_circle_buffer\n",j);
		free(p_pid_group->member[j].p_circle_buffer);
	}
		

	

	
}

static unsigned short  task_updated()
{
	int i,j,k;

	TASK_INFO *p_tasks=NULL;
	p_tasks=(TASK_INFO*)(p_map_tasks+MMAP_TASK_START_OFFS);
	
	time_t now_time=0;

	int update_flag=0;

	myflock(tasklockfd, 1);
	for(i=0;i<MAX_CHLD_NUM;i++)
	{
		
		if (p_tasks->pid &&p_tasks->isupdated==1)
		{
			update_flag=1;
			break;
		}
		p_tasks++;
		
	}
	myflock(tasklockfd, 2);
	if(update_flag)
		return p_tasks->pid;
	else
		return 0;
}
int main(int argc,char **argv)
{
	int i;
	int ret=0;
	int chld_pid=0;
	char buffer[FRAME_LEN*1000];
	unsigned short updated_pid;

	TASK_INFO *p_tasks=NULL;

	char pid_files_dir[128];
	read_config("pid_files_dir", pid_files_dir);
		
	init();

	while(1)
	{
		if(gstop)
			exit(0);
		updated_pid=task_updated();
		if(updated_pid)
		{
			

			locate_p_pid_group(updated_pid);
			

			//printf("pid:%d updated!\n",p_pid_group->pid);
			
			get_one_pid_group_info();
			
			gen_pid_file_of_one_group();
			
		}
		else
		{
			sleep(3);
		}
	}

	
}


