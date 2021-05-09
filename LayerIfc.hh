#ifndef __layerifc__hh
#define __layerifc__hh

#include "common.hh"
#include "parselinks.h"

class Node;

class LayerIfc {

public:
	LayerIfc(Node* node) {
	    _parent = node;
	}
        	
	virtual void print_route_table() {
	    printf("base implementation for print_route_table\n");		;
       	}

	virtual void transfer_application_to_ip(struct in_addr& , uint8_t  , char *  , int ){
		printf("no implementation for transfer_application_to_ip \n");	
	}
	virtual void transfer_rip_to_ip(int  ,char * , int ) {
	    printf("no implementation for transfer_rip_to_ip\n");
	}

	virtual void transfer_ip_to_rip(char * , struct in_addr ) {
	    printf("no implementation for transfer_ip_to_rip\n");
	}

	virtual void transfer_link_to_ip(char * , int , struct in_addr  ) {
	    printf("no implementation for transfer_link_to_ip \n");
	
	}

	virtual void get_next_address(struct in_addr , struct in_addr &) {
	    printf("no implementation for get_next_address \n");	
	}

	virtual void add_ifc_entry (struct in_addr ) {
	    printf("no implementation for add_ifc_entry \n");
	}

	virtual void remove_ifc_entry(struct in_addr ) {
	    printf("no implementation for remove_ifc_entry \n");
	}

protected:
	Node *_parent;
};

#endif

