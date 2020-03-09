// ServerLibrary.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//


#include "stdafx.h"
#define SOCKET_BUFSIZE (1024 * 10)

using namespace std;

typedef enum
{
	IO_READ,
	IO_WRITE,
} IO_OPERATION;

struct IO_DATA
{
	OVERLAPPED overlapped;
	WSABUF	wsaBuf;
	IO_OPERATION ioType;

	int totalBytes;
	int currentBytes;
	char buffer[SOCKET_BUFSIZE];
};

struct SOCKET_DATA
{
	SOCKET socket;
	SOCKADDR_IN addrInfo;
	char ipAddress[16];
	IO_DATA ioData;
};

DWORD WINAPI AcceptThread(LPVOID context);
DWORD WINAPI WorkerThread(LPVOID context);

SOCKET listen_socket;


int main(int argc, _TCHAR* argv[])
{
	_tprintf(L"* Server Starting\n");

	WSADATA wsa;
	SYSTEM_INFO sysInfo;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_tprintf(L"! wsa startup fail\n");
		return 1;
	}

	_tprintf(L"# Initialize network base\n");
	GetSystemInfo(&sysInfo);

	HANDLE iocp;

	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, sysInfo.dwNumberOfProcessors);
	if (!iocp)
	{
		_tprintf(L"* iocp net created io completion port. \n");
		return -1;
	}

	for (int i = 0; i < sysInfo.dwNumberOfProcessors; i++)
	{
		HANDLE thread = CreateThread(NULL, 0, WorkerThread, iocp, 0, 0);
		CloseHandle(thread);
	}

	listen_socket = WSASocket(AF_INET, SOCK_STREAM, NULL, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (listen_socket == INVALID_SOCKET)
	{
		return FALSE;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(9000);
	serverAddr.sin_family = AF_INET;

	try
	{
		int socketError = ::bind(listen_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		if (socketError == SOCKET_ERROR)
		{
			throw "! bind error";
		}

		socketError = ::listen(listen_socket, 5);
		if (socketError == SOCKET_ERROR)
		{
			throw "! listen error";
		}
		_tprintf(L"# server listening....\n");
	}

	catch (TCHAR* msg)
	{
		_tprintf(msg);
	}

	AcceptThread(iocp);

	::closesocket(listen_socket);
	WSACleanup();
	_tprintf(L"# End network base\n");
    return 0;
}

VOID CloseClient(SOCKET_DATA *socket)
{
	if (!socket)
	{
		return;
	}

	::closesocket(socket->socket);
	delete socket;
	socket = nullptr;
}

DWORD WINAPI AcceptThread(LPVOID iocpHandler)
{
	HANDLE iocp = (HANDLE)iocpHandler;

	while (true)
	{
		SOCKET acceptSocket = INVALID_SOCKET;
		SOCKADDR_IN recvAddr;
		static int addrLen = sizeof(recvAddr);
		acceptSocket = WSAAccept(listen_socket, (struct sockaddr*)&recvAddr, &addrLen, NULL, 0);
		if (acceptSocket == SOCKET_ERROR)
		{
			_tprintf(L"! Accept fail\n");
			continue;
		}

		getpeername(acceptSocket, (struct sockaddr*)&recvAddr, &addrLen);
		char clientAddr[64];
		inet_ntop(AF_INET, &(recvAddr.sin_addr), clientAddr, _countof(clientAddr));

		SOCKET_DATA *session = new SOCKET_DATA;
		if (session == NULL)
		{
			_tprintf(L"! accept alloc fail\n");
			break;
		}

		ZeroMemory(session, sizeof(SOCKET_DATA));
		session->ioData.ioType = IO_READ;
		session->ioData.totalBytes = sizeof(session->ioData.buffer);
		session->ioData.currentBytes = 0;
		session->ioData.wsaBuf.buf = session->ioData.buffer;
		session->ioData.wsaBuf.len = sizeof(session->ioData.buffer);
		iocp = CreateIoCompletionPort((HANDLE)acceptSocket, iocp, (DWORD)session, NULL);
		if (!iocp)
		{
			::closesocket(acceptSocket);
			return NULL;
		}

		session->socket = acceptSocket;
	}

	return 0;
}