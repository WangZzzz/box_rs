/***********************
 * Filename : heart_beat.c
 * Date : 2015-05-09
 * *********************/

#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include <time.h>
#include "ghttp.h"

# define box_db "/ailvgo/www/database/box.db"
# define traffic_db "/ailvgo/system/traffic/traffic.db"
# define traffic_total_db "/ailvgo/system/traffic/traffic_total.db"
# define traffic_saved_db "/ailvgo/system/traffic/traffic_saved.db"


void bail(char *s)
{
	fputs(s, stderr);
    	fputc('\n', stderr);
}

int ailvgobox_monitor()
{
	FILE *stream;
	char buf_ailvgobox[1024] = {0};
	int ailvgobox_break = 1;
	
	memset(buf_ailvgobox, 0, sizeof(buf_ailvgobox));
	
	printf("ping -c 1 www.ailvgobox.com\n");
    	stream = popen("ping -c 1 www.ailvgobox.com", "r");
	fread(buf_ailvgobox, sizeof(char), sizeof(buf_ailvgobox), stream);
	buf_ailvgobox[1023] = '\0';
    	pclose(stream);
	printf("buf_ailvgobox : %s\n", buf_ailvgobox);
	
	if(strstr(buf_ailvgobox, "1 received"))
		ailvgobox_break = 0;

	if(ailvgobox_break == 1)
		return 1;
	else
		return 0;
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

main()
{
	ghttp_request *req;
	char http_body[1024] = {0}, *errmsg = NULL;
	sqlite3 *db = NULL;
	int rc = 0, result = 0;
    	sqlite3_stmt *ppstmt = NULL;
    	char sql_cmd[100] ={0};
	const char *box_id, *wifi_detect;
	char box_id_tmp[10] = {0}, wifi_detect_tmp[5] = {0};
	const char *trafficnum = NULL;
	char trafficnum_tmp[10] = {0};
	char request_url[5000] = {0}, traffic_list[1500] = {0}, time_list[3000] = {0};
	int t = 0, select_cnt, ailvgobox_break;
       
    	printf("****************************************\n"); 
	printf("heart_beat : start\n");
    	printf("****************************************\n");
	
	printf("box.db : check box_id, wifi_detect\n");
	
	select_cnt = 0;
	while(select_cnt < 20)
	{
		if(sqlite3_open(box_db, &db))
			printf("cannot open box.db!\n");

    		strcpy(sql_cmd, "select box_id, wifi_detect from box_info");
		sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
    		rc = sqlite3_step(ppstmt);
		
		if(rc == SQLITE_ROW)
    		{
			box_id = sqlite3_column_text(ppstmt, 0);
			strcpy(box_id_tmp, box_id);
			printf("box_id : %s\n", box_id_tmp);
        		wifi_detect = sqlite3_column_text(ppstmt, 1);
			strcpy(wifi_detect_tmp, wifi_detect);
			printf("wifi_detect : %s\n", wifi_detect_tmp);
    			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			break;
		}
		else
		{
			printf("select box_id,wifi_detect failure!\n");
    			sqlite3_finalize(ppstmt);
			sqlite3_close(db);
			sleep(1);
			select_cnt++;
		}
	}

    	if(strcmp(wifi_detect_tmp, "off") == 0)
	{
		printf("wifi detect : off, traffic = null!\n");
		strcpy(trafficnum_tmp, "null");
	}
    	else
    	{
		printf("wifi detect : on, collect traffic!\n");

		t = time(NULL)+8*3600;
		
		printf("traffic.db : check new traffic\n");

		select_cnt = 0;
		while(select_cnt < 20)
		{
			rc = sqlite3_open(traffic_db, &db);
			if(rc == SQLITE_ERROR)
				printf("cannot open traffic.db!\n");
        
			ppstmt = NULL;
			sprintf(sql_cmd, "select count(*) from traffic_info where time>=%d-650 and time<=%d", t, t);
			sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
			rc = sqlite3_step(ppstmt);
			
			if(rc == SQLITE_ROW)
			{
				trafficnum = sqlite3_column_text(ppstmt, 0);
				strcpy(trafficnum_tmp, trafficnum);
				printf("new traffic in last 10 minutes : %s\n", trafficnum);
				sqlite3_finalize(ppstmt);
				sqlite3_close(db);
				break;
			}
			else
			{
				printf("select count(*) from traffic.db failure!\n");
				sqlite3_finalize(ppstmt);
				sqlite3_close(db);
				sleep(10);
				select_cnt++;
			}
		}

		printf("traffic_total.db : check total traffic\n");

		select_cnt = 0;
		while(select_cnt < 20)
		{
			rc = sqlite3_open(traffic_total_db, &db);
			if(rc == SQLITE_ERROR)
				printf("cannot open traffic_total.db!\n");

			ppstmt = NULL;
			sprintf(sql_cmd, "select count(*) from traffic_total where time>=%d-650 and time<=%d", t, t);
			sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
			rc = sqlite3_step(ppstmt);

			if(rc == SQLITE_ROW)
			{
				trafficnum = sqlite3_column_text(ppstmt, 0);
				if(atoi(trafficnum) < atoi(trafficnum_tmp))
				{
					printf("total traffic in last 10 minutes : %s\n", trafficnum);
					printf("collect in traffic_total.db in progress, retry\n");
					sqlite3_finalize(ppstmt);
					sqlite3_close(db);
					sleep(10);
					select_cnt++;
				}
				else
				{
					strcat(trafficnum_tmp, "*");
					strcat(trafficnum_tmp, trafficnum);
					printf("total traffic in last 10 minutes : %s\n", trafficnum);
					printf("trafficnum_tmp : %s\n", trafficnum_tmp);
					sqlite3_finalize(ppstmt);
					sqlite3_close(db);
					break;
				}
			}
			else
			{
				printf("select count(*) from traffic_total.db failure!\n");
				sqlite3_finalize(ppstmt);
				sqlite3_close(db);
				sleep(10);
				select_cnt++;
			}
		}
	}
        
	printf("------------ailvgobox monitor---------------\n");
	ailvgobox_break = ailvgobox_monitor();
	
	if(ailvgobox_break == 0)
	{	
		printf("ailvgobox server : online!\n");

		printf("------------send heart_beat request---------------\n");
		sprintf(request_url, "http://www.ailvgobox.com/box_manage_2/heart_beat_1.php?box_id=%s&traffic=%s", box_id_tmp, trafficnum_tmp);
		printf("request_url : %s\n", request_url);
		req = ghttp_request_new();
		strcpy(http_body, send_http_request(req, request_url));
		ghttp_request_destroy(req);

		printf("http_body : %s\n", http_body);
		printf("length of http_body : %d\n", strlen(http_body));

		if(strlen(http_body) == 0)
		{    
			printf("HTTP failure!\n");

			if(strcmp(trafficnum_tmp, "null") == 0)
				printf("Wifi detect : off, skip save traffic\n");
			else
			{
				printf("Wifi detect : on, save traffic...\n");
				
				select_cnt = 0;
				while(select_cnt < 20)
				{
					rc = sqlite3_open(traffic_saved_db, &db);
					if(rc == SQLITE_ERROR)
						printf("cannot open traffic_saved.db!\n");

					memset(sql_cmd, 0, sizeof(sql_cmd));
					sprintf(sql_cmd, "insert into traffic_saved values('%s', %d)", trafficnum_tmp, t);
					printf("sql_cmd : %s\n", sql_cmd);
					result = sqlite3_exec(db, sql_cmd, NULL, NULL, &errmsg);
	
					if(result == SQLITE_OK)
					{
						printf("insert traffic_saved.db successfully!\n");
						sqlite3_free(errmsg);
						sqlite3_close(db);
						break;
					}
					else
					{
						printf("insert traffic_saved.db error : %s", errmsg);
						sqlite3_free(errmsg);
						sqlite3_close(db);
						sleep(1);
						select_cnt++;
					}
				}
			}
		}
		else 
		{
			printf("HTTP success!\n");

			if(strcmp(trafficnum_tmp, "null") == 0)
				printf("Wifi detect : off, skip send saved_traffic\n");
			else
			{
				printf("Wifi detect : on, send saved_traffic...\n");

				printf("traffic_saved.db : check record\n");

				rc = sqlite3_open(traffic_saved_db, &db);
				if(rc == SQLITE_ERROR)
					printf("cannot open traffic_saved.db!\n");

				ppstmt = NULL;
				int saved_cnt_tmp = 0;
				sprintf(sql_cmd, "select count(*) from traffic_saved");
				sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
				rc = sqlite3_step(ppstmt);
				if(rc == SQLITE_ROW)
				{
					saved_cnt_tmp = sqlite3_column_int(ppstmt, 0);
					printf("saved_cnt_tmp : %d\n", saved_cnt_tmp);
				}

				if(saved_cnt_tmp == 0)
					printf("No saved traffic, skip traffic resend!\n");

				if(saved_cnt_tmp > 0)
				{
					ppstmt = NULL;
					printf("records in traffic_saved.db : %d\n", saved_cnt_tmp);
					if(saved_cnt_tmp <= 200)
						sprintf(sql_cmd, "select traffic, time from traffic_saved");
					else if(saved_cnt_tmp > 200)
						sprintf(sql_cmd, "select traffic, time from traffic_saved limit %d, %d", saved_cnt_tmp-200, 200);
					sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
					rc = sqlite3_step(ppstmt);
					while(rc == SQLITE_ROW)
					{
						trafficnum = sqlite3_column_text(ppstmt, 0);
						strcpy(trafficnum_tmp, trafficnum);
						printf("traffic : %s\n", trafficnum);
						t = sqlite3_column_int(ppstmt, 1);
						printf("time : %d\n", t);
								
						strcat(traffic_list, trafficnum_tmp);
						strcat(traffic_list, "-");
						sprintf(time_list, "%s%d-", time_list, t);
						rc = sqlite3_step(ppstmt);	
					}
					traffic_list[strlen(traffic_list)-1] = '\0';
					time_list[strlen(time_list)-1] = '\0';
			        
					printf("-----------send saved_traffic request-----------\n");
					ghttp_request *req_traffic_saved;
					req_traffic_saved = ghttp_request_new();
					sprintf(request_url, "http://www.ailvgobox.com/box_manage_2/traffic_resend_1.php?box_id=%s&traffic=%s&time=%s", box_id_tmp, traffic_list, time_list);
					printf("request_url : %s\n", request_url);
					strcpy(http_body, send_http_request(req_traffic_saved, request_url));
					ghttp_request_destroy(req_traffic_saved);

					printf("http_body : %s\n", http_body);              
			      
					if(strlen(http_body) > 0)
					{
						printf("traffic resend success, delete all records in traffic_saved.db\n");
						result = sqlite3_exec(db, "delete from traffic_saved", NULL, NULL, &errmsg);
						if(result != SQLITE_OK)
							printf("delete database error : %s", errmsg);
					}
				}
			
				sqlite3_free(errmsg);
				sqlite3_finalize(ppstmt);
				sqlite3_close(db);
			}
		}
	}
	else
	{
		printf("ailvgobox server : offline!\n");

		if(strcmp(trafficnum_tmp, "null") == 0)
			printf("Wifi detect : off, skip save traffic\n");
		else
		{
			printf("Wifi detect : on, save traffic...\n");

			select_cnt = 0;
			while(select_cnt < 20)
			{
				rc = sqlite3_open(traffic_saved_db, &db);
				if(rc == SQLITE_ERROR)
					printf("cannot open traffic_saved.db!\n");

				memset(sql_cmd, 0, sizeof(sql_cmd));
				sprintf(sql_cmd, "insert into traffic_saved values('%s', %d)", trafficnum_tmp, t);
				printf("sql_cmd : %s\n", sql_cmd);
				result = sqlite3_exec(db, sql_cmd, NULL, NULL, &errmsg);
			
				if(result == SQLITE_OK)
				{
					printf("insert traffic_saved.db successfully!\n");
					sqlite3_free(errmsg);
					sqlite3_close(db);
					break;
				}
				else
				{
					printf("insert traffic_saved.db error : %s\n", errmsg);
					sqlite3_free(errmsg);
					sqlite3_close(db);
					sleep(1);
					select_cnt++;
				}
			}
					
		}
	}

	printf("heart_beat : complete!\n");
}

