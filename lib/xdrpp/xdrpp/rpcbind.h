// -*- C++ -*-

//! \file rpcbind.h Support for registering and (upon exit)
//! unregistering with rpcbind.

#ifndef _XDRPP_RPCBIND_H_HEADER_INCLUDED_
#define _XDRPP_RPCBIND_H_HEADER_INCLUDED_ 1

#include <xdrpp/socket.h>

namespace xdr {

std::unique_ptr<sockaddr> lookup_rpc(const char *host, std::uint32_t prog,
				     std::uint32_t vers, socklen_t *lenp,
				     int family, int sotype);

//! Create a TCP connection to an RPC server on \c host, first
//! querying \c rpcbind on \c host to determine the port.
unique_sock tcp_connect_rpc(const char *host,
			    std::uint32_t prog, std::uint32_t vers,
			    int family = AF_UNSPEC);

//! Register a service listening on \c sa with \c rpcbind.
void rpcbind_register(const sockaddr *sa, socklen_t salen, int so_type,
		      std::uint32_t prog, std::uint32_t vers);
void rpcbind_register(sock_t s, std::uint32_t prog, std::uint32_t vers);

//! Extract the port number from an RFC1833 / RFC5665 universal
//! network address (uaddr).
int parse_uaddr_port(const std::string &uaddr);

//! Create a uaddr for a local address or file descriptor.
std::string make_uaddr(const sockaddr *sa, socklen_t salen);
std::string make_uaddr(sock_t s);

}

#endif // !_XDRPP_RPCBIND_H_HEADER_INCLUDED_
