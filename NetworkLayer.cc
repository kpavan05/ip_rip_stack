#include "Node.hh"
#include "LinkLayer.hh"
#include "RipLayer.hh"
#include "NetworkLayer.hh"
#include "ipsum.h"

#define RIP_PROTO 200
#define TTL_OFFSET 8
#define CSUM_OFFSET 10
#define MAX_BUFF 1024

NetworkLayer::NetworkLayer(Node *node): LayerIfc(node) {
        _incomingqueue.initialize();
	_outgoingqueue.initialize();
	pthread_create(&_thread1, nullptr, &NetworkLayer::thread_func1, this);
	pthread_create(&_thread2, nullptr, &NetworkLayer::thread_func2, this);
}

        
void NetworkLayer::transfer_link_to_ip(char *data, int len, struct in_addr next) {
       
	struct rcvmsg msg;

	memcpy(&msg.datalen, &len, sizeof(int));
	memcpy(&msg.next, &next, sizeof(struct in_addr));
	memcpy(&msg.ippkt, data, len);

	_outgoingqueue.push(msg);
        	
}
void NetworkLayer::transfer_data(char * data, int len, struct in_addr next) {

	struct packet *pkt = (struct packet *)data;
	uint8_t datapos = 4 * (pkt->hdr.ihl);
	uint32_t dest = ntohl(pkt->hdr.daddr);
	uint8_t proto = pkt->hdr.protocol;
	uint16_t orig_sum = pkt->hdr.check;
	int interfaceid = _parent->get_interfaceid(next);

        memset(data + CSUM_OFFSET, 0, sizeof(uint16_t));
	uint16_t cur_sum = ip_sum(data, sizeof(struct iphdr));
	if (orig_sum != cur_sum){
		fprintf(stdout, "difference in check sum, original:%d, current:%d\n", orig_sum, cur_sum);
		return;
	}

	// DEBUG PRINT
	// END DEBUG PRINT
	
	if (proto == RIP_PROTO) {
	   LinkLayer *ifc = _parent->get_interface(next);
	   if (!ifc->is_up()) return;
	   _parent->get_rip_layer()->transfer_ip_to_rip(data + datapos, next);	
	
	}else {
	
	   if (_parent->is_destined_to_me(dest) ) {
		//char content[MAX_BUFF];
		//memcpy(content, data+datapos, sizeof(len));
		data[len]= '\0';
		
		char s1[INET_ADDRSTRLEN], s2[INET_ADDRSTRLEN];
		fprintf(stdout,"---Node received packet!---\n");
		fprintf(stdout,"\tarrived link: %d\n", interfaceid);
		fprintf(stdout,"\tsource IP: %s\n\treceived IP: %s\n",\
			inet_ntop(AF_INET, (struct in_addr *)&(pkt->hdr.saddr), s1, INET_ADDRSTRLEN), \
                        inet_ntop(AF_INET, (struct in_addr *)&(pkt->hdr.daddr), s2, INET_ADDRSTRLEN));
		fprintf(stdout,"\tprotocol: %hu\n\tpayload length: %d\n\tpayload:%s\n", proto, len-datapos, data + datapos);

		return;

	   } else {
		uint16_t ttl = ntohs(pkt->hdr.ttl) - 1;
		if (ttl == 0 ) {
		   fprintf(stdout, "ttl expired\n");
		   return;
		}
		ttl = htons(ttl);
		memcpy(data + TTL_OFFSET , &ttl , sizeof(uint16_t));

		memset(data + CSUM_OFFSET, 0, sizeof(uint16_t));
		uint16_t csum = ip_sum(data, sizeof(iphdr));
		memcpy(data +  CSUM_OFFSET, &csum, sizeof(uint16_t));

		transfer_transit_data(dest, data, len);
	   }

	}


}
void NetworkLayer::transfer_rip_to_ip(int id, char * data, int datasz) {
	LinkLayer *ifc = _parent->get_interfaces()[id];
       	struct in_addr src = ifc->get_local_vip();
	struct in_addr dest = ifc->get_remote_vip();

	   // DEBUG PRINTING
/*	   char str1[INET_ADDRSTRLEN], str2[INET_ADDRSTRLEN];
           fprintf(stdout, "entering src: %s, dest addr: %s in ipheader for ifc:%p\n", \ 
	              inet_ntop(AF_INET, (struct in_addr *)&src, str1, INET_ADDRSTRLEN),  \
		      inet_ntop(AF_INET, (struct in_addr *)&dest, str2, INET_ADDRSTRLEN), ifc );
*/
	   // END  DEBUG PRINTING
	   	
	struct ipmsg msg ;
	msg.ifc = ifc;
	build_ip_packet(msg.packet, src, dest, RIP_PROTO, data, datasz);
	msg.size = MIN_HDR_LEN*4 + datasz;
	_incomingqueue.push(msg);
}

        
void NetworkLayer::transfer_application_to_ip (struct in_addr& dest, uint8_t proto,\
	       					char *data , int datasz) {
	struct in_addr hdest;
        hdest.s_addr =	ntohl(dest.s_addr);
	struct in_addr next_addr = {DEFAULT_ADDR };
        _parent->get_rip_layer()->get_next_address(hdest, next_addr);
	LinkLayer *ifc = _parent->get_interface(next_addr);

	if (ifc == nullptr) {
	   fprintf(stdout, "destination cannot be reached\n");
           return;
	}
	struct in_addr src = ifc->get_local_vip();

	struct ipmsg msg;
	msg.ifc = ifc;
	build_ip_packet(msg.packet, src, hdest, proto, data, datasz);
        msg.size = MIN_HDR_LEN*4 + datasz; 
	_incomingqueue.push(msg);
}

