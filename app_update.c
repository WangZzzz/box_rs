/**********************
 *Filename : app_update.c
 *Date : 2015-03-05
 * *******************/

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

# define box_db "/ailvgo/www/database/box.db"
# define app_db "/ailvgo/www/database/app.db"

# define logfile "/ailvgo/system/log/sys_log"

void sys_log(char *str)
{
	FILE *fp;
	char content[100]={0};
        
	struct tm *timeinfo;
	time_t boxtime = 0;
	time(&boxtime);
	timeinfo = localtime(&boxtime);

	fp = fopen(logfile, "a");
	sprintf(content, "%s ----- %s\n", asctime(timeinfo), str);
	fputs(content,fp);
	fclose(fp);
}

char *send_http_request(ghttp_request *req, char *uri);

void bail(char *s)
{
        fputs(s, stderr);
        fputc('\n', stderr);
}

void ParseJSON(cJSON *node)
{
	cJSON *appArray = cJSON_GetObjectItem(node, "app_update");
	char *type = NULL, *version = NULL, *md5 = NULL;
	char type_f[15] = {0}, version_f[10] = {0}, md5_f[33] = {0};

	if(!appArray)
        {
               bail("parse result : json has no node named app_update, no app update!");
               return;
        }

	cJSON *appList = appArray->child;
	while(appList != NULL)
	{
		type = cJSON_GetObjectItem(appList, "type")->valuestring;
		version = cJSON_GetObjectItem(appList, "version")->valuestring;
		md5 = cJSON_GetObjectItem(appList, "md5")->valuestring;
		printf("app info : %s-%s-%s\n", type, version, md5);
		strcpy(type_f, type);
		strcpy(version_f, version);
		strcpy(md5_f, md5);

		if(strcmp(type, "android_phone") == 0)
			process(type_f, version_f, md5_f);
		if(strcmp(type, "android_pad") == 0)
			process(type_f, version_f, md5_f);
		if(strcmp(type, "ios_phone") == 0)
			process(type_f, version_f, md5_f);
		if(strcmp(type, "ios_pad") == 0)
			process(type_f, version_f, md5_f);

		appList = appList->next;
	}
}

