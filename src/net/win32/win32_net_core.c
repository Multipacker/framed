#pragma comment(lib, "Ws2_32.lib")

internal Void
net_socket_init(Void)
{
	WSADATA wsa_data;
	S32 error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (error)
	{
		log_error("WSAStartup failed: %d\n", error);
	}
}

internal Net_Socket
net_socket_alloc(Net_Protocol protocol)
{
	Net_Socket result = { 0 };

	return(result);
}

internal Void
net_socket_free(Net_Socket socket)
{
}

internal Void
net_socket_bind(Net_Socket socket, Net_Address address)
{
}

internal Void
net_socket_accept(Net_Socket socket)
{
}

internal U64
net_socket_send(Net_Socket socket, Str8 data)
{
	U64 result = 0;
	return(result);
}

internal U64
net_socket_recieve(Net_Socket socket, U8 *buffer, U64 buffer_size)
{
	U64 result = 0;
	return(result);
}
