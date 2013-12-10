#include "ds.h"



main(int argc,char **argv)
{
//init tasks mmap
	char pidmmap[128];

	read_config("pid_list",pidmmap);
	
	char *p_map_pids;
	p_map_pids=init_mmap_use(pidmmap);

	if(p_map_pids==NULL)
	{
		printf("memory map failed!\n");
		exit(0);
	}

	//TASK_INFO *p_mmapinfo=NULL;
	PID_GROUP *p_mmapinfo=NULL;
	

	int i;
	int pid_num=0;
	char timeStr[32];
	
	while(1)
	{
		pid_num=0;
		p_mmapinfo=(PID_GROUP*)(p_map_pids+MMAP_PID_START_OFFS);
		
		printf("\033[2J");
		printf("\033[0;0H");
		printf("\033[0;32mID    PID       TSNUM     FD        SECNUM    DISTANCE      PREDISTNS     FILESIZE  INTERVAL  MEMBER    STATUS    TIME                   FILE\033[0m\n");
		for(i=1;i<=MAX_PID_GROUP_NUM;i++)
		{
			if(p_mmapinfo->pid)
			{
				if(argc==1 || (argc==2) && (atoi(argv[1])==p_mmapinfo->pid) )
				{
					printf("%-6d%-10d%-10d%-10d%-10d%-14f%-14f%-10d%-10d%-10d%-10d%-23s%-23s  %d\n",
					i,
					p_mmapinfo->pid,
					p_mmapinfo->all_frame_number,
					p_mmapinfo->fd,
					p_mmapinfo->all_section_number,
					p_mmapinfo->distance,
					p_mmapinfo->pre_distance,
					p_mmapinfo->filesize,
					p_mmapinfo->interval,
					p_mmapinfo->member_number,
					p_mmapinfo->status,
					timet_to_str(p_mmapinfo->uptm,timeStr),
					p_mmapinfo->filename,
					p_mmapinfo->last_position
					);
					pid_num++;
				}
			}

			p_mmapinfo++;
		}
		printf("\n");
		system("date");
		printf("pid num:[\033[0;31m%d\033[0m]    \n",pid_num);
		sleep(1);
	}
}	

