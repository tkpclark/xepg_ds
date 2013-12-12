#define FRAME_LEN 188
#include "DST_send.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
int logfd;
char mdname[]="DSTcmd";
main(int argc,char **argv)
{
	int n;
	
	if(!strcmp(argv[1],"stop"))
		n=DST_send_cmd10(atoi(argv[2]),argv[3],atoi(argv[4]));
	else if(!strcmp(argv[1],"query"))
		n=DST_send_cmd2(atoi(argv[2]),argv[3],atoi(argv[4]));
	else
	{
		printf("what you've entered is wrong!\n");

		printf("stop:stop clitid key\n");
		printf("query:query clitid key\n");
	}

	printf("return:%d\n",n);
}

