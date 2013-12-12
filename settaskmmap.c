#include "ds.h"



main()
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

	p_mmapinfo=(TASK_INFO*)(p_map_tasks+MMAP_TASK_START_OFFS);
	for(i=0;i<*(int*)p_map_tasks;i++)
	{

		p_mmapinfo->isupdated=1;

		p_mmapinfo++;
	}

}

