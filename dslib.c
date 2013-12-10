#include "ds.h"
/*
extern int logfd;
extern char mdname[];
static char logbuf[10240];
*/
static TABLE_INFO table_info[]=
{
	{0x40,"NIT Actural"},
	{0x4a,"BAT"},
	{0x42,"SDT Actural"},
	{0x46,"SDT Other"},
	{0x4e,"EIT PF Actural"},
	{0x4f,"EIT PF Other"},
	{0x50,"EIT SC Actural"},
	{0x60,"EIT SC Other"},
	{0x70,"TDT"},
	{0x73,"TOT"},
	{0xaa,"AAT"},
	{0xab,"PDT actual"},
	{0xac,"PDT other"},
	{0xad,"PFT"},
	{0xae,"FTT ad"},
	{0x0,"PAT"},
	{0xaf,"FST"},
	{0x2,"PMT"},
	{-1,"Unknown"}
};
void proclog(int fd, char *mdname,char *str)
{
	char ts[32];
	char buf[10240];
	time_t tt;
	
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%x %X",(const struct tm *)localtime(&tt));
	sprintf(buf,GREEN"[%s]"NONE YELLOW"[%s]"NONE"%s\n",ts,mdname,str);
	write(fd,buf,strlen(buf));
}

void get_cur_time(char *tmStr)
{
	time_t tt;
	
	tt=time(0);
	strftime(tmStr,30,"%F %X",(const struct tm *)localtime(&tt));
}
char * timet_to_str(time_t time,char *tmStr)
{
	strftime(tmStr,30,"%F %X",(const struct tm *)localtime(&time));
	return tmStr;
}
void get_table_name(char tid,char *table_name)
{
	int i=0;
	for(i=0;i<1000;i++)
	{
		//printf("i:%d,table_id:%d,tid:%d\n",i,table_info[i].table_id,tid);
		if( (table_info[i].table_id==-1)||(table_info[i].table_id==tid) )
		{
			strcpy(table_name,table_info[i].table_name);
			return;
		}
	}
}
void print_HEX(char *p,int length)
{
	int i;
	for(i=0;i<length;i++)
	{
		printf("%02X ",*(unsigned char*)(p+i));
	}
	printf("\n");
}
char* init_mmap_use(const char *pathname)
{
	int fd;
	char *map=NULL;
	struct stat statbuf;
	fd=open(pathname,2);
	if(fd<0)
	{
		return NULL;
	}
	if (fstat(fd, &statbuf) < 0)
	{

		return NULL;
	}
	if ((map = mmap(0, statbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0)) == MAP_FAILED)
	{

		return NULL;
	}
	if (map==(char*)-1)
		map=0;
	close(fd);
	//syslog(LOG_INFO,"%d",statbuf.st_size);
	return map;
}

char *init_mmap0(const char *pathname,unsigned int msize)
{
	int fd;
	char *p=NULL;
	char *p_map=NULL;
	fd=open(pathname,O_CREAT|O_TRUNC|O_RDWR,0600);
	if(fd<0)
	{
		return NULL;
	}
	p=malloc(msize);
	memset(p,0,msize);

	if(write(fd,p,msize)!=msize)
	{
		return NULL;
	}
	
	if ((p_map = mmap(0, msize, PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0)) == MAP_FAILED)
	{
		return NULL;
	}
	if (p_map==(char*)-1)
		p_map=0;
	close(fd);

	return p_map;
}

