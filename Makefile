CC = g++
CFLAGS = -g -Wall -Wextra -Werror -D_REENTRANT -DCOLOR \
				 -D__BSD_VISIBLE -DREADLINE -Isupport -I.
LDFLAGS = -lpthread -lreadline -lrt

SRCS = node.cc NodeImpl.cc RipLayer.cc NetworkLayer.cc LinkLayer.cc dbg.c parselinks.c

all: node 

node : $(SRCS) ip.h ipsum.h parselinks.h common.hh MessageQueue.hh LayerIfc.hh Node.hh RipLayer.hh LinkLayer.hh NetworkLayer.hh
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f node 
