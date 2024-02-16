#pragma comment(lib, "Ws2_32.lib")

#define net_win32_assert(expr) if (!(expr)) { net_win32_print_error_message(); }

internal Void
net_win32_print_error_message(Void)
{
	int error = WSAGetLastError();
	if (error != WSAEWOULDBLOCK)
	{
		U8 buffer[1024] = {0};
		U64 size = FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &buffer, 1024, 0
		);
		log_error((CStr) buffer);
		assert(false);
	}
}

internal struct sockaddr_in
net_win32_sockaddr_in_from_address(Net_Address address)
{
	struct sockaddr_in result = {0};
	result.sin_addr.S_un.S_un_b.s_b1 = address.ip.u8[0];
	result.sin_addr.S_un.S_un_b.s_b2 = address.ip.u8[1];
	result.sin_addr.S_un.S_un_b.s_b3 = address.ip.u8[2];
	result.sin_addr.S_un.S_un_b.s_b4 = address.ip.u8[3];
	result.sin_port = u16_big_to_local_endian(address.port);
	switch (address.address_family)
	{
		case Net_AddressFamily_INET:
		{
			result.sin_family = AF_INET;
		} break;
		case Net_AddressFamily_INET6:
		{
			result.sin_family = AF_INET6;
		} break;
		case Net_AddressFamily_UNIX:
		{
			result.sin_family = AF_UNIX;
		} break;
		invalid_case;
	}
	return(result);
}

internal struct Net_Address
net_win32_address_from_sockaddr_in(struct sockaddr_in sockadd)
{
	Net_Address result = {0};
	result.ip.u8[0] = sockadd.sin_addr.S_un.S_un_b.s_b1;
	result.ip.u8[1] = sockadd.sin_addr.S_un.S_un_b.s_b2;
	result.ip.u8[2] = sockadd.sin_addr.S_un.S_un_b.s_b3;
	result.ip.u8[3] = sockadd.sin_addr.S_un.S_un_b.s_b4;
	result.port = u16_big_to_local_endian(sockadd.sin_port);
	switch (sockadd.sin_family)
	{
		case AF_INET:
		{
			result.address_family = Net_AddressFamily_INET;
		} break;
		case AF_INET6:
		{
			result.address_family = Net_AddressFamily_INET6;
		} break;
		case AF_UNIX:
		{
			result.address_family = Net_AddressFamily_UNIX;
		} break;
		invalid_case;
	}
	return(result);
}

internal Void
net_socket_init(Void)
{
	WSADATA wsa_data;
	int error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	net_win32_assert(error == 0);
}

internal Net_Socket
net_socket_alloc(Net_Protocol protocol, Net_AddressFamily address_family)
{
	Net_Socket result = {0};
	int ipproto = 0;
	int stream = 0;
	switch (protocol)
	{
		case Net_Protocol_TCP:
		{
			ipproto = IPPROTO_TCP;
			stream = SOCK_STREAM;
		} break;
		case Net_Protocol_UDP:
		{
			ipproto = IPPROTO_UDP;
			stream = SOCK_DGRAM;
		} break;
		invalid_case;
	}
	int af = 0;
	switch (address_family)
	{
		case Net_AddressFamily_INET:
		{
			af = AF_INET;
		} break;
		case Net_AddressFamily_INET6:
		{
			af = AF_INET6;
		} break;
		case Net_AddressFamily_UNIX:
		{
			af = AF_UNIX;
		} break;
		invalid_case;
	}
	SOCKET sock = socket(af, stream, ipproto);
	net_win32_assert(sock != INVALID_SOCKET);
	result.u64[0] = (U64) sock;
	return(result);
}

internal Void
net_socket_free(Net_Socket socket)
{
	SOCKET sock = (SOCKET) socket.u64;
	closesocket(sock);
}

