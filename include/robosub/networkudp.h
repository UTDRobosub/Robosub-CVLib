
#pragma once

#include "common.h"

#ifdef WINDOWS
	#define NETWORKUDP_WINSOCK
#endif

#include <string>
#include <iostream>
#include <string.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netdb.h>

#ifdef NETWORKUDP_WINSOCK
    #include <WS2tcpip.h>
    #include <Winsock2.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

#ifdef NETWORKUDP_WINSOCK
    #define NETWORKUDP_GETERROR WSAGetLastError()
#else
    #define NETWORKUDP_GETERROR errno
#endif

namespace robosub {
	//class for receiving over UDP
	//handles one port
	//call initRecv on the port to listen on before calling recv
	//do not use multiple instances on the same port on one device, as receiving requires binding to the port
	class UDPR{
		sockaddr_in raddr; //address info to recv on
		int rsock; //socket info for recving
		int initrecv; //1 if initRecv has succeeded
		
		char *recvbuf; //buffer to insert excess received data into, for later usage
		int recvbuflen; //number of bytes in the buffer
		const int maxrecvbuflen = 1024*1024*7;
		
		int readEntireBuffer();
		
		public:

        EXPORT UDPR();
        EXPORT ~UDPR();

		EXPORT int initRecv(int port, int timeoutMicroseconds);
		EXPORT int stopRecv();
		EXPORT int recv(int maxLength, int& receivedLength, char* buffer);
		EXPORT int recvStr(string& output);
	};

	//class for sending over UDP
	//handles one port and one host
	//call initSend on the port and address to send to before calling send
	class UDPS{
		sockaddr_in saddr; //address info to send on
		int ssock; //socket info for sending
		int initsend; //1 if initSend has succeeded

		public:

	    EXPORT UDPS();
	    EXPORT ~UDPS();

		EXPORT int initSend(int port, string address);
		EXPORT int stopSend();
		EXPORT int send(int length,char* data);
		EXPORT int sendStr(string source);
	};

	//wrapper class for bidirectional communication
	//call init, which does both initSend and initRecv on the same port and the given address, before calling recv or send
	//do not use multiple instances on the same port on one device, as receiving requires binding to the port
	class UDP{
	    UDPR ur;
	    UDPS us;

	    public:

	    EXPORT int init(int,int,string, int);
	    EXPORT int recv(int,int&,char*);
	    EXPORT int recvStr(string&);
	    EXPORT int send(int,char*);
	    EXPORT int sendStr(string);
	};

	//wrapper functions for bidirectional communication
	//for usage, see comments on their monodirectional counterparts
	inline int UDP::init(int rport, int sport, string ssaddr, int rtimeout){
	    return ur.initRecv(rport, rtimeout) + us.initSend(sport,ssaddr);
	}

	inline int UDP::recvStr(string &msg){
	    return ur.recvStr(msg);
	}
	inline int UDP::recv(int mlen, int &len, char *msg){
	    return ur.recv(mlen,len,msg);
	}

	inline int UDP::sendStr(string msg){
	    return us.sendStr(msg);
	}
	inline int UDP::send(int len, char *msg){
	    return us.send(len,msg);
	}
}
