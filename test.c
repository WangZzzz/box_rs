/***********************
* Filename : udp_get.c
* Date : 2015-02-13
************************/

#include <sys/types.h>
#include <sys/socket.h>  
#include <sys/ioctl.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <sqlite3.h>
#include <error.h>
#include <time.h>
#include <stdlib.h>  
#include <string.h>  
#include <stdio.h> 

 
int main()
{

	if(traffic_mv()==0)
            printf("traffic.db mv failure\n");
	else
	    printf("traffic.db mv success!\n");

}


int traffic_mv()
{
	sqlite3 *db;
	char sql[256];
	int rc=0;
	long data=0;
	char *errmsg;
	sqlite3_stmt *ppstmt=NULL;
	
	rc=sqlite3_open("./traffic.db",&db);
	if(rc!=SQLITE_OK)
	{
		fprintf(stderr,"Cannot open traffic.db:%s\n",sqlite3_errmsg(db));
		return 0;
	}
	memset(sql,0,sizeof(sql));
	sprintf(sql,"select count(*) from traffic_info");
	sqlite3_prepare(db,sql,-1,&ppstmt,0);
	rc = sqlite3_step(ppstmt);
	if(rc==SQLITE_ROW)
	{
		data=sqlite3_column_int(ppstmt,0);
		printf("number in traffic.db : %ld\n",data);
	}
	sqlite3_finalize(ppstmt);

	data = 200;
	memset(sql,0,sizeof(sql));
	sprintf(sql,"delete from traffic_info where time in (select time from traffic_info order by time asc limit 0, %ld)", data);
	rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
	if(SQLITE_OK==rc)
	{
		printf("delete traffic.db success!\n");
	}
	else if(rc!=SQLITE_OK)
	{
		printf("delete traffic.db error:%s\n",errmsg);
	}

	memset(sql,0,sizeof(sql));
	sprintf(sql,"vacuum traffic_info");
	rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
	if(SQLITE_OK==rc)
	{
		printf("vacuum traffic.db success!\n");
	}
	else if(rc!=SQLITE_OK)
	{
		printf("vacuum traffic.db error:%s\n",errmsg);
	}
	
	sqlite3_close(db);
        return 1;      
}

