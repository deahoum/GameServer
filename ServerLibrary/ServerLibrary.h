#pragma once
#pragma once
#include "stdafx.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "Winmm.lib")

#include <Ws2tcpip.h>
#include <winsock2.h>
#include <mswsock.h>
#include <Mmsystem.h>
#include <Ws2spi.h>
#include <Mstcpip.h>

#include <windows.h>
#include <iostream>
#include <io.h>
#include <cstdlib>
#include <stdio.h>
#include <cstdint>

#include <assert.h> 
#include <fcntl.h>
#include <algorithm>  
#include <functional>

#include <thread>
#include <mutex>
#include <memory>

#include <string>
#include <tchar.h>

#include <array>
#include <ctime>
#include <random>
#include <typeinfo>    //typeinfo
#include <map>
// TODO: 공용 매크로
//#define CONTEXT_SWITCH     std::this_thread::sleep_for(std::chrono::nanoseconds(1))
#if _DEBUG
#define CONTEXT_SWITCH		Sleep(1)
#else
#define CONTEXT_SWITCH		::SwitchToThread()
#endif

typedef void(*Function)(void *);

// TODO : neccessary header file
#include "./Util/tinyXml/tinyxml.h"

#include "./Util/Singleton.h"
#include "./Util/RandomMT.h"
#include "./Util/Util.h"
#include "./Util/Type.h"
#include "./Util/Config.h"
#include "./Util/Clock.h"
#include "./Util/Lock.h"
#include "./Util/Thread.h"
#include "./Util/Logger.h"


#include "Shutdown.h"