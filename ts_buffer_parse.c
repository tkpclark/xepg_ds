#include "ds.h"


main()
{

	char ts_buffer[32];
	char *p_ts_buffer = NULL;
	int buffer_offset_before,buffer_offset_after;
	read_config("ts_buffer", ts_buffer);

	p_ts_buffer = init_mmap_use(ts_buffer);
	int i;
	int num;
	char time[32];

	while(1)
	{
		buffer_offset_before = *(int *)(p_ts_buffer + TS_BUFFER_READER_OFFS);
		my_nano_sleep(700000000);
		//sleep(1);
		buffer_offset_after = *(int *)(p_ts_buffer + TS_BUFFER_READER_OFFS);
		if(buffer_offset_after-buffer_offset_before<522272)
			printf("%s,%d\n",get_time(time),buffer_offset_after-buffer_offset_before);
		
		
		//printf("%d:%d\n",buffer_offset_before,buffer_offset_after);
		
		num=0;
		for(i=buffer_offset_before;i<buffer_offset_after;i+=188)
		{
			/*
			if(\
				//(*(unsigned char *)(p_ts_buffer+TS_BUFFER_CTL_LEN+i+0)==0x47)\
	//			 (get_pid(p_ts_buffer+TS_BUFFER_CTL_LEN+i)==23)\
			//&&  (*(unsigned char *)(p_ts_buffer+TS_BUFFER_CTL_LEN+i+5)==0xae)\
			//&&  (*(unsigned char *)(p_ts_buffer+TS_BUFFER_CTL_LEN+i+8)==0x4)\
			//&&  (*(unsigned char *)(p_ts_buffer+TS_BUFFER_CTL_LEN+i+9)==0x1a)\
			)
			*/
			if (get_pid(p_ts_buffer+TS_BUFFER_CTL_LEN+i)==23)
				num++;
			//print_HEX(p_ts_buffer+TS_BUFFER_CTL_LEN+buffer_offset_before, 188);
		}
		if(num<700)
		{
			printf("%s,num:%d\n",get_time(time),num);
			
		}
		
		
	}
}
