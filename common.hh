#ifndef __common_hh
#define __common_hh

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <vector>
#include <map>
#include <queue>
#include "ip.h"
#include "MessageQueue.hh"

#define MAX_ENTRIES 64
#define UPDATE_TIMER_ID 1
#define REFRESH_TIMER_ID 2
#define UPDATE_TIMER_VAL 5
#define REFRESH_TIMER_VAL  12
#define DEFAULT_ADDR 0xFFFFFFFF

struct cmpAddress {

	bool operator()(const struct in_addr& a, const struct in_addr& b) const {
		return a.s_addr < b.s_addr;
	}

};

struct entry {
	uint32_t cost;
	uint32_t address;
	uint32_t mask;
}__attribute__((packed));

struct entry_wts {
	struct entry e;
	uint32_t next_hop;
	struct timeval ts;
}__attribute__((packed));

struct rip_msg{
	uint16_t command;
	uint16_t num_entries;
	struct entry entries[MAX_ENTRIES];
}__attribute__((packed));

struct packet {
	struct iphdr hdr;
	char data[1024];
}__attribute__((packed)); 

struct ipmsg {
        void *ifc;
	int size;
        struct packet packet;
}__attribute__((packed)); 

struct rcvmsg {
	int datalen;
	struct in_addr next;
	struct packet ippkt;
}__attribute__((packed));

#endif