int myflock(int lockfd,char type)
{
		if(type==1)
		{	
	        if(flock(lockfd,LOCK_EX))
	        {
	        		return -1;
	        }
	        //proclog(logfd,"lock");
	    }
	    else if(type==2)
	    {
	    	if(flock(lockfd,LOCK_UN))
	        {
				return -1;
	        }
	        //proclog(logfd,"unlock");
	    }
	    return 0;
}
void my_nano_sleep(unsigned long nsec)
{
	struct timespec slptm;
	slptm.tv_sec = nsec/1000000000;
	slptm.tv_nsec = nsec%1000000000; 
	nanosleep(&slptm, NULL);
}
char *get_time(char *buf)
{
	time_t tt;
	
	tt=time(0);
	strftime(buf,30,"%x %X",(const struct tm *)localtime(&tt));
	return buf;
}
int getfilepid(const char *pidfile)
{
	int fd;
	char buf[16]={0};
	
	fd=open(pidfile,0);
	if(fd<0)
	{
		return 100;
	}
	memset(buf,0,sizeof(buf));
	read(fd,buf,sizeof(buf)-1);
	close(fd);
	return atoi(buf);
}
/*
void proclog_HEX(char *p,int length)
{
	char logbuf[10240];
	strcpy(logbuf,"\n");
	int i;
	for(i=0;i<length;i++)
		sprintf(logbuf+1+3*i,"%02x ",*(unsigned char *)(p+i));
	proclog(logfd,mdname,logbuf);
}
*/
/*
void  PrintToFile(int fd,char *mdname,char *format, ...)
{
	char buffer[1024*10];
	
	 va_list argPtr;
	 va_start(argPtr, format);
	 (void)vsprintf((char*)buffer, format, argPtr);
	 va_end(argPtr);
	proclog(fd, mdname,buffer);
}
*/
int is_first_frame(char *p)
{
	int section_header_flag=*(unsigned char *)(p+1)  & 0x40;
	if(section_header_flag==0x40)
		return 1;
	else
		return 0;
}
int is_first_section(char *p)
{
	int section_header_flag=*(unsigned char *)(p+1)  & 0x40;
	unsigned char section_number=*(unsigned char *)(p+0xb)  ;
	if( (section_header_flag==0x40) && (section_number==0x00) )
		return 1;
	else
		return 0;
}
char *get_ts_filename(int cltid,char *filename)
{
	char tasktsdir[128];
	read_config("tasktsdir", tasktsdir);
	sprintf(filename,"%s%d.ts",tasktsdir,cltid);
	return filename;
}
 gen_empty_pkg(char *NULL_buf)
{
	memset(NULL_buf,0xFF,FRAME_LEN);
	NULL_buf[0]=0x47;
	NULL_buf[1]=0x1F;
	NULL_buf[2]=0xFF;
	NULL_buf[3]=0x10;
}
void reverse_short(short *value)
{
	char ch=0;
	ch=*(unsigned char*)(value);
	*((unsigned char*)(value))=*((unsigned char*)(value)+1);
	*((unsigned char*)(value)+1)=ch;
}
unsigned short get_pid(char *p)
{
	unsigned short pid=0;
	char tmp[2];
	memset(tmp,0,sizeof(tmp));
	memcpy(tmp,p+1,2);
	tmp[0]=tmp[0]&0x1F;
	reverse_short((unsigned short *)tmp);
	pid=*(unsigned short *)tmp;
	return pid;

}
unsigned char get_table_id(char *p)
{
	return *(unsigned char*)(p+5);
}

unsigned char get_version(char *p)
{
	return (*(unsigned char *)(p+10) >> 1 & 0x1F);
}
void set_bits16(char *p,short value)
{
	short _value=value;
	//printf("%d[0x%02x]\n ",value,value);
	//printf("[0x%04X]:",*(unsigned short*)(p));
	reverse_short(&_value);
	*(short*)(p)=*(short*)(p)|_value;
	//printf("[0x%04X]\n",*(unsigned short*)(p));
}
off_t get_file_size(int fd)
{
	struct stat statbuf;
	if(!fstat(fd,&statbuf))
	{
		return statbuf.st_size;
	}
	else
	{
		printf("ALERT:get file stat error! %s\n",strerror(errno));
		return 0;
	}

}

int lock_reg(int fd,int cmd,int type,off_t offset,int whence,off_t len)
{
	struct flock lock;
	lock.l_type=type;
	lock.l_start=offset;
	lock.l_whence=whence;
	lock.l_len=len;

	return (fcntl(fd,cmd,&lock));
}

char get_one_section(int fd,SEC_BUFFER_INFO *p_sbi)
{
	int read_bytes=0;
	char tmp[FRAME_LEN];
	//printf("get one section\n");
	memset(p_sbi->buffer,0,sizeof(p_sbi->buffer));
	p_sbi->len=0;
	
	//read the first 188 bytes
	read_bytes=read(fd,p_sbi->buffer,FRAME_LEN);
	if(read_bytes!=FRAME_LEN)
	{
		
		//sprintf(logbuf,"read_bytes!=FRAME_LEN,read_bytes:%d\n",read_bytes);
		//proclog(logfd,mdname,logbuf);
		//exit(0);
		my_nano_sleep(10000000);
		return -1;
	}
	if((p_sbi->buffer[1] & 0x40) != 0x40)//if it's the first 188 of a section
	{
		
		//sprintf(logbuf,"it's not section header %X%X\n",p_sbi->buffer[0],p_sbi->buffer[1]);
		//proclog(logfd,mdname,logbuf);
		my_nano_sleep(10000000);
		//exit(0);
		
		return -1;
	}
	p_sbi->len+=FRAME_LEN;

	/*
	printf("first 188:   ");
	print_HEX(p_sbi->buffer,p_sbi->len);
	
	printf("after 2nd:\n");
	*/
	while(1)
	{
		memset(tmp,0,sizeof(tmp));
		read_bytes=read(fd,tmp,FRAME_LEN);
		
		//check 
		//printf("read bytes:%d   tmp[1] :0x%x\n",read_bytes,tmp[1] );
		//print_HEX(p_sbi->buffer+p_sbi->len,p_sbi->len);
		if(read_bytes == FRAME_LEN)
		{
			if(tmp[0]!=0x47)
			{
				
				//sprintf(logbuf,"ts error!begin with %X\n",tmp[0]);
				//proclog(logfd,mdname,logbuf);
				//exit(0);
				my_nano_sleep(10000000);
				return -1;
			}
			if((tmp[1] & 0x40) == 0x40)//if it's the first 188 of a section
			{
				lseek(fd,-FRAME_LEN,SEEK_CUR);
				//printf("it's a section header,returning\n");
				return 1;
			}
			else
			{
				memcpy(p_sbi->buffer+p_sbi->len,tmp,FRAME_LEN);
				//printf("it's section body\n");
				p_sbi->len+=FRAME_LEN;
			}
		}
		else if(read_bytes==0)
		{
			return 0;
		}
		else
		{
			//sprintf(logbuf,"read failed!");
			//proclog(logfd,mdname,logbuf);
			my_nano_sleep(10000000);
			return -1;
		}
	}

}

