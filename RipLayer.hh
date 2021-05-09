#ifndef __rip_layer_hh
#define __rip_layer_hh

#include "common.hh"
#include "LayerIfc.hh"

class Node;


class RipLayer : public LayerIfc {
public:
	RipLayer(Node * node);

        virtual void transfer_ip_to_rip(char * , struct in_addr );
	virtual void print_route_table(); 
	virtual void get_next_address(struct in_addr , struct in_addr & );
	virtual void add_ifc_entry(struct in_addr );
	virtual void remove_ifc_entry(struct in_addr );

	void * update_refresh_rt();
        void update_route_table(char * , struct in_addr); 
	static void *thread_func(void *obj ) { return ((RipLayer *)obj)->update_refresh_rt(); }
private:
	void refresh_route_table();
	void prepare_msg (uint16_t );
	void prepare_msg_ifc(uint16_t , int  );
private:
	pthread_t _thread;
	pthread_mutex_t _rlock;
	std::map<uint32_t, struct entry_wts > _rtable;
};
#endif
