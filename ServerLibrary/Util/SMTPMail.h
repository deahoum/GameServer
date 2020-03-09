#pragma once
#include "stdafx.h"

//#define CONNECT_SMTP

bool connectSMTP(SOCKET *sock);
bool sendMail(const char* from, const char* to, const char* subject, const char* body);
