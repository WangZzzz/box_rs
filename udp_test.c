/***********************
* Filename : udp_test.c
* Date : 2015-03-06
************************/

#include <sys/types.h>
#include <sys/socket.h>  
#include <sys/ioctl.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <sqlite3.h>
#include <error.h>
#include <time.h>
#include <stdlib.h>  
#include <string.h>  
#include <stdio.h> 
#include <math.h>

#define PORT 10608 // port with wifi-board  

int get_dbm(int m)
{
	int n = m-1;
	int dbm = 0;
	int a[16];
	int i;
	int j;
	for(i=0;i!=16;++i)
	{
		a[16-1-i] = n % 2;
		n /= 2;
	}
	for(i=15;i!=0;i--)
	{
		a[i] = !a[i];
	}
	for(i=15,j=0;i!=0;i--,j++)
	{
		dbm += a[i]*pow(2,j);
	}
	return -dbm;
}

main(int argc, char** argv)
{
	int sockfd = 0, len = 0;  
	int t = 0, t_last = 0, interval = 0, interval_max = 0;
	struct sockaddr_in addr;  
	int addr_len = sizeof(struct sockaddr_in);  
	char buffer_rec[128] = {0}, mac[18] = {0}, ap_mac[18] = {0}, buffer_send[128] = {0};
	int j = 0, k = 0;
	int m, n;
	char signal[5] = {0};
    int signal_t = 0, dbm =0;
	int rc = 0;
    
	if((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{  
		perror("setup socket failure : ");
		return;  
	}

	bzero (&addr, sizeof(addr));  
	addr.sin_family=AF_INET;  
	addr.sin_addr.s_addr=htonl(INADDR_ANY) ;  
	addr.sin_port=htons(PORT);  

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind failure : ");
		return;  
	}

	printf("------------------udp_test : start-----------------\n");
        
	while(1)
	{	
		memset(mac, 0, sizeof(mac));
		memset(signal, 0, sizeof(signal));
		memset(ap_mac, 0, sizeof(ap_mac));
		memset(buffer_rec, 0, sizeof(buffer_rec));

		len = recvfrom(sockfd, buffer_rec, sizeof(buffer_rec), 0, (struct sockaddr *)&addr, &addr_len);
	
		if(len >= 50)
		{	
			sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", (0xff & buffer_rec[15]), (0xff & buffer_rec[16]), (0xff & buffer_rec[17]), (0xff & buffer_rec[18]), (0xff & buffer_rec[19]), (0xff & buffer_rec[20]));
			sprintf(signal, "%02x%02x", (0xff & buffer_rec[26]), (0xff & buffer_rec[27]));
			signal_t = (0xff&buffer_rec[26])*(0x100)+(0xff&buffer_rec[27]);
			dbm = get_dbm(signal_t);
			t=time(NULL)+8*3600;
				
			if(strcmp(mac, *(argv+1)) == 0)
			{
				printf("mac : %s  || signal : %s || dbm : %d || time : %d\n", mac, signal, dbm, t);
				
				if(t_last == 0)
					t_last = t;
				else
				{
					interval = t - t_last;
					t_last = t;
				}

				if(interval > interval_max)
				  interval_max = interval;

				printf("interval_max : %d \n", interval_max);
			}
		}
		
		if((len>20)&&(len<30)) 
		{	
			printf("receive heartbeat!\n");
			sprintf(ap_mac, "%02x:%02x:%02x:%02x:%02x:%02x", (0xff & buffer_rec[2]), (0xff & buffer_rec[3]), (0xff & buffer_rec[4]), (0xff & buffer_rec[5]), (0xff & buffer_rec[6]), (0xff & buffer_rec[7]));
			
			sprintf(buffer_send,"\x65\x82%c%c%c%c%c%c\x01\x01\xB1\x00\x00\x04\x00\x00\x00\x00",buffer_rec[2],buffer_rec[3],buffer_rec[4],buffer_rec[5],buffer_rec[6],buffer_rec[7]);
	
			if(sendto(sockfd, buffer_send, 18, 0, (struct sockaddr *)&addr, addr_len)==-1)  
			{
				perror("response heartbeat failure : ");
			}
		}
	} 
	close(sockfd);
}
