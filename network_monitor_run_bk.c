/***********************
 * Filename : network_monitor_run.c
 * Date : 2015-02-13
 * **********************/

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include <time.h>


#define box_db "/tmp/usbmounts/sda1/www/database/box.db"
#define log_db "/tmp/usbmounts/sda1/www/database/log.db"

#define PING_FREQUENCY 2
#define DELAY 600

void bail(char *s)
{
        fputs(s, stderr);
        fputc('\n', stderr);
        exit(1);
}

int network_monitor()
{
	sqlite3 *db = NULL;
        sqlite3_stmt *ppstmt = NULL;
	int rc = 0;
        int wan_break = 1;
	char sql_cmd[100] = {0};
	char *errorMsg = NULL;
	FILE *stream = NULL;
        char buf[1024] = {0};
	char buf_qq[1024] = {0};
        char buf_baidu[1024] = {0};
        const char *wan_driver = NULL;
        char wan_driver_tmp[5] = {0};
	int cnt = 0, cnt_pppd = 0, cnt_pppd_start = 0;
        time_t timep;
        struct tm *timeinfo;
      
        printf("***************************************\n");
	printf("network_monitor : start\n");
        printf("***************************************\n");

	for(cnt = 0; cnt < PING_FREQUENCY; ++cnt)
	{
                printf("ping www.qq.com...\n");
                stream = popen("ping www.qq.com", "r");
		fread(buf_qq, sizeof(char), sizeof(buf_qq), stream);
		buf_qq[1023] = '\0';
                pclose(stream);
		printf("buf_qq : %s\n", buf_qq);
		if(strstr(buf_qq, "alive!"))
                {   
                     wan_break = 0;
                     break;
                }

                stream = popen("ping www.baidu.com", "r");
                fread(buf_baidu, sizeof(char), sizeof(buf_baidu), stream);
                buf_baidu[1023] = '\0';
                pclose(stream);
                printf("buf_baidu : %s\n", buf_baidu);
                if(strstr(buf_baidu, "alive!"))
                {
                     wan_break = 0;
                     break;
                }
	}
	
	if(wan_break == 1)
	{
                printf("network : disconnected!\n");
               
                printf("box.db : set wan_state = 0\n");

                rc = sqlite3_open(box_db, &db);
                if(rc == SQLITE_ERROR)
                {
                       bail("open box.db failed");
                       return 0;
                }
                strcpy(sql_cmd, "update box_info set wan_state=\"0\"");
                sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
                sqlite3_close(db);
 
                timep = 0;
                time(&timep);
                timeinfo = localtime(&timep);
                printf("network disconnect time is : %s\n", asctime(timeinfo));
                printf("log network disconnect event in database...\n"); 
                
                rc = sqlite3_open(log_db, &db);
                if(rc == SQLITE_ERROR)
                {
                       bail("open log.db failed");
                       return 0;
                }

                sprintf(sql_cmd, "insert into log_info values(\"%s\",'network disconnected, restart');",asctime(timeinfo));
                sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
                sqlite3_close(db);
                   
                sleep(2);
                
                printf("box.db : check wan_driver\n");

                rc = sqlite3_open(box_db, &db);
                if(rc == SQLITE_ERROR)
                {
                        printf("open box.db failed\n");
                        return 0;
                }	
	        ppstmt = NULL;
                memset(sql_cmd, 0, sizeof(sql_cmd));
                strcpy(sql_cmd, "select wan_driver from box_info");
                sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
                rc = sqlite3_step(ppstmt);
                if(rc == SQLITE_ROW)
                {
                        wan_driver = sqlite3_column_text(ppstmt, 0);
                        strcpy(wan_driver_tmp, wan_driver);
                        printf("wan_driver : %s\n", wan_driver_tmp);
                }
                sqlite3_finalize(ppstmt);
                sqlite3_close(db);

                if(strcmp(wan_driver_tmp, "1") == 0)
                {
                        printf("wan module : Leadcore LC5730\n");

                        printf("check pppd ...\n");
                        stream = NULL;
                        stream = popen("ps -ef | grep 'pppd' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $1}' | wc -l", "r");
                        memset(buf, 0, sizeof(buf));
                        fread(buf, sizeof(char), sizeof(buf), stream);
                        buf[1023] = '\0';
                        pclose(stream);               
                        sscanf(buf, "%d", &cnt_pppd);
                        printf("pppd process : %d\n", cnt_pppd);

                        if(cnt_pppd >=1)
                        {
                                printf("pppd exist, no need restart\n");
                        }
                        else
                        {
                                printf("pppd not exist, restart pppd\n");
                        
                                for(cnt_pppd_start = 0; cnt_pppd_start < 2; cnt_pppd_start++)
                                {
                                        system("pppd call gprs-disconnect &");
                                        sleep(30);
                                        printf("killall pppd...\n");
                                        system("killall pppd");
                                        sleep(5);
                                        printf("pppd start...\n");
                                        system("pppd call gprs-connect &");
                                        sleep(30);
                                        
                                        printf("check wan again...\n");
                                        printf("ping www.qq.com ...\n");
                                        stream = NULL;
                                        stream = popen("ping www.qq.com", "r");
                                        fread(buf_qq, sizeof(char), sizeof(buf_qq), stream);
                                        buf_qq[1023] = '\0';
                                        pclose(stream);
                                        printf("buf_qq:%s\n", buf_qq);
                                        if(strstr(buf_qq, "alive!"))
                                        {
                                              printf("network reconnected!\n");
                                              break;
                                        }
                                        sleep(10);
                                }
                        }
                }
                
                if(strcmp(wan_driver_tmp, "2") == 0)
                {
                        printf("wan_module : ZTE MC2716\n");
                        
                        printf("restart network...\n");
                    
                        for(cnt_pppd_start = 0; cnt_pppd_start < 2; cnt_pppd_start++)
                        {
                                printf("pppd stop ...\n");
                                system("pppd call evdo-disconnect &");
                                sleep(30);
                                printf("killall pppd...\n");
                                system("killall pppd");
                                sleep(5);
                                printf("pppd start...\n");
                                system("pppd call evdo-connect &");
                                sleep(30);
                               
                                printf("check wan again...\n"); 
                                printf("ping www.qq.com ...\n");
                                stream = NULL;
                                stream = popen("ping www.qq.com", "r");
                                fread(buf_qq, sizeof(char), sizeof(buf_qq), stream);
                                buf_qq[1023] = '\0';
                                printf("buf_qq:%s\n", buf_qq);
                                pclose(stream);
                                if(strstr(buf_qq, "alive!"))
                                {
                                        printf("network reconnected!\n");
                                        break;
                                }
                                sleep(10);
                        }
                }
    
		if(strcmp(wan_driver_tmp, "3") == 0)
                {
                        printf("wan_module : ZTE MF210\n");
                        
                        printf("restart network...\n");
                    
                        for(cnt_pppd_start = 0; cnt_pppd_start < 2; cnt_pppd_start++)
                        {
                                printf("pppd stop ...\n");
                                system("pppd call wcdma-disconnect &");
                                sleep(30);
                                printf("killall pppd...\n");
                                system("killall pppd");
                                sleep(5);
                                printf("pppd start...\n");
                                system("pppd call wcdma-connect &");
                                sleep(30);
                               
                                printf("check wan again...\n"); 
                                printf("ping www.qq.com ...\n");
                                stream = NULL;
                                stream = popen("ping www.qq.com", "r");
                                fread(buf_qq, sizeof(char), sizeof(buf_qq), stream);
                                buf_qq[1023] = '\0';
                                printf("buf_qq:%s\n", buf_qq);
                                pclose(stream);
                                if(strstr(buf_qq, "alive!"))
                                {
                                        printf("network reconnected!\n");
                                        break;
                                }
                                sleep(10);
                        }
                }

		if(strcmp(wan_driver_tmp, "4") == 0)
                {
                        printf("wan_module : LS 6300v\n");
                        
                        printf("restart network...\n");
                    
                        for(cnt_pppd_start = 0; cnt_pppd_start < 2; cnt_pppd_start++)
                        {
                                printf("pppd stop ...\n");
                                system("pppd call wcdma-ls-disconnect &");
                                sleep(30);
                                printf("killall pppd...\n");
                                system("killall pppd");
                                sleep(5);
                                printf("pppd start...\n");
                                system("pppd call wcdma-ls-connect &");
                                sleep(30);
                               
                                printf("check wan again...\n"); 
                                printf("ping www.qq.com ...\n");
                                stream = NULL;
                                stream = popen("ping www.qq.com", "r");
                                fread(buf_qq, sizeof(char), sizeof(buf_qq), stream);
                                buf_qq[1023] = '\0';
                                printf("buf_qq:%s\n", buf_qq);
                                pclose(stream);
                                if(strstr(buf_qq, "alive!"))
                                {
                                        printf("network reconnected!\n");
                                        break;
                                }
                                sleep(10);
                        }
                }
	}
	else
	{
		printf("network : connected!\n");

                printf("box.db : set wan_state = 1\n");

                rc = sqlite3_open(box_db, &db);
                if(rc == SQLITE_ERROR)
                {
                       bail("open box.db failed");
                       return 0;
                }
                strcpy(sql_cmd, "update box_info set wan_state=\"1\"");
		sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
                sqlite3_close(db);
	}
        printf("network_monitor : complete!\n");
	return 1;

}

int main()
{
	int start_t = 0, end_t = 0;
 
        printf("*****************************************\n");
        printf("network_monitor_run : start\n");
        printf("*****************************************\n");
        
        while(1)
	{
		printf("network monitor : start\n");

                start_t = time(NULL);
                
                if(network_monitor()==0)
                       printf("network monitor : faliure\n");
                else
                       printf("network monitor : success\n");
		
                end_t = time(NULL);

                sleep(DELAY-(end_t-start_t));
	}
	return 1;
}
