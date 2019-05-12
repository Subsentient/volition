/**
* This file is part of Volition.

* Volition is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* Volition is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with Volition.  If not, see <https://www.gnu.org/licenses/>.
**/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <signal.h>

#include <vector>
#include <map>
#include "include/common.h"
#include "include/utils.h"
#include "include/netcore.h"

#ifdef VL_IPV6
#define VL_SOCK_TYPE AF_INET6
#else
#define VL_SOCK_TYPE AF_INET
#endif //VL_IPV6

static SSL_CTX *SSLContext;

static VLString RootCert;

static const VLString PublicCertFilename = "servercert.pem";
static const VLString PrivateKeyFilename = "serverprivatekey.pem";


static bool VerifyCert(SSL *SSLDesc);

void Net::InitNetcore(const bool Server)
{
	signal(SIGPIPE, SIG_IGN); //Ignore SIGPIPE as it does virtually nothing useful but crash us.
	
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	OpenSSL_add_all_digests();
    OpenSSL_add_ssl_algorithms();

	SSL_library_init();
	
    if (Server)
    {
#if defined(LIBRESSL_VERSION_NUMBER) || defined(OPENSSL_IS_BORINGSSL) || OPENSSL_VERSION_NUMBER < 0x10100000L
		if (!(SSLContext = SSL_CTX_new(TLSv1_2_server_method())))
#else
		if (!(SSLContext = SSL_CTX_new(TLS_server_method())))
#endif
		{
			throw Errors::InitError{};
		}

		SSL_CTX_set_ecdh_auto(SSLContext, 1);

		//Load server public key.
		if (SSL_CTX_use_certificate_file(SSLContext, PublicCertFilename, SSL_FILETYPE_PEM) < 1)
		{
			throw Errors::InitError{};
		}

		//Load server private key.
		if (SSL_CTX_use_PrivateKey_file(SSLContext, PrivateKeyFilename, SSL_FILETYPE_PEM) < 1)
		{
			throw Errors::InitError{};
		}

		return;

	}
#if defined(LIBRESSL_VERSION_NUMBER) || defined(OPENSSL_IS_BORINGSSL) || OPENSSL_VERSION_NUMBER < 0x10100000L
	if (!(SSLContext = SSL_CTX_new(TLSv1_2_client_method())))
#else
	if (!(SSLContext = SSL_CTX_new(TLS_client_method())))
#endif
	{
		throw Errors::InitError{};
	}

	SSL_CTX_set_options(SSLContext, SSL_OP_NO_COMPRESSION | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
}

void Net::LoadRootCert(const VLString &Certificate)
{
	RootCert = Certificate;
}

bool Net::AcceptClient(const ServerDescriptor &ServerDesc, ClientDescriptor *const OutDescriptor, char *const OutIPAddr, const size_t IPAddrMaxLen)
{
#ifdef VL_IPV6
	struct sockaddr_in6 ClientInfo{};
	struct sockaddr_in6 Addr{};
#else
	struct sockaddr_in ClientInfo{};
	struct sockaddr_in Addr = sockaddr_in();
#endif
	socklen_t SockaddrSize = sizeof(ClientInfo);
	socklen_t AddrSize = sizeof Addr;
	
	const int ClientDesc = accept(ServerDesc.Descriptor, (struct sockaddr*)&ClientInfo, &SockaddrSize);
	
	if (ClientDesc == -1) //Accept error.
	{
#ifdef DEBUG
		puts("Net::AcceptClient(): Failed to accept.");
#endif
		return false;
	}
	
	//Get client IP.
	getpeername(ClientDesc, (sockaddr*)&Addr, &AddrSize);
	
#ifdef WIN32
	WSAAddressToString((struct sockaddr*)&ClientInfo, sizeof ClientInfo, nullptr, OutIPAddr, (DWORD*)&IPAddrMaxLen);
#else

#ifdef VL_IPV6
	inet_ntop(ServerDesc.Family, (unsigned char*)&((struct sockaddr_in6*)&Addr)->sin6_addr, OutIPAddr, IPAddrMaxLen); //Copy it into OutIPAddr.
#else
	inet_ntop(ServerDesc.Family, (unsigned char*)&Addr.sin_addr, OutIPAddr, IPAddrMaxLen); //Copy it into OutIPAddr.
#endif //VL_IPV6

#endif //WIN32

	SSL *New = SSL_new(SSLContext);
	SSL_set_fd(New, ClientDesc);

	if (SSL_accept(New) < 1)
	{
		Net::Close(ClientDesc);
		return false;
	}
	
	*OutDescriptor = New; //Give them their descriptor.

	return true;
}



Net::ServerDescriptor Net::InitServer(unsigned short PortNum)
{
	struct addrinfo BStruct = addrinfo(), *Res = NULL;
	char AsciiPort[16] =  { '\0' };
	int True = true;
	int False = false;
	int GAIExit;

	snprintf(AsciiPort, sizeof AsciiPort, "%hu", PortNum);

	BStruct.ai_family = VL_SOCK_TYPE;
	BStruct.ai_socktype = SOCK_STREAM;
	BStruct.ai_flags = AI_PASSIVE;

	if ((GAIExit = getaddrinfo(nullptr, AsciiPort, &BStruct, &Res)) != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "Failed to getaddrinfo(): %s\n", gai_strerror(GAIExit));
#endif
		return ServerDescriptor();
	}

	ServerDescriptor Desc = { (int)socket(Res->ai_family, Res->ai_socktype, Res->ai_protocol), Res->ai_family };
	

	if (Desc.Descriptor <= 0)
	{
#ifdef DEBUG
		fprintf(stderr, "Failed to open a socket on port %hu.\n", PortNum);
#endif
		return ServerDescriptor();
	}

	setsockopt(Desc.Descriptor, SOL_SOCKET, SO_REUSEADDR, (const char*)&True, sizeof(int)); //The cast shuts up Windows compilation.

	(void)False;
#ifdef VL_IPV6
	setsockopt(Desc.Descriptor, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&False, sizeof(int)); //The cast shuts up Windows compilation.
#endif
	if (bind(Desc.Descriptor, Res->ai_addr, Res->ai_addrlen) == -1)
	{
		perror("bind()");
		Net::Close(Desc.Descriptor);
		return ServerDescriptor();
	}

	if (listen(Desc.Descriptor, INT_MAX) == -1)
	{
		perror("listen()");
		Net::Close(Desc.Descriptor);
		return ServerDescriptor();
	}

	freeaddrinfo(Res);
	return Desc;
}

