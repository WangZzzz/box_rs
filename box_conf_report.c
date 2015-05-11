/*************************************************************************     
    > File Name: box_conf_report.c
  > Created Time: Wed Apr 22 21:31:13 2015
  > Function: 
 ************************************************************************/     

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<unistd.h>
#include<sys/types.h>
#include<sqlite3.h>
#include<error.h>
#include<time.h>
#include "ghttp.h"

#define box_db "/ailvgo/www/database/box.db"
#define domain_db "/ailvgo/system/domain/domain.db"
#define sysfile_db "/ailvgo/system/box/sysfile.db"
#define staff_ctl "/ailvgo/system/traffic/staff_ctl"
#define wifi_dbm "/ailvgo/system/traffic/wifi_dbm"
#define wifi_log "/ailvgo/system/traffic/wifi_mac"
# define logfile "/ailvgo/system/log/sys_log"

void parse(char *pMsg);
char *send_http_request(ghttp_request *req, char *uri);
void status(ghttp_request *r, char *desc);
char* get_mac();
char* get_wifi_mac();
char* get_box_info();
char* get_sysfile_info();
char* get_domain_control();
char* get_staff_ctl();
char* get_wifi_dbm();

const char* path1 = "http://www.ailvgobox.com/box_manage/box_conf_report.php?box_ver=2.0&";
const char* path2 = "http://www.ailvgobox.com/box_manage_2/box_conf_report_1.php?box_ver=2.0&";

const char* sysfile_path1 = "http://www.ailvgobox.com/box_manage/box_sysfile_report.php?box_ver=1.0&";
const char* sysfile_path2 = "http://www.ailvgobox.com/box_manage_2/box_sysfile_report_1.php?box_ver=2.0&";

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

int main()
{
	printf("\n********************************\n");
	printf("box_conf_report : start\n");
	printf("********************************\n");

	char *buf;
		
	char url[1024] = {0};
	char *mac = get_mac();
	char *sysfile_info = get_sysfile_info();
	char *box_info = get_box_info();
	char *dbm = get_wifi_dbm();
	char *ctl = get_staff_ctl();
	char *enable = get_domain_control();
	char *wifi_mac =get_wifi_mac();
	ghttp_request *req;
	
	//get url
	printf("----------------send http request-------------\n;");
	sprintf(url, "%smac=%s&%s&%s", sysfile_path2, mac, wifi_mac, sysfile_info);
	printf("url : %s\n",url);

	req = ghttp_request_new();
	buf = send_http_request(req,url);
	printf("http_body : %s\n",buf);

	//release resource
	ghttp_request_destroy(req);

	//get url
	printf("----------------send http request-------------\n;");
	sprintf(url, "%smac=%s&%s&%s&%s&%s&%s", path2, mac, wifi_mac, box_info, enable, ctl, dbm);
	printf("url : %s\n",url);

	req = ghttp_request_new();
	buf = send_http_request(req,url);
	printf("http_body : %s\n",buf);

	//release resource
	ghttp_request_destroy(req);

	free(buf);
	free(mac);
	free(box_info);
	free(sysfile_info);
	free(dbm);
	free(ctl);
	free(enable);
	free(wifi_mac);

	printf("----------box_conf_report : complete----------\n");
}


void status(ghttp_request *r, char *desc)
{
	ghttp_current_status st;
	st = ghttp_get_status(r);
	fprintf(stderr, "%s: %s [%d/%d]\n",
				desc,
				st.proc == ghttp_proc_request ? "request" :
				st.proc == ghttp_proc_response_hdrs ? "response-headers" :
				st.proc == ghttp_proc_response ? "response" : "none",
				st.bytes_read, st.bytes_total);
}

