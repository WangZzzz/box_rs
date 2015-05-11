/*******************************
 * Filename : udp_daemon.c
 * Date : 2015-03-06
 * *****************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define udp_get "/ailvgo/system/traffic/udp_get &"

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

int check_udpget_memory()
{
	FILE *stream = NULL;
	char buf[1024] = {0};

	char pid[10] = {0};

	char sh_cmd[100] = {0};

	int memory1 = 0, memory2 = 0, memory3 = 0;

        printf("---------check udp_get memory-----------\n");
 	stream = NULL;
	if((stream = popen("ps -ef | grep 'udp_get' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}'", "r")) == NULL)
                	printf("popen failed\n");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
	sscanf(buf, "%s", &pid);
        pclose(stream);
	
	printf("udp_get pid : %s\n", pid);

	sprintf(sh_cmd, "cat /proc/%s/status | grep 'VmRSS' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}'", pid);
 	
	stream = NULL;
	if((stream = popen(sh_cmd, "r")) == NULL)
                	printf("popen failed\n");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[1023] = '\0';
	sscanf(buf, "%d", &memory1);
        pclose(stream);

	printf("udp_get memery1 : %d\n", memory1);

	if(memory1 > LIMIT)
	{
		sleep(10);
		
		printf("check udp_get memory 2nd time\n");
 		stream = NULL;
		if((stream = popen(sh_cmd, "r")) == NULL)
                	printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &memory2);
        	pclose(stream);

		printf("udp_get memory2 : %d\n", memory2);

		if(memory2 > LIMIT)
		{
			sleep(10);

			printf("check udp_get memory 3rd time\n");
 			stream = NULL;
			if((stream = popen(sh_cmd, "r")) == NULL)
                		printf("popen failed\n");
			memset(buf, 0, sizeof(buf));
			fread(buf, sizeof(char), sizeof(buf), stream);
			buf[1023] = '\0';
			sscanf(buf, "%d", &memory3);
        		pclose(stream);
			
			printf("udp_get memory3 : %d\n", memory3);
			
			if(memory3 > LIMIT)
				return 1;
		}
	}
		
	return 0;
}
	

main()
{
	FILE *stream = NULL;
	char buf[1024] = {0};
	int cnt_udp_get;

	int loop = 0;
      
        printf("**********************************\n");
	printf("udp_daemon : start\n");
        printf("**********************************\n");

        while(1)
        {
                sleep(180);

		loop++;

		printf("**********loop : %d**************\n", loop);
				
                printf("---------check udp_get-----------\n");


 		stream = NULL;
		if((stream = popen("ps -ef | grep 'udp_get' | grep -v 'grep' | grep -v 'sh -c' | awk '{print $2}' | wc -l", "r")) == NULL)
                	printf("popen failed\n");
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), sizeof(buf), stream);
		buf[1023] = '\0';
		sscanf(buf, "%d", &cnt_udp_get);
                pclose(stream);
		printf("udp_get process : %d\n", cnt_udp_get);
		
		if(cnt_udp_get == 0)
		{
                        printf("udp_get : down, restart again...\n");
			sys_log("udp_get : down!");
		}

		if(cnt_udp_get >= 1)
		{
			printf("loop : %d\n", loop%60);
			
			/*if((loop%120) == 0)
			{	
				printf("time up! udp_get restart!\n");
				system("killall udp_get");
				cnt_udp_get = 0;
			}
			else
			{*/
				printf("udp_get : ok!\n");
				printf("udp_get : check memory usage...\n");

				if(check_udpget_memory() == 1)
				{
					printf("udp_get : memory alert!!!!!!\n");
					sys_log("udp_get memory alert! restart immediately!");
					system("killall udp_get");
					cnt_udp_get = 0;
				}
				else
					printf("udp_get memory : ok!\n");
			/*}*/
		}
		
		if(cnt_udp_get == 0)
    		{
			sleep(2);
			printf("udp_get start...\n");
                        system(udp_get);
			loop = 0;
                }
       	}	

       	printf("------------udp_daemon : over------------\n");
	return 0;
}
