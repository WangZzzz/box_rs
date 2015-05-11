/*************************************************************************     
  > File Name: traffic_upload.c
  > Created Time: Wed 06 May 2015 09:19:06 AM CST
  > Function: 
 ************************************************************************/     

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sqlite3.h>
#include<malloc.h>
#include<unistd.h>
#include<time.h>
#include "cJSON.h"
#include "ghttp.h"
#include "md5.h"

#define box_db "/ailvgo/www/database/box.db"
#define START 86400 
#define END 2592000  

char* get_box_id();
char* get_date(int interval);
	
void bail(char *s)
{
        fputs(s, stderr);
        fputc('\n', stderr);
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

int traffic_check(cJSON *node, char *db_path)
{
	cJSON *traffic_file = cJSON_GetObjectItem(node, "traffic_file");

	if(traffic_file)
	{
		char *md5 = cJSON_GetObjectItem(traffic_file, "md5")->valuestring;
		char *md5_value = md5_file(db_path);

		printf("md5_value - md5 : %s - %s\n", md5_value, md5);

		if(strcmp(md5_value, md5) == 0)
			return 0;
		else
			return 1;
	}
	else
		return 1;
}

int main()
{
	printf("----------------------\n");
	printf("udp_upload : start!\n");
	printf("----------------------\n");
	
	ghttp_request *req;
	int i = 0;
	FILE *stream = NULL;
	char result[1024] = {0};
	char *box_id = get_box_id();
	char db_path[100] = {0};
	char db_name[30] = {0};
	char sh_cmd[512] = {0};
	char request_url[200] = {0};

	if(box_id == NULL)
	{
		printf("error:can not get box_id!\n");
		exit(1);
	}
	
	printf("box_id : %s\n", box_id);

	cJSON *node;

	for(i = START; i<=END;)
	{
		memset(db_path, 0, sizeof(db_path));
		memset(db_name, 0, sizeof(db_name));
		char *date = get_date(i);
		sprintf(db_name, "%s_%s.db", box_id, date);
		sprintf(db_path, "/ailvgo/system/traffic/%s", db_name);
		if(access(db_path , F_OK) == 0)
		{
			printf("%s : exist on local, need to be upload!\n", db_name);
           
			sprintf(request_url, "http://www.ailvgobox.com/box_manage_2/traffic_file_query_1.php?box_id=%s&date=%s", box_id, date);
        	printf("request_url : %s\n", request_url);
        	req = ghttp_request_new();
        	char *http_body = send_http_request(req, request_url);
            ghttp_request_destroy(req);
        	printf("http_body : %s\n", http_body);

			if(strlen(http_body) > 10)
			{
				printf("%s : exist on ailvgobox, check md5...\n", db_name);

				node = cJSON_Parse(http_body);
				
				if(traffic_check(node, db_path) == 0)
				{
					printf("%s : uploaded successfully, remove local file\n", db_name);
					memset(sh_cmd, 0, sizeof(sh_cmd));
					sprintf(sh_cmd,"rm -rf %s",db_path);
					system(sh_cmd);
				}
				else
				{
					printf("%s uploaded faliue, upload again!\n");

					memset(sh_cmd, 0, sizeof(sh_cmd));
					sprintf(sh_cmd, "curl -u user:user -T %s ftp://www.ailvgobox.com/traffic_upload/%s/%s", db_path , date, db_name);
					system(sh_cmd);
					sleep(1);
				}
			}
			else
			{
				printf("traffic file not exist on ailvgobox, upload...\n");

				memset(sh_cmd, 0, sizeof(sh_cmd));
				sprintf(sh_cmd, "curl -u user:user -T %s ftp://www.ailvgobox.com/traffic_upload/%s/%s", db_path , date, db_name);
				system(sh_cmd);
				sleep(1);
			}
			free(http_body);
		}
		free(date);
		i = i + START;
	}

	free(box_id);

	printf("udp_upload : complete!\n");

	return 0;
}

char* get_box_id()
{
	sqlite3 *db;
	int rc = 0;
	sqlite3_stmt *ppstmt = NULL;
	const char *box_id = NULL;
	char *buffer = (char *)malloc(sizeof(char)*100);
	char sql[200] = {0};
	int attempt_cnt = 0;
	while(attempt_cnt < 20)
	{
		rc = sqlite3_open(box_db,&db);
		if(rc != SQLITE_OK)
		{
			printf("open database error!\n");
			attempt_cnt++;
			usleep(200);
			continue;
		}

		memset(sql, 0, sizeof(sql));
		sprintf(sql,"select box_id from box_info");
		rc = sqlite3_prepare(db, sql, -1, &ppstmt, 0);
		if(rc != SQLITE_OK)
		{
			fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
			attempt_cnt++;
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			usleep(200);
			continue;
		}
		rc = sqlite3_step(ppstmt);
		if(rc == SQLITE_ROW)
		{
			box_id = sqlite3_column_text(ppstmt,0);
			sprintf(buffer,"%s",box_id);
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			return buffer;
		}else{
			printf("select box_id error!\n");
			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			attempt_cnt++;
			usleep(200);
		}
	}
	
	free(buffer);
	return NULL;
}

char* get_date(int interval)
{
	struct tm *t;
	time_t tt;
	tt = time(NULL) - interval;
	t = localtime(&tt);
	char *buffer = (char *)malloc(sizeof(char)*256);

	sprintf(buffer,"%4d%02d%02d",t->tm_year+1900, t->tm_mon+1, t->tm_mday);
	return buffer;
}



