#include <fcntl.h>

#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/un.h>

internal struct sockaddr *
net_linux_sockaddr_from_address(Arena *arena, Net_Address address)
{
	struct sockaddr *result = 0;

	switch (address.address_family)
	{
		case Net_AddressFamily_INET:
		{
			struct sockaddr_in *inet_address = push_struct(arena, struct sockaddr_in);
			inet_address->sin_family      = AF_INET;
			inet_address->sin_port        = u16_big_to_local_endian(address.port);
			inet_address->sin_addr.s_addr = address.ip.u32[0];

			result = (struct sockaddr *) inet_address;
		} break;
		case Net_AddressFamily_INET6:
		{
			assert_not_implemented();
		} break;
		case Net_AddressFamily_UNIX:
		{
			assert_not_implemented();
		} break;
		invalid_case;
	}

	return(result);
}

internal Net_Address
net_linux_address_from_sockaddr(struct sockaddr *address, socklen_t address_length)
{
	Net_Address result = { 0 };

	if (address_length >= sizeof(struct sockaddr_in) && address->sa_family == AF_INET)
	{
		struct sockaddr_in *inet_address = (struct sockaddr_in *) address;
		result.port           = u16_big_to_local_endian(inet_address->sin_port);
		result.ip.u32[0]      = inet_address->sin_addr.s_addr;
		result.address_family = Net_AddressFamily_INET;
	}
	else if (address_length >= sizeof(struct sockaddr_in6) && address->sa_family == AF_INET6)
	{
		assert_not_implemented();
	}
	else if (address_length >= sizeof(struct sockaddr_un) && address->sa_family == AF_UNIX)
	{
		assert_not_implemented();
	}

	return result;
}

internal socklen_t
net_linux_socklen_from_sockaddr(struct sockaddr *socket)
{
	socklen_t result = 0;

	switch (socket->sa_family)
	{
		case AF_INET:  result = sizeof(struct sockaddr_in);  break;
		case AF_INET6: result = sizeof(struct sockaddr_in6); break;
		case AF_UNIX:  result = sizeof(struct sockaddr_un);  break;
		invalid_case;
	}

	return(result);
}

internal Void
net_socket_init(Void)
{
	// NOTE(simon): Nothing to do.
}

internal Net_Socket
net_socket_alloc(Net_Protocol protocol, Net_AddressFamily address_family)
{
	int socket_type     = 0;
	int socket_protocol = 0;
	switch (protocol)
	{
		case Net_Protocol_TCP:
		{
			socket_protocol = IPPROTO_TCP;
			socket_type    = SOCK_STREAM;
		} break;
		case Net_Protocol_UDP:
		{
			socket_protocol = IPPROTO_UDP;
			socket_type     = SOCK_DGRAM;
		} break;
		invalid_case;
	}

	int socket_domain = 0;
	switch (address_family)
	{
		case Net_AddressFamily_INET:
		{
			socket_domain = AF_INET;
		} break;
		case Net_AddressFamily_INET6:
		{
			socket_domain = AF_INET6;
		} break;
		case Net_AddressFamily_UNIX:
		{
			socket_domain = AF_UNIX;
		} break;
		invalid_case;
	}

	int linux_socket = socket(socket_domain, socket_type, socket_protocol);

	// TODO(simon): Handle errors properly.
	assert(linux_socket != -1);

	Net_Socket result = { 0 };
	result.u64[0] = (U64) linux_socket;

	return(result);
}

internal Void
net_socket_free(Net_Socket socket)
{
	int linux_socket = (int) socket.u64[0];
	close(linux_socket);
}

internal Void
net_socket_bind(Net_Socket socket, Net_Address address)
{
	int linux_socket = (int) socket.u64[0];

	arena_scratch(0, 0)
	{
		struct sockaddr *socket_address = net_linux_sockaddr_from_address(scratch, address);
		socklen_t socket_length = net_linux_socklen_from_sockaddr(socket_address);
		int success = bind(linux_socket, socket_address, socket_length);
		if (success == -1) {
			printf("%s\n", strerror(errno));
		}
		assert(success != -1);
	}
}

