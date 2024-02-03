internal Void
net_socket_init(Void)
{
}

internal Net_Socket
net_socket_alloc(Net_Protocol protocol, Net_AddressFamily address_family)
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
net_socket_connect(Net_Socket socket, Net_Address to)
{
}

internal Net_AcceptResult
net_socket_accept(Net_Socket socket)
{
	Net_AcceptResult result = { 0 };
	return(result);
}

internal Void
net_socket_send(Net_Socket socket, Str8 data)
{
}

internal Void
net_socket_send_to(Net_Socket socket, Net_Address address, Str8 data)
{
}

internal Net_RecieveResult
net_socket_recieve(Net_Socket socket, U8 *buffer, U64 buffer_size)
{
	Net_RecieveResult result = { 0 };
	return(result);
}

internal Net_RecieveResult
net_socket_recieve_from(Net_Socket socket, Net_Address address, U8 *buffer, U64 buffer_size)
{
	Net_RecieveResult result = { 0 };
	return(result);
}
