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
	if (retval == SOCKET_ERROR)
	{
		SErrLog(L"! bindf fail");
		return false;
	}

	array<char, SIZE_64> ip;
	inet_ntop(AF_INET, &(serverAddr.sin_addr), ip.data(), ip.size());
	SLog(L"* server listen socket created, ip: %S, port: %d", ip.data(), port);
	return true;
}

bool IOCPServer::run()
{
	if (MAX_IOCP_THREAD < workerThreadCount)
	{
		SErrLog(L"! workerThread limit[%d], but config setting [%d]", MAX_IOCP_THREAD, workerThreadCount);
		return false;
	}
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, workerThreadCount);
	if (iocp == nullptr)
	{
		return false;
	}

	this->createListenSocket();

	acceptThread = MAKE_THREAD(IOCPServer, acceptThread);
	for (int i = 0; i < workerThreadCount; i++)
	{
		workerThread[i] = MAKE_THREAD(IOCPServer, workerThread);
	}

	this->status = SERVER_READY;
	while (!shutdown)
	{
		wstring cmdLine;
		std::getline(std::wcin, cmdLine);

		SLog(L"Input was: %s", cmdLine.c_str());
		SessionManager::getInstance().runCommand(cmdLine);
	}

	return true;
}

SOCKET IOCPServer::ListenSocket()
{
	return listenSocket;
}

HANDLE IOCPServer::Iocp()
{
	return iocp;
}

void IOCPServer::onAccept(SOCKET accepter, SOCKADDR_IN addrInfo)
{
	IOCPSession *session = new IOCPSession();
	if (session == nullptr)
	{
		SLog(L"! accept session create fail");
		return;
	}

	if (!session->onAccept(accepter, addrInfo))
	{
		SAFE_DELETE(session);
		return;
	}

	if (!SessionManager::getInstance().addSession(session))
	{
		SAFE_DELETE(session);
		return;
	}

	session->ioData[IO_READ].clear();

	HANDLE handle = CreateIoCompletionPort((HANDLE)accepter, this->Iocp(), (ULONG_PTR)&(*session), NULL);

	if (!handle)
	{
		SAFE_DELETE(session);
		return;
	}

	SLog(L"* client accept from [%s]", session->clientAddress().c_str());
	session->recvStandBy();
}

DWORD WINAPI IOCPServer::AcceptThread(LPVOID serverPtr)
{
	IOCPServer *server = (IOCPServer*)serverPtr;

	while (!shutdown)
	{
		SOCKET acceptSocket = INVALID_SOCKET;
		SOCKADDR_IN recvAddr;
		static int addrLen = sizeof(recvAddr);
		acceptSocket = WSAAccept(server->ListenSocket(), (struct sockaddr*)&recvAddr, &addrLen, NULL, 0);
		if (acceptSocket == SOCKET_ERROR)
		{
			if (!server->Status() == SERVER_STOP)
			{
				SLog(L"! Accept fail");
				break;
			}
		}

		if (server->Status() != SERVER_READY)
		{
			break;
		}
	}

	return 0;
}

DWORD WINAPI IOCPServer::WorkerThread(LPVOID serverPtr)
{
	IOCPServer* server = (IOCPServer*)serverPtr;

	while (!shutdown)
	{
		IoData		*ioData = nullptr;
		IOCPSession *session = nullptr;
		DWORD transferSize;

		BOOL ret = GetQueuedCompletionStatus(server->Iocp(), &transferSize, (PULONG_PTR)&session, (LPOVERLAPPED*)&ioData, INFINITE);
		if (!ret)
		{
			continue;
		}

		if (session == nullptr)
		{
			SLog(L"! socket data broken");
			return 0;
		}

		if (transferSize == 0)
		{
			SLog(L"* close by client[%d][%s]", session->Id(), session->ClientAddress().c_str());
			SessionManager.CloseSession(session);
			continue;
		}

		switch (ioData->Type())
		{
		case IO_WRITE:
			session->OnSend((size_t)transferSize);
			continue;
		case IO_READ:
		{
			Package* package = session->OnRecv((size_t)transferSize);
			if (package != nullptr)
			{
				server->putPackage(package);
			}
		}
		continue;
		case IO_ERROR:
			SLog(L"* close by client error [%d][%s]", session->Id(), session->ClientAddress().c_str());
			SessionManager.CloseSession(session);
			continue;
		}
	}
	return 0;
}