int get_all_section_number(int fd)
{
	/*
	char buf[0xf];
	int n=0
	lseek(fd,SEEK_SET,0);
	n=read(fd,buf,sizeof(buf));
	//print_HEX(buf, sizeof(buf));
	if(n!=sizeof(buf))
	{
		//sprintf(logbuf,"failed at get_all_section_number,return %d\n",n);
		//proclog(logfd,mdname,logbuf);
		return -1;
	}
	//sprintf(logbuf,"xxx:%d",*(unsigned char*)(buf+0xc));
	//proclog(logfd, mdname, logbuf);
	return *(unsigned char*)(buf+0xc)+1;
	*/

	int read_bytes=0;
	char tmp[FRAME_LEN];
	int all_section_number=0;

	while(1)
	{
		memset(tmp,0,sizeof(tmp));
		read_bytes=read(fd,tmp,FRAME_LEN);
		
		if(read_bytes == FRAME_LEN)
		{
			if(tmp[0]!=0x47)
			{
				
				//sprintf(logbuf,"ts error!begin with %X\n",tmp[0]);
				//proclog(logfd,mdname,logbuf);
				//exit(0);
				my_nano_sleep(10000000);
				return -1;
			}
			if((tmp[1] & 0x40) == 0x40)//if it's the first 188 of a section
			{
				all_section_number++;
			}
		}
		else if(read_bytes==0)
		{
			break;
		}
		else
		{
			//sprintf(logbuf,"read failed!");
			//proclog(logfd,mdname,logbuf);
			my_nano_sleep(10000000);
			return -1;
		}
	}
	return all_section_number;
}
off_t get_file_size_f(char *path)
{
	int fd;
	off_t size=0;
	fd=open(path,0);
	if(fd<0)
	{
		printf("open %s failed !\n",path);
		exit(0);
	}
	struct stat statbuf;
	if(!fstat(fd,&statbuf))
	{
		size= statbuf.st_size;
	}
	else
	{
		printf("ALERT:get file stat error! %s\n",strerror(errno));
		size= 0;
	}
	close(fd);
	return size;
}



char* get_cache_file_name(char *pCFF,char *filename)
{
	char tmp[32];
	sprintf(tmp,"cache%d",*pCFF);
	read_config(tmp,filename);
	return filename;
}



int get_tsbuffer_rw_distance(int r,int w,int max)
{
	if(w>=r)
		return (w-r)/FRAME_LEN;
	else 
		return (max-r+w)/FRAME_LEN;
}

int  get_clt_ts_data(char *p,TASK_INFO* p_task)
{
	char filename[128];

	get_ts_filename(p_task->cltid,filename);

	int fd=0;
	fd=open(filename,0);
	if(fd<0)
	{
		printf("ALERT:open %s failed in get_clt_ts_data()\n",filename,strerror(errno));
		return -1;
	}
	if(myflock(fd, 1))
	{
		return -1;
	}
	read(fd,p,p_task->ts_len);
	if(myflock(fd, 2))
	{
		return -1;
	}
	close(fd);
	return 0;
	
	
}

int  get_section_number_from_buffer(char *p_buffer,int len)
{
	int i;
	char *p=p_buffer;
	int section_number=0;
	int frame_number=len/FRAME_LEN;
	for(i=0;i<frame_number;i++)
	{
		if(is_first_frame(p+i*FRAME_LEN))
			section_number++;
	}
	return section_number;
}


