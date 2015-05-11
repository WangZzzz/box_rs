/******************************
 *Filname : sysfile_update.c
 *Date : 2015-03-05
 * *****************************/


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

# define sysfile_db "/ailvgo/system/box/sysfile.db"

void bail(char *s)
{
    fputs(s, stderr);
    fputc('\n', stderr);
}

int main()
{
	sqlite3 *db = NULL;
	sqlite3_stmt *ppstmt = NULL;
	int rc = 0;
	char sql_cmd[100] = {0};
	char sh_cmd[200] = {0};
	char *errorMsg = NULL;
	const char *filename =NULL, *filepath = NULL;
	char filename_all[100] = {0};
	int delete_flag = 0;

    printf("**********************************\n");
	printf("sysfile_update : start\n");
    printf("**********************************\n");
        
	/*************************************************/

    //sysfile check
    printf("check sysfile in sysfile_update | sysfile.db\n");

    rc=sqlite3_open(sysfile_db, &db);
    if(rc == SQLITE_ERROR)
    {
		bail("open sysfile.db failed");
    }
    ppstmt=NULL;
    memset(sql_cmd, 0, sizeof(sql_cmd));
    strcpy(sql_cmd, "select filename, filepath from sysfile_update;");
    printf("sql_cmd : %s\n", sql_cmd);
    sqlite3_prepare(db, sql_cmd, -1, &ppstmt, 0);
    while(sqlite3_step(ppstmt) == SQLITE_ROW)
    {
		filename=sqlite3_column_text(ppstmt, 0);
        printf("filename : %s\n", filename);
        filepath=sqlite3_column_text(ppstmt, 1);
        printf("filepath : %s\n", filepath);

		delete_flag = 1;
		
		memset(filename_all, 0, sizeof(filename_all));
		sprintf(filename_all, "/ailvgo/system/temp/%s", filename);
		if(access(filename_all, R_OK) == 0)
		{
			memset(sh_cmd, 0, sizeof(sh_cmd));
			sprintf(sh_cmd, "mv -f %s %s%s", filename_all, filepath, filename);
			printf("sh_cmd : %s\n", sh_cmd);
			system(sh_cmd);
			sleep(1);
		}
    }
    if(delete_flag == 1)
	{
		printf("delete all files in sysfile_update | sysfile.db!\n");
		memset(sql_cmd, 0, sizeof(sql_cmd)); 
		strcpy(sql_cmd, "delete from sysfile_update;");
		printf("sql_cmd : %s\n", sql_cmd);
		sqlite3_exec(db, sql_cmd, NULL, NULL, &errorMsg);
	}
	else
		printf("no file in sysfile_update | sysfile.db\n");

    sqlite3_finalize(ppstmt);
    sqlite3_close(db);

	printf("sysfile_update : complete!\n");

	system("chmod -R 777 /ailvgo/system/");
}
