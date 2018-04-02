CFLAGS += -Wall -Wpedantic -std=c99 -g
CFLAGS += $$(pkg-config vips --cflags)
LDFLAGS += -L./libmongoose
LDLIBS += $$(pkg-config vips --libs) -lm -lcrypto -lmongoose -ljson-c

FILES += db_delete.o db_insert.o db_list.o db_read.o db_utils.o image_content.o dedup.o pictDBM_tools.o error.o


all: pictDBM pictDB_server

db_create.o: pictDB.h db_create.c

db_delete.o: pictDB.h db_delete.c

db_gbcollect.o: pictDB.h db_gbcollect.c

db_insert.o: pictDB.h db_insert.c

db_list.o: pictDB.h db_list.c

db_read.o: pictDB.h db_read.c

db_utils.o: pictDB.h db_utils.c

image_content.o: pictDB.h image_content.c image_content.h

dedup.o: pictDB.h dedup.c dedup.h

pictDBM_tools.o: pictDBM_tools.c pictDBM_tools.h

error.o: error.c error.h


pictDBM.o: pictDB.h pictDBM.c pictDBM_tools.h image_content.h

pictDB_server.o: pictDB.h pictDB_server.c


pictDBM: $(FILES) db_create.o db_gbcollect.o pictDBM.o

pictDB_server: $(FILES) pictDB_server.o


cmd: pictDBM

srv: pictDB_server


.PHONY: clean

clean:
	@rm -rf *.o
	@echo "object files deleted"