#pragma once
#include "stdafx.h"

class Session;
class SessionManager;
class Package;

typedef enum
{
	IO_READ = 0,
	IO_WRITE,
	IO_ERROR,
} IO_OPERATION;

#define IO_DATA_MAX (2)

class IoData
{
	OVERLAPPED overlapped;
	IO_OPERATION ioType;
	size_t totalBytes;
	size_t currentBytes;
	array<char, SOCKET_BUF_SIZE> buffer;

public:
	IoData();
	void Clear();

	bool NeedMoreIO(size_t transferSize);
	int32_t SetupTotalBytes();
	size_t TotalBytes();

	IO_OPERATION &Type();
	void SetType(IO_OPERATION type);

	WSABUF WsaBuf();
	char* Data();
	bool SetData(Stream &stream);
	LPWSAOVERLAPPED Overlapped();
};

class IOCPSession : public Session
{
public:
	array<IoData, IO_DATA_MAX> ioData;

private:
	void Initialize();
	void CheckError(DWORD ret);
	void Recv(WSABUF wsaBuf);
	bool IsRecving(size_t transferSize);
	void Send(WSABUF wsaBuf);

public:
	IOCPSession();

	void OnSend(size_t transferSize);
	void SendPacket(Packet* packet);

	Package* OnRecv(size_t transferSize);
	void RecvStandBy();
};