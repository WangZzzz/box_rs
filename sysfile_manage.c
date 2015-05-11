/**************************
 * Filename : sysfile_manage.c
 * Date : 2015-03-05
 * ***********************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <ghttp.h>
#include "cJSON.h"
#include "md5.h"
#include "sqlite3.h"
#include <time.h>

#define sysfile_db "/ailvgo/system/box/sysfile.db"

#define logfile "/ailvgo/system/log/sys_log"

int download_flag = 0;

void sys_log(char *str)
{
	FILE *fp;
	char content[300]={0};
        
	struct tm *timeinfo;
	time_t boxtime = 0;
	time(&boxtime);
	timeinfo = localtime(&boxtime);

	fp = fopen(logfile, "a");
	sprintf(content, "%s ----- %s\n", asctime(timeinfo), str);
	fputs(content,fp);
	fclose(fp);
}

int download_thread(char *file_name, char *md5);

char *send_http_request(ghttp_request *req, char *uri);
	
void bail(char *s)
{
        fputs(s, stderr);
        fputc('\n', stderr);
        return;
}

void ParseJSON(cJSON *node)
{
	cJSON *fileArray = cJSON_GetObjectItem(node, "sysfile_info");
	char *file_name = NULL, *box_folder = NULL, *version = NULL, *md5 = NULL;
        
	sqlite3 *db = NULL;
    int rc = 0;
    sqlite3_stmt *ppstmt = NULL;
    char sql_cmd[256] ={0};
    char *errorMsg = NULL;
    const char *ver;
	char ver_tmp[5]= {0};

	char loginfo[200] = {0};

	printf("Parse JSON node...\n");
	
    cJSON *fileList = fileArray->child;
	while(fileList != NULL)
	{
		file_name = cJSON_GetObjectItem(fileList, "file_name")->valuestring;
		box_folder = cJSON_GetObjectItem(fileList, "box_folder")->valuestring;
		version = cJSON_GetObjectItem(fileList, "version")->valuestring;
		md5 = cJSON_GetObjectItem(fileList, "md5")->valuestring;
			
		printf("----------parse result-----------\n");
        printf("file_name : %s\n", file_name);
        printf("box_folder : %s\n", box_folder);
        printf("version : %s\n", version);
        printf("md5 : %s\n", md5);
		
		printf("checking %s in sysfile_info | sysfile.db\n", file_name);

		rc = sqlite3_open(sysfile_db, &db);
		if(rc == SQLITE_ERROR)
		{
			printf("cannot open sysfile.db!\n");
            return;
        }

        sprintf(sql_cmd, "select version from sysfile_info where filename=\'%s\'", file_name);
		printf("sql_cmd : %s\n", sql_cmd);

        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        rc = sqlite3_step(ppstmt);
		if(rc == SQLITE_ROW)
        {
            ver = sqlite3_column_text(ppstmt, 0);
            strcpy(ver_tmp, ver);
            printf("version : %s\n", ver_tmp);
			
			if(strcmp(ver_tmp, version) < 0)
			{
				printf("%s isn't newest progream, download!\n", file_name);
				
				if(download_thread(file_name, md5) == 1)
				{
					printf("%s download successfully!\n", file_name);
					
					printf("update version in sysfile_info | sysfile.db\n");
					sprintf(sql_cmd, "update sysfile_info set version=\'%s\' where filename=\'%s\'", version, file_name);
					printf("sql_cmd : %s\n", sql_cmd);
                    if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg) != SQLITE_OK)
						printf("errorMsg : %s\n", errorMsg);
					
					printf("insert sysfile info in sysfile_update | sysfile.db\n");
					sprintf(sql_cmd, "insert into sysfile_update (filename, filepath) values(\'%s\', \'%s\')", file_name, box_folder);
					printf("sql_cmd : %s\n", sql_cmd);
                    if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg) != SQLITE_OK)
						printf("errorMsg : %s\n", errorMsg);

             		sprintf(loginfo, "sysfile : %s, operation : update, version = %s", file_name, version);

					download_flag = 1;
				}
				else
					printf("%s download failure!\n");
			}
			else
				printf("%s is newest program, no download!\n", file_name);
        }
		else
		{		
			printf("%s is new program, download!\n", file_name);
			
			if(download_thread(file_name, md5) == 1)
			{
				printf("%s download successfully!\n", file_name);
					
				printf("insert sysfile info in sysfile_info | sysfile.db\n");
				sprintf(sql_cmd, "insert into sysfile_info (filename, version) values(\'%s\', \'%s\')", file_name, version);
				printf("sql_cmd : %s\n", sql_cmd);
                if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg) != SQLITE_OK)
					printf("errorMsg : %s\n", errorMsg);

				printf("insert sysfile info in sysfile_update | sysfile.db\n");
				sprintf(sql_cmd, "insert into sysfile_update (filename, filepath) values(\'%s\', \'%s\')", file_name, box_folder);
				printf("sql_cmd : %s\n", sql_cmd);
                if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg) != SQLITE_OK)
					printf("errorMsg : %s\n", errorMsg);

             	sprintf(loginfo, "sysfile : %s, operation : insert, version = %s", file_name, version);

				download_flag = 1;
			}
			else
				printf("%s download failure!\n", file_name);
		}
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);

		fileList = fileList->next;
	}
}

int download_thread(char *file_name, char *md5)
{
	FILE *stream = NULL;
	char result[1024] = {0};
	char sh_cmd[512] = {0};
	char rm_cmd[150] = {0};
	char filename_all[100] = {0};
	char request_url[100] = {0};	
	char *md5_value = NULL;
	unsigned int idx = 0, ipid = 0;
        
	printf("---------------------------------\n");
    printf("download sysfile :  start\n");
    printf("---------------------------------\n");

	memset(result, 0, sizeof(result));
    memset(sh_cmd, 0, sizeof(sh_cmd));
    memset(filename_all, 0, sizeof(filename_all));
	memset(request_url, 0, sizeof(request_url));
	sprintf(filename_all, "/ailvgo/system/temp/%s", file_name);
	printf("filename_all : %s\n", filename_all);
    memset(rm_cmd, 0, sizeof(rm_cmd));
    sprintf(rm_cmd, "rm -f %s", filename_all);

	sprintf(sh_cmd, "curl -o /ailvgo/system/temp/%s http://www.ailvgobox.com/sysfile_2.0/%s", file_name, file_name);
    printf("sh_cmd : %s\n", sh_cmd);
	if((stream = popen(sh_cmd, "r")) == NULL)
        printf("popen failed\n");
	memset(result, 0, sizeof(result));
	fread(result, sizeof(char), sizeof(result), stream);
	pclose(stream);	
    result[1023] = '\0';
	printf("curl result : %s\n", result);
	if(strlen(result) > 0)
	{
		printf("curl : error!\n");
		system("killall curl");		
		return 0;
	}

    if(access(filename_all, W_OK) == 0)
    {
		printf("download finish, check md5...\n");
		md5_value = md5_file(filename_all);
        if(!md5_value)
			return 0;
        
		printf("md5 : %s -- %s\n", md5_value, md5);

        if(strcmp(md5, md5_value) == 0)
		{
           	printf("md5 : corret, download ok!\n");

			return 1;
		}
		else
		{
			printf("md5 : error, download failure!\n");
            system(rm_cmd);
			return 0;
		}
    }
	else
	{
		printf("no file exist!\n");
		return 0;
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
	#define MALLOC_SIZE 10240
	ghttp_status req_status;
	unsigned long rec_bytes_total = 0;
	unsigned long buffer_size = 0;
	unsigned long rec_bytes_current = 0;

	char *buffer = (char *)malloc(sizeof(char) * MALLOC_SIZE);
	if(!buffer)
		bail("malloc space error");
	else
	{
        memset(buffer, 0, MALLOC_SIZE);
		buffer_size = MALLOC_SIZE;
	}
 
	if(ghttp_set_uri(req, uri) < 0)
		bail("ghttp_set_uri");
	if(ghttp_prepare(req) < 0)
		bail("ghttp_prepare");
	if(ghttp_set_type(req, ghttp_type_get) == -1)
		bail("ghttp_set_type");
	if(ghttp_set_sync(req, ghttp_async) < 0)
		bail("ghttp_set_sync");
	
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
				printf("****\n");
				buffer = (char *)realloc(buffer, sizeof(char)*(buffer_size + MALLOC_SIZE));
				if(!buffer)
					bail("realloc error");
				buffer_size += MALLOC_SIZE;
			}
			strncpy(buffer, ghttp_get_body(req), rec_bytes_current);
			buffer[rec_bytes_total] = '\0';
		}
	} while(req_status == ghttp_not_done);  
    
	ghttp_flush_response_buffer(req);
	
	ghttp_clean(req);
	return buffer;
}

int main()
{
	ghttp_request *req;
	char http_body[10000] = {0};
	sqlite3 *db = NULL;
    int rc = 0;
    sqlite3_stmt *ppstmt = NULL;
    char sql_cmd[100] ={0};
    char *errorMsg = NULL;
	char request_url[100] = {0};

	memset(http_body, 0, sizeof(http_body));

    printf("*******************************\n");
	printf("sysfile_manage : start\n");
    printf("*******************************\n");
        
	printf("--------------send sysfile_manage request--------------\n");
	strcpy(request_url, "http://www.ailvgobox.com/box_manage_2/sysfile_manage_1.php?hw=2.0");
	printf("request_url : %s\n", request_url);
    req = ghttp_request_new();
	strcpy(http_body, send_http_request(req, request_url));
    ghttp_request_destroy(req);

	printf("http_body : %s\n", http_body);
    printf("length of http_body : %d\n", strlen(http_body));
    
    cJSON *node;

    if(strlen(http_body)==0)
	{
		printf("HTTP failure!\n");
	}
    else
    {
      	printf("HTTP success!\n");

		if(strcmp(http_body, "null") == 0)
			printf("http_body : null, no sysfile_info!\n");
		else
		{
			node = cJSON_Parse(http_body);
			ParseJSON(node);
		}
    }
	
	printf("sysfile_manage : complete !\n");

	if(download_flag == 1)
	{
		printf("sysfile : update! reboot now!\n");
		sleep(2);
		system("reboot");
	}
}

