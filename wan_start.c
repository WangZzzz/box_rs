/*************************************************************************     
  > File Name: wan_start.c
  > Created Time: 2015-05-09
 ************************************************************************/     

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sqlite3.h>

#define box_db "/ailvgo/www/database/box.db"

#define PING_FREQUENCY 2

int sql_data();

int wan_monitor()
{
	FILE *stream;
	char buf_qq[1024] = {0};
	int cnt;
	int wan_break = 1;
	
	memset(buf_qq, 0, sizeof(buf_qq));
	
	for(cnt = 0; cnt < PING_FREQUENCY; ++cnt)
	{
		printf("ping -c 1 www.qq.com\n");
        stream = popen("ping -c 1 www.qq.com", "r");
		fread(buf_qq, sizeof(char), sizeof(buf_qq), stream);
		buf_qq[1023] = '\0';
        pclose(stream);
		printf("buf_qq : %s\n", buf_qq);
		
		if(strstr(buf_qq, "1 received"))
        {   
			wan_break = 0;
			break;
        }

		sleep(5);
	}

	if(wan_break == 1)
		return 1;
	else
	 	return 0;
}

int main()
{
	printf("\n------------------------------\n");
	printf("wan_start : start!\n");
	printf("-------------------------------\n");

	int sh;
	char sh_path[256]={0};

	if(wan_monitor() == 0)
	{
		printf("wan : started!\n");
		exit(0);
	}

	sh=sql_data();

	if(sh==0)
	{
		printf("no wan needed!");
		exit(0);
	}

	if(sh == 1)
	{
		printf("3G module : LC5730\n");
		printf("pppd call gprs-connect\n");
		system("pppd call gprs-connect &");
	}
	
	if(sh == 2)
	{
		printf("3G module : MC2716\n");
		printf("pppd call evdo-connect\n");
		system("pppd call evdo-connect &");
	}

	if(sh == 3)
	{
		printf("3G module : MF210\n");
		printf("pppd call wcdma-connect\n");
		system("pppd call wcdma-connect &");
	}
	
	if(sh == 4)
	{
		printf("3G module : LS6300v\n");
		printf("pppd call wcdma-ls-connect\n");
		system("pppd call wcdma-ls-connect &");
	}

	if(sh == 10)
		printf("usb/wifi module\n");

	if(sh == 20)
		printf("usb/ethernet module\n");

	printf("wan_start : complete!\n");
}

int sql_data()
{
	int rc=0,wan_driver;
	char sql[256]={0};
	sqlite3 *db=NULL;
	sqlite3_stmt *ppstmt=NULL;
	int cnt;

	while(cnt < 20)
	{
		rc=sqlite3_open(box_db, &db);
		if(rc != SQLITE_OK)
			printf("open database error!!\n");

		memset(sql,0,sizeof(sql));
		sprintf(sql,"select wan_driver from box_info");
		sqlite3_prepare(db,sql,-1,&ppstmt,0);
		rc=sqlite3_step(ppstmt);
		if(rc ==SQLITE_ROW)
		{
			wan_driver=sqlite3_column_int(ppstmt,0);
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			return wan_driver;
		}
		else
		{
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			sleep(1);
			cnt++;
		}
	}
	return 0;
}

