#include "stdafx.h"
#include "../Session.h"
#include "IOCPSession.h"
#include "../SessionManager.h"
#include "../Packet/PacketAnalyzer.h"

IoData::IoData()
{
	ZeroMemory(&overlapped, sizeof(overlapped));
	ioType = IO_ERROR;

	this->Clear();
}

void IoData::Clear()
{
	buffer.fill(0);
	totalBytes = 0;
	currentBytes = 0;
}

bool IoData::NeedMoreIO(size_t transferSize)
{
	currentBytes += transferSize;
	if (currentBytes < totalBytes)
	{
		return true;
	}

	return false;
}

int32_t IoData::SetupTotalBytes()
{
	packet_size_t offset = 0;
	packet_size_t packetLen[1] = { 1, };
	if (totalBytes == 0)
	{
		memcpy_s((void*)packetLen, sizeof(packetLen), (void*)buffer.data(), sizeof(packetLen));
		PacketObfuscation::getInstance().gecodingHeader((Byte*)&packetLen, sizeof(packetLen));

		totalBytes = (size_t)packetLen[0];
	}

	offset += sizeof(packetLen);

	return offset;
}

size_t IoData::totalByte()
{
	return this->totalBytes;
}

IO_OPERATION &IoData::Type()
{
	return ioType;
}

void IoData::SetType(IO_IOERATION type)
{
	ioType = type;
}

char* IoData::Data()
{
	return buffer.data();
}

bool IoData::SetData(Stream &stream)
{
	this->Clear();

	if (buffer.max_size() <= stream.size())
	{
		SLog(L"! packet size too big [%d]byte", stream.size());
		return false;
	}

	const size_t packetHeaderSize = sizeof(packet_size_t);
	packet_size_t offset = 0;

	char* buf = buffer.data();

	packet_size_t packetLen[1] = { (packet_size_t)packetHeaderSize + (packet_size_t)stream.size(), };

	memcpy_s(buf + offset, buffer.max_size(), (void*)packetLen, packetHeaderSize);
	offset += packetHeaderSize;

	PacketObfuscation::getInstance().encodingHeader((Byte*)buf, packetHeaderSize);
	PacketObfuscation::getInstance().encodingData((Byte*)stream.data(), stream.size());

	memcpy_s(buf + offset, buffer.max_size(), stream.data(), (int32_t)stream.size());
	offset += (packet_size_t)stream.size();

	totalBytes = offset;
	return true;
}

LPWSAOVERLAPPED IoData::Overlapped()
{
	return &overlapped;
}

WSABUF IoData::WsaBuf()
{
	WSABUF wsaBuf;
	wsaBuf.buf = buffer.data() + currentBytes;
	wsaBuf.len = (ULONG)(totalBytes - currentBytes);
	return wsaBuf;
}

IOCPSession::IOCPSession()
	:Session()
{
	this->Initialize();
}

void IOCPSession::Initialize()
{
	ZeroMemory(&socketData, sizeof(SOCKET_DATA));
	ioData[IO_READ].SetType(IO_READ);
	ioData[IO_WRITE].SetType(IO_WRITE);
}

void IOCPSession::CheckErrorIO(DWORD ret)
{
	if (ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		SLog(L"! socket error : %d", WSAGetLastError());
	}
}

void IOCPSession::Recv(WSABUF wsaBuf)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;
	DWORD errorCode = WSARecv(socketData.socket, &wsaBuf, 1, &recvBytes, &flags, ioData[IO_READ].Overlapped(), NULL);
	this->CheckErrorIO(errorCode);
}

bool IOCPSession::IsRecving(size_t transferSize)
{
	if (ioData[IO_READ].NeedMoreIO(transferSize))
	{
		this->Recv(ioData[IO_READ].WsaBuf());
		return true;
	}

	return false;
}

void IOCPSession::RecvStandBy()
{
	ioData[IO_READ].Clear();

	WSABUF wsaBuf;
	wsaBuf.buf = ioData[IO_READ].Data();
	wsaBuf.len = SOCKET_BUF_SIZE;

	this->Recv(wsaBuf);
}

void IOCPSession::Send(WSABUF wsaBuf)
{
	DWORD flags = 0;
	DWORD sendBytes;
	DWORD errorCode = WSASend(socketData.socket, &wsaBuf, 1, &sendBytes, flags, ioData[IO_WRITE].Overlapped(), NULL);
	this->CheckErrorIO(errorCode);
}

void IOCPSession::OnSend(size_t transferSize)
{
	if (ioData[IO_WRITE].NeedMoreIO(transferSize))
	{
		this->Send(ioData[IO_WRITE].WsaBuf());
	}
}

void IOCPSession::SendPacket(Packet* packet)
{
	Stream stream;
	packet->encode(stream);
	if (!ioData[IO_WRITE].SetData(stream))
	{
		return;
	}

	WSABUF wsaBuf;
	wsaBuf.buf = ioData[IO_WRITE].Data();
	wsaBuf.len = (ULONG)stream.size();

	this->Send(wsaBuf);
	this->RecvStandBy();
}

Package *IOCPSession::OnRecv(size_t transferSize)
{
	packet_size_t offset = 0;
	offset += ioData[IO_READ].SetupTotalBytes();

	if (this->IsRecving(transferSize))
	{
		return nullptr;
	}

	const size_t packetHeaderSize = sizeof(packet_size_t);
	packet_size_t packetDataSize = (packet_size_t)(ioData[IO_READ].TotalBytes() - packetHeaderSize);
	Byte* packetData = (Byte*)ioData[IO_READ].Data() + offset;

	PacketObfuscation::getInstance().decodingData(packetData, packetDataSize);
	Packet* packet = PacketAnalyzer::getInstance().analyzer((const char*)packetData, packetDataSize);
	if (packet == nullptr)
	{
		SLog(L"! invalid packet");
		this->onClose();
		return nullptr;
	}

	this->RecvStandBy();

	Package* package = new Package(this, packet);
	return package;
}