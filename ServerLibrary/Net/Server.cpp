#include "stdafx.h"
#include "Server.h"

Server::Server(ContentsProcess* contentsProcess)
{
	SLog(L"# Initalize network base");

	contentsProcess = contentsProcess;
	status = SERVER_STOP;

	xml_t config;
	if (!loadConfig(&config))
	{
		return;
	}
	this->initialize(&config);
}

Server::~Server()
{
	shutdownServer();

	status = SERVER_STOP;
	SAFE_DELETE(contentsProcess);

	SLog(L"# End network base");
}

void Server::initialize(xml_t *config)
{
	TerminalManager::getInstance().run(this);

	TaskManager::getInstance();

	xmlNode_t *root = config->FirstChildElement("App")->FirstChildElement("Server");
	if (!root)
	{
		SLog(L"@ not exist server setting");
		return;
	}

	xmlNode_t *elem = root->FirstChildElement("IP");
	strcpy_s(ip, elem->GetText());

	elem = root->FirstChildElement("Port");
	sscanf_s(elem->GetText(), "%d", &port);

	elem = root->FirstChildElement("ThreadCount");
	sscanf_s(elem->GetText(), "%d", &workerThreadCount);

	status = SERVER_INITIALZE;

	SLog(L"* IO Worker thread count : %d", workerThreadCount);

	root = config->FirstChildElement("App");
	elem = root->FirstChildElement("Name");

	SLog(L"### %S start!!! ###", elem->GetText());
}

SERVER_STATUS &Server::Status()
{
	return status;
}

void Server::putPackage(Package* package)
{
	contentsProcess->putPackage(package);
}