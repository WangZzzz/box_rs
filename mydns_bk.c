/************************************************************************
  > File Name: mydns.c
  > Date : 2015-02-12
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

#define domain_db "/tmp/usbmounts/sda1/www/cgi-bin/domain/domain.db"

#define SERVER_PORT 53
#define CLIENT_PORT 53
#define MAX_SIZE 256

# define logfile "/tmp/usbmounts/sda1/www/cgi-bin/box/sys_log"

void sys_log(char *str)
{
	FILE *fp;
	char content[100]={0};
        
	struct tm *timeinfo;
	time_t boxtime = 0;
	time(&boxtime);
	timeinfo = localtime(&boxtime);

	fp = fopen(logfile, "a");
	sprintf(content, "%s ----- %s\n", asctime(timeinfo), str);
	fputs(content,fp);
	fclose(fp);
}

int buf_format(char *buffer_rec);
int url_judge(char *input);
int sql_select(char *domain);

int main()
{
	FILE *fp;
        char nameserver[100];
        char dns_ip[50] = {0};

        if((fp = fopen("/usr/local/etc/resolv.conf", "r")) == NULL)
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

        printf("dns_ip : %s\n", dns_ip);

        fclose(fp);

        if(strcmp(dns_ip,"0") == 0)
        {
             printf("dns_ip set to 114.114.114.114\n");
             strcpy(dns_ip, "114.114.114.114");
        }

        struct sockaddr_in addr_server_rec, addr_server_send, addr_client_rec, addr_client_send;
	int sockfd_server, sockfd_client, len_server_rec, len_client_rec;
	char buffer_server_rec[MAX_SIZE], buffer_client_rec[MAX_SIZE];
	int sin_size, flag = 0;
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
		len_server_rec=recvfrom(sockfd_server, buffer_server_rec, sizeof(buffer_server_rec), 0, (struct sockaddr *)&addr_server_send, &sin_size);
		if(len_server_rec<20)
			continue;
		
		// process buffer
		flag=buf_format(buffer_server_rec);

                if(flag == 1)
                {
			printf("dns : allow!\n");
                   
                   	if((sendto(sockfd_client,buffer_server_rec,len_server_rec,0,(struct sockaddr *)&addr_client_send,sin_size))< 0)
                    	{
                        	perror("forward request to dns");
                        	printf("forward request timeout!!!\n");
				goto timeout_flag;
                    	}
                	printf("dns : forward request success!\n");

                	memset(buffer_client_rec,0,sizeof(buffer_client_rec));
			if((len_client_rec=recvfrom(sockfd_client,buffer_client_rec,MAX_SIZE,0,NULL,NULL))<0)
			{
				perror("receive response from dns");
                        	printf("receive response timeout!!!\n");
				goto timeout_flag;
			}
                    	printf("dns : receive response success!\n");

                 	if((sendto(sockfd_server,buffer_client_rec,(len_client_rec),0,(struct sockaddr *)&addr_server_send,sin_size))< 0)
		   	{
				perror("forward response to terminal");
                                printf("forward response timeout!!!\n");
			        goto timeout_flag;
		    	}
                    	printf("dns : forward response success!\n");
                 }
                 
		 if(flag == 0)
		 {
                 	timeout_flag:
                    	
			printf("dns : deny!\n");
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

			printf("buffer_server_rec ready!\n");
            
                   	if((sendto(sockfd_server,buffer_server_rec,len_server_rec,0,(struct sockaddr *)&addr_server_send,sin_size))< 0)
                	{
                         	perror("response 192.168.100.1 to client");
                    		printf("response 192.168.100.1 timeout!!!\n ");
               		}
			else
				printf("response 192.168.100.1 success!\n");
            	  }
        }
	sys_log("mydns : error!");
        return 0;
}

int buf_format(char *buffer_rec)
{
	int cnt,i,j,k,tmp;
	char url[100];
	cnt=12;
	j=0;

	for(i=0;i<6;i++)
        {
		tmp=buffer_rec[cnt];
		k = cnt+tmp;
		if(tmp!=0)
                {
			for(cnt = cnt + 1;cnt <= k;)
			{
				url[j]=buffer_rec[cnt];
				j++;
				cnt++;
			}
			url[j]='.';
			j++;
		}
        	else
        	{
			break;
	
		}
        }
	j--;

	url[j]='\0';
        printf("----------------new url---------------\n");
	printf("url : %s\n",url);

        if(strlen(url) > 4)
        {
		if(url_judge(url) == 1)
		  	 return 1;
           	else
                  	 return 0;
	}
        else
		return 2;
}

int url_judge(char *input)
{
	char output[50];
	int len = strlen(input);
        int i, countdown = 0, cnt_tmp = 0, n;

	for(i = len - 1; i > 0; i--)
	{
		if(input[i] == '.')
                {
			n = i + 1;
			for(cnt_tmp = 0; cnt_tmp < countdown; cnt_tmp++)
			{
				output[cnt_tmp] = input[n];
				n++;
			}
			output[cnt_tmp] = '\0';
			if(sql_select(output)==1)
			{
				return 1;
			}
			countdown++;
		}
		else
		{
			countdown++;
		}
	}

	if(sql_select(input)==1)
	{
		return 1;
	}
	return 0;
}

int sql_select(char *domain)
{
	sqlite3 *db;
	sqlite3_stmt *ppstmt=NULL;
	int rc;
	char sql[256]={0};

	rc = sqlite3_open(domain_db,&db);
	if(rc == SQLITE_ERROR)
	{
		printf("open database failed!\n");
		sqlite3_close(db);
		return 0;
	}

	memset(sql,0,sizeof(sql));
	sprintf(sql,"select * from domain_info where domain = '%s'",domain);
	rc = sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
		sqlite3_close(db);
		return 0;
	}
	rc = sqlite3_step(ppstmt);
	if(rc == SQLITE_ROW)
	{
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
		return 1;
	}
	else
	{
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
		return 0;
	}
}
