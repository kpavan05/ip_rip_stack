# ip-tcp-kpk
ip-tcp-kpk created by GitHub Classroom

class diagram for the implementation is in ipimpl.png


Network Layer:
Network layer object has two thread safe queues one for incoming and another for outgoing message.
When a message is received by network layer object from linked layer object , 
the data is encapuslated and put into outgoing queue and returned.
similarly when a message from rip layer or application layer is received by network layer object, data object is put into
incoming quueue.
These queues are handled by separate threads.

LinkLayer:
select is called with the readfd and writefd set to the socket that is created with local ip and port.
link layer object has a buffer member that is loaded by network layer. 
This triggers the select which makes the sendto be called.

Two global timer objects are declared to handle update and refresh route table events at 5 and 12 seconds respecitively. 
These timer events are handled by their signal handlers. 
