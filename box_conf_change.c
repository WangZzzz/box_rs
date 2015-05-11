/*************************************************************************     
  > File Name: box_conf_change.c
  > Created Time: Wed Apr 22 21:31:13 2015
  > Function: 
 ************************************************************************/     

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<unistd.h>
#include<sys/types.h>
#include<sqlite3.h>
#include<error.h>
#include "ghttp.h"
#include "cJSON.h"
#include <time.h>

#define box_db "/ailvgo/www/database/box.db"
#define domain_db "/ailvgo/system/domain/domain.db"
#define staff_ctl "/ailvgo/system/traffic/staff_ctl"
#define wifi_dbm "/ailvgo/system/traffic/wifi_dbm"

# define logfile "/ailvgo/system/log/sys_log"

char *send_http_request(ghttp_request *req, char *uri);
void status(ghttp_request *r, char *desc);
void change_thread(char *type, char *value);

int change_flag = 0;

void sys_log(char *str)
{
	FILE *fp;
	char content[300]={0};
        
	struct tm *timeinfo;
	time_t boxtime = 0;
	time(&boxtime);
	timeinfo = localtime(&boxtime);

	fp = fopen(logfile,"a");
	sprintf(content, "%s ----- %s\n", asctime(timeinfo), str);
	fputs(content,fp);
	fclose(fp);
}


void ParseJSON(cJSON *node)
{
	cJSON *fileArray = cJSON_GetObjectItem(node, "box_change");
	char *type = NULL, *value = NULL;
        
    printf("Parse JSON node...\n");

	if(!fileArray)
	{
              printf("parse result : json has no node named box_change\n");
              return;
    }
	
    cJSON *fileList = fileArray->child;
	while(fileList != NULL)
	{
		type = cJSON_GetObjectItem(fileList, "type")->valuestring;
        value = cJSON_GetObjectItem(fileList, "value")->valuestring;
		printf("parse result :\n");
        printf("type :  %s\n", type);
        printf("value : %s\n", value);

		change_thread(type, value);
		
		fileList = fileList->next;
	}
}

void change_thread(char *type, char *value)
{
	char target[200] = {0};
	char loginfo[200] = {0};

	strcpy(target, "box_id,province_id,province_name,city_id,city_name,site_id,site_name,area_id,area_name,guide_id,guide_name,type,tel,wifi_detect");

	if(strstr(target, type) != NULL)
	{
		printf("box_info | box.db will change!\n");
		
		sqlite3 *db = NULL;
        int rc = 0;
        char sql_cmd[100] = {0};
		char *errorMsg = NULL;
		int db_cnt = 0;
		
		while(db_cnt < 20)
		{
			rc = sqlite3_open(box_db, &db);
			if(rc == SQLITE_ERROR)
				printf("open box.db failed");

			sprintf(sql_cmd, "update box_info set %s=\'%s\'", type, value);
			printf("sql_cmd : %s\n", sql_cmd);
			if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg) == SQLITE_OK)
			{
				printf("update box.db successfully!\n");
				sqlite3_close(db);
				break;
			}
			else
			{
				printf("update box.db failure! %s\n", errorMsg);
				sqlite3_close(db);
				sleep(1);
				db_cnt++;
			}
		}
		
		sprintf(loginfo, "box_info | box.db, set %s=%s", type, value);
		sys_log(loginfo);
		
		change_flag = 1;

		return;
	}
	else if(strcmp(type, "enable") == 0 )
	{
		printf("domain_control | domain.db will change!\n");
		
		sqlite3 *db = NULL;
        int rc = 0;
        char sql_cmd[100] = {0};
		char *errorMsg = NULL;
		int db_cnt = 0;
		
		while(db_cnt < 20)
		{
			rc = sqlite3_open(domain_db, &db);
        	if(rc == SQLITE_ERROR)
                printf("open domain.db failed");
		
			sprintf(sql_cmd, "update domain_control set enable=\'%s\'", value);
            printf("sql_cmd : %s\n", sql_cmd);
            if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg) == SQLITE_OK)
			{
				printf("update domain.db successfully!\n");
				sqlite3_close(db);
				break;
			}
			else
			{
				printf("update domain.db failure! %s\n", errorMsg);
				sqlite3_close(db);
				sleep(1);
				db_cnt++;
			}
		}

		sprintf(loginfo, "domain_control | domain.db, set %s=%s", type, value);
		sys_log(loginfo);
		
		change_flag = 1;

		return;
	}
	else if(strcmp(type, "staff_ctl") == 0 )
	{
		printf("staff_ctl will change!\n");
		
		FILE *fp;
		fp = fopen(staff_ctl, "w");
		fputs(value, fp);
		fclose(fp);

		sprintf(loginfo, "staff_ctl,  set %s", value);
		sys_log(loginfo);
		
		change_flag = 1;
		
		return;
	}
	else if(strcmp(type, "wifi_dbm") == 0 )
	{
		printf("wifi_dbm will change!\n");
		
		FILE *fp;
		fp = fopen(wifi_dbm, "w");
		fputs(value, fp);
		fclose(fp);

		sprintf(loginfo, "wifi_dbm,  set %s", value);
		sys_log(loginfo);
		
		change_flag = 1;

		return;
	}

}

