#include "ds.h"



main(int argc,char **argv)
{
//init tasks mmap
	char taskmmap[128];
	unsigned int total_rate=0;
	read_config("task_list",taskmmap);
	
	char *p_map_tasks;
	p_map_tasks=init_mmap_use(taskmmap);

	if(p_map_tasks==NULL)
	{
		printf("memory map failed!\n");
		exit(0);
	}

	TASK_INFO *p_mmapinfo=NULL;
	

	int i;
	int task_num=0;
	char timeStr[32];

	char tsbuffermmap[128];
	char *p_ts_buffer = NULL;
	read_config("ts_buffer", tsbuffermmap);

	p_ts_buffer = init_mmap_use(tsbuffermmap);
	
	while(1)
	{
		task_num=0;
		total_rate=0;
		p_mmapinfo=(TASK_INFO*)(p_map_tasks+MMAP_TASK_START_OFFS);
		
		printf("\033[2J");
		printf("\033[0;0H");
		printf("\033[0;32mID    CLTID     BASE      TS        PID       TID       Version    INTERVAL     State     UPDATE        TSLEN       RATE        TABLE_NAME             TASK_NAME\033[0m\n");
		for(i=0;i<*(int*)p_map_tasks;i++)
		{
			if(p_mmapinfo->ts_len)
			{
				if(argc==1 || (argc==2) && (atoi(argv[1])==p_mmapinfo->tid) )
				{
					printf("%-6d%-10d%-10d%-10d%-10d%-10d%-11d%-13d%-10d%-14d%-12d%-12f%-23s%-23s\n",
					i,
					p_mmapinfo->cltid,
					p_mmapinfo->base_id,
					p_mmapinfo->ts_id,
					p_mmapinfo->pid,
					p_mmapinfo->tid,
					p_mmapinfo->version,
					p_mmapinfo->interval,
					p_mmapinfo->state,
					p_mmapinfo->isupdated,
					p_mmapinfo->ts_len,
					(double)p_mmapinfo->rate/1000000,
					p_mmapinfo->tname,
					timet_to_str(p_mmapinfo->uptm,timeStr)
					);
				}
				total_rate+=p_mmapinfo->rate;
			
				task_num++;
			}
			p_mmapinfo++;
		}
		printf("\n");
		system("date");
		printf("task num:[\033[0;31m%d\033[0m]    total rate:%f Mbps\n",task_num,(double)total_rate/1000000);
		sleep(1);
	}
}

