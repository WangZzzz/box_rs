/***********************
 * Filename : network_monitor_run.c
 * Date : 2015-03-05
 * **********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "sqlite3.h"

#define box_db "/ailvgo/www/database/box.db"

#define PING_FREQUENCY 3

# define logfile "/ailvgo/system/log/wan_log"
# define resolvfile "/etc/resolv.conf"

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
    int wan_break;
	char sql_cmd[100] = {0};
	char *errorMsg = NULL;    
	FILE *stream = NULL;
    char buf[1024] = {0};
	char buf_dns[1024] = {0};
    const char *wan_driver = NULL, *wan_state = NULL;
    char wan_driver_tmp[5] = {0};
	int cnt = 0, cnt_pppd = 0;
      
	FILE *fp;
	char nameserver[100];
	char dns_ip[50] = {0};
	
	// get the dns_ip from resolv.conf
    if((fp = fopen(resolvfile, "r")) == NULL)
    {
		perror("resolv_file open error!\n");
	}    
   fgets(nameserver,100,fp);

   int i=0, k=0;
   for(i=11; i< strlen(nameserver); i++)
   {     
	   dns_ip[k] = nameserver[i];
	   k++;
   }
   dns_ip[k+1]='\0';

   fclose(fp);
   printf("upstream dns : %s\n", dns_ip);

   char ping_cmd[100] = {0};
   sprintf(ping_cmd, "ping -c 1 www.qq.com");

    printf("***************************************\n");
	printf("network_monitor_run : start\n");
    printf("***************************************\n");

	while(1)
	{
		printf("\n----------network monitor : start----------\n");

		wan_break = 1;
		memset(buf_dns, sizeof(buf_dns), 0);
	
		for(cnt = 0; cnt < PING_FREQUENCY; ++cnt)
		{
            //printf("ping -c 1 %s\n", dns_ip);
			printf("ping -c 1 www.qq.com\n");
            stream = popen(ping_cmd, "r");
			fread(buf_dns, sizeof(char), sizeof(buf_dns), stream);
			buf_dns[1023] = '\0';
            pclose(stream);
			printf("buf_dns : %s\n", buf_dns);
			if(strstr(buf_dns, "1 received"))
            {   
                wan_break = 0;
                break;
            }
			sleep(5);

        }
	
		if(wan_break == 1)
		{
            sys_log("wan disconnected!");
 
			printf("network : disconnect!\n");

            printf("box.db : check & set wan_state = 0\n");

            rc = sqlite3_open(box_db, &db);
            if(rc == SQLITE_ERROR)
            {
                bail("open box.db failed");
            }
			ppstmt = NULL;
			strcpy(sql_cmd,"select wan_state from box_info");
			sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
            rc = sqlite3_step(ppstmt);
            if(rc == SQLITE_ROW)
            {
                wan_state = sqlite3_column_text(ppstmt, 0);
                printf("wan_state : %s\n", wan_state);
            }
			if(strcmp(wan_state, "0") == 0)
			{
				strcpy(sql_cmd, "update box_info set wan_state=\"0\"");
				sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
			}

            printf("box.db : check wan_driver\n");
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

            switch(atoi(wan_driver_tmp))
            {
				case 0 :
				{
					printf("no wan needed!\n");
					exit(0);
					break;
				}

				case 1 :	
				{
					printf("wan module : Leadcore LC5730\n");

					printf("check pppd ...\n");
					stream = NULL;
					stream = popen("ps -ef | grep 'pppd' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r");
					memset(buf, 0, sizeof(buf));
					fread(buf, sizeof(char), sizeof(buf), stream);
					buf[1023] = '\0';
					pclose(stream);               
					sscanf(buf, "%d", &cnt_pppd);
					printf("pppd process : %d\n", cnt_pppd);

					if(cnt_pppd >=1)
						printf("pppd exist, no need restart\n");
					else
					{
						printf("pppd not exist, restart pppd\n");
                        
						system("pppd call gprs-disconnect");
						sleep(5);
						printf("killall pppd...\n");
						system("killall pppd");
						sleep(5);
						printf("pppd start...\n");
						system("pppd call gprs-connect &");
					} 
					break;
				}

				case 2 : 
				{
					printf("wan_module : ZTE MC2716\n");
                        
					printf("pppd stop ...\n");
					system("pppd call evdo-disconnect");
					sleep(5);
					printf("killall pppd...\n");
					system("killall pppd");
					sleep(5);
					printf("pppd start...\n");
					system("pppd call evdo-connect &");
					break;
				}
    
				case 3 :
				{
					printf("wan_module : ZTE MF210\n");
                        
					printf("pppd stop ...\n");
					system("pppd call wcdma-disconnect");
					sleep(5);
					printf("killall pppd...\n");
					system("killall pppd");
					sleep(5);
					printf("pppd start...\n");
					system("pppd call wcdma-connect &");
					break;
				}
    
				case 4 :
				{
					printf("wan_module : LS 6300v\n");
                                
					printf("pppd stop ...\n");
					system("pppd call wcdma-ls-disconnect");
					sleep(5);
					printf("killall pppd...\n");
					system("killall pppd");
					sleep(5);
					printf("pppd start...\n");
					system("pppd call wcdma-ls-connect &");
					break;
				}
				
				case 10 :
				{
					printf("usb/wifi module\n");
					break;
				}

				case 20:
				{
					printf("usb/ethernet module\n");
					break;
				}

				default :
					printf("unsupported wan_driver in box.db");
			}
		}
		else
		{
			printf("network : connect!\n");

            printf("box.db : check & set wan_state = 1\n");

            rc = sqlite3_open(box_db, &db);
            if(rc == SQLITE_ERROR)
                bail("open box.db failed");
			ppstmt = NULL;
			strcpy(sql_cmd,"select wan_state from box_info");
			sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
            rc = sqlite3_step(ppstmt);
            if(rc == SQLITE_ROW)
            {
                wan_state = sqlite3_column_text(ppstmt, 0);
                printf("wan_state : %s\n", wan_state);
            }
			if(strcmp(wan_state, "0") == 0)
			{
				strcpy(sql_cmd, "update box_info set wan_state=\"1\"");
				sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
			}
            sqlite3_close(db);
		}
        
		printf("-------------network_monitor : complete!------------\n");
		
        sleep(120);
	}
	return 0;
}

