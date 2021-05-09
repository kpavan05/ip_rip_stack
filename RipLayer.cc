#include "Node.hh"
#include "RipLayer.hh"
#include "NetworkLayer.hh"
#include "LinkLayer.hh"

#define RIP_MASK 0xFFFFFFFF
#define RIP_MAX_COST 0x00000010


RipLayer::RipLayer(Node * node) : LayerIfc(node){
	_rtable.clear();

	for(auto ll : node->get_interfaces()) {
		struct in_addr addr = ll->get_local_vip();
		struct entry_wts ets;
		struct entry e ;
	        e.cost = 0;
	        e.address = addr.s_addr;
		e.mask = RIP_MASK;
		/*char str[INET_ADDRSTRLEN];
        	fprintf(stdout,"my interface rip entry address: %s\n",
			       	inet_ntop(AF_INET, (struct in_addr *)&addr, str,INET_ADDRSTRLEN));
		*/
		ets.e = e;
		ets.next_hop = addr.s_addr;
		gettimeofday(&ets.ts, nullptr);
		_rtable[addr.s_addr] = ets;
	}
	prepare_msg(1);
	pthread_mutex_init(&_rlock, nullptr);
	pthread_create(&_thread, nullptr, &RipLayer::thread_func, this);	
}

void RipLayer::transfer_ip_to_rip(char * data, struct in_addr next) {
     uint16_t cmdType;
     memcpy(&cmdType, data, sizeof(uint16_t));
     cmdType = ntohs(cmdType);
     data = data + 2;
     switch(cmdType) {
	case 1:
		//update_route_table(data);
		prepare_msg(2);
		break;
	case 2:
		update_route_table(data, next);
		break;
	default:
		fprintf(stderr, "message type is not recongnized\n");
		break;
     }     

}


void RipLayer::update_route_table(char * data, struct in_addr next) {
     uint16_t nentries;
     memcpy(&nentries, data, sizeof(uint16_t));
     nentries = ntohs(nentries);

     data = data + 2;

     for(int i = 0;  i < nentries; i++){
	struct entry cur_entry;
        memcpy(&cur_entry, data, sizeof(struct entry));	
        /*	
	char s1[INET_ADDRSTRLEN], s2[INET_ADDRSTRLEN];
        fprintf(stdout,"rip entry address: %s cost: %d loc:%s\n", \
		 	inet_ntop(AF_INET, (struct in_addr *)&(cur_entry.address), s1,INET_ADDRSTRLEN),
			ntohl(cur_entry.cost), inet_ntop(AF_INET, &next, s2, INET_ADDRSTRLEN));
	*/
	uint32_t curaddress = ntohl(cur_entry.address);
	uint32_t cost = ntohl(cur_entry.cost);

	if (_parent->is_my_interface(curaddress) || cost == RIP_MAX_COST ) {
	    data +=  sizeof(struct entry);	
	    continue;
	}

	if (_rtable.find(curaddress) == _rtable.end()) {
		struct entry e;
		struct entry_wts ets;
		e.cost = cost + 1;
	        e.address = ntohl(cur_entry.address);
		e.mask = ntohl(cur_entry.mask);
		ets.e = e;
		ets.next_hop = next.s_addr; //already in host order
		gettimeofday(&ets.ts, nullptr);
		pthread_mutex_lock( &_rlock);
		_rtable[curaddress] = ets;
		pthread_mutex_unlock(&_rlock);
	} else {
	   
	   
           struct entry_wts ets = _rtable[curaddress];
	   struct entry e = ets.e;
	   if (e.cost > (cost + 1))  {
		e.cost = cost + 1;
	        e.address = ntohl(cur_entry.address);
		e.mask = ntohl(cur_entry.mask);
		gettimeofday(&ets.ts, nullptr);
		ets.e = e;
		ets.next_hop = next.s_addr; //already in host order
		pthread_mutex_lock(&_rlock);
		_rtable[curaddress] = ets;
	        pthread_mutex_unlock(&_rlock);
	   }
	}

	data += sizeof(struct entry);
     }
}
void RipLayer::prepare_msg(uint16_t msg_type) {

     for(size_t id = 0; id < _parent->get_interfaces().size(); id++) {
         if (_parent->get_interfaces()[id]->is_up())
	     prepare_msg_ifc(msg_type, static_cast<int>(id));
     }
}

void RipLayer::prepare_msg_ifc(uint16_t msg_type, int id) {
     int nentry = 0;
     struct rip_msg msg;
     char *data = nullptr;
     //LinkLayer *ifc = static_cast<LinkLayer *>(ll);
     msg.command = htons(msg_type);
     msg.num_entries = htons(_rtable.size());
     
     uint32_t neighbor = _parent->get_interfaces()[id]->get_local_vip().s_addr;
     std::map<uint32_t, struct entry_wts >::iterator itr = _rtable.begin();

     for(; itr != _rtable.end(); itr++) {
	struct entry_wts ets = itr->second;
        struct entry e = ets.e;	
	msg.entries[nentry].cost = \
	        (e.address != neighbor && neighbor == ets.next_hop) ? htonl(RIP_MAX_COST) : htonl(e.cost);
	msg.entries[nentry].address =  htonl(e.address);
	msg.entries[nentry].mask = htonl(e.mask);
	nentry++;
     }

     int datasz = (sizeof(struct entry))*nentry + 4;
     data = (char *) malloc(datasz *sizeof(char));

     int curpos = 0;
     memcpy(data, &msg.command, sizeof(uint16_t));
     curpos += sizeof(uint16_t);
     memcpy(data + curpos, &msg.num_entries, sizeof(uint16_t));
     curpos += sizeof(uint16_t);
     memcpy(data + curpos, &msg.entries, sizeof(struct entry)*nentry);
     _parent->get_network_layer()->transfer_rip_to_ip(id, data, datasz);     
}

