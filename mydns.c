/************************************************************************
  > File Name: mydns.c
  > Date : 2015-03-24
  > Function: dns request filter & forward
 ************************************************************************/

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

#define domain_db "/ailvgo/system/domain/domain.db"

#define SERVER_PORT 53
#define CLIENT_PORT 53

#define MAX_SIZE 1024
#define CACHE_SIZE 256

#define CACHE_ON 0

# define logfile "/ailvgo/system/sys_log"
# define resolvfile "/etc/resolv.conf"

void sys_log(char *str)
{
	FILE *fp;
	char content[100]={0};
        
	struct tm *timeinfo;
	time_t servertime = 0;
	time(&servertime);
	timeinfo = localtime(&servertime);

	fp = fopen(logfile, "a");
	sprintf(content, "%s ----- %s\n", asctime(timeinfo), str);
	fputs(content,fp);
	fclose(fp);
}

int main()
{
	printf("\n");
	printf("---------------------------------------\n");
	printf("mydns : start\n");
	printf("---------------------------------------\n");
	
	FILE *fp;
	char nameserver[100];
	char dns_ip[50] = {0};
	
	// get the dns_ip from resolv.conf
    if((fp = fopen(resolvfile, "r")) == NULL)
    {
		perror("resolv_file open error!\n");
        
	}    
   fgets(nameserver,100,fp);

   int i=0, k=0;
   for(i=11; i< strlen(nameserver); i++)
   {     
	   dns_ip[k] = nameserver[i];
	   k++;
   }
   dns_ip[k+1]='\0';

   fclose(fp);

   if(strlen(dns_ip) < 4)
   {
	   printf("dns_ip set to 114.114.114.114\n");
	   strcpy(dns_ip, "114.114.114.114");
   }
  
   printf("upstream dns : %s\n", dns_ip);

    struct sockaddr_in addr_server_rec, addr_server_send, addr_client_rec, addr_client_send;
	int sockfd_server, sockfd_client, len_server_rec, len_client_rec;
	char buffer_server_rec[MAX_SIZE];
	int sin_size, flag = 0;

	char cache_url[CACHE_SIZE][100] = {0}, cache_response[CACHE_SIZE][MAX_SIZE] = {0};
	int cache_len[CACHE_SIZE];
	int m = 0;
	char url[100] = {0};
	int flag_cache;

	int cnt, j, tmp, tmp1;

	// load domain.db into allow_url;
	printf("\n");
	printf("domain.db : load url whitelist......\n");

	char allow_url[200][100] = {0};
 	sqlite3 *db = NULL;
	int rc = 0;
	sqlite3_stmt *ppstmt = NULL;
	char sql_cmd[100] ={0};
        char *errorMsg = NULL;
	const char *url_tmp;
	int allow_num = 0;

	char url_sub[50];
	int url_len = 0;
	int n = 0, countdown = 0, cnt_tmp = 0;
     i=0;
        rc = sqlite3_open(domain_db, &db);

        if(rc == SQLITE_ERROR)
        {
                printf("cannot open domain.db!\n");
                return 0;
        }

        ppstmt = NULL;
        memset(sql_cmd, 0, sizeof(sql_cmd));
        strcpy(sql_cmd, "select * from domain_info");
        sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
        rc = sqlite3_step(ppstmt);
        while(rc == SQLITE_ROW)
        {
                url_tmp = sqlite3_column_text(ppstmt, 0);
                strcpy(allow_url[i],  url_tmp);
				allow_num++;
				i++;
                rc = sqlite3_step(ppstmt);
        }

        sqlite3_finalize(ppstmt);
        sqlite3_close(db);

	printf("\n");
	printf("allow_url : %d, list as below :\n", allow_num);

	for(i=0; i<allow_num; i++)
        	printf("allow_url[%d] : %s\n", i, allow_url[i]);
	
	printf("\n");
	// setup dns server
	if((sockfd_server = socket(AF_INET,SOCK_DGRAM,0))<0)
        {
		perror("socket_server");
                return 0;
	}
	bzero(&addr_server_rec,sizeof(struct sockaddr_in));
	addr_server_rec.sin_family = AF_INET;
	addr_server_rec.sin_addr.s_addr=htonl(INADDR_ANY);
	addr_server_rec.sin_port=htons(SERVER_PORT);
	// setup dns client
	bzero(&addr_client_send,sizeof(struct sockaddr_in));
	addr_client_send.sin_family = AF_INET;
	addr_client_send.sin_addr.s_addr = inet_addr(dns_ip);
	addr_client_send.sin_port = htons(CLIENT_PORT);
    
        struct timeval tv_out;
        tv_out.tv_sec = 3; //wait 3 seconds
        tv_out.tv_usec = 0; 
         
        // bind dns server
	if(bind(sockfd_server,(struct sockaddr *)&addr_server_rec,sizeof(struct sockaddr_in))==-1)
	{
		perror("bind_server");
		return 0;
	}
        printf("bind udp server success!\n");

	// init dns client
	if((sockfd_client = socket(AF_INET,SOCK_DGRAM,0))<0)
        {
		perror("socket_client");
		return 0;
	}
        printf("init udp client success!\n");
        
        setsockopt(sockfd_client,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));

        setsockopt(sockfd_server,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
	
	sin_size=sizeof(struct sockaddr_in);

	while(1)
    {
		memset(buffer_server_rec,0,sizeof(buffer_server_rec));
		memset(url, 0, sizeof(url));
		flag_cache = 0;
		flag = 0;
		len_server_rec=recvfrom(sockfd_server, buffer_server_rec, sizeof(buffer_server_rec), 0, (struct sockaddr *)&addr_server_send, &sin_size);
		
		if(len_server_rec<20)
			continue;
		
		// get url from buffer_server_rec
		cnt=12;
		j=0;

		for(i=0;i<6;i++)
        	{
			tmp=buffer_server_rec[cnt];
			k = cnt+tmp;
			if(tmp!=0)
                	{
				for(cnt = cnt + 1;cnt <= k;)
				{
					url[j]=buffer_server_rec[cnt];
					j++;
					cnt++;
				}
				url[j]='.';
				j++;
			}
        		else
				break;
        	}
		j--;
		url[j]='\0';
        
		if(strlen(url) <= 4)
			continue;
        
	 	printf("----------------new url---------------\n");
		printf("url : %s\n",url);

		//check url in allow_url
		if((strcmp(url, "www.ailvgoserver.com") == 0) || (strcmp(url, "app.ailvgo.com") == 0))
			flag = 2;
        else
		{
			url_len = strlen(url);
            countdown=0;

			for(i = url_len - 1; i > 0; i--)
			{
				if(url[i] == '.')
                {
					n = i + 1;
					for(cnt_tmp = 0; cnt_tmp < countdown; cnt_tmp++)
					{
						url_sub[cnt_tmp] = url[n];
						n++;
					}
					url_sub[cnt_tmp] = '\0';

					for(j=0; j<allow_num; j++)
					{
						if(strcmp(url_sub, allow_url[j]) == 0)
						{
							flag = 1;
							break;
						}
					}
					if(flag == 1)
						break;

					countdown++;
				}
				else
				{
					countdown++;
				}	
			}

			if(flag == 0)
			{
				for(j=0; j<allow_num; j++)
				{
					if(strcmp(url, allow_url[j]) == 0)
					{
						flag = 1;
						break;
					}
				}
			}
		}

		switch(flag)
		{
			case 1 :
			{
				printf("url : allow!\n");
			
				if(CACHE_ON)
				{
					printf("serch url in cache...\n");

					//search url in cache
					for(i=0; i<CACHE_SIZE; i++)
					{
						if(strlen(cache_url[i]) < 4)
						{
							printf("cache search stop @ %d\n", i);
							break;
						}

						if(strcmp(url, cache_url[i]) == 0)
						{
							flag_cache = 1;
							break;
						}
					}

					if(flag_cache == 1)
					{
						printf("url found in cache @ %d\n", i);
						cache_response[i][0] = buffer_server_rec[0];
						cache_response[i][1] = buffer_server_rec[1];
						tmp1 = 0;
						while(tmp1<cache_len[i])
						{
							if((tmp1+=sendto(sockfd_server,&(cache_response[i][tmp1]),(cache_len[i]-tmp1),0,(struct sockaddr *)&addr_server_send,sin_size))< 0)
							{
								perror("forward response to terminal");
								printf("forward response timeout!!!\n");
							}
						}
						printf("dns : forward cache response success!\n");
						continue;
					}

					printf("url not found in cache\n");
				}

				tmp1 = 0;
				while(tmp1<len_server_rec)
				{
					if((tmp1+=sendto(sockfd_client,buffer_server_rec+tmp1,(len_server_rec-tmp1),0,(struct sockaddr *)&addr_client_send,sin_size))< 0)
                    {
                       	perror("forward request to upsream dns");
                       	printf("forward request timeout!!!\n");
						goto timeout_flag;
                    }
				}
                printf("dns : forward request success!\n");

                memset(cache_response[m],0,sizeof(cache_response[m]));
				if((len_client_rec=recvfrom(sockfd_client,&(cache_response[m][0]),MAX_SIZE,0,NULL,NULL))<0)
				{
					perror("receive response from dns");
                    printf("receive response timeout!!!\n");
					goto timeout_flag;
				}
                printf("dns : receive response success!\n");
			
				strcpy(cache_url[m], url);
				cache_len[m]=len_client_rec;

				printf("save response in cache @ %d\n", m);
				tmp1 = 0;
				while(tmp1<len_client_rec)
				{
                	if((tmp1+=sendto(sockfd_server,&(cache_response[m][tmp1]),(len_client_rec-tmp1),0,(struct sockaddr *)&addr_server_send,sin_size))< 0)
					{
						perror("forward response to terminal");
                        printf("forward response timeout!!!\n");
			        	goto timeout_flag;
					}
		    	}
                printf("dns : forward response success!\n");
			
				m++;
				if(m==CACHE_SIZE)
					m = 0;
				break;
            }
			case 0 :
			{
				printf("url : deny!\n");
			
				timeout_flag:
                    	
		    	buffer_server_rec[2]='\x81';
                buffer_server_rec[3]='\x80';
                buffer_server_rec[7]='\x01';
                buffer_server_rec[len_server_rec++]='\xc0';
            	buffer_server_rec[len_server_rec++]='\x0c';
           	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x01';
           	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x01';
           	    buffer_server_rec[len_server_rec++]='\x00';
          	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\xa0';
          	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x04';
           	    buffer_server_rec[len_server_rec++]='\xc0';
             	buffer_server_rec[len_server_rec++]='\xa8';
           	    buffer_server_rec[len_server_rec++]='\x64';
           	    buffer_server_rec[len_server_rec++]='\x01';
           	    buffer_server_rec[len_server_rec]='\0';

                tmp1 = 0;
				while(tmp1<len_server_rec)
				{
					if((tmp1+=sendto(sockfd_server,buffer_server_rec+tmp1,(len_server_rec-tmp1),0,(struct sockaddr *)&addr_server_send,sin_size))< 0)
                	{
                       		perror("response 192.168.100.1 to terminal");
                    		printf("response 192.168.100.1 timeout!!!\n ");
               		}
				}
				break;
            }
			case 2 :
			{
				printf("url : ailvgoserver | ailvgo is found!!\n");
			
		    	buffer_server_rec[2]='\x81';
                buffer_server_rec[3]='\x80';
                buffer_server_rec[7]='\x01';
                buffer_server_rec[len_server_rec++]='\xc0';
            	buffer_server_rec[len_server_rec++]='\x0c';
           	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x01';
           	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x01';
           	    buffer_server_rec[len_server_rec++]='\x00';
          	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\xa0';
          	    buffer_server_rec[len_server_rec++]='\x00';
           	    buffer_server_rec[len_server_rec++]='\x04';
           	    buffer_server_rec[len_server_rec++]='\xc0';
             	buffer_server_rec[len_server_rec++]='\xa8';
           	    buffer_server_rec[len_server_rec++]='\x64';
           	    buffer_server_rec[len_server_rec++]='\x02';
           	    buffer_server_rec[len_server_rec]='\0';

                tmp1 = 0;
				while(tmp1<len_server_rec)
				{
					if((tmp1+=sendto(sockfd_server,buffer_server_rec+tmp1,(len_server_rec-tmp1),0,(struct sockaddr *)&addr_server_send,sin_size))< 0)
                	{
                       		perror("response 192.168.100.2 to terminal");
                    		printf("response 192.168.100.2 timeout!!!\n ");
               		}
				}
				break;
			}
			default :
				printf("error dns request!\n");
		}
	}
	sys_log("mydns : error!");
    return 0;
}
