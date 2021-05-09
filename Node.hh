#ifndef __node_hh
#define __node_hh

#include "common.hh"
#include "dbg.h"
#include "LayerIfc.hh"

class NetworkLayer;
class RipLayer;
class LinkLayer;

class Node {


public:
	Node (char *fname) ;

	LayerIfc *get_network_layer() ;
	LayerIfc *get_rip_layer();
        
	std::vector<LinkLayer *> get_interfaces() ;
	LinkLayer * get_interface (struct in_addr  );
	bool is_my_interface(uint32_t addr );
	int get_interfaceid(struct in_addr );
	int get_interfaceid_with_port (uint16_t );

	void execute_cmd(char *cmd, const char *line, uint8_t *append_hist) {
		*append_hist = 1;
		if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
			help_cmd(line);
			return;
		}
	 	else if (strcmp(cmd, "interfaces") == 0 || strcmp(cmd, "li") == 0) {
			interfaces_cmd(line);
			return;
		}
		else if (strcmp(cmd, "routes") == 0 || strcmp(cmd, "lr") == 0) {
			routes_cmd(line);
			return;
		}
		else if (strcmp(cmd, "up") == 0 ) {
			up_cmd(line);
			return;
		}
		else if (strcmp(cmd, "down") == 0) {
			down_cmd(line);
			return;
		}
		else if (strcmp(cmd, "send") == 0) {
			send_cmd(line);
			return;
		}
		else {
			*append_hist = 0;
			dbg(DBG_ERROR, "error: no valid command specified\n");
			help_cmd(line);
		}
	}
	void help_cmd(const char *line) {
	    	(void) line;

    		printf("- help, h: Print this list of commands\n"
           	"- interfaces, li: Print information about each interface, one per line\n"
          	 "- routes, lr: Print information about the route to each known destination, one per line\n"
       		"- up [integer]: Bring an interface \"up\" (it must be an existing interface, probably one you brought down)\n"
       		"- down [integer]: Bring an interface \"down\"\n"
		"- send [ip] [protocol] [payload]: sends payload with protocol=protocol to virtual-ip ip\n"
		"- q: quit this node\n");
	}

	void interfaces_cmd(const char *line){
 	   	if (strcmp(line, "interfaces") != 0 && strcmp(line, "li") != 0) {
        		printf("incorrect command issued\n");
        		return;
    	   	}
		print_interfaces();
    		//dbg(DBG_ERROR, "interfaces_cmd: NOT YET IMPLEMENTED\n");
	}

	void routes_cmd(const char *line){
     		if (strcmp(line, "routes") != 0 && strcmp(line, "lr") != 0) {
        		printf("incorrect command issued\n");
        		return;
    		}

    		if (_iplayer == nullptr || _riplayer == nullptr) {
        		printf("network layer is not set up\n");
        		return;
   		 }

    		_riplayer->print_route_table();
    		//dbg(DBG_ERROR, "routes_cmd: NOT YET IMPLEMENTED\n");
	}                                                                                                                         

	void down_cmd(const char *line);

	void up_cmd(const char *line);

	void send_cmd(const char *line);

	bool is_destined_to_me (uint32_t ) ;

	static MessageQueue<int> _timerQueue;
private:
	void add_interfaces( char *);
	void print_interfaces();
private:
	LayerIfc * _iplayer;
	LayerIfc * _riplayer;
	std::vector<LinkLayer *> _interfaces;
	
};

#endif