int process(char *type, char *version, char *md5)
{
	FILE *stream = NULL;
	char result[1024] = {0};
	char sh_cmd[512] = {0};
	char rm_cmd[150] = {0};
	char filename_all[100] = {0};
	char sql_cmd[300] = {0};
	char *md5_value = NULL, *errorMsg = NULL;
	unsigned int idx = 0, ipid = 0;
	sqlite3 *db_update = NULL;
        int rc_update = 0;

        sqlite3 *db = NULL;
        int rc =0;

	char loginfo[100] ={0};

        memset(sh_cmd, 0, sizeof(sh_cmd));
        memset(filename_all, 0, sizeof(filename_all));

	printf("type : %s\n", type);
	if(strcmp(type, "android_phone") == 0)
	{
		sprintf(sh_cmd, "curl -o /ailvgo/temp/android_phone.zip http://www.ailvgobox.com/app/android_phone/android_phone.zip");
		strcpy(filename_all, "/ailvgo/temp/android_phone.zip");
	}
	else if(strcmp(type, "android_pad") == 0)
	{
		sprintf(sh_cmd, "curl -o /ailvgo/temp/android_pad.zip http://www.ailvgobox.com/app/android_pad/android_pad.zip");
		strcpy(filename_all, "/ailvgo/temp/android_pad.zip");
	}
	else if(strcmp(type, "ios_phone") == 0)
	{
		sprintf(sh_cmd, "curl -o /ailvgo/temp/ios_phone.zip http://www.ailvgobox.com/app/ios_phone/ios_phone.zip");
		strcpy(filename_all, "/ailvgo/temp/ios_phone.zip");
	}	
	else if(strcmp(type, "ios_pad") == 0)
	{
		sprintf(sh_cmd, "curl -o /ailvgo/temp/ios_pad.zip http://www.ailvgobox.com/app/ios_pad/ios_pad.zip");
		strcpy(filename_all, "/ailvgo/temp/ios_pad.zip");
	}
	
	memset(rm_cmd, 0, sizeof(rm_cmd));
	sprintf(rm_cmd, "rm -f %s", filename_all);
	if(access(filename_all, W_OK) == 0)
	{
		printf("app zipfile exist, check md5...\n");

                md5_value = md5_file(filename_all);
               
                printf("md5 : %s -- %s\n", md5_value, md5);

                if(md5_value && strcmp(md5, md5_value) == 0)
                {
                        printf("md5 : correct, download ok!\n");
                       
                        printf("unzip and cp app zipfile...\n");

                        printf("make apptemp folder...\n");
                        system("mkdir -p /ailvgo/tmep/apptemp/");
                        sleep(1);
                        printf("unzip file in apptemp folder...\n");
                        memset(sh_cmd, 0, sizeof(sh_cmd));
                        sprintf(sh_cmd, "unzip -o %s -d /ailvgo/temp/apptemp/", filename_all);
                        printf("sh_cmd : %s\n", sh_cmd);
                        system(sh_cmd);
                        sleep(10);
                        printf("cp files to app folder...\n");
                        system("cp -rf /ailvgo/temp/apptemp/* /ailvgo/www/app/");
                        sleep(10);
                        printf("rm apptemp folder...\n");
                        system("rm -rf /ailvgo/temp/apptemp/");
                        sleep(10);
                        printf("delete zipfile...\n");
                        memset(sh_cmd, 0, sizeof(sh_cmd));
                        sprintf(sh_cmd, "rm -f %s", filename_all);
                        printf("sh_cmd : %s\n", sh_cmd);
                        system(sh_cmd);
                        sleep(2);
                        
                        rc_update = sqlite3_open(app_db, &db_update);
                        if(rc_update == SQLITE_ERROR)
                        {
                                printf("cannot open app.db!\n");
                                return 0;
                        }
			sprintf(sql_cmd, "update app_info set version=\"%s\" where type=\"%s\"", version, type);
                        printf("sql_cmd : %s\n", sql_cmd);
                        sqlite3_exec(db_update, sql_cmd, NULL, NULL, &errorMsg);
			sqlite3_close(db_update);
                       
             		sprintf(loginfo, "app = %s, update version = %s", type, version);
                        sys_log(loginfo);
                        return 1;
                }
		printf("md5 : error, delete appfile...\n");
                system(rm_cmd);
	}

        printf("sh_cmd : %s\n", sh_cmd);
	if((stream = popen(sh_cmd, "r")) == NULL)
        	printf("popen failed\n");
	sleep(15);
	memset(result, 0, sizeof(result));
        fread(result, sizeof(char), sizeof(result), stream);
        result[1023] = '\0';
        printf("curl result : %s\n", result);
	pclose(stream);
        if(strlen(result) > 0)
        {
		printf("curl : error!\n");
		system("killall curl");
		return 0;
        }
        
        if(access(filename_all, W_OK) == 0)
        {
		printf("download app zipfile finish, md5 check...\n");
                md5_value = md5_file(filename_all);
        	if(!md5_value)
		{
			return 0;
		}
        	printf("md5 : %s -- %s\n", md5_value, md5);

        	if(strcmp(md5, md5_value) == 0)
		{
                	printf("md 5 : corret, download ok!\n");
			printf("unzip and cp app zipfile...\n");
                        
                        printf("make apptemp folder...\n");
                        system("mkdir -p /ailvgo/temp/apptemp/");
                        sleep(1);
                        printf("unzip file in apptemp folder...\n");
                        memset(sh_cmd, 0, sizeof(sh_cmd));
                        sprintf(sh_cmd, "unzip -o %s -d /ailvgo/temp/apptemp/", filename_all);
                        printf("sh_cmd : %s\n", sh_cmd);
                        system(sh_cmd);
                        sleep(10);
                        printf("cp files to app folder...\n");
                        system("cp -rf /ailvgo/temp/apptemp/* /ailvgo/www/app/");
                        sleep(10);
                        printf("rm apptemp folder...\n");
                        system("rm -rf /ailvgo/temp/apptemp/");
                        sleep(10);
                        printf("delete zipfile...\n");
                        memset(sh_cmd, 0, sizeof(sh_cmd));
                        sprintf(sh_cmd, "rm -f %s", filename_all);
                        printf("sh_cmd : %s\n", sh_cmd);
                        system(sh_cmd);
                        sleep(2);
 
			rc_update = sqlite3_open(app_db, &db_update);
	        	if(rc_update == SQLITE_ERROR)
        		{
                		printf("cannot open app.db!\n");
                		return 0;
        		}
			sprintf(sql_cmd, "update app_info set version=\"%s\" where type=\"%s\"", version, type);
			printf("sql_cmd : %s\n", sql_cmd);
                        sleep(2);     
                        if(sqlite3_exec(db_update, sql_cmd, NULL, NULL, &errorMsg) != SQLITE_OK)
                        {
                                printf("errorMsg_app_update : %s\n", errorMsg);
                        }
			sqlite3_close(db_update);

             		sprintf(loginfo, "app = %s, update version = %s", type, version);
                        sys_log(loginfo);
		}
        	else
		{
			return 0;
		}
	}
                
	return 1;
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
	#define MALLOC_SIZE 5120
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
				buffer = (char *)realloc(buffer, sizeof(char)*(buffer_size + MALLOC_SIZE));
				if(!buffer)
					bail("realloc error");
				buffer_size += MALLOC_SIZE;
			}
			
			strncat(buffer, ghttp_get_body(req), rec_bytes_current);
                        buffer[rec_bytes_total] = '\0';
                        ghttp_flush_response_buffer(req);
		}
	} while(req_status == ghttp_not_done);  
	
	ghttp_clean(req);
	return buffer;
}

