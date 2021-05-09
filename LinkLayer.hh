#ifndef _linklayer_hh
#define _linklayer_hh

#include "common.hh"

class Node;

class LinkLayer {

public:

	LinkLayer (char * localip, uint16_t localport, char *remoteip, uint16_t remoteport,\
		       	struct in_addr vip_in, struct in_addr vip_out) {
		
		this->vip_local.s_addr = ntohl(vip_in.s_addr);
		this->vip_remote.s_addr = ntohl(vip_out.s_addr);
		_localip.assign(localip);
		_localport = localport;		

		_remoteport = remoteport; 
		_isup = true;
		/*
		char str1[INET_ADDRSTRLEN], str2[INET_ADDRSTRLEN];
                fprintf(stdout,"my interface local ip: %s, local port: %d, \
			       	remote ip: %s, remote port: %d\n",
			       	inet_ntop(AF_INET, (struct in_addr *)&vip_in, str1, INET_ADDRSTRLEN),localport,
				inet_ntop(AF_INET, (struct in_addr *)&vip_out, str2, INET_ADDRSTRLEN),remoteport);
		*/
		this->s_local = create_socket(localip, localport);
		set_remote_address(remoteip, remoteport);
	
		this->_sbuffer = nullptr;
		this->_sbufferReady = false;
		pthread_mutex_init(&_bufferlck, nullptr);
		pthread_cond_init(&_cv, nullptr);
		pthread_create(&_thread, nullptr, &LinkLayer::thread_func, this);
	}

	void set_parent(Node * node);

	struct in_addr get_remote_vip() { return this->vip_remote; }
	struct in_addr get_local_vip() { return this->vip_local; }
	int get_remote_port() { return this->_remoteport; }

	void up() ;
	void down() ;

	bool is_up() {return this->_isup ; }

	void send_frame(char *data, int sz );
	void * send_and_receive() ;

	static void * thread_func(void * obj) {return ((LinkLayer *)obj)->send_and_receive();}

private:
	
	void recv_data();
	void send_data(); 

	void set_remote_address(char *ip, uint16_t port) {
		int rv;
		char service[8];
            
		sprintf(service, "%d", port);
                
		struct addrinfo hints, *ai;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;

		if ((rv = getaddrinfo(ip, service, &hints, &ai)) != 0)
		{
			fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(rv));
		}

		remote_addr = (struct addrinfo *)malloc(sizeof(struct addrinfo));
		remote_addr->ai_flags = ai->ai_flags;
		remote_addr->ai_family = ai->ai_family;
		remote_addr->ai_socktype = ai->ai_socktype;
		remote_addr->ai_protocol = ai->ai_protocol;
		remote_addr->ai_addrlen = ai->ai_addrlen;
		remote_addr->ai_addr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
		remote_addr->ai_addr->sa_family = ai->ai_addr->sa_family;
		memcpy(remote_addr->ai_addr->sa_data, ai->ai_addr->sa_data, 14);
		remote_addr->ai_next = nullptr;

		freeaddrinfo(ai);
	}

	int create_socket(char * ip, uint16_t port) {
		int rv;
		int sockfd ;
		int yes = 1;
		char service[8];
          	memset(service,0, sizeof(service));
                sprintf(service, "%hu", port);
		service[strlen(service)] = '\0';

		struct addrinfo hints, *ai, *p;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;

		if ((rv = getaddrinfo(ip, service, &hints, &ai)) != 0)
		{
			fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(rv));
		}


		for (p = ai; p != NULL; p = p->ai_next) {
			if ( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
			{
				perror("server:socket");
				continue;
			}
			/*
			char straddr[INET6_ADDRSTRLEN];
		       inet_ntop (p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),straddr, INET6_ADDRSTRLEN);
   			fprintf(stdout, "address:%s, family:%d, port:%d\n", straddr, p->ai_family, \
           				ntohs(get_port((struct sockaddr *)p->ai_addr)) );
			*/
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) ) == -1)
			{
				perror("setsockopt");
				continue;
			}
			fprintf(stdout, "sockfd:%d\n", sockfd);
			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
			{
		
				close(sockfd);
         			perror("server:bind");
				continue;
	    		}     	
    			fprintf(stdout, "bind succesful\n");
			break;
		}
		if (p == NULL) {
 			fprintf(stderr, "talker: failed to create socket\n");
 			exit(1);
 		}
		freeaddrinfo(ai);
		return sockfd;

	}

	// utility methods for printing address
	void *get_in_addr(struct sockaddr *sa) {
  		if (sa->sa_family == AF_INET)
    			return &(((struct sockaddr_in *)sa)->sin_addr);
  		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
	}
	in_port_t get_port(struct sockaddr *sa) {
  		if (sa->sa_family == AF_INET)
    			return (((struct sockaddr_in *)sa)->sin_port);
  	return (((struct sockaddr_in6 *)sa)->sin6_port);
	}


private:
	int s_local;
	std::string _localip;
	uint16_t _localport;
	uint16_t _remoteport;
	struct addrinfo *remote_addr;

	bool _isup;
	Node *_parent;
	pthread_t _thread;	
	char *_sbuffer;
	bool _sbufferReady;
	int _sbuffersz;

	struct in_addr vip_local;
	struct in_addr vip_remote;
	pthread_mutex_t _bufferlck;
	pthread_cond_t _cv;
	
};

#endif
