#include"DST_send.h"
#include"sk.h"
static char logbuf[2048];
extern int logfd;
extern char mdname[];



void DST_com_header(DST_HDR *p_dst_hdr,DST_STREAM *p_dst_stream)
{
	//cmd
	*(int*)(p_dst_stream->buffer+DST_CMD_OFFS)=htonl(p_dst_hdr->cmd);
	
	//client id
	*(int*)(p_dst_stream->buffer+DST_CLTID_OFFS)=htonl(p_dst_hdr->cltid);

	//key
	*(int*)(p_dst_stream->buffer+DST_CLTKEY_OFFS)=htonl(p_dst_hdr->cltkey);
}


void DST_parse_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream)
{
	p_dst_resp->code=ntohl(*(int*)(p_dst_stream->buffer+DST_CODE_OFFS));
}

void DST_com_cmd1(DST_CMD1 *p_dst_cmd1,DST_STREAM *p_dst_stream)
{
	p_dst_stream->buffer=malloc(p_dst_cmd1->ts_len+DST_TS_BUF_OFFS);
	//DST_HDR
	DST_com_header(p_dst_cmd1->p_dst_hdr,  p_dst_stream);

	//DeviceID
	*(int*)(p_dst_stream->buffer+DST_DEVID_OFFS)=htonl(p_dst_cmd1->DeviceId);

	//ChannelID
	*(int*)(p_dst_stream->buffer+DST_CHNID_OFFS)=htonl(p_dst_cmd1->ChannelId);
	
	//interval
	*(int*)(p_dst_stream->buffer+DST_INTERVAL_OFFS)=htonl(p_dst_cmd1->interval);

	//length
	*(int*)(p_dst_stream->buffer+DST_TS_LEN_OFFS)=htonl(p_dst_cmd1->ts_len);

	//ts_buffer
	memcpy(p_dst_stream->buffer+DST_TS_BUF_OFFS,p_dst_cmd1->ts_buffer,p_dst_cmd1->ts_len);

	p_dst_stream->len=DST_TS_BUF_OFFS+p_dst_cmd1->ts_len;
}


void DST_com_cmd10(DST_CMD10 *p_dst_cmd10,DST_STREAM *p_dst_stream)
{
	//DST_HDR
	p_dst_stream->buffer=malloc(DST_HDR_LEN);
	DST_com_header(p_dst_cmd10->p_dst_hdr,  p_dst_stream);
	p_dst_stream->len=DST_HDR_LEN;
}

void DST_com_cmd2(DST_CMD2 *p_dst_cmd2,DST_STREAM *p_dst_stream)
{
	//DST_HDR
	p_dst_stream->buffer=malloc(DST_HDR_LEN);
	DST_com_header(p_dst_cmd2->p_dst_hdr,  p_dst_stream);
	p_dst_stream->len=DST_HDR_LEN;
}

void DST_com_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream)
{
	*(int*)(p_dst_stream->buffer+DST_CODE_OFFS)=htonl(p_dst_resp->code);
	p_dst_stream->len=4;
	
}


int DST_send_data(DST_STREAM *p_dst_stream,DST_STREAM *p_dst_stream_resp,char *dip,short dport)
{
	skt_s *sp;
	int maxredo=2;
	int n=0;
	int send_bytes=0;
	sp=tconnect(dip,dport,maxredo);
	if(!sp)
	{
		return -1;
	}
	//send data

	writeall(sp->sd, p_dst_stream->buffer, p_dst_stream->len);

	//printf("send %d bytes to ds server!\n",n);
	if((n=read(sp->sd,p_dst_stream_resp->buffer,sizeof(DST_RESP)))!=sizeof(DST_RESP))
	{
		printf("send data to %s:%d failed!\n",dip,dport);
		sclose(sp);
		return -3;
	}
	p_dst_stream_resp->len=n;
	

	//recv response
	sclose(sp);
	return 0;
}