internal Void
net_socket_connect(Net_Socket socket, Net_Address to)
{
	int linux_socket = (int) socket.u64[0];

	arena_scratch(0, 0)
	{
		struct sockaddr *socket_address = net_linux_sockaddr_from_address(scratch, to);
		socklen_t socket_length = net_linux_socklen_from_sockaddr(socket_address);
		int success = connect(linux_socket, socket_address, socket_length);
		assert(success != -1);
	}
}

// TODO(simon): This should not be called for UDP sockets according to man listen.2 and man accept.2
internal Net_AcceptResult
net_socket_accept(Net_Socket socket)
{
	Net_AcceptResult result = { 0 };

	int linux_socket = (int) socket.u64[0];

	// TODO(simon): What should the backlog value be?
	listen(linux_socket, 128);

	arena_scratch(0, 0)
	{
		// TODO(simon): Use the size of the largest socket type.
		socklen_t       client_address_length = sizeof(struct sockaddr_in);
		struct sockaddr *client_address       = (struct sockaddr *) arena_push(scratch, client_address_length);
		int client_socket = accept(linux_socket, client_address, &client_address_length);
		result.succeeded = client_socket != -1;

		result.address = net_linux_address_from_sockaddr(client_address, client_address_length);
		result.socket.u64[0] = (U64) client_socket;
	}

	return(result);
}

internal Void
net_socket_send(Net_Socket socket, Str8 data)
{
	int linux_socket = (int) socket.u64[0];
	send(linux_socket, data.data, (size_t) data.size, 0);
}

internal Void
net_socket_send_to(Net_Socket socket, Net_Address address, Str8 data)
{
	int linux_socket = (int) socket.u64[0];

	arena_scratch(0, 0)
	{
		struct sockaddr *socket_address = net_linux_sockaddr_from_address(scratch, address);
		socklen_t socket_length = net_linux_socklen_from_sockaddr(socket_address);
		sendto(linux_socket, data.data, (size_t) data.size, 0, socket_address, socket_length);
	}
}

internal Net_RecieveResult
net_socket_recieve(Net_Socket socket, U8 *buffer, U64 buffer_size)
{
	int linux_socket = (int) socket.u64[0];

	ssize_t bytes_recieved = recv(linux_socket, buffer, (size_t) buffer_size, 0);
	if (bytes_recieved == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
	{
		bytes_recieved = 0;
	}

	Net_RecieveResult result = { 0 };
	result.bytes_recieved = (U64) bytes_recieved;
	return(result);
}

internal Net_RecieveResult
net_socket_recieve_from(Net_Socket socket, Net_Address *address, U8 *buffer, U64 buffer_size)
{
	Net_RecieveResult result = { 0 };

	int linux_socket = (int) socket.u64[0];

	arena_scratch(0, 0)
	{
		// TODO(simon): Use the size of the largest socket type.
		socklen_t socket_length = sizeof(struct sockaddr_in);
		struct sockaddr *socket_address = (struct sockaddr *) arena_push(scratch, socket_length);
		ssize_t bytes_recieved = recvfrom(linux_socket, buffer, (size_t) buffer_size, 0, socket_address, &socket_length);
		if (bytes_recieved == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			bytes_recieved = 0;
		}

		*address = net_linux_address_from_sockaddr(socket_address, socket_length);
		result.bytes_recieved = (U64) bytes_recieved;
	}

	return(result);
}

internal Void
net_socket_set_blocking_mode(Net_Socket socket, B32 should_block)
{
	int linux_socket = (int) socket.u64[0];
	int old_flags = fcntl(linux_socket, F_GETFL);
	int flags = (old_flags & ~O_NONBLOCK) | (should_block ? 0 : O_NONBLOCK);
	fcntl(linux_socket, F_SETFL, flags);
}
