#ifndef NET_CORE_H
#define NET_CORE_H

typedef struct Net_Socket Net_Socket;

typedef enum Net_Protocol Net_Protocol;
enum Net_Protocol
{
	Net_Protocol_TCP,
	Net_Protocol_UDP,
	
	Net_Protocol_COUNT,
};

#endif