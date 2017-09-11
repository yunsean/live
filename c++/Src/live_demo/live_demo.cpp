//
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
#include "IOAccess.h"

void on_read(struct bufferevent *bev, void *ctx) {
	//evutil_socket_t client_fd = (evutil_socket_t)(uint64_t)ctx;
	char data[1024];
	while (true) {
		int read = bufferevent_read(bev, data, 1024);
		if (read <= 0) break;
		// send(client_fd, data, read, 1);
 		int writen = bufferevent_write(bev, data, read);
 		if (writen != 0) {
 			cpe(w"echo failed");
 		}
	}
}

void on_write(struct bufferevent *bev, void *ctx) {
	cpm(w"write data to socket[%d]", ctx);
}

void on_error(struct bufferevent *bev, short what, void *ctx) {
	cpw(w"socket [%d] closed with %d", ctx, what);
}

void on_accept(evutil_socket_t fd, short ev, void* arg) {
	evutil_socket_t client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0) {
		cpe(w"accept socket failed.");
		return;
	}
	if (setnonblock(client_fd) < 0) {
		cpw(w"set non block socket failed.");
	}
	event_base* evbase = reinterpret_cast<event_base*>(arg);
	bufferevent* buf_ev = bufferevent_socket_new(evbase, client_fd, 0);
	bufferevent_setcb(buf_ev, on_read, on_write, on_error, (void*)client_fd);
	bufferevent_enable(buf_ev, EV_READ);
	cpj(w"accepted a socket [%d]", client_fd);
}

#ifdef _WIN32
#include <Winternl.h>
#endif
#include "zlib.h"
#define CHUNK 0x4000
#define CALL_ZLIB(x) {                                                  \
        int status;                                                     \
        status = x;                                                     \
        if (status < 0) {                                               \
            fprintf (stderr,                                            \
                     "%s:%d: %s returned a bad status of %d.\n",        \
                     __FILE__, __LINE__, #x, status);                   \
            exit (EXIT_FAILURE);                                        \
        }                                                               \
    }
