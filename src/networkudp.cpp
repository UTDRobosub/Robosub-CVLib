
#include "robosub/networkudp.h"

namespace robosub {

	UDPR::UDPR(){
	    initrecv=0;
	}
	UDPR::~UDPR(){
	    if(initrecv)stopRecv();
	}

	UDPS::UDPS(){
	    initsend=0;
	}
	UDPS::~UDPS(){
	    if(initsend)stopSend();
	}

	//set-up receiving on the specified port
	//since it binds to the port, only one instance can receive on the same port on any device
	//because of this, two bidirectional instances cannot be used on the same device on the same port
	int UDPR::initRecv(int port){
		if((rsock=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
			return 2;
		}

		memset((char*)&raddr, 0, sizeof(raddr));
	    raddr.sin_family=AF_INET;
	    raddr.sin_addr.s_addr=htonl(INADDR_ANY);
	    raddr.sin_port=htons(port);

	    //bind to port; tell OS to send all incoming messages on this port to this instance
	    //will err if already bound by another instance
	    if(bind(rsock, (struct sockaddr*)&raddr, sizeof(raddr)) < 0){
		return 4;
	    }

	    initrecv=1;
	    return 0;
	}

	//closes the socket
	//should unbind the port and allow it to be bound again, but the OS can take several minutes to actually unbind the port after doing this
	int UDPR::stopRecv(){
	    if(!initrecv)return 128;

	    close(rsock);
	    initrecv=0;

	    return 0;
	}

	//read up to mlen characters from the receive queue and writes them into memory starting at msg, returning len by reference as the number of bytes read.
	//does not wait for a message; will read 0 characters
	int UDPR::recv(int mlen, int& len, char *msg){
	    if(!initrecv)return 32;

	    socklen_t addrlen=sizeof(raddr);

	    int rlen;
	    if((rlen=recvfrom(rsock, msg, mlen, 0, (struct sockaddr*)&raddr, &addrlen)) < 0){
		return 64;
	    }

	    len=rlen;

	    return 0;
	}

	//expands null characters to \0 and \ to \\ so strings containing nulls don't terminate early
	//nmsg (buffer for results) must be twice the size of len
	//nlen is the length post-expansion, it can be from len to len*2
	void expandNull(int len, int &nlen, char *msg, char *nmsg){

	    int offset=0;

	    for(int i=0;i<len;i++){
		if(msg[i]=='\\'){
		    nmsg[i+offset]='\\';
		    offset++;
		    nmsg[i+offset]='\\';
		}else if(msg[i]=='\0'){
		    nmsg[i+offset]='\\';
		    offset++;
		    nmsg[i+offset]='0';
		}else{
		    nmsg[i+offset]=msg[i];
		}
	    }

	    nlen=len+offset;
	}

	//calls recv and replaces the given c++ string with its result, up to 4096 characters
	//expands null characters to \0 and \ to \\ so the string doesn't terminate early
	//the resulting string can be up to 8192 characters long because of this
	int UDPR::recvStr(string &msg){
		if(!initrecv)return 32;

		char buffer[4096];
	    int rlen;
	    int err;
	    if((err=recv(4096,rlen,buffer)) != 0){
		return err;
	    }

	    char nbuffer[8193];
	    int nrlen;
	    expandNull(rlen,nrlen,buffer,nbuffer);

	    nbuffer[nrlen]='\0'; //add null terminator
	    msg=nbuffer; //copy null-terminated char array into return string

	    return 0;
	}

	//set-up sending to the address in string form, on the specified port
	//any amount of instances can send from or to any device
	int UDPS::initSend(int port, string ssaddr){
		if((ssock=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		return 1;
		}

		memset((char*)&saddr, 0, sizeof(saddr));
	    saddr.sin_family=AF_INET;
	    inet_pton(AF_INET, ssaddr.c_str(), &saddr.sin_addr.s_addr);
	    saddr.sin_port=htons(port);

	    initsend=1;
	    return 0;
	}

	//closes the sending socket
	//basically a formality since it doesn't bind, maybe it frees memory or something
	int UDPS::stopSend(){
	    if(!initsend){
		return 256;
	    }
	    close(ssock);
	    initsend=0;

	    return 0;
	}

	//transmits len bytes in the char array msg
	int UDPS::send(int len, char *msg){
	    if(!initsend)return 8;

	    if(sendto(ssock, msg, len, 0, (struct sockaddr*)&saddr, sizeof(saddr)) < 0){
		return 16;
	    }

	    return 0;
	}

	//transmits a std::string of length <= 4095
	int UDPS::sendStr(string msg){
		if(!initsend)return 8;

		char buffer[4096];
	    memcpy((void*)&(buffer[0]), (void*)msg.c_str(), msg.size()); //copy string contents into char array

	    int err;
		if((err=send(msg.length(), buffer)) != 0){
		return err;
		}

		return 0;
	}

	//int main(){return 0;}
}
