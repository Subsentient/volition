#ifndef __VL_LUAFFI_PRELUDE__
#define __VL_LUAFFI_PRELUDE__

#include "../libvolition/include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#endif

static constexpr const char *const LuaFFI_StaticPrelude = 
{
	"typedef uint32_t socklen_t;"
	"typedef uint16_t sa_family_t;"
	"struct sockaddr {"
	"	sa_family_t sa_family;"
	"	char sa_data[14];"
	"};"
	"struct addrinfo {"
	"	int ai_flags;"
	"	int ai_family;"
	"	int ai_socktype;"
	"	int ai_protocol;"
	"	socklen_t ai_addrlen;"
	"	struct sockaddr *ai_addr;"
	"	char *ai_canonname;"
	"	struct addrinfo *ai_next;"
	"};"
	"int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);"
	"int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);"
	"int socket(const int domain, const int type, const int protocol);"
	"int connect(const int sockfd, const struct sockaddr *addr, const socklen_t addrlen);"
	"int puts(const char*);"
	"void *fopen(const char*, const char*);"
	"int fclose(void *);"
	"void rewind(void *desc);"
	"size_t fread(void *ptr, const size_t size, const size_t nmemb, void *desc);"
	"int fseek(void *desc, long offset, int whence);"
	"size_t fwrite(const void *ptr, const size_t size, const size_t nmemb, void *desc);"
	"int printf(const char*, ...);"
	"size_t strlen(const char*);"
	"const char *strdup(const char*);"
	"void *malloc(size_t);"
	"void free(void *);"
	"int listen(int sock, int);"
	"int getsockname(const int sockfd, struct sockaddr *addr, socklen_t *addrlen);"
	"int accept(const int sockfd, struct sockaddr *addr, socklen_t *addrlen);"
	"void *calloc(size_t, size_t);"
	"void *realloc(void *, size_t);"
	"int snprintf(char *str, size_t size, const char *format, ...);"
	"ssize_t recv(int sockfd, void *buf, size_t len, int flags);"
	"ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);"
	"ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);"
	"ssize_t read(int fd, void *buf, size_t count);"
	"ssize_t write(int fd, const void *buf, size_t count);"
	"ssize_t send(int sockfd, const void *buf, size_t len, int flags);"
	"ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);"
	"ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);"
	"int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);"
	"int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);"
	"void freeaddrinfo(struct addrinfo *res);"
	"void *memcpy(void *dest, const void *src, const size_t n);"
	"void *memmove(void *dest, const void *src, const size_t n);"
#ifdef LINUX
#endif //LINUX
#ifdef WIN32
	"int closesocket(int fd);"
	"int ioctlsocket(int fd, long, unsigned long*);"
#else
	"int close(int);"
	"int ioctl(int fd, unsigned long request, ...);"
	"int open(const char *path, int oflag, ...);"
	"int fcntl(int, int, ...);"
#endif //WIN32
};

static const std::map<const char*, int64_t> LuaFFI_IntegerDefs
{
#ifdef WIN32
	{ TOKEN_VALKEYPAIR(FIONBIO) },
#else
	{ TOKEN_VALKEYPAIR(AF_UNIX) },
	{ TOKEN_VALKEYPAIR(O_NONBLOCK) },
#endif //WIN32
	{ TOKEN_VALKEYPAIR(IPPROTO_IPV6) },
	{ TOKEN_VALKEYPAIR(IPV6_V6ONLY) },
	{ TOKEN_VALKEYPAIR(AF_UNSPEC) },
	{ TOKEN_VALKEYPAIR(AF_INET) },
	{ TOKEN_VALKEYPAIR(AF_INET6) },
	{ TOKEN_VALKEYPAIR(SOCK_STREAM) },
	{ TOKEN_VALKEYPAIR(SOCK_DGRAM) },
	{ TOKEN_VALKEYPAIR(SOCK_RAW) },
	{ TOKEN_VALKEYPAIR(SOL_SOCKET) },
	{ TOKEN_VALKEYPAIR(SO_REUSEADDR) },
	{ TOKEN_VALKEYPAIR(SO_KEEPALIVE) },
};
#endif //__VL_LUAFFI_PRELUDE__
