#include"DST_recv.h"
#include"sk.h"


void DST_parse_header(DST_HDR *p_dst_hdr,DST_STREAM *p_dst_stream) 
{
	//cmd
	p_dst_hdr->cmd=ntohl(*(int*)(p_dst_stream->buffer+DST_CMD_OFFS));
	
	//client id
	p_dst_hdr->cltid=ntohl(*(int*)(p_dst_stream->buffer+DST_CLTID_OFFS));

	//key
	p_dst_hdr->cltkey=ntohl(*(int*)(p_dst_stream->buffer+DST_CLTKEY_OFFS));
	
}

void DST_com_resp(DST_RESP *p_dst_resp,DST_STREAM *p_dst_stream)
{
	*(int*)(p_dst_stream->buffer+DST_CODE_OFFS)=htonl(p_dst_resp->code);
	p_dst_stream->len=4;
	
}