char *send_http_request(ghttp_request *req, char *uri)
{
	#define malloc_size 5120
	ghttp_status req_status;
	unsigned long rec_bytes_total = 0;
	unsigned long buffer_size = 0;
	unsigned long rec_bytes_current = 0;

	char *buffer = (char *)malloc(sizeof(char) * malloc_size);
	if(!buffer)
	{
		printf("malloc space error\n");
		return NULL;
	}
	else
	{
		memset(buffer, 0, malloc_size);
		buffer_size = malloc_size;
	}

	if(ghttp_set_uri(req, uri) < 0)
	{
		printf("ghttp_set_uri\n");
		return NULL;
	}
	if(ghttp_prepare(req) < 0)
	{
		printf("ghttp_prepare\n");
		return NULL;
	}
	if(ghttp_set_type(req, ghttp_type_get) == -1)
	{
		printf("ghttp_set_type\n");
		return NULL;
	}
	if(ghttp_set_sync(req, ghttp_async) < 0)
	{
		printf("ghttp_set_sync\n");
		return NULL;
	}

	do {
		status(req, "conn");
		req_status = ghttp_process(req);

		if(req_status == ghttp_error)
		{
			fprintf(stderr, "ghttp_process: %s\n", ghttp_get_error(req));
			return NULL;
		}
		if(req_status != ghttp_error && ghttp_get_body_len(req) > 0)
		{
			rec_bytes_current = ghttp_get_body_len(req);
			rec_bytes_total += rec_bytes_current;

			while(rec_bytes_total > buffer_size)
			{
				buffer = (char *)realloc(buffer, buffer_size + malloc_size);
				if(!buffer)
				{
					printf("realloc error\n");
					return NULL;
				}
				buffer_size += malloc_size;
			}

			strncat(buffer, ghttp_get_body(req), rec_bytes_current);
			buffer[rec_bytes_total] = '\0';
			ghttp_flush_response_buffer(req);
		}
	} while(req_status == ghttp_not_done);

	ghttp_clean(req);
	return buffer;
}

char* get_mac()
{
	FILE *stream = NULL;
	char buf[50] = {0};
	char *mac = (char*)malloc(sizeof(char)*1024);
	
	printf("-----------get mac address-----------\n");

	if((stream=popen("ifconfig | grep 'HWaddr' | head -1 | awk '{print $5}'", "r")) == NULL)
	{
		printf("popen failed\n");
		free(mac);
		return "";
	}
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), stream);
	buf[17] = '\0';
    pclose(stream);
	strcpy(mac, buf);
	printf("Hw address : %s\n", mac);
	return mac;
}

char* get_box_info()
{
	sqlite3 *db;
	int rc;
	sqlite3_stmt *ppstmt;
	const char *box_id = NULL, *province_id = NULL, *province_name = NULL, *city_id = NULL, 
		  *city_name = NULL, *site_id = NULL, *site_name = NULL, *area_id = NULL,*area_name = NULL, *guide_id = NULL,
		  *guide_name = NULL, *type = NULL, *tel = NULL, *wifi_detect = NULL, *wan_driver = NULL;
	char sql[512] = {0};
	char *buffer = (char*)malloc(sizeof(char)*1024);

	printf("-----------read box.db-----------\n");
	
	rc = sqlite3_open(box_db,&db);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
		free(buffer);
		return "";
	}
	memset(sql,0,sizeof(sql));
	sprintf(sql,"select box_id,province_id,province_name,city_id,city_name,site_id,site_name,area_id,area_name,guide_id,guide_name,type,tel,wifi_detect,wan_driver from box_info");
	rc=sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
	}
	rc=sqlite3_step(ppstmt);
	if(rc==SQLITE_ROW)
	{
		box_id = sqlite3_column_text(ppstmt,0);
		printf("box_id : %s\n", box_id);
		province_id = sqlite3_column_text(ppstmt,1);
		printf("province_id : %s\n", province_id);
		province_name = sqlite3_column_text(ppstmt,2);
		printf("province_name : %s\n", province_name);
		city_id=sqlite3_column_text(ppstmt,3);
		printf("city_id : %s\n", city_id);
		city_name=sqlite3_column_text(ppstmt,4);
		printf("city_name : %s\n", city_name);
		site_id=sqlite3_column_text(ppstmt,5);
		printf("site_id : %s\n", site_id);
		site_name=sqlite3_column_text(ppstmt,6);
		printf("site_name : %s\n", site_name);
		area_id=sqlite3_column_text(ppstmt,7);
		printf("area_id : %s\n", area_id);
		area_name=sqlite3_column_text(ppstmt,8);
		printf("area_name : %s\n", area_name);
		guide_id=sqlite3_column_text(ppstmt,9);
		printf("guide_id : %s\n", guide_id);
		guide_name=sqlite3_column_text(ppstmt,10);
		printf("guide_name: %s\n", guide_name);
		type=sqlite3_column_text(ppstmt,11);
		printf("type : %s\n", type);
		tel=sqlite3_column_text(ppstmt,12);
		printf("tel : %s\n", tel);
		wifi_detect = sqlite3_column_text(ppstmt,13);
		printf("wifi_detect : %s\n", wifi_detect);
		wan_driver = sqlite3_column_text(ppstmt,14);
		printf("wan_driver : %s\n", wan_driver);
	}
	else
	{
		fprintf(stderr,"SQL error:%s \n",sqlite3_errmsg(db));
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
		printf("box.db : N/A\n");
		sprintf(buffer,"box_id=null&province_id=null&province_name=null&city_id=null&city_name=null&site_id=null&site_name=null&area_id=null&area_name=null&guide_id=null&guide_name=null&type=null&tel=null&wifi_detect=null&wan_driver=null");
		return buffer;
	}
	sprintf(buffer,"box_id=%s&province_id=%s&province_name=%s&city_id=%s&city_name=%s&site_id=%s&site_name=%s&area_id=%s&area_name=%s&guide_id=%s&guide_name=%s&type=%s&tel=%s&wifi_detect=%s&wan_driver=%s",box_id,province_id,province_name,city_id,city_name,site_id,site_name,area_id,area_name,guide_id,guide_name,type,tel,wifi_detect,wan_driver);
	sqlite3_finalize(ppstmt);
	sqlite3_close(db);
	return buffer;
}

