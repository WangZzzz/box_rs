/***********************
 * Filename : heart_beat_run.c
 * Date : 2015-03-05
 * *********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define heart_beat "/ailvgo/system/box/heart_beat &"

int main()
{	
    int delay=600;
 
    printf("*****************************************\n");
    printf("heart_beat_run : start\n");
    printf("period : 600 seconds!\n");
    printf("*****************************************\n");
        
    while(1)
	{
		sleep(delay-30);
        
		printf("heart_beat : start\n");
               
        system(heart_beat);

		sleep(30);

        system("killall heart_beat");
	}
}
