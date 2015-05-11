#include <stdio.h>
#include <stdlib.h>
#include "md5.h"

int main()
{
	printf("md5 value : %s\n", md5_file("/home/hbr/box/cgi/md5-file/test"));
	return 1;
}
