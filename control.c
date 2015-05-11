/******************************
 *Filname : control.c
 *Date : 2015-05-09
 * *****************************/


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include <time.h>

# define MAXRUNTIME 180

# define box_db "/ailvgo/www/database/box.db"
# define file_db "/ailvgo/www/database/file.db"

# define network_monitor_run "/ailvgo/system/box/network_monitor_run &"

# define wan_start "/ailvgo/system/wan_shell/wan_start &"
# define mydns "/ailvgo/system/domain/mydns &"
# define sys_daemon "/ailvgo/system/box/sys_daemon &"
# define load_daemon "/ailvgo/system/box/load_daemon &"
# define box_conf_change "/ailvgo/system/box/box_conf_change &"
# define box_conf_report "/ailvgo/system/box/box_conf_report &"
# define box_login "/ailvgo/system/box/box_login &"
# define sysfile_manage "/ailvgo/system/box/sysfile_manage &"
# define app_update "/ailvgo/system/box/app_update &"
# define file_manage "/ailvgo/system/box/file_manage &"
# define file_upload "/ailvgo/system/box/file_upload &"
# define udp_upload "/ailvgo/system/traffic/udp_upload &"

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

int main()
{
	sqlite3 *db = NULL;
	sqlite3_stmt *ppstmt = NULL;
	int rc = 0;
	char sql_cmd[100] = {0};
	char sh_cmd[200] = {0};
	char *errorMsg = NULL;
	FILE *stream = NULL;
	char buf[1024] = {0};
	int cnt = 0, idx = 0;
	const char *wan_state = NULL, *wan_driver=NULL;
	char wan_driver_tmp[5] = {0};
	int db_cnt;
      
    printf("**********************************\n");
	printf("control : start\n");
   	printf("**********************************\n");

	// set wan_state=0 in box.db
    printf("\n-------------------------------\n");
	printf("box.db : check & set wan_state = 0\n");

	db_cnt = 0;
	while(db_cnt < 20)    
	{
		rc = sqlite3_open(box_db, &db);
    	if(rc == SQLITE_ERROR)
			bail("open box.db failed");

		strcpy(sql_cmd, "update box_info set wan_state=\"0\"");
		if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg)!=SQLITE_OK)
		{
			printf("errorMsg : %s\n", errorMsg);
			sqlite3_close(db);
			sleep(1);
			db_cnt++;
		}
		else
		{
			printf("wan_state = 0!\n");
    		sqlite3_close(db);
			break;
		}
	}
	
    // wan_start
	printf("\n-------------------------------\n");
    printf("wan_start : start\n");

	system(wan_start);
    sleep(20);
	
    //mydns run
	printf("\n-------------------------------\n");
    printf("mydns : start \n");

    stream = NULL;
	if((stream = popen("ps -ef | grep 'mydns' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("mydns process : %d\n", cnt);
    if(cnt >= 1)
		printf("mydns : running!\n");
    else
    {
		system(mydns);
    	sleep(1);
	}

	//load_daemon start
    printf("\n-------------------------------\n");
    printf("load_daemon : start\n");

    stream = NULL;
	if((stream = popen("ps -ef | grep 'load_daemon' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("load_daemon process : %d\n", cnt);
    if(cnt >= 1)
		printf("load_daemon : running!\n");
    else
    {
		system(load_daemon);
    		sleep(1);
	}
	
	//sys_daemon start
	printf("\n-------------------------------\n");
    printf("sys_daemon : start\n");

    stream = NULL;
	if((stream = popen("ps -ef | grep 'sys_daemon' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("sys_daemon process : %d\n", cnt);
    if(cnt >= 1)
		printf("sys_daemon : running!\n");
    else
    {
		system(sys_daemon);
    	sleep(1);
	}

    /*************************************************************/

	// check wan_driver
    printf("\n-------------------------------\n");
    printf("box.db : check wan_driver\n");

	ppstmt=NULL;
    memset(sql_cmd, 0, sizeof(sql_cmd));
    strcpy(sql_cmd, "select wan_driver from  box_info");

	db_cnt = 0;
	while(db_cnt < 20)
	{
    	rc = sqlite3_open(box_db, &db);
    	if(rc == SQLITE_ERROR)
			bail("open box.db failed");

  		sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
    	rc = sqlite3_step(ppstmt);
    	if(rc == SQLITE_ROW)
    	{  
        	wan_driver=sqlite3_column_text(ppstmt, 0);
			strcpy(wan_driver_tmp, wan_driver);
			printf("wan_driver : %s\n", wan_driver_tmp);
        	sqlite3_finalize(ppstmt);
        	sqlite3_close(db);
			break;
    	}
		else
		{
			printf("select wan_driver failure!\n");
        	sqlite3_finalize(ppstmt);
        	sqlite3_close(db);
			sleep(1);
			db_cnt++;
		}
	}

    if(strcmp(wan_driver_tmp, "0") == 0)
    {
        printf("no wan needed, control finish!\n"); 
        exit(1);
    }
         	
    // network_monitor_run start
	printf("\n-------------------------------\n");
    printf("network_monitor_run : start\n");
	
    stream = NULL;
	if((stream = popen("ps -ef | grep 'network_monitor' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("network_monitor_run process : %d\n", cnt);
    if(cnt >= 1)
		printf("network_monitor_run : running!\n");
    else
    {
		system(network_monitor_run);
    	sleep(1);
	}
        
    /************************************************************/
       
    //check wan_state
   	printf("\n-------------------------------\n");
    printf("box.db: check wan_state\n");

    printf("start wan_state check...\n");
	while(1)
	{
		sleep(10);
		
		rc = sqlite3_open(box_db, &db);
        if(rc == SQLITE_ERROR)
            bail("open database failed");
		
		ppstmt = NULL;
		memset(sql_cmd, 0, sizeof(sql_cmd));
		strcpy(sql_cmd, "select wan_state from box_info");
		sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
		rc = sqlite3_step(ppstmt);
		if(rc == SQLITE_ROW)
		{
			wan_state = sqlite3_column_text(ppstmt, 0);
			printf("wan_state : %s\n", wan_state);
			if(strcmp(wan_state, "1") == 0)
			{	
				printf("wan connected!!\n");
				sqlite3_finalize(ppstmt);
				sqlite3_close(db);
				break;
			}
		}
		sqlite3_finalize(ppstmt);	
		sqlite3_close(db);	
	}
        
    printf("Network : connected!\n");
   
    //check ailvgobox server online & offline
    printf("\n-------------------------------\n");
    printf("ailvgobox : check online \n");

	int ailvgobox_break = 1;

    printf("start ailvgobox check...\n");
	
	for(idx = 0; idx < 10; ++idx)
	{
		if(ailvgobox_monitor() == 1)
		{
			printf("ailvgobox server : offline!\n");
			sys_log("ailvgobox : offline");
			ailvgobox_break = 1;
			sleep(20);
		}
		else
		{
			printf("ailvgobox server : online!\n");
			ailvgobox_break = 0;
			break;
		}
	}

	if(ailvgobox_break == 1)
	{
		printf("ailvgobox : offline, try again every 20 mins\n");
	
		printf("network connected & ailvgobox offline, set iptables by default!\n");
		
		switch(atoi(wan_driver_tmp))
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

		while(1)
		{
			sleep(1200);
			if(ailvgobox_monitor() == 1)
			{
				printf("ailvgobox server : offline!\n");
				sys_log("ailvgobox : offline");
			}
			else
			{
			  printf("ailvgobox server : online!\n");
			  break;
			}
		}
	}

    //box_conf_change
	printf("\n-------------------------------\n");
    printf("box_conf_change : start\n");
    
	system(box_conf_change);

	for(idx = 0; idx < 6; ++idx)
    {
		sleep(10);
        
		stream = NULL;
		if((stream = popen("ps -ef | grep 'box_conf' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
			printf("popen failed\n");
        memset(buf, 0, sizeof(buf));
        fread(buf, sizeof(char), sizeof(buf), stream);
        buf[1023] = '\0';
        sscanf(buf, "%d", &cnt);
        pclose(stream);
        printf("box_conf_change process : %d\n", cnt);
        
		if(cnt >= 1)
            continue;
        else
            break;
    }

	if(idx == 6)
    {
        printf("killall box_conf_change\n");
		system("killall box_conf_change");
    }

    //box_conf_report start
	printf("\n-------------------------------\n");
    printf("box_conf_report : start\n");
    
	system(box_conf_report);
	
	for(idx = 0; idx < 6; ++idx)
    {
		sleep(10);
        
		stream = NULL;
		if((stream = popen("ps -ef | grep 'box_conf' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
			printf("popen failed\n");
        memset(buf, 0, sizeof(buf));
        fread(buf, sizeof(char), sizeof(buf), stream);
        buf[1023] = '\0';
        sscanf(buf, "%d", &cnt);
        pclose(stream);
        printf("box_conf_report process : %d\n", cnt);
        
		if(cnt >= 1)
            continue;
        else
            break;
    }

	if(idx == 6)
    {
        printf("killall box_conf_report\n");
		system("killall box_conf_report");
    }

    //box_login
	printf("\n-------------------------------\n");
    printf("box_login: start\n");

	stream = NULL;
	if((stream = popen("ps -ef | grep 'box_login' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
		printf("popen failed\n");
    memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf), stream);
    buf[1023] = '\0';
    sscanf(buf, "%d", &cnt);
    pclose(stream);
    printf("box_login process : %d\n", cnt);
        
    if(cnt >= 1)
		printf("box_login : running!\n");
    else
    {
		system(box_login);
    	sleep(20);
	}

	//sysfile_manage
	printf("\n--------------------------------\n");
	printf("sysfile_manage : start\n");

	system(sysfile_manage);

	for(idx = 0; idx < MAXRUNTIME; ++idx)
    {
		sleep(10);
        
		stream = NULL;
		if((stream = popen("ps -ef | grep 'sysfile_manage' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
			printf("popen failed\n");
        memset(buf, 0, sizeof(buf));
        fread(buf, sizeof(char), sizeof(buf), stream);
        buf[1023] = '\0';
        sscanf(buf, "%d", &cnt);
        pclose(stream);
        printf("sysfile_manage process : %d\n", cnt);
        
		if(cnt >= 1)
            continue;
        else
            break;
    }
	
	if(idx == MAXRUNTIME)
    {
        printf("killall sysfile_manage\n");
		system("killall sysfile_manage");
        sleep(1);
        printf("killall curl\n");
        system("killall curl");
    }

    //app_update
    printf("\n-------------------------------\n");
    printf("app_update : start\n");

    system(app_update);

    for(idx = 0; idx < MAXRUNTIME; ++idx)
    {
        sleep(15);

        stream = NULL;
		if((stream = popen("ps -ef | grep 'app_update' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
			printf("popen failed\n");
        memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
        buf[1023] = '\0';
        sscanf(buf, "%d", &cnt);
        pclose(stream);
        printf("app_update process : %d\n", cnt);
        
		if(cnt >= 1)
            continue;
        else
            break;
    }
	
	if(idx == MAXRUNTIME)
    {
        printf("killall app_update\n");
        system("killall app_update");
        sleep(1);
        printf("killall curl\n");
        system("killall curl");
    }
        
    //file_manage
    printf("\n-------------------------------\n");
    printf("file_manage : start\n");

	system(file_manage);
	
	for(idx = 0; idx < MAXRUNTIME; ++idx)
    {
        sleep(15);
        stream = NULL;
		if((stream = popen("ps -ef | grep 'file_manage' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
			printf("popen failed\n");
        memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
       	buf[1023] = '\0';
        sscanf(buf, "%d", &cnt);
        pclose(stream);
        printf("file_manage process : %d\n", cnt);
       
	 	if(cnt >= 1)
            continue;
        else
            break;
    }
	
	if(idx == MAXRUNTIME)
    {
		printf("killall file_manage\n");
        system("killall file_manage");
        sleep(1);
        printf("killall curl\n");
        system("killall curl");
    }
        
    //file_upload
    printf("\n-------------------------------\n");
    printf("file_upload : start\n");

    system(file_upload);
    
	for(idx = 0; idx < MAXRUNTIME; ++idx)
    {
		sleep(15);
        stream = NULL;
		if((stream = popen("ps -ef | grep 'file_upload' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
			printf("popen failed\n");
        memset(buf, 0, sizeof(buf));
        fread(buf, sizeof(char), sizeof(buf), stream);
        buf[1023] = '\0';
        sscanf(buf, "%d", &cnt);
        pclose(stream);
        printf("file_upload process : %d\n", cnt);
        
		if(cnt >= 1)
            continue;
        else
            break;
    }
	
	if(idx == MAXRUNTIME)
    {
		printf("killall file_upload\n");
        system("killall file_upload");
        sleep(1);
        printf("killall curl\n");
        system("killall curl");
    }
        
    //udp_upload
    printf("\n-------------------------------\n");
    printf("udp_upload : start\n");

    system(udp_upload);

    for(idx = 0; idx < MAXRUNTIME; ++idx)
   	{
		sleep(15);
        stream = NULL;
		if((stream = popen("ps -ef | grep 'udp_upload' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
			printf("popen failed\n");
        memset(buf, 0, sizeof(buf));
        fread(buf, sizeof(char), sizeof(buf), stream);
        buf[1023] = '\0';
        sscanf(buf, "%d", &cnt);
        pclose(stream);
        printf("udp_upload process : %d\n", cnt);
        
		if(cnt >= 1)
            continue;
        else
            break;
    }
	
	if(idx == MAXRUNTIME)
    {
		printf("killall udp_upload\n");
        system("killall udp_upload");
        sleep(1);
        printf("killall curl\n");
        system("killall curl");
    }
        
    printf("\n**********************************\n");
    printf("control : complete!\n");
	printf("**********************************\n");

}
