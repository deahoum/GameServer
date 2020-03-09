#include "stdafx.h"
#include "Config.h"

bool loadConfig(xml_t *config)
{
	if (!config->LoadFile("./Config.xml")) {
		printf("! not exist config file.");
		return false;
	}
	return true;
}