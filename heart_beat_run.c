/***********************
 * Filename : heart_beat_run.c
 * Date : 2015-03-05
 * *********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define heart_beat "/ailvgo/system/box/heart_beat"

int main()
{	
        int start_t=0, end_t=0;
        int delay=600;
 
        printf("*****************************************\n");
        printf("heart_beat_run : start\n");
        printf("period : 600 seconds!\n");
        printf("*****************************************\n");
        
        while(1)
	{
	       sleep(delay);
        
               start_t=time(NULL);

               printf("heart_beat : start\n");
               
               system(heart_beat);

               end_t=time(NULL);

               delay = 600-(end_t-start_t);
	}
        return 0;
}