char* get_domain_control()
{
	sqlite3 *db;
	int rc;
	sqlite3_stmt *ppstmt;
	const char *enable = NULL;
	char sql[512] = {0};
	char *buffer = (char*)malloc(sizeof(char)*1024);

	printf("-----------get domain.db-----------\n");
	
	rc = sqlite3_open(domain_db,&db);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"SQL error : %s \n",sqlite3_errmsg(db));
		free(buffer);
		return "";
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql,"select enable from domain_control");
	rc=sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"SQL error : %s \n",sqlite3_errmsg(db));
	}
	rc=sqlite3_step(ppstmt);
	if(rc==SQLITE_ROW)
	{
		enable = sqlite3_column_text(ppstmt,0);
		printf("enable : %s\n", enable);
	}else{
		fprintf(stderr,"SQL error : %s \n",sqlite3_errmsg(db));
		sqlite3_finalize(ppstmt);
		sqlite3_close(db);
		printf("enable : N/A\n");
		sprintf(buffer,"enable=null");		
		return buffer;
	}
	sprintf(buffer,"enable=%s",enable);
	sqlite3_finalize(ppstmt);
	sqlite3_close(db);
	return buffer;
}

char* get_staff_ctl()
{
	 FILE *fp;
	 int tmp;
	 char *buffer = (char*)malloc(sizeof(char)*1024);

	 printf("-----------get staff_ctl-----------\n");

	 if(access(staff_ctl, W_OK) == 0)
	 {
		 if((fp = fopen(staff_ctl, "r")) == NULL)
		   perror("staff_ctl open error!\n");
		 fscanf(fp,"%d",&tmp);
		 fclose(fp);
		 printf("staff_ctl : %d\n", tmp);
		 sprintf(buffer,"staff_ctl=%d",tmp);
		 return buffer;
	 }
	 else
	 {
		 printf("staff_ctl : N/A\n");
		 sprintf(buffer,"staff_ctl=null");
		 return buffer;
	 }
}

