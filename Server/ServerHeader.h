#include "stdio.h"
#include "conio.h"
#include "string.h"
#include "ws2tcpip.h"
#include "winsock2.h"
#include "process.h"
#include "tchar.h"
#pragma comment(lib, "WS2_32.lib")

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define NOT_CONTAIN_ENDING -1
#define USERNAME_SIZE 100
#define PASSWORD_SIZE 100
#define MAX_CLIENT 2048
#define MAX_ACCOUNT 2100
#define ENDING_DELIMITER "\r\n"
#define DELIM " "

/**
* @struct CLIENT_INFO: store information about a connected object (including Client and Machine)

* @property connSock: store information of the socket connected to server
* @property curUsername: store username of the account logging in at this client
* @property idUsername: numerical order of curUsername in the account list
* @property streamData: the data received from client needed to proccess streaming
**/
typedef struct {
	SOCKET connSock;
	char curUsername[USERNAME_SIZE];
	int idUsername;
	char streamData[BUFF_SIZE];
} CLIENT_INFO;

/**
* @struct ACCOUNT: store information about an account

* @property username: store username of the account
* @property state: store state of the account (true if logged in, false if vice versa)
* @property password: store password of the account
**/
typedef struct {
	char username[USERNAME_SIZE];
	bool state;
	char password[PASSWORD_SIZE];
} ACCOUNT;

/**
* @struct MACHINE: store information about a machine

* @property code: store the code of machine
* @property password: store the password of machine
* @property ipAddr: store the IP address of machine
* @property port: store the port of machine to communicate
* @property totalUsers: store the password of machine
**/
typedef struct {
	char code[BUFF_SIZE];
	char password[BUFF_SIZE];
	char ipAddr[INET_ADDRSTRLEN];
	int port;
	int totalUsers;
	char users[BUFF_SIZE][USERNAME_SIZE];
} MACHINE;