bool Net::Connect(const char *InHost, unsigned short PortNum, ClientDescriptor *const OutDescriptor)
{
	const VLString &FailMsg = "Failed to establish a connection to the server:";
	struct addrinfo Hints{}, *Res = nullptr;
	
	Hints.ai_family = AF_UNSPEC;
	Hints.ai_socktype = SOCK_STREAM;
	
	char AsciiPort[256]{};
	snprintf(AsciiPort, sizeof AsciiPort, "%hu", PortNum);
	
	if (getaddrinfo(InHost, AsciiPort, &Hints, &Res) != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "Failed to resolve hostname \"%s\".\n", InHost);
#endif
		return 0;
	}
	
	int IntDesc = 0;
	if ((IntDesc = socket(Res->ai_family, Res->ai_socktype, Res->ai_protocol)) == -1)
	{
		perror(FailMsg);
		return false;
	}

	
	if (connect(IntDesc, Res->ai_addr, Res->ai_addrlen) != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
#endif
		Net::Close(IntDesc);
		return false;
	}

	SSL *New = SSL_new(SSLContext);
	SSL_set_fd(New, IntDesc);

	if (SSL_connect(New) != 1)
	{
#ifdef DEBUG
		fputs("SSL handshake failed!\n", stderr);
#endif
		Net::Close(IntDesc);
		return false;
	}

	if (!VerifyCert(New))
	{
		Net::Close(New);
		return false;
	}
	
	*OutDescriptor = { New };
	return true;
}

