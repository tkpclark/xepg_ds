#include "DST_recv.h"

static int logfd;
static char *mdname="DST_cmd_recv";
static char logbuf[1024];
static int DSTsd;
static char *p_map=NULL;

static DST_STREAM dst_stream;
static DST_HDR dst_hdr;
static DST_RESP dst_resp;

static int tasklockfd;
static void proclog_HEX(char *p,int length)
{
	strcpy(logbuf,"\n");
	int i;
	for(i=0;i<length;i++)
		sprintf(logbuf+1+3*i,"%02x ",*(unsigned char *)(p+i));
	proclog(logfd,mdname,logbuf);
}

static read_DST_header()
{
	int n;
	dst_stream.buffer=realloc(dst_stream.buffer,dst_stream.len+DST_HDR_LEN);
	dst_stream.len+=DST_HDR_LEN;
	n=read(DSTsd,dst_stream.buffer,DST_HDR_LEN);
	if(n!=DST_HDR_LEN)
	{
		sprintf(logbuf,"read failed:%d|DST_HDR_LEN \n",n);
		proclog(logfd, mdname,logbuf );
	}
	DST_parse_header(&dst_hdr, &dst_stream);

	//proclog_HEX(dst_stream.buffer, 12);
	
	//sprintf(logbuf,"DST header info: cmd:%d cltid:%d cltkey:%d\n",dst_hdr.cmd,dst_hdr.cltid,dst_hdr.cltkey);
	//proclog(logfd, mdname,logbuf);
}
static init()
{
	//for tcpserver
	DSTsd=open("/dev/null",0);
	dup2(0,DSTsd);
	close(0);
	close(1);

	//open log
	char logfile[128];
	read_config("logfile", logfile);
	logfd=open(logfile,O_CREAT|O_WRONLY|O_APPEND,0600);
	if(logfd==-1)
	{
		printf("open logfile error!\n");
		exit(0);
	}
	//proclog(logfd, mdname, "starting...\n");

	dst_stream.len=0;
	//init mmap
	char mmapfile[128];
	read_config("task_list", mmapfile);
	p_map=init_mmap_use(mmapfile);

	//init lockfd
	tasklockfd=open("task.lck",O_CREAT|O_WRONLY|O_TRUNC,S_IRWXU);
	if(tasklockfd==-1)
	{
		printf("open lock file error!\n");
		exit(0);
	}
	
}
static void get_dsinfo(char *p_mmap_task,int base_id,int ts_id,char *buff)//type:CLTID|PID|TID|INTERVAL|State|TSLEN|RATE|TASK_NAME|TASK_TIME,
{
	int count = 0;
	int str_len = 0;
	char timeStr[64];

	//sprintf(logbuf,"base:%d,ts:%d\n",base_id,ts_id);
	//proclog(logfd,mdname,logbuf);

	TASK_INFO *p_mmapinfo=NULL;
	p_mmapinfo = (TASK_INFO*)(p_mmap_task+MMAP_TASK_START_OFFS);

	for(count = 0;count < *(int *)p_mmap_task;count++)
	{
		#if 1
		//str_len = strlen(buff);
		//buff += str_len;
		if(!p_mmapinfo->cltid)
		{
			p_mmapinfo++;
			continue;
		}
		if(base_id && p_mmapinfo->base_id!=base_id)
		{
			p_mmapinfo++;
			continue;
		}	
		if(ts_id && p_mmapinfo->ts_id!=ts_id)
		{
			p_mmapinfo++;
			continue;
		}
		
		sprintf(buff,"%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s,",
			p_mmapinfo->cltid,p_mmapinfo->base_id,p_mmapinfo->ts_id,
			p_mmapinfo->pid,p_mmapinfo->tid,p_mmapinfo->version,
			p_mmapinfo->interval,p_mmapinfo->state,p_mmapinfo->ts_len,
			p_mmapinfo->rate,p_mmapinfo->tname,
			timet_to_str(p_mmapinfo->uptm,timeStr)
			);
		#endif
		p_mmapinfo++;
		buff +=strlen(buff);
	}
	*(unsigned char *)(buff-1)=0;
	
}
static int write_ts_file(DST_CMD1* p_dst_cmd1)
{
	if(p_dst_cmd1->ts_len==6)//null task
		return 0;
	int fd;
	char ts_file[128];
	get_ts_filename(p_dst_cmd1->p_dst_hdr->cltid,ts_file);
	fd=open(ts_file,O_CREAT|O_TRUNC|O_WRONLY,0600);
	if(fd<0)
	{
		sprintf(logbuf,"open %s failed \n",ts_file);
		proclog(logfd, mdname, logbuf);
		dst_resp.code=10;
		return -1;
	}
	if(myflock(fd, 1))
	{
		sprintf(logbuf,"flock failed! %s",strerror(errno));
		proclog(logfd,mdname,logbuf);
	}
	if(write(fd,p_dst_cmd1->ts_buffer,p_dst_cmd1->ts_len)!=p_dst_cmd1->ts_len)
	{
		sprintf(logbuf,"writing %s failed \n",ts_file);
		proclog(logfd, mdname, logbuf);
		dst_resp.code=11;
		close(fd);
		return -1;
	}
	if(myflock(fd, 2))
	{
		sprintf(logbuf,"flock failed! %s",strerror(errno));
		proclog(logfd,mdname,logbuf);
	}
	close(fd);

	//sprintf(logbuf,"ts file location: %s\n",ts_file);
	//proclog(logfd, mdname, logbuf);
	return 0;
}
static int write_mmap_file(DST_CMD1* p_dst_cmd1)
{
	int i;
	int task_type=2;
	TASK_INFO *p;
	int find_flag=0;


	unsigned short newpid=get_pid(p_dst_cmd1->ts_buffer);
	unsigned short newtableid=get_table_id(p_dst_cmd1->ts_buffer);
	
	myflock(tasklockfd, 1);

	p=(TASK_INFO *)(p_map+MMAP_TASK_START_OFFS);

	//check if there is already a task in the mmap
	for(i=0;i<*(int*)p_map;i++)
	{
		//if(p->cltid==p_dst_cmd1->p_dst_hdr->cltid)
		if(p->pid==newpid && p->tid==newtableid)
		{
			find_flag=1;
			break;
		}
		p++;
	}
	//if there is no this task ,then make a new place
	if(!find_flag)
	{
		p=(TASK_INFO *)(p_map+MMAP_TASK_START_OFFS);
		for(i=0;i<*(int*)p_map;i++)
		{
			if(p->cltid==0)//means empty
			{
				break;
			}
			p++;
		}
	}

	//whether ocuppy has been changed
	if(i>=*(int*)p_map)
	{
		(*(int*)p_map)+=1;
	}


	//copy infomation to mmap

	//memset(p,0,sizeof(TASK_INFO));
	
	p->DeviceId=p_dst_cmd1->DeviceId;
	p->ChannelId=p_dst_cmd1->ChannelId; 
	p->cltid=p_dst_cmd1->p_dst_hdr->cltid;
	p->interval=p_dst_cmd1->interval;
	p->ts_len=p_dst_cmd1->ts_len;
	p->base_id=p_dst_cmd1->base_id;
	p->ts_id=p_dst_cmd1->stream_id;
	p->pid=newpid;
	p->tid=newtableid;
	if( (p->tid!=0x70)&&(p->tid!=0x73) )
		p->version=get_version(p_dst_cmd1->ts_buffer);
	else
		p->version=99;//no version
	p->uptm=time(0);
	get_table_name(p->tid, p->tname);
	if(p_dst_cmd1->ts_len==6) //null task
	{
		p->state=4;
	}
	else
	{
		p->state=1;
	}
	p->isupdated=1;

	
	myflock(tasklockfd, 2);

	sprintf(logbuf,"cltid[%d]baseid[%d]tsid[%d]pid[%d]tid[%d]version[%d]interval[%d]tslen[%d]tm[%d]",
				p->cltid,
				p->base_id,
				p->ts_id,
				p->pid,
				p->tid,
				p->version,
				p->interval,
				p->ts_len,
				p->uptm);
	proclog(logfd, mdname, logbuf);
	
	//sprintf(logbuf,"ocupy_num:%d\n",*(int*)p_map);
	//proclog(logfd, mdname, logbuf);


	
			
	
	
}
static char is_in_version_table(int cltid,char *version_clt_buf)
{
	char tmp[16];
	sprintf(tmp,"%d",cltid);
	if(strstr(version_clt_buf,tmp))
	{
		return 1;
	}
	else
	{
		//sprintf(logbuf,"%d not in version table ,quit it!\n",cltid);
		//proclog(logfd,mdname,logbuf);
		return 0;
	}
}
static ps_cltid(char *version_clt_buf)
{
	int count = 0;

	TASK_INFO *p_mmapinfo=NULL;
	p_mmapinfo = (TASK_INFO*)(p_map+MMAP_TASK_START_OFFS);

	for(count = 0;count < *(int *)p_map;count++)
	{
		if(p_mmapinfo->state==1)
		{
			if(!is_in_version_table(p_mmapinfo->cltid,version_clt_buf))
			{
				p_mmapinfo->state=3;
				p_mmapinfo->isupdated=1;
				p_mmapinfo->uptm=time(0);
				sprintf(logbuf,"%d is not in xepg_version,quit it",p_mmapinfo->cltid);
				proclog(logfd,mdname,logbuf);
			}
		}
		p_mmapinfo++;
	}
}
static ps_DST_cmd1()//start a task
{
	int n;
	DST_CMD1 dst_cmd1;
	/*read interval and ts_len*/
	dst_stream.buffer=realloc(dst_stream.buffer,dst_stream.len+ DST_TS_BUF_OFFS-DST_CMD1_BODY_OFFS);
	dst_stream.len+=DST_TS_BUF_OFFS-DST_CMD1_BODY_OFFS;
	
	n=read(DSTsd,dst_stream.buffer+DST_CMD1_BODY_OFFS,DST_TS_BUF_OFFS-DST_CMD1_BODY_OFFS);
	if(n!=DST_TS_BUF_OFFS-DST_CMD1_BODY_OFFS)
	{
		sprintf(logbuf,"read failed:%d|8 \n",n);
		proclog(logfd, mdname,logbuf );
	}

	//proclog_HEX(dst_stream.buffer, 40);
	//hdr
	dst_cmd1.p_dst_hdr=&dst_hdr;
	
	//deviceID
	dst_cmd1.DeviceId=ntohl(*(int*)(dst_stream.buffer+DST_DEVID_OFFS));

	//channelID
	dst_cmd1.ChannelId=ntohl(*(int*)(dst_stream.buffer+DST_CHNID_OFFS));
	
	//interval
	dst_cmd1.interval=ntohl(*(int*)(dst_stream.buffer+DST_INTERVAL_OFFS));

	//stream_id
	dst_cmd1.stream_id=ntohs(*(unsigned short*)(dst_stream.buffer+DST_STREAM_OFFS));

	//base
	dst_cmd1.base_id=ntohl(*(int*)(dst_stream.buffer+DST_BASE_OFFS));

	//length
	dst_cmd1.ts_len=ntohl(*(int*)(dst_stream.buffer+DST_TS_LEN_OFFS));

	//sprintf(logbuf,"ts[%d]base[%d]interval[%d]ts_len[%d]pid[]",\
		//dst_cmd1.stream_id,dst_cmd1.base_id,dst_cmd1.interval,dst_cmd1.ts_len);
	//proclog(logfd, mdname,logbuf );	

	
	/*read ts buffer*/
	
	dst_stream.buffer=realloc(dst_stream.buffer,dst_stream.len+dst_cmd1.ts_len);//for interval and ts_len
	dst_stream.len+=dst_cmd1.ts_len;
	
	n=recv(DSTsd,dst_stream.buffer+DST_TS_BUF_OFFS,dst_cmd1.ts_len,MSG_WAITALL);
	if(n!=dst_cmd1.ts_len)
	{
		sprintf(logbuf,"read failed:%d|%d \n",n,dst_cmd1.ts_len);
		proclog(logfd, mdname,logbuf );
	}

	dst_cmd1.ts_buffer=malloc(dst_cmd1.ts_len);

	memcpy(dst_cmd1.ts_buffer,dst_stream.buffer+DST_TS_BUF_OFFS,dst_cmd1.ts_len);

	/*write_ts_file*/
	write_ts_file(&dst_cmd1);

	/*write mmap file*/
	write_mmap_file(&dst_cmd1);
}
static ps_DST_cmd2()//query one task's staus
{
	int i;
	DST_CMD2 dst_cmd2;
	dst_cmd2.p_dst_hdr=&dst_hdr;
	TASK_INFO *p;
	p=(TASK_INFO *)(p_map+MMAP_TASK_START_OFFS);
	for(i=0;i<*(int*)p_map;i++)
	{
		if(p->cltid==dst_cmd2.p_dst_hdr->cltid)
		{
			if(p->state==1)//is sending
				dst_resp.code=1;
			else //is not sending
				dst_resp.code=0;
			
			return;
		}
		p++;
	}
	dst_resp.code=-1;//no this task
}

