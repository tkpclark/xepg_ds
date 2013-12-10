#include "ds.h"

main(int argc,char **argv)
{
	if(argc!=2)
	{
		printf("ts_buffer or task_list or pid_list?\n ");
		exit(0);
	}
	char mmapfile[32];
	int mmap_len;
	
	strcpy(mmapfile,argv[1]);
	if(!strcmp(mmapfile,"task_list"))
	{
		read_config(mmapfile, mmapfile);
		printf("mmapfile:%s\n",mmapfile);
		mmap_len=4+sizeof(TASK_INFO)*MAX_CHLD_NUM;
	}
	else if(!strcmp(mmapfile,"ts_buffer"))
	{
		read_config(mmapfile, mmapfile);
		printf("mmapfile:%s\n",mmapfile);
		mmap_len=TS_BUFFER_MAX_LEN;

	}
	else if(!strcmp(mmapfile,"pid_list"))
	{
		read_config(mmapfile, mmapfile);
		printf("mmapfile:%s\n",mmapfile);
		mmap_len=4+sizeof(PID_GROUP)*MAX_PID_GROUP_NUM;

	}
	else
	{
		printf("filename is wrong!\nfilename:%s\n",mmapfile);
		exit(0);
	}

	
	//create mmap
	if(init_mmap0(mmapfile, mmap_len)!=NULL)
	{
		printf("%s init successfully! Length:%d\n",mmapfile,mmap_len);
		
	}
	else
	{
		printf("%s init failed!\n",mmapfile);
	}

}

