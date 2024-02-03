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

typedef struct Net_Address Net_Address;
struct Net_Address
{
	U16 port;
	U32 ip;
};

internal Void       net_socket_init(Void);
internal Net_Socket net_socket_alloc(Net_Protocol protocol);
internal Void       net_socket_free(Net_Socket socket);
internal Void       net_socket_bind(Net_Socket socket, Net_Address address);
internal Void       net_socket_accept(Net_Socket socket);
internal U64        net_socket_send(Net_Socket socket, Str8 data);
internal U64        net_socket_recieve(Net_Socket socket, U8 *buffer, U64 buffer_size);

#endif