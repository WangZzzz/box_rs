/******************************
 *Filname : control.c
 *Date : 2015-03-05
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
# define box_login "/ailvgo/system/box/box_login &"
# define app_update "/ailvgo/system/box/app_update &"
# define file_manage "/ailvgo/system/box/file_manage &"
# define file_upload "/ailvgo/system/box/file_upload &"

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

void bail(char *s)
{
        fputs(s, stderr);
        fputc('\n', stderr);
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
    char buf_wifi[1024] = {0};
	int cnt = 0, idx = 0;
	const char *box_id = NULL, *wan_state = NULL, *wan_driver=NULL;
    const char *filename = NULL, *filepath = NULL;
      
    printf("**********************************\n");
	printf("control : start\n");
    printf("**********************************\n");
        
	/*************************************************/

        //sysfile update
        printf("-------------------------------\n");
        printf("sysfile update\n");
        printf("file.db : check sysfile...\n");

        rc=sqlite3_open(file_db, &db);
        if(rc == SQLITE_ERROR)
        {
                bail("open file.db failed");
        }
        ppstmt=NULL;
        memset(sql_cmd, 0, sizeof(sql_cmd));
        strcpy(sql_cmd, "select filename, filepath from file_info;");
        printf("sql_cmd : %s\n", sql_cmd);
        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        while(sqlite3_step(ppstmt) == SQLITE_ROW)
        {
                filename=sqlite3_column_text(ppstmt, 0);
                printf("file_name : %s\n", filename);
                filepath=sqlite3_column_text(ppstmt, 1);
                printf("box_folder : %s\n", filepath);

                printf("make temp folder...\n");
                system("mkdir -p /ailvgo/temp/temp/");
                sleep(1);
                printf("unzip file in temp folder...\n");
                memset(sh_cmd, 0, sizeof(sh_cmd));
                sprintf(sh_cmd, "unzip -o /ailvgo/www/temp/%s -d /ailvgo/www/temp/temp/", filename);
                printf("sh_cmd : %s\n", sh_cmd);
                system(sh_cmd);
                sleep(10);
                printf("cp files to destination path...\n");
                memset(sh_cmd, 0, sizeof(sh_cmd));
                sprintf(sh_cmd, "cp -rf /ailvgo/temp/temp/* %s", filepath);
                printf("sh_cmd : %s\n", sh_cmd);
                system(sh_cmd);
                sleep(10);
                printf("remove temp folder...\n");
                system("rm -rf /ailvgo/www/temp/temp/");
                sleep(10);
                printf("delete zipfile...\n");
                memset(sh_cmd, 0, sizeof(sh_cmd));
                sprintf(sh_cmd, "rm -f /ailvgo/www/temp/%s", filename);
                printf("sh_cmd : %s\n", sh_cmd);
                system(sh_cmd);
                sleep(2);
                printf("sysfile update complete!\n");
        }
        
        printf("delete all files in file.db!\n");
        memset(sql_cmd, 0, sizeof(sql_cmd)); 
        strcpy(sql_cmd, "delete from file_info;");
        printf("sql_cmd : %s\n", sql_cmd);
        sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
        sqlite3_finalize(ppstmt);
        sqlite3_close(db);
        
        sleep(1);

	//set wan_state=0
        printf("\n");
        printf("-------------------------------\n");
        printf("box.db : set wan_state = 0\n");

        rc = sqlite3_open(box_db, &db);
        memset(sql_cmd, 0, sizeof(sql_cmd));
        strcpy(sql_cmd, "update box_info set wan_state=\"0\"");
        if(rc == SQLITE_ERROR)
        {       
                bail("open box.db failed");
        }
        if(sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg) != SQLITE_OK)
                printf("errorMsg_control : %s\n", errorMsg);
        else
                printf("set wan_state = 0 ok!\n");
        sqlite3_close(db);

	    sleep(1);
	    
        // wan_start
        printf("\n");
	    printf("-------------------------------\n");
        printf("wan_start : start\n");

        system(wan_start);
        sleep(20);
        

        //mydns or dnsmasq run
        if(access("/ailvgo/system/domain/mydns", W_OK) == 0)
        {
        	//my_dns run
        	printf("\n");
		    printf("-------------------------------\n");
        	printf("mydns : start \n");

        	system(mydns);
        	sleep(1);
 	    }
        else
        {
        	//dnsmasq run
        	printf("\n");
		    printf("-------------------------------\n");
        	printf("dnsmasq : start \n");

        	system("service dnsmasq restart");
        	sleep(1);
        }
       
	//sys_daemon start
	    printf("\n");
	    printf("-------------------------------\n");
        printf("sys_daemon : start\n");

        system(sys_daemon);
        sleep(1);

	//load_daemon start
	    printf("\n");
	    printf("-------------------------------\n");
        printf("load_daemon : start\n");

        system(load_daemon);
        sleep(1);
	
        /*************************************************************/

	// check wan_driver
        printf("\n");
        printf("-------------------------------\n");
        printf("box.db : check wan_driver\n");

	    ppstmt=NULL;
        memset(sql_cmd, 0, sizeof(sql_cmd));
        strcpy(sql_cmd, "select wan_driver from  box_info");
        rc = sqlite3_open(box_db, &db);
        if(rc == SQLITE_ERROR)
        {       
                bail("open box.db failed");
        }
        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        rc = sqlite3_step(ppstmt);
        if(rc == SQLITE_ROW)
        {  
              wan_driver=sqlite3_column_text(ppstmt, 0);
              printf("wan_driver : %s\n", wan_driver);
        }
        if(strcmp(wan_driver, "0") == 0)
        {
              sqlite3_finalize(ppstmt);
              sqlite3_close(db);
              printf("no wan needed, control finish!\n"); 
              exit(1);
        }
	    sqlite3_finalize(ppstmt);	
	    sqlite3_close(db);
        
         	
       	// network_monitor_run start
        printf("/n");
	    printf("-------------------------------\n");
        printf("network_monitor_run : start\n");
	
	    system(network_monitor_run);
        sleep(1);
        
    	/************************************************************/
       
       	//check wan_state
    printf("\n");
    printf("-------------------------------\n");
    printf("box.db: check wan_state\n");

    printf("start wan_state check...\n");
	while(1)
	{
		sleep(15);
		
		rc = sqlite3_open(box_db, &db);
        if(rc == SQLITE_ERROR)
       	{
             bail("open database failed");
        }
		
		ppstmt = NULL;
		memset(sql_cmd, 0, sizeof(sql_cmd));
		strcpy(sql_cmd, "select box_id, wan_state from box_info");
        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
       	rc = sqlite3_step(ppstmt);
        if(rc == SQLITE_ROW)
        {
                box_id = sqlite3_column_text(ppstmt, 0);
                printf("box_id : %s | ", box_id);
                wan_state = sqlite3_column_text(ppstmt, 1);
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
        
        printf("Network connected!\n");
        
        //box_login
	printf("\n");
	printf("-------------------------------\n");
        printf("box_login: start");

	system(box_login);
	sleep(20);

        //file_manage
        printf("\n");
        printf("-------------------------------\n");
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
                {
                        cnt = 0;
                        continue;
                }
                else
                        break;
        }
	if(idx == MAXRUNTIME)
        {
              printf("killall file_manage\n");
              system("killall file_manage");
              sleep(5);
              printf("killall curl\n");
              system("killall curl");
        }
        
        //app_update
	printf("\n");
        printf("-------------------------------\n");
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
                {
                        cnt = 0;
                        continue;
                }
                else
                        break;
        }
	if(idx == MAXRUNTIME)
        {
               printf("killall app_update\n");
               system("killall app_update");
               sleep(5);
               printf("killall curl\n");
               system("killall curl");
        }
        
        //file_upload
        printf("\n");
        printf("-------------------------------\n");
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
                {
                        cnt = 0;
                        continue;
                }
                else
                        break;
        }
	if(idx == MAXRUNTIME)
        {
                printf("killall file_upload\n");
                system("killall file_upload");
                sleep(5);
                printf("killall curl\n");
                system("killall curl");
        }
        
        printf("\n");
        printf("**********************************\n");
        printf("control finish\n");
        printf("**********************************\n");

	return 0;
}
