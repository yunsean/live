#ifdef _WIN32
#include <Winsock2.h>
#include <WS2tcpip.h>
#endif
#include "xsystem.h"
#include "net.h"
#include "file.h"
#include "xsystem.h"
#include "ConsoleUI.h"
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

static evutil_socket_t make_tcp_socket() {
	int on = 1;
	evutil_socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	evutil_make_socket_nonblocking(sock);
#ifdef WIN32  
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&on, sizeof(on));
#else  
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
#endif  
	return sock;
}

evutil_socket_t sock = 0;
struct event* ev_write = NULL;
struct event* ev_read = NULL;
enum {
	OPTIONS = 0, DESCRIBE = 1, SETUP = 2, PLAY = 3, PLAYING};
int stage = OPTIONS;
void action() {
	switch (stage++) {
	case OPTIONS: {
		const char* options = "OPTIONS rtsp://101.204.30.107:30030 RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\n\r\n";
		send(sock, options, strlen(options), 0);
		printf("%s\n", options);
		break;
	}
	case DESCRIBE: {
		const char* options = "DESCRIBE rtsp://101.204.30.107:30030 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\nAccept: application/sdp\r\n\r\n";
		send(sock, options, strlen(options), 0);
		printf("%s\n", options);
		break;
	}
	case SETUP: {
		const char* options = "SETUP rtsp://101.204.30.107:30030/trackID=1 RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\nTransport: RTP/AVP/TCP; unicast; interleaved=0-1\r\n\r\n";
		send(sock, options, strlen(options), 0);
		printf("%s\n", options);
		break;
	}
	case PLAY: {
		const char* options = "PLAY rtsp://101.204.30.107:30030/ RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\nSession: 1272981758\r\nRange: npt = 0.000 -\r\n\r\n";
		send(sock, options, strlen(options), 0);
		printf("%s\n", options);
		break;
	}
	default:
		break;
	}
}
void on_read(evutil_socket_t sock, short flags, void * args) {
	while (true) {
		char buf[1025];
		int ret = recv(sock, buf, 1024, 0);
		if (ret > 0) {
			if (stage <= PLAYING) {
				buf[ret] = 0;
				printf("recv:%s\n", buf);
			}
			else {
				printf("recv %d bytes\n", ret);
			}
		}
		else if (ret <= 0) {
			break;
		}
	}
	action();
}
void on_write(evutil_socket_t sock, short flags, void * args) {
	action();
	event_add(ev_write, NULL);
}

int main2() {
	init_sockets();
	CConsoleUI::Singleton().Initialize(true);
	cpj(w"Begin\n");

	sock = make_tcp_socket();
	struct sockaddr_in serverAddr;
	struct timeval tv = { 10, 0 };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(30030);
#ifdef WIN32  
	serverAddr.sin_addr.S_un.S_addr = inet_addr("101.204.30.107");
#else  
	serverAddr.sin_addr.s_addr = inet_addr("101.204.30.107");
#endif  
	memset(serverAddr.sin_zero, 0x00, 8);
	connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	struct event ev_accept;
	event_base* base = event_base_new();
	ev_write = event_new(base, sock, EV_READ | EV_PERSIST, on_read, NULL);
	ev_read = event_new(base, sock, EV_WRITE, on_write, NULL);
	event_add(ev_read, &tv);
	event_base_dispatch(base);
}