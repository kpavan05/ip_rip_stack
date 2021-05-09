#ifndef _network_layer_hh
#define _network_layer_hh

#include "common.hh"
#include "LayerIfc.hh"
#include "MessageQueue.hh"

#define MAX_COST 16
#define MIN_HDR_LEN 5
#define IP_VERSION 4

class Node;

class NetworkLayer : public LayerIfc{

public:
	NetworkLayer(Node *node);
	

	virtual void transfer_application_to_ip(struct in_addr& dest, uint8_t proto, char *data , int ) ;
	virtual void transfer_rip_to_ip(int , char  * , int );
        virtual void transfer_link_to_ip(char *, int , struct in_addr ); 

	void * process_in_queue();
	void * process_out_queue();
	static void *thread_func1(void *obj ) { return ((NetworkLayer *)obj)->process_in_queue(); }
	static void *thread_func2(void *obj ) { return ((NetworkLayer *)obj)->process_out_queue(); }

private:
        void build_ip_packet(struct packet& ,struct in_addr& src, struct in_addr& dest, uint8_t proto, char * data , int) ;
	void transfer_transit_data(uint32_t  , char * , int );
	void transfer_data(char * , int , struct in_addr );

private:
	pthread_t _thread1;
	pthread_t _thread2;
	MessageQueue<struct ipmsg > _incomingqueue;
        MessageQueue<struct rcvmsg > _outgoingqueue;

	//std::map<struct in_addr, struct in_addr, cmpAddress> _table;
};

#endif
