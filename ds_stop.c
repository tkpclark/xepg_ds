#include "ds.h"

int logfd=0;
char mdname[]="ds_stop";

main(int argc,char **argv)
{
	if(argc!=2)
	{
		printf("please input tableid which you wanna stop!\n");
		printf("EIT PF actual:78\n");
		printf("EIT PF other:79\n");
		printf("FST:175\n");
		printf("PFT:173\n");
		exit(0);
	}
	//init tasks mmap
	char taskmmap[128];
	char cmd_buffer[256];
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

	char tsbuffermmap[128];
	char *p_ts_buffer = NULL;
	read_config("ts_buffer", tsbuffermmap);

	p_ts_buffer = init_mmap_use(tsbuffermmap);
	
	task_num=0;
	total_rate=0;
	p_mmapinfo=(TASK_INFO*)(p_map_tasks+MMAP_TASK_START_OFFS);
	for(i=0;i<*(int*)p_map_tasks;i++)
	{
		if(p_mmapinfo->tid==atoi(argv[1]))
		{	printf("stopping cltid:%d,	%s......\n",p_mmapinfo->cltid,p_mmapinfo->tname);
			sprintf(cmd_buffer,"./DSTcmd stop %d 127.0.0.1 9999 > /dev/null ",p_mmapinfo->cltid);
			system(cmd_buffer);
		}
		p_mmapinfo++;
		
	}
}

