/***********************
 * Filename : udp_move.c
 * Date : 2015-03-05
 * *********************/

#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include <time.h>

# define traffic_total_db "/ailvgo/system/traffic/traffic_total.db"


main(int argc, char** argv)
{
    printf("****************************************\n"); 
	printf("udp_move : start\n");
    printf("****************************************\n");
	
	int rc = 0;
	char sql_cmd[250] = {0};
	sqlite3 *db;
	sqlite3_stmt *ppstmt;
	const char *mobile = NULL;
	char *errmsg = NULL;
	int t =0;
	char upload_mac[5000][18] = {0};
	int upload_t[5000];
	int i = 0, j = 0;
	int	cnt = 0;
	char db_path [200] = {0};

	t = time(NULL)+8*3600;

	printf("--------get mac from traffic_total.db in 10mins----------\n");

	while(cnt < 20)
	{
		rc = sqlite3_open(traffic_total_db, &db);
		if(rc == SQLITE_ERROR)
		{
			printf("open traffic_total.db failed");
			return;
		}

		ppstmt = NULL;
		memset(sql_cmd, 0, sizeof(sql_cmd));
		sprintf(sql_cmd, "select mobile,time from traffic_total where time>%d-600 and time<=%d", t, t);
		sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
		rc = sqlite3_step(ppstmt);
		if(rc != SQLITE_ROW)
		{
			printf("select traffic_total.db error!\n");
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			usleep(300);
			cnt++;
			continue;
		}
		else
		{
			while(rc == SQLITE_ROW)
			{
				mobile = sqlite3_column_text(ppstmt, 0);
				strcpy(upload_mac[i], mobile);
				printf("upload_mac : %s\n", upload_mac[i]);
				t = sqlite3_column_int(ppstmt, 1);
				upload_t[i] = t;

				rc = sqlite3_step(ppstmt);
				i++;
			}
		}
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
		break;
	}


	printf("--------store mac in boxid_date.db in 10mins----------\n");

	printf("boxid_date.db : %s\n", *(argv+1));

	rc = sqlite3_open(*(argv+1), &db);
	if(rc == SQLITE_ERROR)
	{
		printf("open boxid_date.db error!\n");
		return;
	}

	for(j = 0; j < i; j++)
	{
		memset(sql_cmd, 0, sizeof(sql_cmd));
		sprintf(sql_cmd, "insert into traffic_info (mobile, time) values ('%s','%ld')", upload_mac[j], upload_t[j]);
		printf("sql_cmd : %s\n", sql_cmd);
		rc = sqlite3_exec(db,sql_cmd,NULL,NULL,&errmsg);
		if(SQLITE_OK!=rc)
			printf("insert %s error : %s\n", *(argv+1), errmsg);
	}
	
	printf("traffic_move : complete!\n");
	
	exit(0);
}

