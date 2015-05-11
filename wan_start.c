/*************************************************************************     
  > File Name: wan_start.c
  > Created Time: 2015-03-06
  > Function: 
 ************************************************************************/     

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sqlite3.h>

#define box_db "/ailvgo/www/database/box.db"

int sql_data();

int main()
{
	int sh;
	char sh_path[256]={0};

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
	{
		printf("usb/wifi module\n");
	}

	if(sh == 20)
	{
		printf("usb/ethernet module\n");
	}
}

int sql_data()
{
	int rc=0,wan_driver;
	char sql[256]={0};
	sqlite3 *db=NULL;
	sqlite3_stmt *ppstmt=NULL;

	rc=sqlite3_open(box_db, &db);
	if(rc != SQLITE_OK)
	{
		printf("open database error!!\n");
		return 0;
	}

	memset(sql,0,sizeof(sql));
	sprintf(sql,"select wan_driver from box_info");
	sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"SQL error:%s\n",sqlite3_errmsg(db));
	}
	rc=sqlite3_step(ppstmt);
	if(rc ==SQLITE_ROW)
	{
		wan_driver=sqlite3_column_int(ppstmt,0);
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
		//printf("wan_driver : %d\n",wan_driver);
		return wan_driver;
	}
	else
	{
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
		return 0;
	}
}


