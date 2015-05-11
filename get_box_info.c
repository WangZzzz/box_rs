/*************************************************************************     
    > File Name: get_box_info.c
  > Created Time: 2014年09月10日 星期三 15时01分04秒
  > Function:获取为服务器信息 
 ************************************************************************/     

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sqlite3.h>
#include<time.h>
#include"cJSON.h"
#define MODEL 2
//1--develop model  2-- user model
//struct tm* get_time();//获取时间的函数
int  get_time(int n);//获取时间的函数
void data_ins(char *ptr1,char *ptr2);
void data_sel();
//void json_print();

int main()
{
	char *msg=NULL;
	char mobile[200]={0},time[100]={0},time_dest[100]={0};

	msg = getenv("QUERY_STRING");
//	printf("env:%s\n",getenv("QUERY_STRING"));
//	printf("msg:%s\n",msg);
	if(msg==NULL)
	{
//	  printf("1\n");
	  printf("error:no msg!\n");
	  return 0;
	}
	sscanf(msg,"mobile=%[^&]&time=%s",mobile,time);
//	printf("msg:%s\n",msg);
//	printf("mobile:%s\n",mobile);
//	printf("time:%s\n",time);
	if(get_time(1)>2010)
	{
		sprintf(time_dest,"%4d%02d%02d%02d%02d",get_time(1),get_time(2),get_time(3),get_time(4),get_time(5));
	}
	else
	{
		strcpy(time_dest,time);
	}
	printf("Content-type:text/html;charset=utf-8\n\n");
	data_ins(mobile,time_dest);//更新数据库
	data_sel();
//	json_print();//输出json数据
	
	return 0;
}

int get_time(int n) //获取时间
{
	struct tm *t;
	time_t tt;     
	time(&tt);
	t=localtime(&tt);
	switch(n)
	{
		case 1:
			return t->tm_year+1900;
			break;
		case 2:
			return t->tm_mon+1;
			break;
		case 3:
			return t->tm_mday;
			break;
		case 4:
			return t->tm_hour;
			break;
		case 5:
			return t->tm_min;
			break;
		default:
			return 1;
	}
}
void data_ins(char *ptr1,char *ptr2)
{
	int rc=0;
	char sql[256]={0};
	char *errmsg;
	sqlite3 *db;

	rc = sqlite3_open("/ailvgo/www/database/mobile.db",&db);
	if((rc != SQLITE_OK)&&(MODEL==1))
	{
		printf("open database error_ins!\n");
	}
	memset(sql,0,sizeof(sql));
	sprintf(sql,"insert into mobile_info (mobile,time) values('%s','%s') ",ptr1,ptr2);
	rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
	if((rc!=SQLITE_OK)&&(MODEL==1))
	{
		printf("%s\n",errmsg);
	}
//	if(rc == SQLITE_OK)
//	{
//		printf("ok");
//	}
	sqlite3_close(db);
}

void data_sel()
{
	int rc=0;
	char *out;
	char sql[250]={0};
	const char *box_id=NULL,*city_id=NULL,*city_name=NULL,*site_id=NULL,*site_name=NULL,*area_id=NULL,*area_name=NULL,*guide_id=NULL,*guide_name=NULL,*type=NULL,*tel=NULL,*gps=NULL,*province_id=NULL,*province_name=NULL;
	sqlite3 *db;
	sqlite3_stmt *ppstmt=NULL;
	cJSON *root;

	rc = sqlite3_open("/ailvgo/www/database/box.db",&db);
	if((rc != SQLITE_OK)&&(MODEL==1))
	{
		printf("open database error_sel!\n");
	}
//	printf("mobile:%s\n",ptr1);
//	printf("time:%s\n",ptr2);
	memset(sql,0,sizeof(sql));
	sprintf(sql,"select box_id,city_id,city_name,site_id,site_name,area_id,area_name,guide_id,guide_name, type,province_id,province_name,tel,gps from box_info");
	rc=sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if((rc != SQLITE_OK)&&(MODEL==1))
	{
		fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
	}
	rc=sqlite3_step(ppstmt);
	if(rc==SQLITE_ROW)
	{
		root=cJSON_CreateObject();
		while(rc == SQLITE_ROW)
		{
			box_id=sqlite3_column_text(ppstmt,0);
			city_id=sqlite3_column_text(ppstmt,1);
			city_name=sqlite3_column_text(ppstmt,2);
			site_id=sqlite3_column_text(ppstmt,3);
		//	printf("site_id:%s\n",site_id);
			site_name=sqlite3_column_text(ppstmt,4);
			area_id=sqlite3_column_text(ppstmt,5);
			area_name=sqlite3_column_text(ppstmt,6);
			guide_id=sqlite3_column_text(ppstmt,7);
			guide_name=sqlite3_column_text(ppstmt,8);
			type=sqlite3_column_text(ppstmt,9);
			province_id=sqlite3_column_text(ppstmt,10);
			province_name=sqlite3_column_text(ppstmt,11);
			tel=sqlite3_column_text(ppstmt,12);
			gps=sqlite3_column_text(ppstmt,13);

			cJSON_AddStringToObject(root,"type",  type);  	
			cJSON_AddStringToObject(root,"box_id",  box_id);  	
			cJSON_AddStringToObject(root,"province_id",  province_id);  	
			cJSON_AddStringToObject(root,"province_name",province_name); 	
			cJSON_AddStringToObject(root,"city_id", city_id);  	
			cJSON_AddStringToObject(root,"city_name",city_name);
			if(NULL != site_id)			  
			{
				cJSON_AddStringToObject(root,"gps",gps);
				cJSON_AddStringToObject(root,"tel",tel);
				cJSON_AddStringToObject(root,"site_id",site_id);
				cJSON_AddStringToObject(root,"site_name",site_name);
				cJSON_AddStringToObject(root,"area_id",area_id);
				cJSON_AddStringToObject(root,"area_name",area_name);
				if(NULL != guide_id)
				{
					cJSON_AddStringToObject(root,"guide_id",guide_id);
					cJSON_AddStringToObject(root,"guide_name",guide_name);
				}
			}
			rc = sqlite3_step(ppstmt);
		}
		out =cJSON_Print(root);
		printf("%s\n",out);
//		printf("1");
		cJSON_Delete(root);
		free(out);
	}
	else
	{
		printf("");
	}

	sqlite3_finalize(ppstmt);
	sqlite3_close(db);

}

/*
void json_print()
{
	FILE *fp;
	char json[500]={0};
	fp=fopen("json1.txt","r");
	if(fp==NULL)
	{
		printf("open file failed!");
	}
	fread(json,1,500,fp);
	printf("%s\n",json);
	fclose(fp);
}
*/
