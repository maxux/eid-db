EXEC = eid-dump-db
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

CFLAGS += -g -std=gnu99 -O2 -W -Wall -Wextra -I/home/maxux/git/eid-mw/plugins_tools/eid-viewer/include -I/home/maxux/git/eid-mw/plugins_tools/util
LDFLAGS += -L/home/maxux/git/eid-mw/plugins_tools/eid-viewer/.libs -leidviewer -ljansson -lsqlite3

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) *.o

mrproper: clean
	$(RM) $(EXEC)
	$(RM) *.gcno *.gcda *.gcov
