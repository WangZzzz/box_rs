/*******************************
 * Filename : sys_daemon.c
 * Date : 2015-05-09
 * *****************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define apache2 "service apache2 start"

#define mydns "/ailvgo/system/domain/mydns &"

#define logfile "/ailvgo/system/log/sys_log"

#define LIMIT 5000

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

int check_mydns_memory()
{
	FILE *stream = NULL;
	char buf[1024] = {0};

	char pid[10] = {0};
	char sh_cmd[200] = {0};

	int memory1 = 0, memory2 = 0;

    printf("---------check mydns memory-----------\n");
 	stream = NULL;
	if((stream = popen("ps -ef | grep 'mydns' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}'", "r")) == NULL)
                printf("popen failed\n");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
	sscanf(buf, "%s", &pid);
    	pclose(stream);

	printf("mydns pid : %s\n", pid);

	sprintf(sh_cmd, "cat /proc/%s/status | grep 'VmRSS' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}'", pid);
 	
	stream = NULL;
	if((stream = popen(sh_cmd, "r")) == NULL)
                printf("popen failed\n");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
	sscanf(buf, "%d", &memory1);
    pclose(stream);

	printf("mydns memery1 : %d\n", memory1);

	if(memory1 > LIMIT)
	{
		sleep(10);
		
		printf("check mydns memory 2nd time\n");
 		stream = NULL;
		if((stream = popen(sh_cmd, "r")) == NULL)
                	printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &memory2);
        	pclose(stream);
		
		printf("mydns memery2 : %d\n", memory2);

		if(memory2 > LIMIT)
			return 1;
	}
		
	return 0;
}
	
main()
{
	FILE *stream = NULL;
	char buf[1024] = {0};
	int cnt_apache2 = 0, cnt_mydns = 0;

	int loop = 0;

	float box_uptime;
      
    printf("**********************************\n");
	printf("sys_daemon : start\n");
    printf("**********************************\n");

    while(1)
    {
        sleep(180);

		loop++;

        printf("---------check apache2-----------\n");
 		stream = NULL;
		if((stream = popen("ps -ef | grep 'apache2' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
            printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &cnt_apache2);
        pclose(stream);
		printf("apache2 process : %d\n", cnt_apache2);
		if(cnt_apache2 == 0)
    	{
            printf("apache2 : down, restart again...\n");
            sys_log("apache2 down!");
			system(apache2);
        }
		else if((loop%20) == 0)
		{
			printf("apache2 : 1hour timeup, restart again\n");
			system("service apache2 restart");
		}


			
        printf("---------check mydns-----------------\n");
 		stream = NULL;
		if((stream = popen("ps -ef | grep 'mydns' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
            printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &cnt_mydns);
        pclose(stream);
		printf("mydns process : %d\n", cnt_mydns);
        sleep(1);

		if(cnt_mydns >= 1)
		{
			if((loop%20) == 0)
			{
			    printf("time up! mydns restart!\n");
				system("killall mydns");
				cnt_mydns = 0;
			}
			else
			{
				printf("mydns : ok!\n");
				printf("check mydns memory ....\n");
				if(check_mydns_memory() == 1)
				{
					printf("mydns memory : alert!!!!!!!\n");
					sys_log("mydns memory alert, restart immediately!\n");
					system("killall mydns");
					cnt_mydns = 0;
				}
			}
		}

		if(cnt_mydns == 0)
    	{
			printf("mydns : down, restart again...\n");
            system(mydns);
        }

		// check box uptime
 		stream = NULL;
		if((stream = popen("cat /proc/uptime | grep -v 'grep' | grep -v 'sh -c' | awk '{print $1}'", "r")) == NULL)
            printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%f", &box_uptime);
        pclose(stream);

		printf("box_uptime : %.2f\n", box_uptime);

		if(box_uptime > 3600*24 )
		{
			printf("system run more than 24 hours, reboot after 10 seconds!!!\n");
			sys_log("system run too long reboot!");
			sleep(10);
			system("reboot");
		}

    }
 
    printf("------------sys_daemon : over------------\n");
    return 0;
}
