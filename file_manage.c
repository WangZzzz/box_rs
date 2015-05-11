/**************************
 * Filename : file_manage.c
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


#define box_db "/ailvgo/www/database/box.db"
#define file_db "/ailvgo/www/database/file.db"

#define logfile "/ailvgo/system/log/sys_log"

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

int delete_thread(const char *box_id, char *id, char *file_name, char *md5, char *box_folder);
int update_thread(const char *box_id, char *id, char *file_name, char *md5, char *box_folder, char *is_sysfile);
int upload_thread(const char *box_id, char *id, char *file_name, char *md5, char *box_folder);

char *send_http_request(ghttp_request *req, char *uri);
	
void bail(char *s)
{
        fputs(s, stderr);
        fputc('\n', stderr);
        return;
}

void ParseJSON(cJSON *node, const char *box_id)
{
	char *result_url = NULL;
	cJSON *fileArray = cJSON_GetObjectItem(node, "sysfile_manage");
	char *type = NULL, *id = NULL, *file_name = NULL, *md5 = NULL, *box_folder = NULL, *is_sysfile = NULL;
        
        printf("Parse JSON node...\n");

	if(!fileArray)
	{
              bail("parse result : json has no node named sysfile_manage, no file_manage!\n");
              return;
        }
	
        cJSON *fileList = fileArray->child;
	while(fileList != NULL)
	{
		type = cJSON_GetObjectItem(fileList, "type")->valuestring;
		id = cJSON_GetObjectItem(fileList, "id")->valuestring;
		file_name = cJSON_GetObjectItem(fileList, "file_name")->valuestring;
		md5 = cJSON_GetObjectItem(fileList, "md5")->valuestring;
		box_folder = cJSON_GetObjectItem(fileList, "box_folder")->valuestring;
                is_sysfile = cJSON_GetObjectItem(fileList, "is_sysfile")->valuestring;
		printf("parse result :\n");
                printf("type :  %s\n", type);
                printf("id : %s\n", id);
                printf("file_name : %s\n", file_name);
                printf("md5 : %s\n", md5);
                printf("box_folder : %s\n", box_folder);
                printf("is_sysfile : %s\n", is_sysfile);

		if(strcmp(type, "delete") == 0)
			delete_thread(box_id, id, file_name, md5, box_folder);
		if(strcmp(type, "update") == 0)
			update_thread(box_id, id, file_name, md5, box_folder, is_sysfile);
		if(strcmp(type, "upload") == 0)
			upload_thread(box_id, id, file_name, md5, box_folder);

		fileList = fileList->next;
	}
}

int delete_thread(const char *box_id, char *id, char *file_name, char *md5, char *box_folder)
{
	FILE *stream = NULL;
	char result[1024] = {0};
	char sh_cmd[512] = {0};
	char request_url[100] = {0};
	ghttp_request *req_delete_feedback;
        
        printf("-------------------------------\n");
        printf("delete file : start\n");
        printf("-------------------------------\n"); 
        
	memset(result, 0, sizeof(result));
	memset(sh_cmd, 0, sizeof(sh_cmd));
	memset(request_url, 0, sizeof(request_url));	

	sprintf(sh_cmd, "rm -f %s%s", box_folder, file_name);
	printf("%s\n", sh_cmd);
	
	if((stream = popen(sh_cmd, "r")) == NULL)
		printf("popen run failed\n");
	fread(result, sizeof(char), sizeof(result), stream);
	result[1023] = '\0';
	pclose(stream);
		
	if(strstr(result, "Directory not empty"))
	{
		printf("result needed feedback, rm failed\n");
		return 0;
	}
	else
	{
		req_delete_feedback = ghttp_request_new();
		sprintf(request_url, "http://www.ailvgobox.com/box_manage/sysfile_feedback.php?box_id=%s&id=%s", box_id, id);
		printf("request_url:%s", request_url);
		send_http_request(req_delete_feedback, request_url);
		ghttp_request_destroy(req_delete_feedback);
		
		char loginfo[200] ={0};
                sprintf(loginfo, "delete file : %s%s", box_folder, file_name);
                sys_log(loginfo);                
	}
	return 1;
}

int update_thread(const char *box_id, char *id, char *file_name, char *md5, char *box_folder, char *is_sysfile)
{
	FILE *stream = NULL;
	char result[1024] = {0};
	char sh_cmd[512] = {0};
	char rm_cmd[150] = {0};
	char filename_all[100] = {0};
	char request_url[100] = {0};	
	char *md5_value = NULL;
	unsigned int idx = 0, ipid = 0;
	ghttp_request *req_update_feedback;
        
        printf("---------------------------------\n");
        printf("update file :  start\n");
        printf("---------------------------------\n");

        sqlite3 *db = NULL;
        int rc =0;
        char *errorMsg = NULL;
        char sql_cmd[300] ={0};

	memset(result, 0, sizeof(result));
        memset(sh_cmd, 0, sizeof(sh_cmd));
        memset(filename_all, 0, sizeof(filename_all));
	memset(request_url, 0, sizeof(request_url));
	sprintf(filename_all, "/ailvgo/temp/%s", file_name);
	printf("filename_all : %s\n", filename_all);
        memset(rm_cmd, 0, sizeof(rm_cmd));
        sprintf(rm_cmd, "rm -f %s", filename_all);
	
        if(access(filename_all, W_OK) == 0)
        {
                printf("zipfile exist, md5 check...\n");

                md5_value = md5_file(filename_all);
       
                printf("md5 : %s -- %s\n", md5_value, md5);

                if(md5_value && strcmp(md5, md5_value) == 0)
                {
                        printf("md5 : correct, download ok\n");
        		
                        if(strcmp(is_sysfile, "1") == 0)
                        {
                            printf("zipfile is sysfile, unzip and cp when next startup...\n");
                          
                            printf("file_name : %s\n", file_name);
                            printf("box_folder : %s\n", box_folder);

                            printf("insert file in file.db...\n");

                            rc = sqlite3_open(file_db, &db);
                            if(rc == SQLITE_ERROR)
                            {
                                 bail("open file.db failed");
                                 return 0;
                            }

                            sprintf(sql_cmd, "insert into file_info values(\'%s\',\'%s\');", file_name, box_folder);
                            printf("sql_cmd : %s\n", sql_cmd);
                            sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
                            sqlite3_close(db);
                        }
                        else if(strcmp(is_sysfile, "0") == 0)
                        {
                            printf("zipfile isn't sysfile, unzip and cp now...\n");
                            
                            printf("file_name : %s\n", file_name);
                            printf("box_folder : %s\n", box_folder);

                            printf("make temp folder...\n");
                            system("mkdir -p /ailvgo/temp/temp/");
                            sleep(1);
                            printf("unzip file in temp folder...\n");
                            memset(sh_cmd, 0, sizeof(sh_cmd));
                            sprintf(sh_cmd, "unzip -o /ailvgo/temp/%s -d /ailvgo/temp/temp/", file_name);
                            printf("sh_cmd : %s\n", sh_cmd);
                            system(sh_cmd);
                            sleep(10);
                            printf("cp files to destination path...\n");
                            memset(sh_cmd, 0, sizeof(sh_cmd));
                            sprintf(sh_cmd, "cp -rf /ailvgo/temp/temp/* %s", box_folder);
                            printf("sh_cmd : %s\n", sh_cmd);
                            system(sh_cmd);
                            sleep(10);
                            printf("remove temp folder...\n");
                            system("rm -rf /ailvgo/temp/temp/");
                            sleep(10);
                            printf("delete zipfile...\n");
                            memset(sh_cmd, 0, sizeof(sh_cmd));
                            sprintf(sh_cmd, "rm -f /ailvgo/temp/%s", file_name);
                            printf("sh_cmd : %s\n", sh_cmd);
                            system(sh_cmd);
                            sleep(2);
                            printf("update complete!\n");
                        } 

                        req_update_feedback = ghttp_request_new();
                        sprintf(request_url, "http://www.ailvgobox.com/box_manage/sysfile_feedback.php?box_id=%s&id=%s", box_id, id);
                        printf("request_rul : %s\n", request_url);
                        send_http_request(req_update_feedback, request_url);
                        ghttp_request_destroy(req_update_feedback);

			char loginfo[200] ={0};
               		sprintf(loginfo, "update file : %s%s", box_folder, file_name);
                	sys_log(loginfo);
                
			return 1;
                }
		printf("md5 : error, delete zipfile...\n");
                system(rm_cmd);
	}

	sprintf(sh_cmd, "curl -o /ailvgo/www/zipfile/%s http://www.ailvgobox.com/sysfile_manage/update/%s", file_name, file_name);
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
		{
			return 0;
		}
        	printf("md5 : %s -- %s\n", md5_value, md5);

        	if(strcmp(md5, md5_value) == 0)
		{
                	printf("md5 : corret, download ok!\n");
		
                        if(strcmp(is_sysfile, "1") == 0)
                        {
                            printf("zipfile is sysfile, unzip and cp when  next startup...\n");
                            
                            printf("file_name : %s\n", file_name);
                            printf("box_folder : %s\n", box_folder);

                            printf("insert file in file.db...\n");

                            rc = sqlite3_open(file_db, &db);
                            if(rc == SQLITE_ERROR)
                            {
                                 bail("open file.db failed");
                                 return 0;
                            }

                            sprintf(sql_cmd, "insert into file_info values(\'%s\',\'%s\');", file_name, box_folder);
                            printf("sql_cmd : %s\n", sql_cmd);
                            sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
                            sqlite3_close(db);
                        }
                        else if(strcmp(is_sysfile, "0") == 0)
                        {
                            printf("zipfile isn't sysfile, unzip and cp now...\n");

                            printf("file_name : %s\n", file_name);
                            printf("box_folder : %s\n", box_folder);

                            printf("make temp folder...\n");
                            system("mkdir -p /ailvgo/temp/temp/");
                            sleep(1);
                            printf("unzip file in temp folder...\n");
                            memset(sh_cmd, 0, sizeof(sh_cmd));
                            sprintf(sh_cmd, "unzip -o /ailvgo/temp/%s -d /ailvgo/temp/temp/", file_name);
                            printf("sh_cmd : %s\n", sh_cmd);
                            system(sh_cmd);
                            sleep(10);
                            printf("cp files to destination path...\n");
                            memset(sh_cmd, 0, sizeof(sh_cmd));
                            sprintf(sh_cmd, "cp -rf /ailvgo/temp/temp/* %s", box_folder);
                            printf("sh_cmd : %s\n", sh_cmd);
                            system(sh_cmd);
                            sleep(10);
                            printf("remove temp folder...\n");
                            system("rm -rf /ailvgo/temp/temp/");
                            sleep(10);
                            printf("delete zipfile...\n");
                            memset(sh_cmd, 0, sizeof(sh_cmd));
                            sprintf(sh_cmd, "rm -f /ailvgo/temp/%s", file_name);
                            printf("sh_cmd : %s\n", sh_cmd);
                            system(sh_cmd);
                            sleep(2);
                            printf("update complete!\n");
                        }

                        req_update_feedback = ghttp_request_new();
                	sprintf(request_url, "http://www.ailvgobox.com/box_manage/sysfile_feedback.php?box_id=%s&id=%s", box_id, id);
			printf("request_url : %s\n", request_url);
                	send_http_request(req_update_feedback, request_url);
			ghttp_request_destroy(req_update_feedback);

			char loginfo[200] ={0};
               		sprintf(loginfo, "update file : %s%s", box_folder, file_name);
                	sys_log(loginfo);
		}
		else
		{
			printf("md5 : error, delete zipfile...\n");
                        system(rm_cmd);
			return 0;
		}
        }
	return 1;
}

int upload_thread(const char *box_id, char *id, char *file_name, char *md5, char *box_folder)
{
	FILE *stream = NULL;
	char result[100] = {0}, today[50] = {0}, yesterday[50] = {0}, uploadfile_folder[50] = {0}, filename[100] = {0};
	char sh_cmd[512] = {0};
	char *filename_all;	
	char request_url[200] = {0};
	ghttp_request *req_upload_feedback, *req_upload;
	unsigned int ipid = 0;
	char http_body[200] = {0};
        
        printf("----------------------------------\n");
        printf("upload file : start\n");
        printf("----------------------------------\n");

        char *md5_value = NULL, *md5_feedback = NULL;
	time_t timep=0;
	struct tm *timeinfo;

	memset(result, 0, sizeof(result));
        memset(sh_cmd, 0, sizeof(sh_cmd));
	memset(request_url, 0, sizeof(request_url));

        if((stream = popen("date \"+%Y_%m_%d\"", "r")) == NULL)
                printf("popen failed\n");
	fread(result, sizeof(char), sizeof(result), stream);
	result[strlen(result)-1] = '\0';
        result[99] = '\0';
	strcpy(today, result);
        printf("today : %s\n", today);
        pclose(stream);
 
	time(&timep);
	timep -= 60*60*24;
        timeinfo = localtime(&timep);
	sprintf(yesterday, "%d_%02d_%02d", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday);
        printf("yesterday : %s\n", yesterday);

	sprintf(sh_cmd, "curl -u user:user -T %s%s ftp://www.ailvgobox.com/sysfile_manage/upload/%s_%s_%s", box_folder, file_name, box_id, today, file_name);
	strcpy(uploadfile_folder, "/sysfile_manage/upload/");
	printf("sh_md : %s\n", sh_cmd);
	filename_all = (char *)malloc(sizeof(char)*(strlen(box_folder)+strlen(file_name)+10));
	sprintf(filename_all, "%s%s", box_folder, file_name);
	if(access(filename_all, W_OK) == -1)
	{
		printf("result :upload file not exist\n");
		req_upload_feedback = ghttp_request_new();
		sprintf(request_url, "http://www.ailvgobox.com/box_manage/sysfile_feedback.php?box_id=%s&id=%s", box_id, id);
		printf("request_url : %s\n", request_url);
		send_http_request(req_upload_feedback, request_url);
        	ghttp_request_destroy(req_upload_feedback);


		return 0;
	}
	else
	{
		md5_value = md5_file(filename_all);
		printf("md5 : %s\n", md5_value);
                memset(filename, 0, sizeof(filename));
		sprintf(filename, "%s_%s_%s", box_id, today, file_name);
                printf("filename : %s\n", filename);
		memset(request_url, 0, sizeof(request_url));            
		sprintf(request_url, "http://www.ailvgobox.com/box_manage/file_query.php?file_name=%s&folder=%s", filename, uploadfile_folder);
                printf("request_url : %s\n", request_url);
                req_upload = ghttp_request_new();                
		strcpy(http_body, send_http_request(req_upload, request_url));
		ghttp_request_destroy(req_upload);
                //fprintf(stderr, "%s\n", http_body);

		if(strcmp(http_body, "null") == 0)
		{
			memset(filename, 0, sizeof(filename));
                	sprintf(filename, "%s_%s_%s", box_id, yesterday, file_name);
                        printf("filename : %s\n", filename);
                	memset(request_url, 0, sizeof(request_url));
                	sprintf(request_url, "http://www.ailvgobox.com/box_manage/file_query.php?file_name=%s&folder=%s", filename, uploadfile_folder);
                	printf("request_url : %s\n", request_url);
                	req_upload = ghttp_request_new();
                	strcpy(http_body, send_http_request(req_upload, request_url));
			ghttp_request_destroy(req_upload);
                	//fprintf(stderr, "%s\n", http_body);
		}

		if(strcmp(http_body, "null") == 0)
		{
			printf("JSON text transfer to cJSONNODE error\n");
                        printf("today&yesterday not upload!\n");
		}
		else
		{
			cJSON *node = cJSON_Parse(http_body);
                        cJSON *file_query = cJSON_GetObjectItem(node, "file_query");
			if(file_query)
			{
				md5_feedback = cJSON_GetObjectItem(file_query, "md5")->valuestring;
				printf("md5_value - md5 : %s - %s\n", md5_value, md5_feedback);
				if(strcmp(md5_value, md5_feedback) == 0)
				{
					req_upload_feedback = ghttp_request_new();
                			sprintf(request_url, "http://www.ailvgobox.com/box_manage/sysfile_feedback.php?box_id=%s&id=%s", box_id, id);
                			printf("request_url : %s\n", request_url);
			                send_http_request(req_upload_feedback, request_url);
                			ghttp_request_destroy(req_upload_feedback);
					
					char loginfo[200] = {0};
        				sprintf(loginfo, "update file : %s%s", box_folder, file_name);
					sys_log(loginfo);

                                        return 1;
				}
			}
		}
	}

	printf("upload cmd : %s\n", sh_cmd);
	printf("upload filename : %s\n", file_name);
	if((stream = popen(sh_cmd, "r")) == NULL)
	{
		printf("popen failed\n");
		return 0;
	}
	memset(result, 0, sizeof(result));
        fread(result, sizeof(char), sizeof(result), stream);
        result[99] = '\0';
        printf("curl result : %s\n", result);
        if(strlen(result) > 0)
        {
		printf("curl : error!\n");		
		system("killall curl");
                return 0;
        }

	pclose(stream);
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
	char http_body[500] = {0};
	sqlite3 *db = NULL;
        int rc = 0;
        sqlite3_stmt *ppstmt = NULL;
        char sql_cmd[100] ={0};
        const char *box_id = NULL;
        char *errorMsg = NULL;
	char request_url[100] = {0};
        char box_id_tmp[10] = {0};

        printf("*******************************\n");
	printf("file_manage : start\n");
        printf("*******************************\n");
        
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
                printf("box_id : %s\n", box_id_tmp);
        }
         
        sqlite3_finalize(ppstmt);
        sqlite3_close(db);
	
	printf("--------------send file_manage request--------------\n");
	sprintf(request_url, "http://www.ailvgobox.com/box_manage/sysfile_manage_new.php?box_id=%s", box_id_tmp);
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
        	printf("HTTP success!\n");

                if(strcmp(http_body, "null") == 0)
                      printf("http_body : null, no file manage!\n");
                else
                {
	              node = cJSON_Parse(http_body);
                      ParseJSON(node, box_id_tmp);
                }
        }
	
	printf("file_manage : complete!\n");

        return 1;
}
