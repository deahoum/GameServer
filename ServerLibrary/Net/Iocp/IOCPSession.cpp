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

		totalBytes = (size_t)packetLenp[0];
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

