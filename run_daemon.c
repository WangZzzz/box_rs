/*******************************
 * Filename : run_daemon.c
 * Date : 2015-03-06
 * *****************************/


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define network_monitor_run "/ailvgo/system/box/network_monitor_run &"
#define heart_beat_run "/ailvgo/system/box/heart_beat_run &"
#define remote_control_run "/ailvgo/system/box/remote_control_run &"

#define logfile "/ailvgo/system/log/sys_log"

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

int main()
{
	FILE *stream = NULL;
	char buf[1024] = {0};
	int  cnt_network_monitor_run = 0, cnt_heart_beat_run = 0, cnt_remote_control_run = 0;
      
        printf("**********************************\n");
	printf("run_daemon : start\n");
        printf("**********************************\n");

        while(1)
        {
                sleep(600);
                
                printf("---------check network_monitor_run-----------\n");
 		stream = NULL;
		if((stream = popen("ps -ef | grep 'network_monitor' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
                        printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &cnt_network_monitor_run);
               	pclose(stream);
		printf("network_monitor_run process : %d\n", cnt_network_monitor_run);
		if(cnt_network_monitor_run>= 1)
			printf("network_monitor_run : ok!\n");
		else
    		{
                        printf("network_monitror_run : down, restart again...\n");
			sys_log("network_monitor_run : down!");
                        system(network_monitor_run);
                        sleep(5);
		}
               	
                printf("---------check heart_beat_run-----------\n");
 		stream = NULL;
		if((stream = popen("ps -ef | grep 'heart_beat' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
                        printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &cnt_heart_beat_run);
               	pclose(stream);
		printf("heart_beat_run process : %d\n", cnt_heart_beat_run);
		if(cnt_heart_beat_run >= 1)
			printf("heart_beat_run : ok!\n");
		else
    		{
                        printf("heart_beat_run : down, restart again...\n");
			sys_log("heart_beat_run : down!");
                        system(heart_beat_run);
                        sleep(5);
		}
                        
                printf("---------check remote_control_run-----------\n");
 		stream = NULL;
		if((stream = popen("ps -ef | grep 'remote_control' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
                        printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &cnt_remote_control_run);
               	pclose(stream);
		printf("remote_control_run process : %d\n", cnt_remote_control_run);
		if(cnt_remote_control_run>= 1)
			printf("remote_control_run : ok!\n");
		else
    		{
                        printf("remote_control_run : down, restart again...\n");
			sys_log("remote_control_run : down!");
                        system(remote_control_run);
                        sleep(5);
		}
         }

         printf("------------run_daemon : over------------\n");
         return 0;
}
