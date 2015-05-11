/*******************************
 * Filename : load_daemon.c
 * Date : 2015-03-06
 * *****************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define logfile "/ailvgo/system/sys_log"

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

int check_load()
{
	FILE *stream = NULL;
	char buf[1024] = {0};

	float load_1, load_5, load_15;

	char loadinfo[20] = {0};

	struct tm *timeinfo;
	time_t boxtime = 0;
	time(&boxtime);
	timeinfo = localtime(&boxtime);

        printf("---------check load average-----------\n");

 	stream = NULL;
	if((stream = popen("cat /proc/loadavg | awk '{print $1}'", "r")) == NULL)
                printf("popen failed\n");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
	sscanf(buf, "%f", &load_1);
        pclose(stream);

 	stream = NULL;
	if((stream = popen("cat /proc/loadavg | awk '{print $2}'", "r")) == NULL)
                printf("popen failed\n");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
	sscanf(buf, "%f", &load_5);
        pclose(stream);

 	stream = NULL;
	if((stream = popen("cat /proc/loadavg | awk '{print $3}'", "r")) == NULL)
                printf("popen failed\n");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
	sscanf(buf, "%f", &load_15);
        pclose(stream);
	
	printf("load_1 : %.2f, load_5 : %.2f, load_15 : %.2f ------%s\n", load_1, load_5, load_15, asctime(timeinfo));
	
	if((load_1 > load_5 )&&(load_5 > load_15))
	{
		printf("loadavg is increasing...\n");
		if(load_15 > 3)
		{
			sprintf(loadinfo, "load_1 : %.2d, load_2 : %.2f, load_3: %.2f", load_1, load_5, load_15);
			sys_log(loadinfo);
			return 1;
		}
	}
	
	return 0;
}
	

main()
{
        printf("**********************************\n");
	printf("load_daemon : start\n");
        printf("**********************************\n");
	
        while(1)
        {
                sleep(60);

		if(check_load() == 1)
		{
			printf("check_load : loadavg alert!!!!!!\n");
			
			// enter in protective mode
			sys_log("enter in sleep mode, killall udp_get&mydns!\n");
			system("killall udp_get");
			sleep(2);
			system("killall mydns");
			sleep(2);
			system("service apache2 stop");
			sleep(2);
		}
	}
}
