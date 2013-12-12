#include "ds.h"

#define DEBUG_PRINT		0
#define EMPTY_PACK_PID	8191


typedef struct
{
	unsigned short pid;
	unsigned char tid;
}package_info;

package_info package_info_record[256];


unsigned char get_tid(unsigned char *p_ts_buffer)
{
	unsigned char tid = 0xff;
	unsigned char payload_unit_start_indicator;
	unsigned char adp_field;
	unsigned char pointer_field;

	payload_unit_start_indicator = *(p_ts_buffer + 1) & 0x40;

	if(payload_unit_start_indicator == 0x00)
		return 0xff;//data that belong to the before table;

	adp_field = *(p_ts_buffer+3);

	adp_field = adp_field&0x30;
	
	if(adp_field == 0x10)
		{
			pointer_field = *(p_ts_buffer + 4);
			tid = *(p_ts_buffer + 4 + 1 + pointer_field);
		}

	else if(adp_field == 0x30)
		{
			pointer_field = *(p_ts_buffer + 4 + *(p_ts_buffer + 4));
			tid = *(p_ts_buffer + 4 + 1 + pointer_field  + *(p_ts_buffer + 4));
		}
}


void inforecord(unsigned short pid,unsigned char tid,package_info  *info_rd)
{
	int i;
	
	for(i=0;i<256;i++)
	{
		if(pid==(info_rd+i)->pid)// && (info_rd+i)->tid==tid)
		{
			(info_rd+i)->tid=tid;
			break;
		}
		
		else if(8191==(info_rd+i)->pid && (info_rd+i)->tid==0xff)
		{
			(info_rd+i)->pid = pid;
			(info_rd+i)->tid = tid;
			return;
		}
		
	}
}

unsigned char  findrecordtid(unsigned short pid,package_info  *info_rd)
{
	int i;
	
	for(i = 255;i>=0;i--)
	{
		if((info_rd+i)->pid==pid)
		{
			return (info_rd+i)->tid;
		}
	}

	return -1;
	
}

void clearpidbuffer(package_info *info_rd)
{
	int i;
	for(i=0;i<256;i++)
	{
		(info_rd+i)->pid=8191;
		(info_rd+i)->tid = 0xff;
	}
}



