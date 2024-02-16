#ifndef NET_CORE_H
#define NET_CORE_H

typedef struct Net_Socket Net_Socket;
struct Net_Socket
{
	U64 u64[1];
};

typedef enum Net_Protocol Net_Protocol;
enum Net_Protocol
{
	Net_Protocol_TCP,
	Net_Protocol_UDP,

	Net_Protocol_COUNT,
};

typedef enum Net_AddressFamily Net_AddressFamily;
enum Net_AddressFamily
{
	Net_AddressFamily_INET,
	Net_AddressFamily_INET6,
	Net_AddressFamily_UNIX,

	Net_AddressFamily_COUNT,
};

typedef struct Net_Address Net_Address;
struct Net_Address
{
	U16 port;
	union
	{
		U32 u32[1];
		U8 u8[4];
	} ip;
	Net_AddressFamily address_family;
};

typedef struct Net_RecieveResult Net_RecieveResult;
struct Net_RecieveResult
{
	U64 bytes_recieved;
};

typedef struct Net_AcceptResult Net_AcceptResult;
struct Net_AcceptResult
{
	Net_Socket socket;
	Net_Address address;
	B32 succeeded;
};

internal Void              net_socket_init(Void);
internal Net_Socket        net_socket_alloc(Net_Protocol protocol, Net_AddressFamily address_family);
internal Void              net_socket_free(Net_Socket socket);
internal Void              net_socket_bind(Net_Socket socket, Net_Address address);
internal Void              net_socket_connect(Net_Socket socket, Net_Address to);
internal Net_AcceptResult  net_socket_accept(Net_Socket socket);
internal Void              net_socket_send(Net_Socket socket, Str8 data);
internal Void              net_socket_send_to(Net_Socket socket, Net_Address address, Str8 data);
internal Net_RecieveResult net_socket_recieve(Net_Socket connected_socket, U8 *buffer, U64 buffer_size);
internal Net_RecieveResult net_socket_recieve_from(Net_Socket listen_socket, Net_Address *address, U8 *buffer, U64 buffer_size);
internal Void              net_socket_set_blocking_mode(Net_Socket socket, B32 should_block);

#endif
