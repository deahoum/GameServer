#include "stdafx.h"
#include "IOCPServer.h"
#include "IOCPSession.h"

IOCPServer::IOCPServer(ContentsProcess* contentsProcess)
	: Server(contentsProcess)
{

}

IOCPServer::~IOCPServer()
{
	::closesocket(listenSocket);
}

bool IOCPServer::createListenSocket()
{
	listenSocket = WSASocket(AF_INET, SOCK_STREAM, NULL, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET)
	{
		SErrLog(L"! listenSocket fail");
		return false;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((u_short)port);
	inet_pton(AF_INET, ip, &(serverAddr.sin_addr));

	int reUseAddr = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reUseAddr, (int)sizeof(reUseAddr));

	int retval = ::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
}