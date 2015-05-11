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

#define PING_FREQUENCY 2

# define logfile "/ailvgo/system/log/wan_log"
# define resolvfile "/etc/resolv.conf"

int check_wanip()
{
	FILE *stream;
	char buf[1024] = {0};

    stream = popen("ifconfig | grep 'inet addr' | grep -v '192.168.100.1' | grep -v '192.168.100.2' | grep -v '127.0.0.1' | awk '{print $2}'", "r");
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
    pclose(stream);
	printf("buf : %s\n", buf);

	if(strlen(buf)>11)
		return 1;
	else
		return 0;
}

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

int wan_monitor()
{
	FILE *stream;
	char buf_qq[1024] = {0};
	int cnt;
	int wan_break = 1;
	
	for(cnt = 0; cnt < PING_FREQUENCY; ++cnt)
	{
		memset(buf_qq, 0, sizeof(buf_qq));
		printf("ping -c 1 www.qq.com\n");
        	stream = popen("ping -c 1 www.qq.com", "r");
		fread(buf_qq, sizeof(char), sizeof(buf_qq), stream);
		buf_qq[1023] = '\0';
        	pclose(stream);
		printf("buf_qq : %s\n", buf_qq);
		
		if(strstr(buf_qq, "1 received"))
        	{   
			wan_break = 0;
			break;
        	}
		sleep(5);
	}

	if(wan_break == 1)
		return 1;
	else
		return 0;
}


void wan_reconnect(int wan_driver)
{
	FILE *stream = NULL;
    	char buf[1024] = {0};
	int cnt_pppd = 0;

	switch(wan_driver)
	{
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

			printf("checking wlan0 inet_addr\n");
                    
			if(check_wanip())
				printf("wlan0 : ip addr\n");
			else
			{
				printf("wlan0 :  no ip addr\n");
				printf("service networking restart\n");
				system("service networking restart");
			}
			break;
		}

		case 20:
		{
			printf("usb/ethernet module\n");
					
			printf("checking ethernet inet_addr\n");
                    
			if(check_wanip())
				printf("eth1 : ip addr\n");
			else
			{
				printf("eth1 : no ip addr\n");
				printf("service networking restart\n");
				system("service networking restart");
			}
			break;
		}
	
		default :
			printf("unsupported wan_driver in box.db");
	}
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
    	const char *wan_driver = NULL, *wan_state = NULL;
    	char wan_driver_tmp[5] = {0}, wan_state_tmp[3] = {0};
	int wan_break_cnt = 0, set_cnt = 0;
      
    	printf("***************************************\n");
	printf("network_monitor_run : start\n");
    	printf("***************************************\n");

	while(1)
	{
		printf("\n----------network monitor : start----------\n");

		wan_break = wan_monitor();
	
		if(wan_break == 1)
		{
            		sys_log("network disconnected!");
 
			printf("network : disconnect!\n");

            		printf("box.db : check wan_driver & set wan_state = 0\n");

			set_cnt = 0;
			while(set_cnt < 20)
			{
				rc = sqlite3_open(box_db, &db);
				if(rc == SQLITE_ERROR)
					bail("open box.db failed");
				
				ppstmt = NULL;
				strcpy(sql_cmd,"select wan_driver,wan_state from box_info");
				sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
				rc = sqlite3_step(ppstmt);
				if(rc == SQLITE_ROW)
				{
					wan_driver = sqlite3_column_text(ppstmt, 0);
					strcpy(wan_driver_tmp, wan_driver);
					printf("wan_driver : %s\n", wan_driver_tmp);
					wan_state = sqlite3_column_text(ppstmt, 1);
					strcpy(wan_state_tmp, wan_state);
					printf("wan_state : %s\n", wan_state_tmp);
					sqlite3_finalize(ppstmt);
					sqlite3_close(db);
					break;
				}
				else
				{
					printf("select wan_driver,wan_state failure!\n");
					sqlite3_finalize(ppstmt);
					sqlite3_close(db);
					sleep(1);
					set_cnt++;
				}
			}

			if(strcmp(wan_state_tmp, "0") != 0)
			{
				set_cnt = 0;
				while(set_cnt < 20)
				{
					rc = sqlite3_open(box_db, &db);
					if(rc == SQLITE_ERROR)
						bail("open box.db failed");
				
					strcpy(sql_cmd, "update box_info set wan_state=\"0\"");
					rc = sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
					if(rc == SQLITE_OK)
					{
						printf("update wan_state = 0 successfully!\n");
						sqlite3_close(db);
						break;
					}
					else	
					{
						printf("update wan_state = 0 failure!\n");
						sqlite3_close(db);
						sleep(1);
						set_cnt++;
					}
				}
			}

			while(1)
			{
				wan_break_cnt++;

				printf("wan_break_cnt : %d\n", wan_break_cnt);

				if(wan_break_cnt == 10)
				{
					printf("reboot due to reconnect 10 times!\n");
					sys_log("reboot due to wan_break 10 times!");
					sleep(1);
					system("reboot");
				}
				
				wan_reconnect(atoi(wan_driver_tmp));
			
				sleep(20);

				if(wan_monitor() == 0)
				{
					printf("wan reconnect successfully!\n");
					wan_break = 0;
					break;
				}
				else
					printf("wan reconnect fail!\n");
				
				sleep(10);
			}
		}
		
		if(wan_break == 0)
		{
			printf("network : connect!\n");

			wan_break_cnt = 0;

            		printf("box.db : check & set wan_state = 1\n");

			set_cnt = 0;
			while(set_cnt < 20)
			{
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
					strcpy(wan_state_tmp, wan_state);
					printf("wan_state : %s\n", wan_state_tmp);
					sqlite3_finalize(ppstmt);
					sqlite3_close(db);
					break;
				}
				else
				{
					printf("select wan_state failure!\n");
					sqlite3_finalize(ppstmt);
					sqlite3_close(db);
					sleep(1);
					set_cnt++;
				}
			}
			
			if(strcmp(wan_state_tmp, "0") == 0)
			{
				set_cnt = 0;
				while(set_cnt < 20)
				{
					rc = sqlite3_open(box_db, &db);
					if(rc == SQLITE_ERROR)
						bail("open box.db failed");
				
					strcpy(sql_cmd, "update box_info set wan_state=\"1\"");
					rc = sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
					if(rc == SQLITE_OK)
					{
						printf("update wan_state = 1 successfully!\n");
						sqlite3_close(db);
						break;
					}
					else
					{
						printf("update wan_state = 1 failure!\n");
						sqlite3_close(db);
						sleep(1);
						set_cnt++;
					}
				}
			}
		}
        
		printf("-------------network_monitor : complete!------------\n");
		
        	sleep(180);
	}
}

