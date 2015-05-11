/*************************************************************************     
  > File Name: udp_start.c
  > Date :2015-03-06
 ************************************************************************/     

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sqlite3.h>

#define udp_get "/ailvgo/system/traffic/udp_get &"

#define udp_daemon "/ailvgo/system/box/udp_daemon &"

#define box_db  "/ailvgo/www/database/box.db"

int sql_select();

int main()
{
	if(sql_select() == 1)
	{
		system(udp_get);
		sleep(5);
		system(udp_daemon);
		sleep(5);
	}
	return 0;
}

int sql_select()
{
	int rc=0;
	char sql[256]={0};
	sqlite3 *db;
	sqlite3_stmt *ppstmt;
	const char *wifi_detect=NULL;

	rc=sqlite3_open(box_db,&db);
	if(rc!=SQLITE_OK)
	{
		printf("open database error!\n");
		return 0;
	}
	memset(sql,0,sizeof(sql));
	sprintf(sql,"select wifi_detect from box_info");
	rc=sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if(rc!=SQLITE_OK)
	{
		fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
	}
	rc=sqlite3_step(ppstmt);
	if(rc==SQLITE_ROW)
	{
		wifi_detect=sqlite3_column_text(ppstmt,0);
		if(strcmp(wifi_detect,"on")==0)
		{
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			return 1;
		}
		else if(strcmp(wifi_detect,"off")==0)
		{
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			return 0;
		}
	}
	sqlite3_finalize(ppstmt);
	sqlite3_close(db);
	return 0;
}
