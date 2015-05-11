include ../MakeConfig

TOOLPATH = ../toolchain_mipsel

COMMON = udp_get_new.c sqlite3.c #cJSON.c #md5.c

SOURCE = $(COMMON) 

TESTSOURCE = $(COMMON) test.cpp

EXE = ./output/udp_get

SOURCEOBJS = $(SOURCE:%.cpp=%.o)

TESTSOURCEOBJS = $(TESTSOURCE:%.cpp=%.o)

CFLAGS=-D_GNU_SOURCE -s -D_USE_SQLITE
#CFLAGS=-g3 -D_GNU_SOURCE -D_DEBUG -D_USE_SQLITE -ggdb 
#CFLAGS=-g -s -D_DEBUG -D_GNU_SOURCE -D_USE_SQLITE
#CFLAGS=-g

#LIBS=-lpthread -lmysqlclient -lz -L$(ORACLE_HOME)/lib -lclntsh 
LIBS=-L./obj
INC=-I./inc -I../common -I/usr/include/mysql/ -I/usr/local/include


TEST=sms_test

all: $(EXE)

$(LIBNAME) : $(SOURCEOBJS)
	#set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done
	$(rm) $(LIBNAME)
	$(AR)  $(LIBNAME) $(SOURCEOBJS)

test:$(TEST)

$(TEST):$(TESTSOURCEOBJS)
	$(CC) $(CFLAGS) $(INC) $(LIBS) $(TESTSOURCEOBJS) -o $@

	
$(EXE):$(SOURCEOBJS)
	$(CC) $(CFLAGS) $(INC) $(LIBS) $(SOURCEOBJS) $(TOOLPATH)/lib/libsupc++.a $(TOOLPATH)/lib/libuClibc++.a $(TOOLPATH)/lib/libpthread.a $(TOOLPATH)/lib/libsqlite3.a ./libghttp.a -o $@ -ldl -lm ./libghttp.a
	
	
%.o:%.cpp
	$(CC) $(CFLAGS) $(INC) -c $< -o $@
#%.o:%.cpp
#	$(CC) $(CFLAGS) $(INC) -c $< -o ./obj/$(notdir $@)

clean:
	$(RM) *.o ../common/*.o ./wsp/*.o ./sms/*.o $(EXE) $(LIBNAME)
