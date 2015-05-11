/**************************
 * Filename: box_login.c
 * Date : 2015-05-09
 * ***********************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "ghttp.h"
#include "cJSON.h"
#include "sqlite3.h"
#include <sys/time.h>
#include <time.h>

# define box_db "/ailvgo/www/database/box.db"
# define mobile_db "/ailvgo/www/database/mobile.db"

# define udp_start "/ailvgo/system/traffic/udp_start &"
# define heart_beat_run "/ailvgo/system/box/heart_beat_run &"
# define remote_control_run "/ailvgo/system/box/remote_control_run &"
# define run_daemon "/ailvgo/system/box/run_daemon &"

# define logfile "/ailvgo/system/log/sys_log"

void sys_log(char *str)
{
	FILE *fp;
	char content[100]={0};
        
	struct tm *timeinfo;
	time_t boxtime = 0;
	time(&boxtime);
	timeinfo = localtime(&boxtime);

	fp = fopen(logfile,"a");
	sprintf(content, "%s ----- %s\n", asctime(timeinfo), str);
	fputs(content,fp);
	fclose(fp);
}

void bail(char *s)
{
	fputs(s, stderr);
	fputc('\n', stderr);
}

int ParseJSON(cJSON *jnode, char wan_driver[5], int iptables_flag)
{
	char sh_cmd[200] = {0};
	const char *wan_ip = NULL;
 
    struct tm *timeinfo;
    sqlite3 *db = NULL;
    int rc = 0;
    char sql_cmd[100] = {0};
    char *errorMsg = NULL;
           
	cJSON *node = cJSON_GetObjectItem(jnode, "box_logon");
	char *time = cJSON_GetObjectItem(node, "time")->valuestring;	
	time_t boxtime = atol(time)-8*3600;
	stime(&boxtime);
        
    timeinfo = localtime(&boxtime);
    printf("box_login time is : %s\n",asctime(timeinfo));

    cJSON *wanipArray = cJSON_GetObjectItem(node, "wan_ip");
    if(!wanipArray)
    {
        bail("parse result : json has no node named wan_ip");
        return 0;
    }	

	cJSON *wanipList = wanipArray->child;
        
    if((wan_ip = cJSON_GetObjectItem(wanipList, "ip")->valuestring) != NULL)
        printf("ip : %s\n", cJSON_GetObjectItem(wanipList, "ip")->valuestring);

	printf("wan_ip : %s\n", wan_ip);

	if(strcmp(wan_ip, "any") == 0)
	{
		printf("iptables : set to any!\n");

        if(iptables_flag == 1)
			printf("iptables has set successfully!\n");
		else
			printf("iptables set immediately...\n");

		switch(atoi(wan_driver))
		{
			case 10 :
			{
				printf("iptables -t nat -A POSTROUTING -o wlan0 -s 192.168.100.0/24 -j MASQUERADE\n");
				system("iptables -t nat -A POSTROUTING -o wlan0 -s 192.168.100.0/24 -j MASQUERADE");
				break;
			}
			case 20 :
			{
				printf("iptables -t nat -A POSTROUTING -o eth1 -s 192.168.100.0/24 -j MASQUERADE\n");
				system("iptables -t nat -A POSTROUTING -o eth1 -s 192.168.100.0/24 -j MASQUERADE");
				break;
			}
			default :
			{
				printf("iptables -t nat -A POSTROUTING -o ppp0 -s 192.168.100.0/24 -j MASQUERADE\n");
				system("iptables -t nat -A POSTROUTING -o ppp0 -s 192.168.100.0/24 -j MASQUERADE");
				break;
			}
		}
		sys_log("box_login, wan = any!");
	}
	else
	{
		printf("iptables : set to none!\n"); 
        sys_log("box_login, wan = none!");
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
		bail("malloc space error");
	else
	{
		memset(buffer, 0, malloc_size);
		buffer_size = malloc_size;
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
				buffer = (char *)realloc(buffer, buffer_size + malloc_size);
				if(!buffer)
					bail("realloc error");
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

int main()
{
	ghttp_request *req;
	char http_body[100] = {0};
	sqlite3 *db = NULL;
    sqlite3_stmt *ppstmt = NULL, *ppstmt1 = NULL;
    int rc = 0, app_count = 0;
    char sql_cmd[100] = {0};
	char *errorMsg = NULL;
	const char *box_id = NULL, *wan_driver = NULL;
    char box_id_tmp[10] = {0}, wan_driver_tmp[5] = {0};
	char request_url[150] = {0};
	int iptables_flag = 0;
	
	FILE *stream = NULL;
	char buf[1024] = {0};
	int cnt = 0;

	int db_cnt;

    printf("*******************************\n"); 
   	printf("box_login : start\n");
    printf("*******************************\n");

    //heart_beat_run start
	printf("-------------------------------\n");
    printf("heart_beat_run: start\n");

	stream = NULL;
	if((stream = popen("ps -ef | grep 'heart_beat' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    
	memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("heart_beat process : %d\n", cnt);
        
    if(cnt >= 1)
		printf("heart_beat_run is running!\n");
    else
    {
    		system(heart_beat_run);
    		sleep(1);
	}

    //remote_control_run start
	printf("-------------------------------\n");
    printf("remote_control_run : start\n");
	
	stream = NULL;
	if((stream = popen("ps -ef | grep 'remote_control' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    
	memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("remote_control_run process : %d\n", cnt);
        
    if(cnt >= 1)
		printf("remote_control_run is running!\n");
    else
    {
    	system(remote_control_run);
		sleep(1);
	}

    //run_daemon start
	printf("-------------------------------\n");
    printf("run_daemon : start\n");
	
	stream = NULL;
	if((stream = popen("ps -ef | grep 'run_daemon' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    
	memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("run_daemon process : %d\n", cnt);
        
    if(cnt >= 1)
		printf("run_daemon is running!\n");
    else
    {
    	system(run_daemon);
		sleep(1);
	}

	//udp_start start
	printf("-------------------------------\n");
    printf("udp_start: start\n");
    	
	stream = NULL;
	if((stream = popen("ps -ef | grep 'udp_get' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    
	memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("udp_get process : %d\n", cnt);
        
    if(cnt >= 1)
		printf("udp_get is running!\n");
    else
    {
		system(udp_start);
		sleep(1);
	}

	db_cnt = 0;
	while(db_cnt < 20)
	{
		rc = sqlite3_open(box_db, &db);
    	if(rc == SQLITE_ERROR)
        	bail("open box.db failed");

		ppstmt = NULL;
    	memset(sql_cmd, 0, sizeof(sql_cmd));
    	strcpy(sql_cmd, "select box_id, wan_driver from box_info");
    	sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
    	rc = sqlite3_step(ppstmt);
    		
		if(rc == SQLITE_ROW)
    	{
			box_id = sqlite3_column_text(ppstmt, 0);
            strcpy(box_id_tmp, box_id);
            printf("box_id : %s\n", box_id_tmp);
            wan_driver = sqlite3_column_text(ppstmt, 1);
            strcpy(wan_driver_tmp, wan_driver);
            printf("wan_driver : %s\n", wan_driver_tmp);
    		sqlite3_finalize(ppstmt);
    		sqlite3_close(db);
			break;
    	}
		else
		{
			printf("select box_id,wan_driver failure!\n");
    		sqlite3_finalize(ppstmt);
    		sqlite3_close(db);
			sleep(1);
			db_cnt++;
		}
	}

	db_cnt = 0;
	while(db_cnt < 20)
	{
    	rc = sqlite3_open(mobile_db, &db);
    	if(rc == SQLITE_ERROR)
            bail("open mobile.db failed");

		ppstmt1 = NULL;
    	memset(sql_cmd, 0, sizeof(sql_cmd));
    	strcpy(sql_cmd, "select count(*) from mobile_info");
    	sqlite3_prepare(db, sql_cmd, -1, &ppstmt1, 0);
    	rc = sqlite3_step(ppstmt1);
    
		if(rc == SQLITE_ROW)
    	{
          	app_count = atoi(sqlite3_column_text(ppstmt1, 0));
            printf("app_count : %d\n", app_count);
    		sqlite3_finalize(ppstmt1);
    		sqlite3_close(db);
			break;
		}
		else
		{
			printf("select count(*) failure!\n");
    		sqlite3_finalize(ppstmt1);
    		sqlite3_close(db);
			sleep(1);
			db_cnt++;
		}
	}
			
	printf("--------------send login request -------------\n");
	memset(request_url, 0, sizeof(request_url));
	sprintf(request_url, "http://www.ailvgobox.com/box_manage_2/box_logon_1.php?box_id=%s&app_count=%d", box_id_tmp, app_count);
	request_url[149] = '\0';
	printf("request_url : %s\n", request_url);
	req = ghttp_request_new();
	strcpy(http_body, send_http_request(req, request_url));
    ghttp_request_destroy(req);
	
	printf("http_body : %s\n", http_body);
    printf("length of http_body : %d\n", strlen(http_body));
        
	cJSON *node;

    if(strlen(http_body) == 0)
    {
        printf("HTTP failure!\n");
		printf("iptables : set to any!\n");
		
		switch(atoi(wan_driver_tmp))
		{
			case 10 :
			{
				system("iptables -t nat -A POSTROUTING -o wlan0 -s 192.168.100.0/24 -j MASQUERADE");
				iptables_flag = 1;
				sys_log("login failure, wan = any by default!");
				break;
			}
			case 20 :
			{
				system("iptables -t nat -A POSTROUTING -o eth1 -s 192.168.100.0/24 -j MASQUERADE");
				iptables_flag = 1;
				sys_log("login failure, wan = any by default!");
				break;
			}
			default :
			{
				system("iptables -t nat -A POSTROUTING -o ppp0 -s 192.168.100.0/24 -j MASQUERADE");
				iptables_flag = 1;
				sys_log("login failure, wan = any by default!");
				break;
			}
		}

		while(1)
		{
			sleep(600);

			printf("--------send login request again---------\n");

			printf("request_url : %s\n", request_url);

			req = ghttp_request_new();
			strcpy(http_body, send_http_request(req, request_url));
       		ghttp_request_destroy(req);
	
			printf("http_body : %s\n", http_body);
        	printf("length of http_body : %d\n", strlen(http_body));
			
			if(strlen(http_body) == 0)
				printf("HTTP failure after retry!\n");
			else
				break;
		}
	}
    
	if(strlen(http_body) > 0)
		printf("HTTP success!\n");

    if(strcmp(http_body, "null") == 0)
    {
        printf("http_body : null, illegal box_id!\n");
        printf("box_login failure!");
        sys_log("login failure, illegal box_id");
    }
    else
    {
		node = cJSON_Parse(http_body);
        ParseJSON(node, wan_driver_tmp, iptables_flag);
	}

    printf("box_login : complete!\n");
	
    return 1;
}

 

