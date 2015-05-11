/************************
 * Filename : file_upload.c
 * Date : 2015-03-05
 * *********************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <ghttp.h>
#include "cJSON.h"
#include "md5.h"
#include "sqlite3.h"

#define userfile_db "/ailvgo/www/database/userfile.db"


char *send_http_request(ghttp_request *req, char *uri);
	
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

upload_file(const char *filename, const char *type, sqlite3 *db)
{
	FILE *stream = NULL;
	char sh_cmd[512] = {0};
	char filename_all[100] = {0};
        char filename_query[100] = {0};
	char sql_cmd[100] = {0}, del_cmd[100] = {0}, request_url[200] = {0};
	char buf[100] = {0}, uploadfile_folder[50] = {0};
	char *errorMsg = NULL, *md5_value = NULL, *md5 = NULL;
        int rc = 0;
	ghttp_request *req_upload;
        char http_body[200] = {0};

        memset(sh_cmd, 0, sizeof(sh_cmd));
	memset(filename_all, 0, sizeof(filename_all));
	memset(sql_cmd, 0, sizeof(sql_cmd));

	if(strcmp(type, "1") == 0)
	{
		sprintf(sh_cmd, "curl -u user:user -T /ailvgo/www/userfile/pic/%s.jpg ftp://www.ailvgobox.com/userfile_manage/pic/%s.jpg", filename, filename);
		sprintf(filename_all, "/ailvgo/www/userfile/pic/%s.jpg", filename);
		strcpy(uploadfile_folder, "/userfile_manage/pic/");
                sprintf(filename_query, "%s.jpg", filename);
	}
	if(strcmp(type, "2") == 0)
	{
		sprintf(sh_cmd, "curl -u user:user -T /ailvgo/www/userfile/audio/%s.mp3 ftp://www.ailvgobox.com/userfile_manage/audio/%s.mp3 ", filename, filename);
		sprintf(filename_all, "/ailvgo/www/userfile/audio/%s.mp3", filename);
		strcpy(uploadfile_folder, "/userfile_manage/audio/");
                sprintf(filename_query, "%s.mp3", filename);
	}
	if(strcmp(type, "3") == 0)
	{
		sprintf(sh_cmd, "curl -u user:user -T /ailvgo/www/userfile/video/%s.mp4 ftp://www.ailvgobox.com/userfile_manage/video/%s.mp4 ", filename, filename);
		sprintf(filename_all, "/ailvgo/www/userfile/video/%s.mp4", filename);
		strcpy(uploadfile_folder, "/userfile_manage/video/");
                sprintf(filename_query, "%s.mp4", filename);
	}
	printf("sh_md : %s\n", sh_cmd);
	sprintf(sql_cmd, "update userfile_info set upload=\"1\" where filename=\"%s\"", filename);
	printf("sql_cmd : %s\n", sql_cmd);
	if(access(filename_all, W_OK) == -1)
	{
		memset(del_cmd, 0, sizeof(del_cmd));
		sprintf(del_cmd, "delete from userfile_info where filename=\"%s\"", filename);
		printf("del_cmd : %s\n", del_cmd);
		printf("result : upload file not exist\n");
		sqlite3_exec(db, del_cmd, NULL, NULL, &errorMsg);
                if (errorMsg)
                      printf("errorMsg : %s\n", errorMsg);
		return 0;
	}
	else
	{
		md5_value = md5_file(filename_all);
		sprintf(request_url, "http://www.ailvgobox.com/box_manage_2/file_query_1.php?file_name=%s&folder=%s", filename_query, uploadfile_folder);
        	printf("request_url : %s\n", request_url);
        	req_upload = ghttp_request_new();
        	strcpy(http_body, send_http_request(req_upload, request_url));
                ghttp_request_destroy(req_upload);
        	printf("http_body : %s\n", http_body);

        	cJSON *node;

        	if(strlen(http_body) == 0)
		{
                	printf("HTTP failure!\n");
		}
		else
		{
			if(strcmp(http_body, "null") == 0)
				printf("server has no file!\n");
			else
			{
				node = cJSON_Parse(http_body);
				cJSON *file_query = cJSON_GetObjectItem(node, "file_query");
				if(file_query)
				{
					char *md5 = cJSON_GetObjectItem(file_query, "md5")->valuestring;
					printf("md5_value - md5 : %s - %s", md5_value, md5);
					if(strcmp(md5_value, md5) == 0)
					{
						printf("update databse...\n");
        					sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
        					if(errorMsg)
                					printf("errorMsg : %s\n", errorMsg);
						return 1;
					}
				}
			}
		}
	}
		
	printf("upload cmd : %s\n", sh_cmd);
	
        sleep(2);
        printf("upload start...\n");
        system(sh_cmd);
        sleep(2);

	return 1;
}

int main()
{
	sqlite3 *db = NULL, *db_net = NULL;
        int rc = 0;
        sqlite3_stmt *ppstmt = NULL;
        char sql_cmd[100] ={0};
        char *errorMsg = NULL;
	const char *filename = NULL, *type = NULL, *wan_state = NULL;
       
        printf("************************************\n"); 
        printf("file_upload : start\n");
        printf("************************************\n");
        
        printf("userfile.db : check record with upload = 0\n");

        rc = sqlite3_open(userfile_db, &db);

        if(rc == SQLITE_ERROR)
        {
                printf("cannot open userfile.db!\n");
                return 0;
        }
	
        ppstmt = NULL;
	memset(sql_cmd, 0, sizeof(sql_cmd));
        strcpy(sql_cmd, "select filename, type from userfile_info where upload=\"0\"");
        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        rc = sqlite3_step(ppstmt);
	while(rc == SQLITE_ROW)
        {
                filename = sqlite3_column_text(ppstmt, 0);
                type = sqlite3_column_text(ppstmt, 1);
                printf("filename : %s\n", filename);
                printf("type : %s\n", type);
		upload_file(filename, type, db);
		rc = sqlite3_step(ppstmt);
        }

        sqlite3_finalize(ppstmt);
	sqlite3_close(db);

        printf("file_upload : complete!\n");
	return 1;
}