int DST_send_cmd1(TS_BUFFER_INFO *p_ts_buffer_info,int cltid,int interval,unsigned short stream_id,int base_id,char *ip,short port)
{
	int i;
	char temp[128];
	DST_HDR dst_hdr;
	dst_hdr.cltid=cltid;
	dst_hdr.cltkey=333;
	dst_hdr.cmd=1;
	
	DST_CMD1 dst_cmd1;
	dst_cmd1.ChannelId=0;
	dst_cmd1.DeviceId=0;
	dst_cmd1.interval=interval;
	dst_cmd1.stream_id=stream_id;
	dst_cmd1.base_id=base_id;
	dst_cmd1.p_dst_hdr=&dst_hdr;
	dst_cmd1.ts_buffer=malloc(p_ts_buffer_info->len);
	memcpy(dst_cmd1.ts_buffer,p_ts_buffer_info->buffer,p_ts_buffer_info->len);
	dst_cmd1.ts_len=p_ts_buffer_info->len;

	

	DST_STREAM dst_stream;
	DST_com_cmd1(&dst_cmd1, &dst_stream);
	
	free(dst_cmd1.ts_buffer);

	sprintf(logbuf,"ip[%s]port[%d]cmd[%d]cltid[%d]key[%d]itv[%d]len[%d]",
		ip,
		port,
		dst_cmd1.p_dst_hdr->cmd,
		dst_cmd1.p_dst_hdr->cltid, 
		dst_cmd1.p_dst_hdr->cltkey,
		dst_cmd1.interval,
		dst_cmd1.ts_len);
	
	if(p_ts_buffer_info->len==0)
	{
		proclog(logfd,mdname,logbuf);
		return -1;
	}
	
	int n;
	DST_STREAM dst_stream_resp;
	DST_RESP dst_resp;
	dst_stream_resp.buffer=malloc(sizeof(DST_RESP));
	
	n=DST_send_data(&dst_stream, &dst_stream_resp,ip, port);
	sprintf(temp,"rt[%d]",n);
	strcat(logbuf,temp);
	if(n<0)
	{
		proclog(logfd,mdname,logbuf);
		return -1;
	}
	DST_parse_resp(&dst_resp, &dst_stream_resp);

	sprintf(temp,"code[%d]",dst_resp.code);
	strcat(logbuf,temp);
	proclog(logfd,mdname,logbuf);
	return dst_resp.code;
	//printf("return code:%d\nHEX:",dst_resp.code);
	//print_HEX(dst_stream_resp.buffer,dst_stream_resp.len);
	
}

int DST_send_cmd10(int cltid,char *ip,short port)//stop a task
{
	DST_HDR dst_hdr;
	dst_hdr.cltid=cltid;
	dst_hdr.cltkey=333;
	dst_hdr.cmd=10;

	DST_CMD10 dst_cmd10;
	dst_cmd10.p_dst_hdr=&dst_hdr;

	DST_STREAM dst_stream;
	DST_com_cmd10(&dst_cmd10, &dst_stream);

	printf("cmd:%d\ncltid:%d\ncltkey:%d\n",
		dst_cmd10.p_dst_hdr->cmd,
		dst_cmd10.p_dst_hdr->cltid, 
		dst_cmd10.p_dst_hdr->cltkey);

	int n;
	DST_STREAM dst_stream_resp;
	DST_RESP dst_resp;
	dst_stream_resp.buffer=malloc(sizeof(DST_RESP));
	
	n=DST_send_data(&dst_stream, &dst_stream_resp,ip, port);
	if(n<0)
	{
		return -1;
	}
	DST_parse_resp(&dst_resp, &dst_stream_resp);
	return dst_resp.code;
	//printf("return code:%d\nHEX:",dst_resp.code);
	//print_HEX(dst_stream_resp.buffer,dst_stream_resp.len);
}

int DST_send_cmd2(int cltid,char *ip,short port)//querya task
{
	DST_HDR dst_hdr;
	dst_hdr.cltid=cltid;
	dst_hdr.cltkey=333;
	dst_hdr.cmd=2;

	DST_CMD2 dst_cmd2;
	dst_cmd2.p_dst_hdr=&dst_hdr;

	DST_STREAM dst_stream;
	DST_com_cmd2(&dst_cmd2, &dst_stream);

	/*
	printf("cmd:%d\ncltid:%d\ncltkey:%d\n",
		dst_cmd2.p_dst_hdr->cmd,
		dst_cmd2.p_dst_hdr->cltid, 
		dst_cmd2.p_dst_hdr->cltkey);

	*/
	int n;
	DST_STREAM dst_stream_resp;
	DST_RESP dst_resp;
	dst_stream_resp.buffer=malloc(sizeof(DST_RESP));
	
	n=DST_send_data(&dst_stream, &dst_stream_resp,ip, port);
	if(n<0)
	{
		return -1;
	}
	DST_parse_resp(&dst_resp, &dst_stream_resp);
	return dst_resp.code;
	//printf("return code:%d\nHEX:",dst_resp.code);
	//print_HEX(dst_stream_resp.buffer,dst_stream_resp.len);
}


