#include "Node.hh"
#include "LayerIfc.hh"
#include "RipLayer.hh"
#include "NetworkLayer.hh"
#include "LinkLayer.hh"

Node::Node (char *fname) {
	_interfaces.clear();
	add_interfaces(fname);
	_iplayer = new NetworkLayer(this);
	_riplayer = new RipLayer(this);
}

LayerIfc * Node::get_network_layer() { return _iplayer; }
LayerIfc * Node::get_rip_layer() { return _riplayer; }
std::vector<LinkLayer *> Node::get_interfaces() { return _interfaces; }


void Node::add_interfaces(char *fname) {
	lnxinfo_t *info = parse_links(fname);
	if (info == nullptr) {
		exit(1);
	}
    	lnxbody_t *links = info->body;
	char local_phys_host[16] ;
	sprintf(local_phys_host, "%s", "localhost");
    	while (links) {
		LinkLayer *ll = new LinkLayer(local_phys_host, info->local_phys_port, \
				links->remote_phys_host, links->remote_phys_port, \
				links->local_virt_ip, links->remote_virt_ip);

		ll->set_parent(this);
		_interfaces.push_back(ll);
		links = links->next;

	}	
}



void Node::print_interfaces() {
	fprintf(stdout,"id\trem\t\tloc\n");
	int id = 0;
	for(auto ifc: _interfaces) {
		if (!ifc->is_up()) continue;
		struct in_addr local;
	        local.s_addr = htonl(ifc->get_local_vip().s_addr);
		struct in_addr remote;
	       	remote.s_addr = htonl(ifc->get_remote_vip().s_addr);
		
		char str1[INET_ADDRSTRLEN], str2[INET_ADDRSTRLEN];
                fprintf(stdout,"%d\t%s\t%s\n",id, \
                        inet_ntop(AF_INET, (struct in_addr *)&remote, str1, INET_ADDRSTRLEN),\
                        inet_ntop(AF_INET, (struct in_addr *)&local, str2, INET_ADDRSTRLEN));
		id++;
	}
}

bool Node::is_my_interface(uint32_t addr) {
	for(auto ll: _interfaces) {
	   if(ll->get_local_vip().s_addr == addr) return true;
	}
	return false;
}
      
LinkLayer * Node::get_interface(struct in_addr next_addr) {
	for(auto ll: _interfaces) {
		if (ll->get_local_vip().s_addr == next_addr.s_addr)
			return ll;
	}
	return nullptr;
}

void Node::send_cmd(const char *line){
	char ip_string[INET_ADDRSTRLEN];
	struct in_addr ip_addr;
	uint8_t protocol;
	int num_consumed;
	char *data;
	if (sscanf(line, "send %s %" SCNu8 "%n", ip_string, &protocol, &num_consumed) != 2) {
		dbg(DBG_ERROR, "syntax error (usage: send [ip] [protocol] [payload])\n");
		return;
	 }

	if (inet_pton(AF_INET, ip_string, &ip_addr) == 0) {
		dbg(DBG_ERROR, "syntax error (malformed ip address)\n");
		return;
	}

	data = ((char *)line) + num_consumed + 1;

	if (strlen(data) < 1) {
		dbg(DBG_ERROR, "syntax error (payload unspecified)\n");
		return;
	}
	 // send
	this->get_network_layer()->transfer_application_to_ip(ip_addr, protocol, data, strlen(data));
	//dbg(DBG_ERROR, "send_cmd: NOT YET IMPLEMENTED\n");
}

 void Node::down_cmd(const char *line){
	unsigned interface;

	if (sscanf(line, "down %u", &interface) != 1) {
		dbg(DBG_ERROR, "syntax error (usage: down [interface])\n");
		return;
	}

	LinkLayer *ifc = _interfaces[interface];
	ifc->down();
	this->get_rip_layer()->remove_ifc_entry(ifc->get_local_vip());
	//dbg(DBG_ERROR, "down_cmd: NOT YET IMPLEMENTED\n");
}

void Node::up_cmd(const char *line){
	unsigned interface;

	if (sscanf(line, "up %u", &interface) != 1) {
		dbg(DBG_ERROR, "syntax error (usage: up [interface])\n");
		return;
	}
	LinkLayer *ifc = _interfaces[interface];
	ifc->up();
	this->get_rip_layer()->add_ifc_entry(ifc->get_local_vip());
	//dbg(DBG_ERROR, "up_cmd: NOT YET IMPLEMENTED\n");
}

bool Node::is_destined_to_me(uint32_t addr) {

	for(auto ifc:_interfaces) {
	   if (ifc->get_local_vip().s_addr == addr) return true;
	}
	return false;
}

int Node::get_interfaceid(struct in_addr addr) {
	int index = 0;
	for(auto ifc:_interfaces) {
	   if (ifc->get_remote_vip().s_addr == addr.s_addr) return index;
	   index++;
	}
	return -1;
}

int Node::get_interfaceid_with_port(uint16_t port) {
	int index =0;
	for (auto ifc:_interfaces) {
		if (ifc->get_remote_port() == port ) return index;
		index++;
	}
	return -1;

}
