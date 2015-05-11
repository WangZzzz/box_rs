/***********************
* Filename : udp_get.c
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

#define traffic_db "/ailvgo/system/traffic/traffic.db"
#define traffic_total_db "/ailvgo/system/traffic/traffic_total.db"
#define wifi_dbm "/ailvgo/system/traffic/wifi_dbm"
#define staff_db "/ailvgo/system/traffic/staff.db"
#define staff_ctl "/ailvgo/system/traffic/staff_ctl"
 
#define PORT 10608 // port with wifi-board  
#define CNT 15000   // buffer_mac size
#define SEC 43200  // 3600*12
#define BORDER 10000  // traffic backup threshold
#define NUM 600 // loop_mac size in 60 seconds
#define STAFF_SIZE 100

# define logfile "/ailvgo/system/sys_log"

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

int get_time(int n);
int traffic_mv();
void traffic_collet(char *mac_get,int time_get, sqlite3 *db);
void traffic_total_collect(char *mac_get,int time_get, sqlite3 *db);
void ap_socket();
void error_log(char *str);
void record_staff();

const char *db_back="/ailvgo/system/traffic/traffic_";

int main()
{
	int rc = 0;
	sqlite3 *db=NULL;
	char *errmsg;
	
	printf("----------check traffic.db & traffic_total.db---------\n");
        if(access(traffic_db, W_OK) == -1)
        {
        	printf("traffic.db not exist!\n");
        	sys_log("traffic.db not exist!");
        	return 0;
 	}
	else
		printf("traffic.db : ok!\n");
	
        if(access(traffic_total_db, W_OK) == -1)
        {
        	printf("traffic_total.db not exist!\n");
        	sys_log("traffic_total.db not exist!");
        	return 0;
 	}
	else
		printf("traffic_total.db : ok!\n");

	printf("-----------delete & vacuum traffic_total.db------------\n");
	rc=sqlite3_open(traffic_total_db,&db);
	if(rc!=SQLITE_OK)
	{
		fprintf(stderr,"Cannot open traffic_total.db:%s\n",sqlite3_errmsg(db));
	}

	rc = sqlite3_exec(db,"delete from traffic_total",NULL,NULL,&errmsg);
	if(SQLITE_OK == rc)
		printf("delete traffic_total.db success!\n");
	else if(rc!=SQLITE_OK)
		printf("delete traffic_total.db error:%s\n",errmsg);
	
	rc = sqlite3_exec(db,"vacuum traffic_total",NULL,NULL,&errmsg);
	if(SQLITE_OK == rc)
		printf("vacuum traffic_total.db success!\n");
	else if(rc!=SQLITE_OK)
		printf("vacuum traffic_total.db error:%s\n",errmsg);

	sqlite3_close(db);

	printf("----------------backup traffic.db---------------------\n");

	if(traffic_mv()==0)
        	printf("traffic.db mv failure!\n");
	else
		printf("traffic.db mv success!\n");

	ap_socket();

	printf("udp_get over!\n");

	sys_log("udp_get error");
	
	return 0;
}

void traffic_total_collect(char *mac_get,int time_get, sqlite3 *db)
{
	int rc = 0;
        char sql[250]={0};
	char *errmsg; 
	sqlite3_stmt *ppstmt=NULL;

	memset(sql,0,sizeof(sql));
	sprintf(sql,"select time from traffic_total where mobile='%s'",mac_get);
	sqlite3_prepare(db,sql,-1,&ppstmt,0);
	rc = sqlite3_step(ppstmt);
	if(rc!=SQLITE_ROW)
	{
		memset(sql,0,sizeof(sql));
		sprintf(sql,"insert into traffic_total (mobile,time) values('%s','%ld')",mac_get,time_get);
		rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
		if(SQLITE_OK!=rc)
		{
			printf("insert traffic_total.db error : %s\n",errmsg);
			return;
		}
	}
	if(rc==SQLITE_ROW)
	{
		memset(sql,0,sizeof(sql));
		sprintf(sql,"update traffic_total set time='%ld' where mobile='%s'",time_get,mac_get);
		rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
		if(SQLITE_OK!=rc)
		{
			printf("update traffic_total.db error : %s\n",errmsg);
			return;
		}
	}
	sqlite3_finalize(ppstmt);
}

void traffic_collect(char *mac_get,int time_get, sqlite3 *db)
{
    int rc = 0;
	char sql[250]={0};
	char *errmsg;
	int time_data=0;
	sqlite3_stmt *ppstmt=NULL;

	memset(sql,0,sizeof(sql));
	sprintf(sql,"select time from traffic_info where mobile='%s' order by time desc limit 1",mac_get);
	sqlite3_prepare(db,sql,-1,&ppstmt,0);
	rc = sqlite3_step(ppstmt);
	if(rc!=SQLITE_ROW)
	{
		memset(sql,0,sizeof(sql));
		sprintf(sql,"insert into traffic_info (mobile,time) values('%s','%ld')",mac_get,time_get);
		rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
		if(SQLITE_OK!=rc)
		{
			printf("insert traffic.db error : %s\n",errmsg);
			return;
		}
	}
	if(rc==SQLITE_ROW)
	{
		time_data=sqlite3_column_int(ppstmt,0);
		if(time_get - time_data > SEC)
		{
			memset(sql,0,sizeof(sql));
			sprintf(sql,"insert into traffic_info (mobile,time) values('%s','%ld')",mac_get,time_get);
			rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
			if(SQLITE_OK!=rc)
			{
				printf("insert traffic.db error : %s\n",errmsg);
				return;
			}
		}
	}
	sqlite3_finalize(ppstmt);
}


void ap_socket()
{
	int sockfd = 0, len = 0;  
	int t;
	struct sockaddr_in addr;  
	int addr_len = sizeof(struct sockaddr_in);  
	char buffer_rec[128] = {0}, mac[18] = {0}, ap_mac[18] = {0}, buffer_send[128] = {0};
	int j = 0, k = 0, flag_new, flag_total;
	char buffer_mac[CNT][18] = {0};
	int buffer_t[CNT];
	int loop_t1 = 0;
	char loop_mac[NUM][18] = {0};
	int loop_t[NUM];
	int m, n;
	char signal[5] = {0};
        int signal_t = 0, dbm =0;
	int rc = 0;
	char sql[250] = {0}, staff_mac[STAFF_SIZE][18] = {0};
	const char *tmp_mobile = NULL;
	int num = 0,staff_flag = 0;
	sqlite3 *db=NULL;
	sqlite3_stmt *ppstmt = NULL;

	// get theshold dbm from wifi_dbm
	printf("--------------check wifi_dbm------------\n");
	int threshold = -95;
	FILE *fp;
        if(access(wifi_dbm, W_OK) == 0)
	{
        	if((fp = fopen(wifi_dbm, "r")) == NULL)
             		perror("wifi_dbm open error!\n");
		
		fscanf(fp,"%d", &threshold);
		fclose(fp);
		printf("wifi_dbm : %d\n", threshold);
	}
	else
		printf("wifi_dbm not found!\n");

	//check staff_mac is on/off
	printf("----------check staff_mac on/off---------\n");
	int staff=0;
	if(access(staff_ctl, W_OK) == 0)
	{
        	if((fp = fopen(staff_ctl, "r")) == NULL)
             		perror("staff_ctl open error!\n");
		
		fscanf(fp,"%d", &staff);
		fclose(fp);
		printf("staff_ctl : %d\n", staff);
	}
	else
		printf("staff_ctrl not found!\n");
	
	if(staff == 1)
	{
		// get staff mac from traffic.db
		printf("staff mac control : on\n");
		record_staff();
		rc = sqlite3_open(staff_db, &db);
		if(rc != SQLITE_OK)
		{
			printf("open staff.db error!!!\n");
		}
		memset(sql,0,sizeof(sql));
		sprintf(sql,"select mobile from staff_info order by time desc limit 100");
		rc = sqlite3_prepare(db,sql,-1,&ppstmt,0);
		if(rc != SQLITE_OK)
		{
			fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
		}
		rc = sqlite3_step(ppstmt);
		while(rc == SQLITE_ROW)
		{
			tmp_mobile = sqlite3_column_text(ppstmt,0);
			strcpy(staff_mac[num],tmp_mobile);
			num++;
			rc = sqlite3_step(ppstmt);
		}
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
	}
	else
		printf("staff mac control : off\n");
	
	// setup udp server
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

	memset(buffer_mac, 0, sizeof(buffer_mac));
	memset(buffer_t, 0, sizeof(buffer_t));

	printf("------------------udp_get : start-----------------\n");
        
	while(1)
	{	
		loop_t1 = time(NULL);

		printf("********** new loop | loop_t1 : %d ************\n", loop_t1);

		m = 0;

		while(1)
		{
			memset(mac, 0, sizeof(mac));
			memset(signal, 0, sizeof(signal));
			memset(ap_mac, 0, sizeof(ap_mac));
			memset(buffer_rec, 0, sizeof(buffer_rec));

	        	flag_new = 0, flag_total = 0;

			len = recvfrom(sockfd, buffer_rec, sizeof(buffer_rec), 0, (struct sockaddr *)&addr, &addr_len);
	
			if(len >= 50)
			{	
				staff_flag = 0;
				sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", (0xff & buffer_rec[15]), (0xff & buffer_rec[16]), (0xff & buffer_rec[17]), (0xff & buffer_rec[18]), (0xff & buffer_rec[19]), (0xff & buffer_rec[20]));
				sprintf(signal, "%02x%02x", (0xff & buffer_rec[26]), (0xff & buffer_rec[27]));
				signal_t = (0xff&buffer_rec[26])*(0x100)+(0xff&buffer_rec[27]);
				dbm = get_dbm(signal_t);
				t=time(NULL);
				printf("mac : %s  || signal : %s || dbm : %d || time : %d\n", mac, signal, dbm, t);
				
				// check signal beyond/lower threshold
				if((dbm < threshold) && (dbm > -100))
				{
					printf("mac : invalid, signal less than %ddbm, skip!\n", threshold);
					continue;
				}
				else
					printf("mac : valid, signal greater that %ddbm!\n", threshold);

				//check staff mac
				if(staff == 1)
				{
					for(num = 0;num<STAFF_SIZE;num++)
					{
						if(strlen(staff_mac[num])<4)
							break;

						if(strcmp(staff_mac[num],mac)==0)
						{
							staff_flag = 1;
							break;
						}
					}
				}
				
				if(staff_flag == 1)
				{
					printf("mac : staff, skip!\n");
					continue;
				}
				
				//search mac in buffer
				for (k = 0; k < CNT; k++)
				{
					if(strlen(buffer_mac[k]) < 4)
					{	
						printf("buffer search stop @ %d\n", k);
						break;
					}
				
					if(strcmp(buffer_mac[k], mac) == 0)
					{
						flag_new = 1;
						printf("mac : exist in buffer!\n");
						if((t-buffer_t[k]) >= 180)
							flag_total = 1;
						break;
					}
				}	
	
				if(flag_new == 0)
				{
					strcpy(buffer_mac[j], mac);
					buffer_t[j] = t;
					strcpy(loop_mac[m], mac);
					loop_t[m] = t;
					printf("mac : new | save in buffer_mac[%d], loop_mac[%d]\n", j, m);
					j++;
					m++;
					if(j == CNT)
						j=0;
				}
			
				if(flag_total == 1)
				{
					printf("mac : buffer time update!\n");
					buffer_t[k] = t;
					printf("mac : loop time update | save in loop_mac[%d]\n", m);
					strcpy(loop_mac[m], mac);
					loop_t[m] = t;
					m++;
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
		
			if(m == NUM)
			{
				printf("~~~~~~~~~~~~~~~loop_mac full~~~~~~~~~~~~~~~\n");
				break;
			}
	
			if(t-loop_t1 > 60)
			{
				printf("~~~~~~~~~~~~~~~~60s timeup~~~~~~~~~~~~~~~~~~\n");
				break;
			}
		}

		// saved loop_mac into traffic.db & traffic_total.db
		if(m == 0)
		{
			printf("loop_mac is null!\n");
			continue;
		}
		else
		{	
			//insert into traffic.db and traffic_total.db

			printf("saved loop_mac into traffic.db | %d\n", m);

			rc = sqlite3_open(traffic_db,&db);
			if(rc!= SQLITE_OK)
			{
				printf("open traffic.db error!!!!!!\n");
				return;
			}

			for(n=0	; n<m;	n++)
				traffic_collect(loop_mac[n], loop_t[n], db);

			sqlite3_close(db);

			printf("saved loop_mac into traffic_total.db | %d\n", m);

			rc = sqlite3_open(traffic_total_db,&db);
			if(rc!= SQLITE_OK)
			{
				printf("open traffic_total.db error!!!!!!\n");
				return;
			}
		
			for(n=0	; n<m;	n++)
				traffic_total_collect(loop_mac[n], loop_t[n], db);

			sqlite3_close(db);
		}
	} 
	close(sockfd);
        sys_log("udp_get error : ap_socket");
}

int get_time(int n)
{
	struct tm *t; 
	time_t tt;    
	time(&tt);
	t=localtime(&tt);
	switch(n)
	{   
		case 1:
			return t->tm_year+1900;
			break;
		case 2:
			return t->tm_mon+1;
			break;
		case 3:
			return t->tm_mday;
			break;
		case 4:
			return t->tm_hour;
			break;
		case 5:
			return t->tm_min;
			break;
		default:
			return 1;
	}
}

int traffic_mv()
{
	sqlite3 *db;
	char sql[256] = {0}, db_time[100] = {0}, sys[256] = {0};
	int rc = 0;
	long data = 0, reserve = 0;
	char *errmsg;
	sqlite3_stmt *ppstmt = NULL;
	
	rc=sqlite3_open(traffic_db,&db);
	if(rc!=SQLITE_OK)
	{
		printf("open traffic.db failure!\n");
		return 0;
	}

	memset(sql,0,sizeof(sql));
	sprintf(sql,"select count(*) from traffic_info");
	sqlite3_prepare(db,sql,-1,&ppstmt,0);
	rc = sqlite3_step(ppstmt);
	if(rc==SQLITE_ROW)
	{
		data=sqlite3_column_int(ppstmt,0);
		printf("number in traffic.db : %ld\n",data);
	}
	sqlite3_finalize(ppstmt);
	
	if(data<BORDER)
		printf("backup traffic.db is unnecessary!\n");
	else
	{
		printf("backup traffic.db ...\n");
		sprintf(db_time,"%4d%02d%02d%02d%02d",get_time(1),get_time(2),get_time(3),get_time(4),get_time(5));
		sprintf(sys,"cp -rf /ailvgo/system/traffic/traffic.db %s%s.db",db_back,db_time);
		printf("sys : %s\n",sys);
		system(sys);
		sleep(3);

		reserve = data - 500;

		memset(sql,0,sizeof(sql));
		sprintf(sql,"delete from traffic_info where time in (select time from traffic_info order by time asc limit 0, %ld)", reserve);
		printf("sql : %s\n", sql);
		rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
		if(SQLITE_OK==rc)
		{
			printf("delete traffic.db success!\n");
		}
		else if(rc!=SQLITE_OK)
		{
			printf("delete traffic.db error:%s\n",errmsg);
			sqlite3_close(db);
			return 0;
		}

		memset(sql,0,sizeof(sql));
		sprintf(sql,"vacuum traffic_info");
		rc = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
		if(SQLITE_OK==rc)
		{
			printf("vacuum traffic.db success!\n");
		}
		else if(rc!=SQLITE_OK)
		{
			printf("vacuum traffic.db error:%s\n",errmsg);
			sqlite3_close(db);
			return 0;
		}
	}
	sqlite3_close(db);
        return 1;
}

void record_staff()
{
	int rc1 = 0 , rc2 = 0;
	char *errmsg;
	const char *staff_mobile = NULL;
	char sql[250] = {0};
	sqlite3 *db1,*db2;
	sqlite3_stmt *ppstmt1 = NULL,*ppstmt2 = NULL;

	printf("------------check staff mac in traffic.db------------\n");
	rc1 = sqlite3_open(traffic_db, &db1);
	if(rc1 != SQLITE_OK)
	{
		printf("open traffic.db error!\n");
		sqlite3_close(db1);
		return;
	}
	rc2 = sqlite3_open(staff_db, &db2);
	if(rc2 != SQLITE_OK)
	{
		printf("open staff.db erro!\n");
		sqlite3_close(db2);
		return;
	}

	memset(sql,0,sizeof(sql));
	sprintf(sql,"select distinct mobile from traffic_info where mobile in (select mobile from traffic_info group by mobile having count(mobile) > 4)");
	
	rc1 = sqlite3_prepare(db1,sql,-1,&ppstmt1,0);
	if(rc1 != SQLITE_OK)
	{
		fprintf(stderr,"SQL error:%s\n",sqlite3_errmsg(db1));
		return;
	}
	rc1 = sqlite3_step(ppstmt1);

	if(rc1 != SQLITE_ROW)
	{
		printf("no staff mac found in traffic.db!\n");
		sqlite3_finalize(ppstmt1);
		sqlite3_close(db1);
		sqlite3_close(db2);
		return;
	}

	while(rc1 == SQLITE_ROW)
	{
		staff_mobile = sqlite3_column_text(ppstmt1,0);
		printf("staff_mac : %s\n", staff_mobile);
		memset(sql,0,sizeof(sql));
		sprintf(sql,"select * from staff_info where mobile ='%s'", staff_mobile);
		rc2 = sqlite3_prepare(db2,sql,-1,&ppstmt2,0);
		if(rc2 != SQLITE_OK)
	    	{
			fprintf(stderr,"SQL error:%s\n",sqlite3_errmsg(db2));
		}
		rc2 = sqlite3_step(ppstmt2);
		if(rc2 == SQLITE_ROW)
		{
			printf("staff_mac : exist in staff.db, update time\n");
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "update staff_info set time='%d' where mobile='%s'", time(NULL), staff_mobile);
		}
		else
		{
			printf("staff_mac : new, insert into staff.db!\n");
			memset(sql,0,sizeof(sql));
			sprintf(sql,"insert into staff_info (mobile, time) values('%s', %d)",staff_mobile, time(NULL));
		}
		rc2 = sqlite3_exec(db2,sql,NULL,NULL,&errmsg);
		if(rc2 != SQLITE_OK)
		{
			printf("insert/update staff.db error:%s\n",errmsg);
		}
		rc1 = sqlite3_step(ppstmt1);
	}
	sqlite3_finalize(ppstmt1);
	sqlite3_finalize(ppstmt2);
	sqlite3_close(db1);
	sqlite3_close(db2);
}
