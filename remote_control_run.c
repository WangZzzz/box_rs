/***********************
 * Filename : remote_control_run.c
 * Date : 2015-03-05
 * ********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define remote_control "/ailvgo/system/box/remote_control"

int main()
{
        int start_t=0, end_t=0;
        int delay=3600;
        
        printf("****************************************\n");
        printf("remote_control_run : start\n");
        printf("period : 3600 seconds!\n");
        printf("****************************************\n");
   
	while(1)
	{
		sleep(delay);

                printf("remote control : start\n");
                
                start_t = time(NULL);
                
                system(remote_control);
  
                end_t = time(NULL);

		delay = 3600-(end_t-start_t);
	}

        return 0;
}
