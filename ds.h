#define MAX_CHLD_NUM 500
#define MMAP_TASK_START_OFFS 4
#define MMAP_PID_START_OFFS 4
#define MONITOR_INTERVAL 1
#define MAX_PID_GROUP_NUM 200
#define MAX_PID_GROUP_MEMBER 10


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <arpa/inet.h>
#include "ts_buffer.h"
#include "ctrl_flag.h"
#include<dirent.h>
#include "color.h"

typedef struct
{
    	int cltid;
	unsigned int interval;
	unsigned int ts_len;
	int state; // 1:is sending 2:not define 3:stopped 4 ts format error 
	int isupdated;// 0:wait to update pid mmap 1:already updated pid mmap
	int chld_pid;
	int DeviceId;
	int ChannelId;
	unsigned short pid;
	unsigned short tid;
	char tname[32];
	time_t uptm;
	unsigned short ts_id;
	int base_id;
	unsigned char version;
	int rate;
	int pkg_no;
	double send_num;
	double should_send_num;
	int fd;
	char frame_buffer[8192];
	int frame_number;

}TASK_INFO;

typedef struct
{
	TASK_INFO *p_task;
	int section_number;
	char *p_circle_buffer;
	int has_written_section;
	int has_written_frame;
	double section_distance;
	double frame_distance;
	int last_first_section_postion;
	int circle_times;
	int is_finished;
	double miss_distance;
}PID_MEMBER;

typedef struct
{
	int fd;
	char filename[128];
	unsigned int filesize;
	int last_position;
	double theory_position;
	double distance;
	double pre_distance;
	int interval;
	int status;//0:ok, -1 :not ok
	int has_written_frame;
	unsigned short pid;
	int all_section_number;
	int all_frame_number;
	int member_number;
	time_t uptm;
	PID_MEMBER member[MAX_PID_GROUP_MEMBER];
}PID_GROUP;

typedef struct
{
	char buffer[4096*2];
	unsigned int len;
}SEC_BUFFER_INFO;

typedef struct
{
	char table_id;
	char table_name[32];
}TABLE_INFO;





char *init_mmap0(const char *pathname,unsigned int msize);
void proclog(int fd, char *mdname,char *str);
char* init_mmap_use(const char *pathname);
int myflock(int lockfd,char type);
void print_HEX(char *p,int length);
int getfilepid(const char *pidfile);
char *get_ts_filename(int cltid,char *filename);

/*
static const char *MMAP_FILE="data.mmap";
static const char *LOG_FILE="../log/ds.log";
static const char *TS_FILE_DIR="../tsfiles/";
*/

