#ifndef __INC_LIBTHECORE_SOCKET_H__
#define __INC_LIBTHECORE_SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _WIN32
typedef int32_t socklen_t;
#else
#define INVALID_SOCKET -1
#endif

extern int32_t socket_read(socket_t desc, char* read_point, size_t space_left);
extern int32_t socket_write(socket_t desc, const char* data, size_t length);

extern int32_t socket_udp_read(socket_t desc, char* read_point, size_t space_left, struct sockaddr* from,
                           socklen_t* fromlen);
extern socket_t socket_tcp_bind(const char* ip, int32_t port);
extern socket_t socket_udp_bind(const char* ip, int32_t port);

extern socket_t socket_accept(socket_t s, struct sockaddr_in* peer);
extern void socket_close(socket_t s);
extern socket_t socket_connect(const char* host, uint16_t port);

extern void socket_nonblock(socket_t s);
extern void socket_block(socket_t s);
extern void socket_dontroute(socket_t s);
extern void socket_lingeroff(socket_t s);
extern void socket_lingeron(socket_t s);

extern void socket_sndbuf(socket_t s, uint32_t opt);
extern void socket_rcvbuf(socket_t s, uint32_t opt);

#ifdef __cplusplus
}
#endif

#endif