int main()
{
	printf("\n********************************\n");
	printf("box_conf_change : start\n");
	printf("********************************\n");
		
	char url[1024] = {0};
	ghttp_request *req;
	char http_body[1024] = {0};

	sqlite3 *db = NULL;
    sqlite3_stmt *ppstmt = NULL;
    int rc = 0;
    char sql_cmd[100] = {0};
	char *errorMsg = NULL;
	const char *box_id = NULL;
    char box_id_tmp[10] = {0};
	int db_cnt = 0;

	while(db_cnt < 20)
	{
		rc = sqlite3_open(box_db, &db);
		if(rc == SQLITE_ERROR)
            printf("open box.db failed");

        ppstmt = NULL;
        memset(sql_cmd, 0, sizeof(sql_cmd));
        strcpy(sql_cmd, "select box_id from box_info");
        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        rc = sqlite3_step(ppstmt);
        if(rc == SQLITE_ROW)
        {
            box_id = sqlite3_column_text(ppstmt, 0);
            strcpy(box_id_tmp, box_id);
            printf("box_id : %s\n", box_id_tmp);
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			break;
		}
		else
		{
			printf("select box_id failure!\n");
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			sleep(1);
			db_cnt++;
		}
	}

	printf("----------------send http request-------------\n");
	sprintf(url, "http://www.ailvgobox.com/box_manage_2/box_conf_change_1.php?box_id=%s", box_id_tmp);
	printf("request_url : %s\n",url);

	req = ghttp_request_new();
	strcpy(http_body, send_http_request(req, url));
    ghttp_request_destroy(req);
	
	printf("http_body : %s\n", http_body);
    printf("length of http_body : %d\n", strlen(http_body));
        
	cJSON *node;

    if(strlen(http_body) <= 2)
		printf("HTTP failure!\n");
    else
    {
        printf("HTTP success!\n");

        if(strcmp(http_body, "null") == 0)
            printf("http_body : null, no box_conf_change!\n");
        else
        {
	        node = cJSON_Parse(http_body);
            ParseJSON(node);
        }
    }
	
	printf("box_conf_chanage : complete!\n");

	if(change_flag == 1)
	{
		printf("reboot due to conf change!\n");
		sleep(5);
		system("reboot");
	}
}

void status(ghttp_request *r, char *desc)
{
	ghttp_current_status st;
	st = ghttp_get_status(r);
	fprintf(stderr, "%s: %s [%d/%d]\n",
	desc,
	st.proc == ghttp_proc_request ? "request" :
	st.proc == ghttp_proc_response_hdrs ? "response-headers" : 
	st.proc == ghttp_proc_response ? "response" : "none",
	st.bytes_read, st.bytes_total);
}

char *send_http_request(ghttp_request *req, char *uri)
{
	#define malloc_size 5120
	ghttp_status req_status;
	unsigned long rec_bytes_total = 0;
	unsigned long buffer_size = 0;
	unsigned long rec_bytes_current = 0;

	char *buffer = (char *)malloc(sizeof(char) * malloc_size);
	if(!buffer)
	{
		printf("malloc space error\n");
		return NULL;
	}
	else
	{
		memset(buffer, 0, malloc_size);
		buffer_size = malloc_size;
	}

	if(ghttp_set_uri(req, uri) < 0)
	{
		printf("ghttp_set_uri\n");
		return NULL;
	}
	ghttp_set_header(req,http_hdr_Connection,"close");
	char timeout_str[10];
	sprintf(timeout_str,"%d",5000);
	ghttp_set_header(req,http_hdr_Timeout,timeout_str);
	
	if(ghttp_prepare(req) < 0)
	{
		printf("ghttp_prepare\n");
		return NULL;
	}
	if(ghttp_set_type(req, ghttp_type_get) == -1)
	{
		printf("ghttp_set_type\n");
		return NULL;
	}
	if(ghttp_set_sync(req, ghttp_async) < 0)
	{
		printf("ghttp_set_sync\n");
		return NULL;
	}

	do {
		status(req, "conn");
		req_status = ghttp_process(req);

		if(req_status == ghttp_error)
		{
			fprintf(stderr, "ghttp_process: %s\n", ghttp_get_error(req));
			return "";
		}
		if(req_status != ghttp_error && ghttp_get_body_len(req) > 0)
		{
			rec_bytes_current = ghttp_get_body_len(req);
			rec_bytes_total += rec_bytes_current;

			while(rec_bytes_total > buffer_size)
			{
				buffer = (char *)realloc(buffer, buffer_size + malloc_size);
				if(!buffer)
				{
					printf("realloc error\n");
					return NULL;
				}
				buffer_size += malloc_size;
			}

			strncat(buffer, ghttp_get_body(req), rec_bytes_current);
			buffer[rec_bytes_total] = '\0';
			ghttp_flush_response_buffer(req);
		}
	} while(req_status == ghttp_not_done);

	ghttp_clean(req);
	return buffer;
}
