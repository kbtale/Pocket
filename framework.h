#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "targetver.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <sstream>
#include <iomanip>
#include <urlmon.h>
#include <shellapi.h>
#include <pcap.h>

#pragma comment(lib, "comctl32.lib")