internal Void
net_socket_bind(Net_Socket socket, Net_Address bind_address)
{
	struct sockaddr_in sockaddrin = net_win32_sockaddr_in_from_address(bind_address);
	SOCKET sock = (SOCKET) socket.u64[0];
	int error = bind(sock, (struct sockaddr*) &sockaddrin, sizeof(sockaddrin));
	net_win32_assert(error != SOCKET_ERROR);
}

internal Void
net_socket_connect(Net_Socket socket, Net_Address to)
{
	struct sockaddr_in sockaddrin = net_win32_sockaddr_in_from_address(to);
	SOCKET sock = (SOCKET) socket.u64[0];
	int error = connect(sock, (struct sockaddr *) &sockaddrin, sizeof(sockaddrin));
	net_win32_assert(error != SOCKET_ERROR);
}

internal Net_AcceptResult
net_socket_accept(Net_Socket socket)
{
	Net_AcceptResult result = {0};
	SOCKET listen_socket = (SOCKET) socket.u64[0];
	int error = listen(listen_socket, SOMAXCONN);
	net_win32_assert(error != SOCKET_ERROR);
	struct sockaddr_in connected_addr = {0};
	int connected_addrlen = sizeof(connected_addr);
	SOCKET connected_socket = accept(listen_socket, (struct sockaddr *) &connected_addr, &connected_addrlen);
	net_win32_assert(connected_socket != INVALID_SOCKET);
	if (connected_socket != INVALID_SOCKET)
	{
		result.address = net_win32_address_from_sockaddr_in(connected_addr);
		result.socket.u64[0] = (U64) connected_socket;
	}
	return(result);
}

internal Void
net_socket_send(Net_Socket socket, Str8 data)
{
	SOCKET sock = (SOCKET) socket.u64[0];
	int error = send(sock, (char *) data.data, (U32) data.size, 0);
	net_win32_assert(error != SOCKET_ERROR);
}

internal Void
net_socket_send_to(Net_Socket socket, Net_Address address, Str8 data)
{
	SOCKET sock = (SOCKET) socket.u64[0];
	struct sockaddr_in sockaddrin = net_win32_sockaddr_in_from_address(address);
	int error = sendto(sock, (char *) data.data, (int) data.size, 0, (struct sockaddr *) &sockaddrin, sizeof(sockaddrin));
	net_win32_assert(error != SOCKET_ERROR);
}

internal Net_RecieveResult
net_socket_recieve(Net_Socket connected_socket, U8 *buffer, U64 buffer_size)
{
	Net_RecieveResult result = {0};
	SOCKET sock = (SOCKET) connected_socket.u64[0];
	int bytes_recieved = recv(sock, (char *) buffer, (int) buffer_size, 0);
	net_win32_assert(bytes_recieved != SOCKET_ERROR);
	if (bytes_recieved < 0)
	{
		bytes_recieved = 0;
	}
	result.bytes_recieved = bytes_recieved;
	return(result);
}

internal Net_RecieveResult
net_socket_recieve_from(Net_Socket listen_socket, Net_Address *address, U8 *buffer, U64 buffer_size)
{
	Net_RecieveResult result = {0};
	SOCKET sock = (SOCKET) listen_socket.u64[0];
	struct sockaddr_in sockaddrin = {0};
	int from_len = sizeof(sockaddrin);
	int bytes_recieved = recvfrom(sock, (char *) buffer, (int) buffer_size, 0, (struct sockaddr *) &sockaddrin, &from_len);
	net_win32_assert(bytes_recieved != SOCKET_ERROR || bytes_recieved == WSAEWOULDBLOCK);
	result.bytes_recieved = bytes_recieved;
	*address = net_win32_address_from_sockaddr_in(sockaddrin);
	return(result);
}

internal Void
net_socket_set_blocking_mode(Net_Socket socket, B32 should_block)
{
	u_long mode = should_block ? 0 : 1;
	SOCKET sock = (SOCKET) socket.u64[0];
	ioctlsocket(sock, FIONBIO, &mode);
}