static bool VerifyCert(SSL *SSLDesc)
{
	X509 *ServerCert = SSL_get_peer_certificate(SSLDesc);

	if (!ServerCert)
	{
#ifdef DEBUG
		fputs("Failed to get server SSL certificate!\n", stderr);
#endif
		return false;
	}
	

	BIO *RootCertBIO = BIO_new_mem_buf((void*)+RootCert, -1);

	X509 *RCert = PEM_read_bio_X509(RootCertBIO, nullptr, 0, nullptr);

	if (!RCert)
	{
		throw Net::Errors::InitError{};
	}

	EVP_PKEY *RootPubKey = X509_get_pubkey(RCert);
	
	const bool Verified = X509_verify(ServerCert, RootPubKey);

	EVP_PKEY_free(RootPubKey);
	BIO_free_all(RootCertBIO);
	X509_free(ServerCert);
	X509_free(RCert);
	
	if (!Verified)
	{
#ifdef DEBUG
		fputs("Server's X509 certificate is invalid! Refusing to connect.\n", stderr);
#endif
		return false;
	}

	return true;
}
	
bool Net::Write(const ClientDescriptor &Descriptor, const void *const Bytes, const uint64_t ToTransfer,
				Net::NetRWStatusForEachFunc StatusFunc, void *PassAlongWith)
{
	SSL *Desc = static_cast<SSL*>(Descriptor.Internal);
	
	uint64_t Transferred = 0, TotalTransferred = 0;

	//Initialize status reporting with a zero value.
	if (StatusFunc) StatusFunc(0, ToTransfer, PassAlongWith);

	do
	{
		//Force frequent iterations so status functions actually work.
		const size_t ChunkSize = ToTransfer - TotalTransferred > NET_MAX_CHUNK_SIZE ? NET_MAX_CHUNK_SIZE : ToTransfer - TotalTransferred;

		Transferred = SSL_write(Desc, (const char*)Bytes + TotalTransferred, ChunkSize);
		
		if (Transferred == (uint64_t)-1) /*This is ugly I know, but it's converted implicitly, so shut up.*/
		{
			throw Net::Errors::IOError();
		}
		
		TotalTransferred += Transferred;
		
		if (StatusFunc) StatusFunc(TotalTransferred, ToTransfer, PassAlongWith);
	} while (ToTransfer > TotalTransferred);
	
	//Saying we're done.
	if (StatusFunc) StatusFunc(-1ll, ToTransfer, PassAlongWith);
	
	return true;
}

bool Net::HasRealDataToRead(const ClientDescriptor &Descriptor)
{
	uint64_t QWord;
	
	return SSL_peek((SSL*)Descriptor.Internal, &QWord, sizeof QWord) > 0;
}

bool Net::Read(const ClientDescriptor &Descriptor, void *const OutStream_, const uint64_t MaxLength,
				Net::NetRWStatusForEachFunc StatusFunc, void *PassAlongWith)
{
	SSL *Desc = static_cast<SSL*>(Descriptor.Internal);
	
	unsigned char *OutStream = static_cast<unsigned char*>(OutStream_);
	unsigned TotalReceived = 0;
	int Received = 0;
	
	if (StatusFunc) StatusFunc(0, MaxLength, PassAlongWith); //Start up status reporting with a zero value.

	do
	{
		//We have to force frequent iterations to get decent status reports.
		const size_t ChunkSize = MaxLength - TotalReceived > NET_MAX_CHUNK_SIZE ? NET_MAX_CHUNK_SIZE : MaxLength - TotalReceived;
		
		Received = SSL_read(Desc, (char*)OutStream, ChunkSize);
		if (Received > 0)
		{
			OutStream += Received;
			TotalReceived += Received;
			if (StatusFunc) StatusFunc(TotalReceived, MaxLength, PassAlongWith);
		}
	} while (TotalReceived < MaxLength && Received > 0);
	
	if (StatusFunc) StatusFunc(-1ll, MaxLength, PassAlongWith);
	
	if (Received <= 0) return false;
	
	return true;
}


bool Net::Close(const int Descriptor)
{
#ifdef WIN32
	return !closesocket(Descriptor);
#else
	return !close(Descriptor);
#endif
}

bool Net::Close(const ClientDescriptor &Descriptor)
{
	SSL *Desc = static_cast<SSL*>(Descriptor.Internal);
	
	if (!Desc) return false;

	const int IntDesc = SSL_get_fd(Desc);
	
	SSL_shutdown(Desc);
	SSL_free(Desc);
	
	return Net::Close(IntDesc);
}

int Net::ToRawDescriptor(const ClientDescriptor &Desc)
{
	return SSL_get_fd(static_cast<SSL*>(Desc.Internal));
}
