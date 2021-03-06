/***********************
 * Filename : remote_control_run.c
 * Date : 2015-02-13
 * ********************/

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include <time.h>
#include <malloc.h>
#include "ghttp.h"
#include "cJSON.h"

# define box_db "/tmp/usbmounts/sda1/www/database/box.db"
# define log_db "/tmp/usbmounts/sda1/www/database/log.db"

void bail(char *s)
{
        fputs(s, stderr);
        fputc('\n', stderr);
}

char *send_http_request(ghttp_request *req, char *uri);

int str_rep(char *source, char src, char des)
{
	char *pos = NULL;
	if(!source)
		return 1;
	while((pos = strchr(source, src)) != NULL)
	{
		*pos = des;
		source = ++pos;
	}
	return 1;
}

int ParseJSON(cJSON *node, const char *box_id)
{
	FILE *stream = NULL;
	char buf[512] = {0};
	char *result_url;
	cJSON *cmdArray = cJSON_GetObjectItem(node, "remote_control");
        ghttp_request *req_remotectl_feedback;
        char *id = NULL, *content = NULL;
        char *command = NULL, *ptr = NULL, *p = NULL;

        if(!cmdArray)
	{
                bail("parse result : json has no node named remote_control");
		return 0;
	}

        char *errorMsg = NULL;
        cJSON *cmdList = cmdArray->child;
        while(cmdList != NULL)
        {
                if(cJSON_GetObjectItem(cmdList, "id")->valuestring != NULL)
                {
                        id = cJSON_GetObjectItem(cmdList, "id")->valuestring;
                        printf("id : %s\n", id);
                }
		
		if(cJSON_GetObjectItem(cmdList, "content")->valuestring != NULL)
                {
                        content = cJSON_GetObjectItem(cmdList, "content")->valuestring; 
                        printf("content : %s\n", content);
                }
                 
		stream = NULL;
		
                if(strcmp(content, "reboot") == 0)
		{
			printf("command : reboot\n");
                        printf("send http feedback...\n");
                        result_url = (char *)malloc(150);
        	        req_remotectl_feedback = ghttp_request_new();
                	sprintf(result_url, "http://www.ailvgobox.com/box_manage/remote_feedback.php?box_id=%s&id=%s&result=reboot_ok", box_id, id);
                        printf("result_url : %s\n", result_url);
                        result_url[149] = '\0';
                        send_http_request(req_remotectl_feedback, result_url);
                        ghttp_request_destroy(req_remotectl_feedback);
                        free(result_url);
                        printf("command : reboot, run!\n");
			stream = popen(content, "r");
                        pclose(stream);
		}
		
                ptr = strstr(content, "@");

                if(!ptr)
                {
                        printf("content : one command!\n");
                    
                        printf("command : %s, run!\n", content);

                        stream = popen(content, "r");
		        memset(buf, 0, 512);
                        fread(buf, sizeof(char), sizeof(buf), stream);
                        buf[511] = '\0';
                        printf("buf : %s\n", buf);
                        pclose(stream);
			str_rep(buf, '\n', ',');
                        str_rep(buf, ' ', '-');
			if(strlen(buf) == 0)
                              strcpy(buf, "no_result"); 
 
                        printf("send http feedback...\n");
			result_url = (char *)malloc(strlen(buf)+150);
			memset(result_url, 0, strlen(buf)+150);
			req_remotectl_feedback = ghttp_request_new();
			sprintf(result_url, "http://www.ailvgobox.com/box_manage/remote_feedback.php?box_id=%s&id=%s&result=%s", box_id, id, buf);
			printf("result_url : %s\n", result_url);
			result_url[strlen(buf)+150-1] = '\0';
			send_http_request(req_remotectl_feedback, result_url);
			ghttp_request_destroy(req_remotectl_feedback);
			free(result_url);
                }
                else
                {
                        printf("content : multiple command!\n");
                        
                        command = strtok_r(content, "@", &p);

                        while(command)
                        {  
                              printf("command : %s\n", command);

                              if(strcmp(command, "reboot") == 0)
                              {
                                      printf("command : reboot\n");
                                      printf("send http feedback...\n");
                                      result_url = (char *)malloc(150);
                                      req_remotectl_feedback = ghttp_request_new();
                                      sprintf(result_url, "http://www.ailvgobox.com/box_manage/remote_feedback.php?box_id=%s&id=%s&result=reboot_ok", box_id, id);
                                      printf("result_url : %s\n", result_url);
                                      result_url[149] = '\0';
                                      send_http_request(req_remotectl_feedback, result_url);
                                      ghttp_request_destroy(req_remotectl_feedback);
                                      free(result_url);
                                      printf("command : reboot, run!\n");
                                      stream = popen(command, "r");
                                      pclose(stream);
                               }
                              else
                              {
                                      printf("command : %s, run!\n", command);
                                      stream = popen(command, "r");
                                      memset(buf, 0, 512);
                                      fread(buf, sizeof(char), sizeof(buf), stream);
                                      buf[511] = '\0';
                                      printf("buf : %s\n", buf);
                                      pclose(stream);
                                      str_rep(buf, '\n', ',');
                                      str_rep(buf, ' ', '-');
                                      if(strlen(buf) == 0)
                                              strcpy(buf, "no_result");
                                      printf("send http feedback...\n");
                                      result_url = (char *)malloc(strlen(buf)+150);
                                      memset(result_url, 0, strlen(buf)+150);
                                      req_remotectl_feedback = ghttp_request_new();
                                      sprintf(result_url, "http://www.ailvgobox.com/box_manage/remote_feedback.php?box_id=%s&id=%s&result=%s", box_id, id, buf);
                                      printf("result_url : %s\n", result_url);
                                      result_url[strlen(buf)+150-1] = '\0';
                                      send_http_request(req_remotectl_feedback, result_url);
                                      ghttp_request_destroy(req_remotectl_feedback);
                                      free(result_url);
                               }  
                              
                              command = strtok_r(NULL, "@", &p);  
                        }
                }
                
		cmdList = cmdList->next;
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
		bail("malloc space error");
		return NULL;
	}
	else
	{
		memset(buffer, 0, malloc_size);
		buffer_size = malloc_size;
	}

	if(ghttp_set_uri(req, uri) < 0)
	{
		bail("ghttp_set_uri");
		return NULL;
	}
	if(ghttp_prepare(req) < 0)
	{
		bail("ghttp_prepare");
		return NULL;
	}
	if(ghttp_set_type(req, ghttp_type_get) == -1)
	{
		bail("ghttp_set_type");
		return NULL;
	}
	if(ghttp_set_sync(req, ghttp_async) < 0)
	{
		bail("ghttp_set_sync");
		return NULL;
	}
	
	do {
                status(req, "conn");
                req_status = ghttp_process(req);

                if(req_status == ghttp_error)
                {
                        fprintf(stderr, "ghttp_process: %s\n", ghttp_get_error(req));
                        return NULL;
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
					bail("realloc error");
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

int remote_control()
{
	ghttp_request *req;
	char *http_body = NULL;
	req = ghttp_request_new();
	sqlite3 *db = NULL;
        int rc = 0;
        sqlite3_stmt *ppstmt = NULL;
        char sql_cmd[100] ={0};
        const char *box_id;
        char box_id_tmp[10] = {0};
        const char *wan_state;
        char *errorMsg = NULL;
	char request_url[200] = {0};

	printf("****************************************\n");
        printf("remote_control : start\n");
        printf("****************************************\n");

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

	printf("send http request...\n");
        sprintf(request_url, "http://www.ailvgobox.com/box_manage/remote_control.php?box_id=%s", box_id_tmp);
        printf("request_url  : %s\n", request_url);
	http_body = send_http_request(req, request_url);
	fprintf(stderr, "http_body : %s\n", http_body);

	cJSON *node;
	
        if(http_body)
        {
        	printf("HTTP success!\n");

                if(strcmp(http_body, "null") == 0)
                     printf("http_body : null, no remote_control!\n");
                else
                {
                     node = cJSON_Parse(http_body);
                     ParseJSON(node, box_id_tmp);
                }
        }
	else
        {
                printf("HTTP failure!\n");
        }

        if(http_body)
               free(http_body);

        ghttp_request_destroy(req);
        free(request_url);
        printf("remote_control : complete!\n");
        return 1;
}

int main()
{
        int start_t=0, end_t=0;
        
        printf("****************************************\n");
        printf("remote_control_run : start\n");
        printf("period : 3600 seconds!\n");
        printf("****************************************\n");
   
	while(1)
	{
		sleep(1200);
                printf("remote control : start\n");
                
                start_t = time(NULL);
                
                if(remote_control()==0)
                     printf("remote_control : faliure\n");
                else
                     printf("remote_control : success\n");
  
                end_t = time(NULL);

		sleep(2400-(end_t-start_t));
	}
        return 0;
}