void NetworkLayer::transfer_transit_data(uint32_t dest, char * data, int datasz) {
	struct in_addr hdest;
        hdest.s_addr =	dest;
	struct in_addr next_addr = {DEFAULT_ADDR};
	_parent->get_rip_layer()->get_next_address(hdest, next_addr);
	LinkLayer *ifc = _parent->get_interface(next_addr);

	if (ifc == nullptr) {
		fprintf(stdout, "destination cannot be reached\n");
		return;
	}

	struct ipmsg msg;
	msg.ifc = ifc;
	memcpy(&msg.packet, data, datasz);
        msg.size = datasz; 
	_incomingqueue.push(msg);

}

void NetworkLayer::build_ip_packet(struct packet &pkt, struct in_addr& src, struct in_addr& dest, \
	       					uint8_t proto, char * data, int datasz) {

	pkt.hdr.ihl = MIN_HDR_LEN;
	pkt.hdr.version = IP_VERSION;
	pkt.hdr.tot_len = htons(pkt.hdr.ihl *4 + datasz);
	pkt.hdr.ttl = MAX_COST;
	pkt.hdr.protocol = proto;
	pkt.hdr.saddr = htonl(src.s_addr);
	pkt.hdr.daddr = htonl(dest.s_addr);
	pkt.hdr.frag_off = 0;
	pkt.hdr.tos = 0;
	pkt.hdr.id = 0;
	pkt.hdr.check = 0;
	char *header = (char *)&(pkt.hdr);
        
	uint16_t cksum = ip_sum(header, sizeof(struct iphdr));
	pkt.hdr.check = cksum;
	memcpy(pkt.data, data, datasz*sizeof(char));
}
/*
void NetworkLayer::print_route_table() {
	std::map<struct in_addr, struct in_addr>::iterator itr = _table.begin();
	printf("Network \t \t  Next Address\n");
	for(; itr != _table.end(); itr++) {
		printf("%d \t \t %d\n", itr->first.s_addr,itr->second.s_addr);
       	}
}

struct in_addr NetworkLayer::get_next_address(struct in_addr dest) {
	std::map<struct in_addr, struct in_addr>::iterator itr = _table.begin();
	for(; itr != _table.end(); itr++ ){
		if (itr->first.s_addr == dest.s_addr) return itr->second;
	}

	std::vector<LinkLayer *> ifcs = _parent->get_interfaces();
	if (ifcs.size() == 1) {
		return ifcs[0]->get_remote_vip();
	} else {
		uint32_t max = 0;	
		struct in_addr sel_vip, cur_vip;
		for(auto ll : ifcs) {
	            cur_vip = ll->get_remote_vip();
 		
	     	    if ((cur_vip.s_addr & dest.s_addr) > max) {
	 		 sel_vip = cur_vip;
			 max = cur_vip.s_addr & dest.s_addr;
		    }
		}
	     	return sel_vip;
	}
}
*/
void *NetworkLayer::process_in_queue(){
	while(1) {
		struct ipmsg msg =  _incomingqueue.pop();
		LinkLayer *ifc = (LinkLayer *)msg.ifc;
		char *data = (char *) &msg.packet;

		// DEBUG PRINTING
/*      	char str1[INET_ADDRSTRLEN], str2[INET_ADDRSTRLEN];
//		struct in_addr local = ifc->get_local_vip();
//		struct in_addr remote = ifc->get_remote_vip();
//
//                fprintf(stdout,"my interface local ip: %s,  remote ip: %s for ifc: %p\n", \
//                                inet_ntop(AF_INET, (struct in_addr *)&local, str1, INET_ADDRSTRLEN),\
//                                inet_ntop(AF_INET, (struct in_addr *)&remote, str2, INET_ADDRSTRLEN), ifc);
*/
		// END DEBUG PRINTING
		ifc->send_frame(data, msg.size);

		msg.ifc = nullptr;
	}
	return nullptr;
}

void *NetworkLayer::process_out_queue(){
        while(1) {
                struct rcvmsg msg =  _outgoingqueue.pop();
                int len = msg.datalen;
		struct in_addr next = msg.next;

		struct packet pkt;
		memcpy(&pkt, &msg.ippkt, len);
		char *data = (char *) &pkt;
	
		transfer_data(data, len, next);
        }
        return nullptr;
}

