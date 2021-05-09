#include "Node.hh"
#include "LayerIfc.hh"
#include "LinkLayer.hh"

#define MAXBUFF 1024

void LinkLayer::set_parent(Node * node) { 
	_parent = node;
}

void LinkLayer::send_frame(char *data, int sz ) {
    	if (this->s_local == -1) {
		return;
	}

	pthread_mutex_lock(&_bufferlck);

	while(_sbufferReady) {
	   pthread_cond_wait(&_cv, &_bufferlck);	
	}

	_sbuffer = (char *)malloc(sz*sizeof(char ));
	memcpy(_sbuffer, data, sz*sizeof(char));
	
	_sbuffersz = sz;
	_sbufferReady = true;

	pthread_mutex_unlock(&_bufferlck);
}

void * LinkLayer::send_and_receive() {
	fd_set read_fds;
	fd_set write_fds;

	while(true) {

	    if (this->s_local == -1) continue;

	    FD_ZERO(&read_fds);
	    FD_ZERO(&write_fds);
	    FD_SET(this->s_local, &read_fds);
	    FD_SET(this->s_local, &write_fds);

	    int fdmax = this->s_local;
	    if (select(fdmax+1, &read_fds, &write_fds, nullptr, nullptr) == -1) {

		perror("select");
	    }		    

	    if (FD_ISSET(this->s_local, &read_fds)) {
		recv_data();
	    }

	    if (_sbufferReady && FD_ISSET(this->s_local, &write_fds)) {
		send_data(); 
	    }
	    
	}
	return nullptr;
}


void LinkLayer::recv_data() {
	struct sockaddr_storage their_addr;
	int numbytes, totalbytes;
	socklen_t addr_len;

	char buf[MAXBUFF];
	uint16_t frame_length = 0;
	addr_len = sizeof their_addr;
	numbytes = totalbytes = 0;

	while (1) {

		if ((numbytes = recvfrom(this->s_local, buf, MAXBUFF , 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			fprintf(stderr, "recvfrom failed to get all data\n");
		 }
		if (totalbytes == 0 ) {
			memcpy(&frame_length, buf + 2, sizeof(uint16_t));
			frame_length = ntohs(frame_length);
		}
		totalbytes += numbytes;
		if (totalbytes >= frame_length ) break;
	}

	uint16_t port = ntohs(get_port((struct sockaddr *)&their_addr));
	int id = _parent->get_interfaceid_with_port(port);
	LinkLayer * ifc = _parent->get_interfaces()[id];

	// DEBUG PRINTING
	/*
	char str[INET_ADDRSTRLEN];
	struct sockaddr_in *sin = (struct sockaddr_in *)&their_addr;
	fprintf(stdout, "recv data from %s and port: %hu\n",\
		       	inet_ntop(AF_INET, (struct in_addr *)&sin->sin_addr, str,INET_ADDRSTRLEN), port);
	*/
	// END DEBUG PRINTING
	/*	
	uint16_t nentries;
	memcpy(&nentries, buf+22, sizeof(uint16_t));
        nentries = ntohs(nentries);

        int i = 0; 

        while(i < nentries) {
        struct entry cur_entry;
        memcpy(&cur_entry, buf+ 24 + i*sizeof(struct entry), sizeof(struct entry));
	
        char s1[INET_ADDRSTRLEN], s2[INET_ADDRSTRLEN];
	struct in_addr next = ifc->get_remote_vip();
        fprintf(stdout,"entry: %d, entry address: %s cost: %u, port :%hu, next: %s, id: %d\n", i, \
                        inet_ntop(AF_INET, (struct in_addr *)&(cur_entry.address), s1,INET_ADDRSTRLEN),
                        ntohl(cur_entry.cost), port, inet_ntop(AF_INET, &next, s2, INET_ADDRSTRLEN), id);
        
	i++;
	}
	*/
	_parent->get_network_layer()->transfer_link_to_ip(buf, totalbytes, ifc->get_local_vip());

}


void LinkLayer::send_data() {
	int remBytes = _sbuffersz;
	int numBytes;
	// DEBUG PRINTING
/*	struct packet *pkt = (struct packet *)_sbuffer;
	char str[INET_ADDRSTRLEN];
	uint32_t haddr = ntohl(pkt->hdr.daddr);
	printf("total bytes: %d, protocol: %d send address:%s\n", ntohs(pkt->hdr.tot_len), pkt->hdr.protocol,\
			inet_ntop(AF_INET, (struct in_addr *)&haddr, str, INET_ADDRSTRLEN));
	char *d1 = pkt->data;
	uint16_t command = ntohs( get_length_from_bytes(d1[0], d1[1]));
        uint16_t nentries = ntohs( get_length_from_bytes(d1[2], d1[3]));
	struct entry e;
	memcpy(&e, d1+4, sizeof(struct entry));
	printf("msg type:%d, entries: %d, entry1: %ud, %ud, %ud\n", command, nentries, ntohl(e.cost),\
			ntohl(e.address), ntohl(e.mask));
*/
	// END DEBUG PRINTING

	while(remBytes > 0) {
		
		numBytes = sendto(this->s_local, _sbuffer, remBytes, 0, remote_addr->ai_addr, remote_addr->ai_addrlen );
		
		if (numBytes == -1) {
		   perror("send");
		   break;
		}
		remBytes -= numBytes;
		_sbuffer += numBytes;

	}
	_sbuffer -= _sbuffersz;
	if (_sbuffer != nullptr)free(_sbuffer);
	_sbuffer = nullptr;
	_sbufferReady = false;
	pthread_cond_signal(&_cv);
}

void LinkLayer::up() {
	if (this->s_local != -1) {
		fprintf(stdout,  "interface is already up\n");
	}
	this->s_local = create_socket(const_cast<char *>(_localip.c_str()), _localport);
	this->_isup = true;
}

void LinkLayer::down() {
	if (this->s_local == -1) {
		fprintf(stdout, "interface is already down\n");
	}
	fprintf(stdout, "interface socket %d is closing\n", this->s_local);
	close(this->s_local);
	this->s_local = -1;
	this->_isup = false;
}

