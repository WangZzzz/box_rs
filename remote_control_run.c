/***********************
 * Filename : remote_control_run.c
 * Date : 2015-03-05
 * ********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define remote_control "/ailvgo/system/box/remote_control &"

int main()
{
    int delay=3600;
        
    printf("****************************************\n");
    printf("remote_control_run : start\n");
    printf("period : 3600 seconds!\n");
    printf("****************************************\n");
   
	while(1)
	{
		sleep(delay);

        printf("remote control : start\n");
				
        system(remote_control);
  
        sleep(30);

		system("killall remote_control");
	}
}