static ps_DST_cmd5()//query tasks info ,for monitor of operation platform
{
	char buf[102400];
	int num=0;
	DST_CMD5 dst_cmd5;

	int n=0;
	n=read(DSTsd,dst_stream.buffer+12,8);
	if(n!=8)
	{
		sprintf(logbuf,"read failed:%d|8 \n",n);
		proclog(logfd, mdname,logbuf );
	}
	
	dst_cmd5.base_id=ntohl(*(int*)(dst_stream.buffer+DST_CMD5_BASE_OFFS));

	dst_cmd5.ts_id=ntohl(*(int*)(dst_stream.buffer+DST_CMD5_TSID_OFFS));

	//get info
	get_dsinfo(p_map, dst_cmd5.base_id,dst_cmd5.ts_id,buf+4);
	num=strlen(buf+4);
	*(int*)buf=htonl(num);

	//print length
	//sprintf(logbuf,"len:%d\n",num);
	//proclog(logfd,mdname,logbuf);

	//print content
	//proclog(logfd, mdname, buf+4);

	//convert to network
	//htonl(*(int*)buf);

	//send
	if(writeall(DSTsd,buf,num+4))
		proclog(logfd,mdname,"send failed!\n");
	else
		;//proclog(logfd,mdname,"send successfully!\n");

	exit(0);
}

