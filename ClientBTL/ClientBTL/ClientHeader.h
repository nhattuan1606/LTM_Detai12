#include <stdio.h>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#define BUFF_SIZE 2048
#define ENDING_DELIMITER "\r\n"
#define DELIM " "
#define NOT_CONTAIN_ENDING -1
#pragma comment(lib, "Ws2_32.lib")