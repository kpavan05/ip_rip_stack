#ifndef __message_queue_hh
#define __message_queue_hh


#include "common.hh"

template <class T>
class MessageQueue {

public:

	MessageQueue() {
	}

	void initialize() {
		pthread_mutex_init(&_mlock, nullptr);
		pthread_cond_init(&_cv, nullptr);
	}

	void push(T item) {

		pthread_mutex_lock(&_mlock);
		_q.push(item);
		pthread_cond_signal(&_cv);
		pthread_mutex_unlock(&_mlock);
	}

	T pop() {

		pthread_mutex_lock(&_mlock);
		while( _q.empty() ) {

		     pthread_cond_wait(&_cv, &_mlock);
		}	

		T item = _q.front();
		_q.pop();
		pthread_mutex_unlock(&_mlock);
		return item;
	}
private:
	std::queue<T> _q;
	pthread_mutex_t _mlock;
	pthread_cond_t _cv;
};
#endif
