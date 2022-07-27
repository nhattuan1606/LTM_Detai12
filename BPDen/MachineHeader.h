#include "stdio.h"
#include "conio.h"
#include "string.h"
#include "ws2tcpip.h"
#include "winsock2.h"
#include "process.h"
#include "tchar.h"
#include "ctime"
#include "string"
#include "cstring"
#include "sstream"
#define BUFF_SIZE 2048
#define NOT_CONTAIN_ENDING -1
#define USERNAME_SIZE 100
#define PASSWORD_SIZE 1024
#define MAX_CLIENT 2048
#define MAX_ACCOUNT 2100
#define ENDING_DELIMITER "\r\n"
#pragma comment(lib, "WS2_32.lib")