int main()
{
	ghttp_request *req;
	char http_body[300] = {0};
	sqlite3 *db = NULL;
        int rc = 0;
        sqlite3_stmt *ppstmt = NULL;
        char sql_cmd[150] ={0};
	char request_url[200] = {0};
	char android_phone_version[10], android_pad_version[10], ios_phone_version[10], ios_pad_version[10];
        const char *box_id = NULL, *type = NULL, *android_phone = NULL, *ios_phone = NULL, *android_pad = NULL, *ios_pad = NULL;
        char box_id_tmp[10] = {0};
        char *errorMsg = NULL;
        
        printf("**********************************\n");
        printf("app_update : start\n");
        printf("**********************************\n");

	rc = sqlite3_open(box_db, &db);
        if(rc == SQLITE_ERROR)
        {
                printf("cannot open box.db!\n");
                return 0;
        }

        strcpy(sql_cmd, "select box_id from box_info");
        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        rc = sqlite3_step(ppstmt);
	if(rc == SQLITE_ROW)
        {
                box_id = sqlite3_column_text(ppstmt, 0);
                strcpy(box_id_tmp, box_id);
                printf("box_id : %s\n", box_id);
        }
        sqlite3_finalize(ppstmt);
        sqlite3_close(db);

        rc = sqlite3_open(app_db, &db);
        if(rc == SQLITE_ERROR)
        {
                printf("cannot open app.db!\n");
                return 0;
        }

	memset(sql_cmd, 0, sizeof(sql_cmd));
	strcpy(sql_cmd, "select type, version from app_info");
	sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        rc = sqlite3_step(ppstmt);
        while(rc == SQLITE_ROW)
        {
		type = sqlite3_column_text(ppstmt, 0);
		printf("type : %s\n", type);
		if(strcmp(type, "android_phone") == 0)
		{
			android_phone = sqlite3_column_text(ppstmt, 1);
			strcpy(android_phone_version, android_phone);
                        printf("android_phone version : %s\n", android_phone_version);
		}
		if(strcmp(type, "ios_phone") == 0)
		{
			ios_phone = sqlite3_column_text(ppstmt, 1);
			strcpy(ios_phone_version, ios_phone);
                        printf("ios_phone version : %s\n", ios_phone_version);
		}
		if(strcmp(type, "android_pad") == 0)
		{
			android_pad = sqlite3_column_text(ppstmt, 1);
			strcpy(android_pad_version, android_pad);
                        printf("android_pad version : %s\n", android_pad_version);
		}
		if(strcmp(type, "ios_pad") == 0)
		{
			ios_pad = sqlite3_column_text(ppstmt, 1);
			strcpy(ios_pad_version, ios_pad);
                        printf("ios_pad version : %s\n", ios_pad_version);
		}
		rc = sqlite3_step(ppstmt);
        }
        sqlite3_finalize(ppstmt);
        sqlite3_close(db);

	printf("------------send app_update request------------\n");
	sprintf(request_url, "http://www.ailvgobox.com/box_manage/app_update.php?box_id=%s&android_phone=%s&ios_phone=%s&android_pad=%s&ios_pad=%s", box_id_tmp, android_phone_version, ios_phone_version, android_pad_version, ios_pad_version);
	printf("request_url : %s\n", request_url);	
	req = ghttp_request_new();
	strcpy(http_body, send_http_request(req, request_url));
        ghttp_request_destroy(req);

	printf("http_body : %s\n", http_body);
        printf("length of http_body : %d\n", strlen(http_body));

	cJSON *node;

	if(strlen(http_body)==0)
                printf("HTTP failure!\n");
        else
        {
                printf("HTTP success\n");
		if(strcmp(http_body, "null") == 0)
                      printf("http_body : null, no app update!\n");
                else
                {
	              node = cJSON_Parse(http_body);
                      ParseJSON(node);
                }
        }

	printf("app_update : complete!\n");
        return 1;
}
