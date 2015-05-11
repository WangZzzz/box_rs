/*******************************
 * Filename : load_daemon.c
 * Date : 2015-03-06
 * *****************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define logfile "/ailvgo/system/log/load_log"

void sys_log(char *str)
{
	FILE *fp;
	char content[250]={0};
        
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
	char buf[8] = {0};

	float load[5];

	char loadinfo[150] = {0};
    int i;
	printf("\n---------check load average-----------\n");

	for(i=0; i<5; i++)
	{
		stream = NULL;
		if((stream = popen("cat /proc/loadavg | awk '{print $1}'", "r")) == NULL)
			    printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[7] = '\0';
		sscanf(buf, "%f", &load[i]);
		pclose(stream);
		printf("load[%d] : %.2f\n", i, load[i]);
		if(i<4)
		  sleep(10);
	}
	
	if(load[0]>0.8)
	{
		sprintf(loadinfo, "load : %.2f  %.2f  %.2f  %.2f  %.2f", load[0], load[1], load[2], load[3], load[4]);
		sys_log(loadinfo);
	}

	if((load[0]<load[1]) && (load[1]< load[2]) && (load[2]<load[3]) && (load[3]<load[4]))
	{
		printf("loadavg is increasing...\n");
		if(load[0] > 3)
		{
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
	
	int protect_flag=0;

    while(1)
    {
		if(check_load() == 1)
		{
			printf("check_load : loadavg alert!!!!!!\n");

			if(protect_flag == 1)
			{
				sys_log("reboot due to overloaded!");
				system("reboot");
			}
			
			if(protect_flag == 0)
			{
				// enter in protect mode
				sys_log("protect mode due to overloaded!\n");
				system("killall udp_get");
				sleep(2);
				system("killall mydns");
				sleep(2);
				system("service apache2 stop");
				sleep(2);
				protect_flag=1;
			}
		}
		else
			printf("check_load : loadavg ok!\n");

        sleep(60);
	}
}