char* get_wifi_dbm()
{
	 FILE *fp;
	 int tmp;
	 char *buffer = (char*)malloc(sizeof(char)*1024);
	 
	 printf("-----------get wifi_dbm-----------\n");

	 if(access(wifi_dbm, W_OK) == 0)
	 {
		 if((fp = fopen(wifi_dbm, "r")) == NULL)
		   perror("wifi_dbm open error!\n");
		 fscanf(fp,"%d",&tmp);
		 fclose(fp);
		 printf("wifi_dbm : %d\n", tmp);
		 sprintf(buffer,"wifi_dbm=%d",tmp);
		 return buffer;
	 }
	 else
	 {
		 printf("wifi_dbm : N/A\n");
		 sprintf(buffer,"wifi_dbm=null");
		 return buffer;
	 }
}
char* get_wifi_mac()
{
	 FILE *fp;
	 char buf[30] = {0};
	 char *buffer = (char*)malloc(sizeof(char)*1024);
	 
	 printf("-----------get wifi_mac-----------\n");

	 if(access(wifi_log, W_OK) == 0)
	 {
		 if((fp = fopen(wifi_log, "r")) == NULL)
		   perror("wifi_log open error!\n");
		 fread(buf, sizeof(char), sizeof(buf), fp);		 
		 fclose(fp);
		 printf("wifi_mac : %s\n", buf);
		 sprintf(buffer,"wifi_mac=%s",buf);
		 return buffer;
	 }
	 else
	 {
		 printf("wifi_mac : N/A\n");
		 sprintf(buffer,"wifi_mac=null");
		 return buffer;
	 }
}

char* get_sysfile_info()
{
	sqlite3 *db;
	int rc;
	sqlite3_stmt *ppstmt;
	const char *box_id = NULL, *filename = NULL, *version = NULL;
	char sql[512] = {0};
	char *buffer = (char*)malloc(sizeof(char)*2048);
	char filelist[500]={0}, verlist[500]={0}, box_id_tmp[10]={0};

	printf("---------get box.db----------\n");

	rc = sqlite3_open(box_db, &db);
	if(rc != SQLITE_OK)
	{
		printf("SQL error : %s\n", sqlite3_errmsg(db));
		free(buffer);
		return "";
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select box_id from box_info");
	rc = sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if(rc != SQLITE_OK)
	{
		printf("SQL error : %s \n", sqlite3_errmsg(db));
	}
	rc = sqlite3_step(ppstmt);
	if(rc==SQLITE_ROW)
	{
		box_id = sqlite3_column_text(ppstmt,0);
		strcpy(box_id_tmp, box_id);
		printf("box_id : %s\n", box_id_tmp);
	}
	sqlite3_finalize(ppstmt);
	sqlite3_close(db);

	printf("----------get sysfile.db--------\n");

	rc = sqlite3_open(sysfile_db, &db);
	if(rc != SQLITE_OK)
	{
		printf("SQL error : %s\n", sqlite3_errmsg(db));
		free(buffer);
		return "";
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select filename,version from sysfile_info");
	rc = sqlite3_prepare(db,sql,-1,&ppstmt,0);
	if(rc != SQLITE_OK)
	{
		printf("SQL error : %s \n", sqlite3_errmsg(db));
	}
	rc = sqlite3_step(ppstmt);
	while(rc == SQLITE_ROW)
	{
		filename = sqlite3_column_text(ppstmt,0);
		printf("filename : %s\n", filename);
		version = sqlite3_column_text(ppstmt,1);
		printf("version : %s\n", version);
		
		strcat(filelist, filename);
		strcat(filelist, "-");
		strcat(verlist, version);
		strcat(verlist, "-");

		rc = sqlite3_step(ppstmt);
	}
	filelist[strlen(filelist)-1] = '\0';
	verlist[strlen(verlist)-1] = '\0';

	sqlite3_finalize(ppstmt);
	sqlite3_close(db);
	
	if(strlen(filelist)<10)
	{
		printf("sysfile.db : null!\n");
		sprintf(buffer, "box_id=%s&filename=null&version=null", box_id_tmp);
		return buffer;
	}
	else
	{
		printf("sysfile.db : ok!\n");
		sprintf(buffer, "box_id=%s&filename=%s&version=%s", box_id_tmp, filelist, verlist);
		return buffer;
	}
}