static ps_DST_cmd6()//compare with xepg_verstion table
{
	char buf[102400];
	int num=0;
	int all_cltid_len=0;
	DST_CMD5 dst_cmd5;

	int n=0;

	//read all_cltid_len
	memset(buf,0,4);
	n=read(DSTsd,buf,4);
	if(n!=4)
	{
		sprintf(logbuf,"read failed:%d|4 \n",n);
		proclog(logfd, mdname,logbuf );
	}
	
	all_cltid_len=ntohl(*(unsigned int *)buf);
	sprintf(logbuf,"all_cltid_len:%d\n",all_cltid_len);
	proclog(logfd,mdname,logbuf);


	//recv all cltid
	if(all_cltid_len > sizeof(buf))
	{
		proclog(logfd,mdname,"all cltid len too long!\n");
		exit(0);
	}
	memset(buf,0,all_cltid_len);
	n=read(DSTsd,buf,all_cltid_len);
	if(n!=all_cltid_len)
	{
		sprintf(logbuf,"read failed:%d|%d \n",n,all_cltid_len);
		proclog(logfd, mdname,logbuf );
	}
	
	proclog(logfd,mdname,buf);


	ps_cltid(buf);
	exit(0);//replied already ,don't need go gurther
}


static ps_DST_cmd10()//cancel a task
{
	int i;
	DST_CMD10 dst_cmd10;
	dst_cmd10.p_dst_hdr=&dst_hdr;
	TASK_INFO *p;
	p=(TASK_INFO *)(p_map+MMAP_TASK_START_OFFS);
	for(i=0;i<*(int*)p_map;i++)
	{
		if(p->cltid==dst_cmd10.p_dst_hdr->cltid)
		{
			p->state=3;
			p->fd=0;
			p->isupdated=1;
			p->uptm=time(0);
			break;
		}
		p++;
	}
	sprintf(logbuf,"cancel task:%d",dst_cmd10.p_dst_hdr->cltid);
	proclog(logfd,mdname,logbuf);
		
}
static DST_response()
{
	int n;
	DST_com_resp(&dst_resp, &dst_stream);
	n=write(DSTsd,dst_stream.buffer,dst_stream.len);
	if(n!=dst_stream.len)
	{
		sprintf(logbuf,"failed to reply the client %d|%d\n",n,dst_stream.len);
		proclog(logfd,mdname,logbuf);
	}

	//sprintf(logbuf,"return to client %d,code:%d\n",dst_hdr.cltid,dst_resp.code);
	//proclog(logfd,mdname,logbuf);
}
main()
{

	alarm(60);
	init();	
	read_DST_header();
	
	switch(dst_hdr.cmd)
	{
		case 1://send task request
			ps_DST_cmd1();
			break;
		case 2://query task state
			ps_DST_cmd2();
			break;
		case 5://query task state
			ps_DST_cmd5();
			break;
		case 6://data of verstion
			ps_DST_cmd6();
			break;
		case 10://cancel sending task
			ps_DST_cmd10();
			break;
		default:
			sprintf(logbuf,"unknow cmd:%d\n",dst_hdr.cmd);
			proclog(logfd,mdname,logbuf);
			
	}
	DST_response();

	
}

