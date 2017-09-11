#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#else 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#endif
#include "SockWriter.h"
#include "LocalLogWriter.h"
#include "LogWriter.h"
#include "SmartRet.h"
#include "xsystem.h"

CSockWriter::CSockWriter(void)
	: m_inited(false)
#ifdef _WIN32
	, m_fd(::closesocket, INVALID_SOCKET)
#else 
	, m_fd(::close, -1)
#endif
{
	ClosePipe();
}

CSockWriter::~CSockWriter(void)
{
}

bool CSockWriter::IsOpend()
{
	return m_fd.GetHdr() != -1;
}

bool CSockWriter::ConnectPipe()
{
	if (m_inited && m_fd.GetHdr() == -1)return false;
	m_fd				= -1;
	m_inited			= true;

#ifdef _WIN32
	CSmartHdr<SOCKET>	fd(::closesocket, INVALID_SOCKET);
#else 
	CSmartHdr<int>		fd(::close, -1);
#endif

	addrinfo					hints;
	CSmartHdr<addrinfo*, void>	peer(::freeaddrinfo);
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags		= AI_PASSIVE;
	hints.ai_family		= AF_INET;
	hints.ai_socktype	= SOCK_STREAM;
	const char*			ip("127.0.0.1");
	const char*			port("44344");
	if (0 != getaddrinfo(ip, port, &hints, &peer))
		return rvaa(false, CLocalLogWriter::Singleton().WriteLog(LogLevel_Warning, std::tid(), std::lastError(), _T("getaddressinfo() failed.")));
	fd					= socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol);
	if (fd.GetHdr() == -1)
		return rvaa(false, CLocalLogWriter::Singleton().WriteLog(LogLevel_Warning, std::tid(), std::lastError(), _T("create socket failed.")));
#ifndef _WIN32
	int					err(connect(fd, peer->ai_addr, peer->ai_addrlen));
	if (err == -1 && errno != EINPROGRESS)
		return rvaa(false, CLocalLogWriter::Singleton().WriteLog(LogLevel_Warning, std::tid(), std::lastError(), _T("connect to local socket failed.")));
	int					val(fcntl(fd, F_GETFL, 0));
	fcntl(fd, F_SETFL, val | O_NONBLOCK);
#else
	int					err(connect(fd, peer->ai_addr, (int)peer->ai_addrlen));
	if (err == -1 && errno != WSA_IO_PENDING)
		return rvaa(false, CLocalLogWriter::Singleton().WriteLog(LogLevel_Warning, std::tid(), std::lastError(), _T("connect to local socket failed.")));
	u_long				mode(1);
	ioctlsocket(fd, FIONBIO, &mode);
#endif

	m_fd				= fd;
	return true;
}

bool CSockWriter::WriteString(const xtstring& strText)
{
	if (m_fd.GetHdr() == -1)return false;

	char*				ptr((char*)strText.GetString());
	int					remain(strText.GetLength() * sizeof(TCHAR));
	do {
		int				err(send(m_fd, ptr, remain, 0));
		if (err > 0) 
		{
			remain		-= err;
			ptr			+= err;
		} 
		else 
		{
			int			nErrno(std::lastError());
#ifndef _WIN32
			if (nErrno != EWOULDBLOCK && nErrno != EAGAIN && nErrno != EINTR)return false;
#else
			if (nErrno != WSAEWOULDBLOCK)return false;
#endif
		}
	} while (remain > 0 && !std::sleep(10));

	return true;
}

void CSockWriter::ClosePipe()
{
	m_fd				= -1;
}