void	 Task_Code_Rate(char* p_task_list,char *p_ts_buffer)	//calculate each task's  package number 
{
	TASK_INFO	*pTask = NULL ;
	char	* p_buffer = p_ts_buffer;
	unsigned int buffer_offset_before = 0;
	unsigned int buffer_offset_after = 0;
	unsigned int read_count,buffer_end;
	unsigned int count,count_task;
	unsigned short pid;
	unsigned char tid;
	
	clearpidbuffer(package_info_record);

	pTask = (TASK_INFO *)(p_task_list + MMAP_TASK_START_OFFS);

	buffer_offset_before = *(int *)(p_buffer + TS_BUFFER_READER_OFFS);//186681180;
	//printf("buffer_offset_before:%d\n",buffer_offset_before);

	sleep(MONITOR_INTERVAL);

	buffer_offset_after = *(int *)(p_buffer + TS_BUFFER_READER_OFFS);//250792;
	//printf("buffer_offset_after:%d\n",buffer_offset_after);

	pTask = (TASK_INFO *)(p_task_list + MMAP_TASK_START_OFFS);

	for(count_task = 0;count_task <*(int*)p_task_list; count_task++)
	{
		pTask->pkg_no = 0;//frame number during MONITOR_INTERVAL
		pTask++;
	}//clear all package record

	pTask = (TASK_INFO *)(p_task_list + MMAP_TASK_START_OFFS);

	if(buffer_offset_after > buffer_offset_before)
	{
		/*
		printf("buffer_offset_after > \
			buffer_offset_before,buffer_offset_after:%d,buffer_offset_before:%d\n",buffer_offset_after,buffer_offset_before);
			*/
		read_count = buffer_offset_after - buffer_offset_before;

		//printf("read_count:%d\n",read_count);

		for(count = 0; count<read_count/FRAME_LEN; count++)
		{
			//printf("%d\n",pTask);
			pid = get_pid(p_buffer + TS_BUFFER_CTL_LEN + buffer_offset_before + count*FRAME_LEN);
			//printf("pid:%d\n",pid);

			if(pid==EMPTY_PACK_PID)//empty package ,not calculate
				continue;
			
			tid = get_tid(p_buffer + TS_BUFFER_CTL_LEN + buffer_offset_before + count*FRAME_LEN);
			if(tid==0xff)
				tid = findrecordtid(pid,package_info_record);//record the tid
			else
				inforecord(pid,tid,package_info_record);

			
			//printf("tid:%d\n",tid);

			//printf("taks_num:%d\n",*(int*)p_task_list);

			for(count_task = 0;count_task <*(int*)p_task_list; count_task++)
			{
				//printf("pTask->pid:%d\n",pTask->pid);
				//printf("pTask->tid:%d\n",pTask->tid);
				
				if(pTask->ts_len)
				{
					if(pTask->pid == pid && pTask->tid == tid)
					{
						pTask->pkg_no++;
						break;
					}
					
				}

				pTask++;
				
			}

			pTask = (TASK_INFO *)(p_task_list + MMAP_TASK_START_OFFS);
			
		}

		pTask = (TASK_INFO *)(p_task_list + MMAP_TASK_START_OFFS);

		for(count_task = 0;count_task <*(int*)p_task_list; count_task++)
		{
			pTask->rate = pTask->pkg_no*FRAME_LEN*8/MONITOR_INTERVAL;
			pTask++;
		}
	}


	else if(buffer_offset_after < buffer_offset_before)
		{
			unsigned int i = 0;
			/*
			printf("buffer_offset_after < \
			buffer_offset_before,buffer_offset_after:%d,buffer_offset_before:%d\n",buffer_offset_after,buffer_offset_before);
			*/
			read_count = (TS_BUFFER_MAX_LEN -TS_BUFFER_CTL_LEN - buffer_offset_before) + buffer_offset_after;

			//printf("read_count:%d\n\n\n",read_count);

			for(count = 0; count<read_count/FRAME_LEN; count++)
			{
				buffer_end =  buffer_offset_before + count *FRAME_LEN;
				
				if(buffer_end < TS_BUFFER_MAX_LEN )
				{
					pid = get_pid(p_buffer + TS_BUFFER_CTL_LEN + buffer_offset_before + count*FRAME_LEN);
					//printf("pid:%d\n",pid);
					if(pid==EMPTY_PACK_PID)//empty package ,not calculate
						continue;
					
					tid = get_tid(p_buffer + TS_BUFFER_CTL_LEN + buffer_offset_before + count*FRAME_LEN);
					if(tid==0xff)
						tid = findrecordtid(pid,package_info_record);//record the tid
					else
						inforecord(pid,tid,package_info_record);
					//printf("tid:%d\n",tid);

				}

				else
				{
					pid = get_pid(p_buffer + TS_BUFFER_CTL_LEN + i*FRAME_LEN);
					//printf("pid:%d\n",pid);
					if(pid==EMPTY_PACK_PID)//empty package ,not calculate
					{
						i++;
						continue;
					}

					tid = get_tid(p_buffer + TS_BUFFER_CTL_LEN  + i*FRAME_LEN);
					i++;
					if(tid==0xff)
						tid = findrecordtid(pid,package_info_record);//record the tid
					else
						inforecord(pid,tid,package_info_record);
					//printf("tid:%d\n",tid);

				}

				for(count_task = 0;count_task <*(int*)p_task_list; count_task++)
				{
					if(pTask->ts_len)
					{
						if(pTask->pid == pid && pTask->tid == tid)
						{
							pTask->pkg_no++;
							break;
						}
					}

					pTask++;
					
				}

				pTask = (TASK_INFO *)(p_task_list + MMAP_TASK_START_OFFS);
			}

			pTask = (TASK_INFO *)(p_task_list + MMAP_TASK_START_OFFS);

		for(count_task = 0;count_task <*(int*)p_task_list; count_task++)
		{
			pTask->rate = pTask->pkg_no*FRAME_LEN*8/MONITOR_INTERVAL;
			pTask++;
		}
			
		}


	else
	{
		for(count_task = 0;count_task <*(int*)p_task_list; count_task++)
		{
			pTask->rate = 0;
			pTask++;
		}
	}
		
}


main()
{
//init tasks mmap
	char taskmmap[128];
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
	
	while(1)
	{
		task_num=0;
		p_mmapinfo=(TASK_INFO*)(p_map_tasks+MMAP_TASK_START_OFFS);

		Task_Code_Rate(p_map_tasks,p_ts_buffer);
	}
}