void RipLayer::refresh_route_table() {
     struct timeval curtv;
     gettimeofday(&curtv, nullptr);

     unsigned long curms = curtv.tv_sec*1000000 + curtv.tv_usec;     
     
     std::map<uint32_t, struct entry_wts >::iterator itr = _rtable.begin();
     std::map<uint32_t, struct entry_wts>::iterator next = itr;
     
     for(; itr != _rtable.end(); itr = next) {
	++next;     
	
	if (_parent->is_my_interface(itr->first)) continue;

	struct entry_wts ets = itr->second;
	struct timeval tv = ets.ts;
	unsigned long ms= tv.tv_sec*1000000 + tv.tv_usec;

	if ((curms-ms) > (REFRESH_TIMER_VAL*1000000)){
	   pthread_mutex_lock(&_rlock);
	   _rtable.erase(itr);
	   pthread_mutex_unlock(&_rlock);
	} 
     }
}

void RipLayer::get_next_address(struct in_addr dest, struct in_addr &next_addr) {
        std::map<uint32_t, struct entry_wts>::iterator itr = _rtable.begin();
        for(; itr != _rtable.end(); itr++ ){
	    //fprintf(stdout, "matching dest: %u, with entry: %u size:%lu\n", dest.s_addr, itr->second.e.address, _rtable.size());
            if (itr->second.e.address == dest.s_addr) {
                next_addr.s_addr =  itr->second.next_hop;
	    }
        }

}

void RipLayer::print_route_table() {
     std::map<uint32_t, struct entry_wts >::iterator itr = _rtable.begin();

     fprintf(stdout, "cost \t\t dst \t\t loc \n");
     
     for(; itr != _rtable.end(); itr++) {
        char s1[INET_ADDRSTRLEN], s2[INET_ADDRSTRLEN];
	struct entry_wts ets = itr->second;
	struct entry e = ets.e;
	//struct timeval tv = ets.ts;
	//unsigned long ms= tv.tv_sec*1000000 + tv.tv_usec;
	uint32_t addr = htonl(e.address);
	uint32_t hop = htonl(ets.next_hop);
	fprintf(stdout, "%u \t %s \t %s\n", e.cost, inet_ntop(AF_INET, (struct in_addr *)&addr,s1, INET_ADDRSTRLEN),
			inet_ntop(AF_INET, (struct in_addr *)&hop,s2, INET_ADDRSTRLEN));
	/*
        fprintf(stdout, "cost: %u, dest: %s, next_hop: %s, update_time:%lu\n", e.cost, \
			inet_ntop(AF_INET, (struct in_addr *)&itr->first, s1, INET_ADDRSTRLEN),
		       	inet_ntop(AF_INET, (struct in_addr *)&addr,s2,INET_ADDRSTRLEN), ms);
	*/
     }


}

void *RipLayer::update_refresh_rt() {
    
    while (true) {
	int type = Node::_timerQueue.pop();
	switch(type) {
	   case 1:
		prepare_msg(2);
	        break;
	   case 2:
		refresh_route_table();		
     		break;
	   default:
     		break;
	}
    }
    return nullptr; 
}


void RipLayer::remove_ifc_entry(struct in_addr daddr) {
     //fprintf(stdout, "route table size: %lu\n", _rtable.size());

     for(auto itr = _rtable.begin(), next_itr = itr; itr != _rtable.end(); itr = next_itr) {
        ++next_itr;
	if (itr->second.next_hop == daddr.s_addr || itr->first == daddr.s_addr) {
	   pthread_mutex_lock(&_rlock);
	   _rtable.erase(itr);
	   pthread_mutex_unlock(&_rlock);
	}
     }
     /*fprintf(stdout, "route table size: %lu\n", _rtable.size());
     for(auto itr = _rtable.begin(); itr != _rtable.end(); itr++) {
     	fprintf(stdout, "address: %u next hop:%u \n", itr->first, itr->second.next_hop);
     }*/

     prepare_msg(2);

}

void RipLayer::add_ifc_entry(struct in_addr addr) {
	struct entry_wts ets;
	struct entry e ;
	e.cost = 0;
	e.address = addr.s_addr;
	e.mask = RIP_MASK;
	ets.e = e;
	ets.next_hop = addr.s_addr;
	gettimeofday(&ets.ts, nullptr);
	pthread_mutex_lock(&_rlock);
	_rtable[addr.s_addr] = ets;
	pthread_mutex_unlock(&_rlock);
}
