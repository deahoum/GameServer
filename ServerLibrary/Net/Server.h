#pragma once
#include "stdafx.h"
#include "SessionManager.h"

typedef enum SERVER_STATUS
{
	SERVER_STOP,
	SERVER_INITIALZE,
	SERVER_READY,
};

class Server
{
protected:
	char			ip[16];
	int				port;
	int				workerThreadCount;

	SERVER_STATUS	status;
	ContentsProcess *contentsProcess;

public:
	Server(ContentsProcess* contentsProcess);
	virtual ~Server();

	virtual void initialize(xml_t *config);
	virtual bool run() = 0;
	SERVER_STATUS &Status();
	void putPackage(Package* package);
};