#define windowBits 15
#define ENABLE_ZLIB_GZIP 32
std::string testzlib() {
	const char * file_name = "test.gz";
	FILE * file;
	z_stream strm = { 0 };
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = in;
	strm.avail_in = 0;
	int result = inflateInit2(&strm, windowBits | ENABLE_ZLIB_GZIP);
#ifdef _WIN32
	file = fopen("T:\\0000000000200000000000000120266.gz", "rb");
#else 
	file = fopen("/mnt/d/0000000000200000000000000120266.gz", "rb");
#endif
	std::string rs;
	while (1) {
		int bytes_read;
		bytes_read = fread(in, sizeof(char), sizeof(in), file);
		strm.avail_in = bytes_read;
		do {
			unsigned have;
			strm.avail_out = CHUNK;
			strm.next_out = out;
			CALL_ZLIB(inflate(&strm, Z_NO_FLUSH));
			have = CHUNK - strm.avail_out;
			rs.append((char*)out, have);
		} while (strm.avail_out == 0);
		if (feof(file)) {
			inflateEnd(&strm);
			break;
		}
	}

	result = 0;
	return rs;
}
int main()
{
	std::string hello = "1112";
	xtstring dd = xtstring::convert(testzlib(), CodePage_GB2312);
	cpm(_T("%s"), dd.c_str());
	//testzlib();
	return 0;
	

// 	SYSTEM_INFO si;
// 	GetSystemInfo(&si);
// 	MEMORYSTATUSEX statex;
// 	statex.dwLength = sizeof(statex);
// 	GlobalMemoryStatusEx(&statex);
	
// 	SYSTEM_PROCESS_INFORMATION* spi = new SYSTEM_PROCESS_INFORMATION[100];
// 	ULONG len(0);
// 	HMODULE hModule(::LoadLibrary(_T("Ntdll.dll")));
// 	typedef NTSTATUS (*FNNtQueryInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
// 	FNNtQueryInformationProcess proc((FNNtQueryInformationProcess)GetProcAddress(hModule, "NtQueryInformationProcess"));
// 	HANDLE hProcess;
// 	PROCESS_BASIC_INFORMATION pbi = { 0 };
// 	DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &hProcess, 0, FALSE, DUPLICATE_SAME_ACCESS);
// 	NTSTATUS resule(proc(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &len));
// 	typedef NTSTATUS (*FNNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
// 	FNNtQuerySystemInformation proc1 = (FNNtQuerySystemInformation)GetProcAddress(hModule, "NtQuerySystemInformation");
// 	resule = proc1(SystemProcessInformation, NULL, 0, &len);

	TCHAR path[1024];
	std::exefile(path);
	cpj(path);
	cpm(_T("%s"), file::exeuteFilePath().c_str());
	IVpfReader* reader(CreateVpfReader());
	IVpfWriter* writer(CreateVpfWriter());
#ifdef _WIN32
	reader->Open(_T("D:\\vod\\1.vpf"));
	writer->OpenFile(_T("D:\\vod\\2.vpf"));
#else 
	reader->Open("/mnt/d/vod/1.vpf");
	writer->OpenFile("/mnt/d/vod/2.vpf");
#endif
	uint32_t begin;
	uint32_t end;
	uint8_t* lpData;
	int szData;
	reader->GetInfo(begin, end);
	reader->GetHeader(lpData, szData);
	writer->WriteHead(lpData, szData);
	cpj(_T("begin=%u, end=%u"), begin, end);
	uint8_t byType;
	uint32_t uTimecode;
	while (!reader->IsEof()) {
		if (!reader->ReadBody(byType, lpData, szData, uTimecode)) {
			cpe(_T("Error: %s"), reader->GetLastErrorDesc());
			break;
		}
		writer->WriteData(byType, lpData, szData, uTimecode);
		cpm(_T("type=%d, size=%d, tc=%u\n"), byType, szData, uTimecode);
	}
	bool valid;
	uint64_t utcBegin;
	uint64_t utcEnd;
	LPCTSTR file(writer->GetInfo(utcBegin, utcEnd));
	writer->CloseFile(&valid);
	cpw(_T("valid=%s, file=%s, begin=%lld, end=%lld"), valid ? _T("true") : _T("false"), file, utcBegin, utcEnd);
	// 	findFiles(_T("*"), [](LPCTSTR file, bool directory) {
	// 		cpj(file);
	// 	}, true);
	// 	return 0;
}
int main1() {
	init_sockets();
	CConsoleUI::Singleton().Initialize(true);
	cpj(w"Hello\n");

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8082);
	int status = inet_aton("0.0.0.0", &server_addr.sin_addr);
	if (status == 0)  {
		errno = EINVAL;
		return -1;
	}
	evutil_socket_t sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		cpe(w"open socket failed.");
		return -1;
	}
	int opt = 1;
	status = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
	if (status != 0) {
		::closesocket(sockfd);
		cpe(w"set socket option failed: %d", errno);
		return -1;
	}
	status = ::bind(sockfd, (const sockaddr*)&server_addr, sizeof(server_addr));
	if (status != 0) {
		::closesocket(sockfd);
		cpe(w"bind socket failed: %d", errno);
		return -1;
	}
	status = ::listen(sockfd, 0);
	if (status == -1) {
		int save_errno = errno;
		::closesocket(sockfd);
		errno = save_errno;  
		return -1;
	}

	status = setnonblock(sockfd);
	if (status == -1) {
		::closesocket(sockfd);
		cpe(w"set non block failed: %d", errno);
		return -1;
	}

	struct event ev_accept;
	event_base* evbase = event_base_new();
	event_assign(&ev_accept, evbase, sockfd, EV_READ | EV_PERSIST, on_accept, evbase);
	event_add(&ev_accept, NULL);
	event_base_dispatch(evbase);

    return 0;
}

