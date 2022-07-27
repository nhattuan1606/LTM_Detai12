#include "MachineHeader.h"

typedef struct {
	SOCKET connSock;
	char streamData[BUFF_SIZE];
	bool state;		// false: is not logged it, true: is logged in
} ClientInfo;

typedef struct {
	char machineCode[BUFF_SIZE];
	char machinePassword[BUFF_SIZE];
	char startTime[6];
	char longTime[6];
	double P;
	bool state;
} MachineInfo;

ClientInfo client[FD_SETSIZE];
MachineInfo machineInfo;
char *MACHINE_ADDR;
int MACHINE_PORT;

void ConfigMachine(int type);
void CommunicateServer(SOCKET connServer, char *buffSend);
void PrintResponse(char responseData[]);
void StartNormally();
void InitClientList();
void ProcessNewClient(SOCKET listenSock, fd_set &initfds);
void ProcessReceiveData(int i, fd_set &readfds, fd_set &initfds);
int CheckEnd(char streamData[], char requestData[]);
bool CheckValid(char valueOfParam[]);
bool CheckValidTime(char valueOfParam[]);
void ShowMenu();
double fRand(double fMax);
char *ProcessConnect(char password[], bool &state);
char *ProcessSetDeviceParam(char nameOfParam[], char valueOfParam[], bool state);
char *ProcessGetData(char nameOfParam[], bool state);
char *ProcessControl(char machineState[], bool state);
char *ProcessRequestData(char requestData[], bool &state);
void GetMachineInfo();
void UpdateFileConfig();

char password[BUFF_SIZE];

int main(int argc, char* argv[]) {
	// Get Machine's address
	MACHINE_ADDR = argv[1];

	// Get Machine's Port
	MACHINE_PORT = atoi(argv[2]);

	GetMachineInfo();

	while (1) {
		ShowMenu();
		char tmpData[BUFF_SIZE];
		gets_s(tmpData, BUFF_SIZE);

		if (strcmp(tmpData, "1") == 0) {
			ConfigMachine(1);
		}
		else if (strcmp(tmpData, "2") == 0) {
			ConfigMachine(2);
		}
		else if (strcmp(tmpData, "3") == 0) {
			StartNormally();
		}
		else {
			printf("Bye!\n");
			return 0;
		}
	}

	return 0;
}

/**
*  @function ConfigMachine: Process if choose send address or change password of machine
*
*  @param type [IN]: = 1: Send address of machine to server
*					 = 2: Change password of machine and send to server
*/
void ConfigMachine(int type) {
	char server_addr[INET_ADDRSTRLEN];
	int server_port;

	printf("Enter server address: ");
	gets_s(server_addr, INET_ADDRSTRLEN);
	printf("Enter server port: ");
	scanf_s("%d", &server_port);

	// get superfluous characters
	char tmp[1];
	gets_s(tmp, 1);

	printf("addr: %s\nport: %d\n", server_addr, server_port);

	// Inittiate WinSock
	WSADATA wsaData;
	WORD wsaVer = MAKEWORD(2, 2);
	if (WSAStartup(wsaVer, &wsaData)) {
		printf("WinSock 2.2 is not supported!\n");
		return;
	}

	// Construct socket
	SOCKET machineSock;
	machineSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (machineSock == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket", WSAGetLastError());
		return;
	}

	// Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(server_port);
	inet_pton(AF_INET, server_addr, &serverAddr.sin_addr);

	// Request to connect server
	if (connect(machineSock, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return;
	}

	printf("Connected server!\n");

	char buff[BUFF_SIZE];
	if (type == 1) {
		snprintf(buff, BUFF_SIZE, "SENDADDR %s %s %s %d", machineInfo.machineCode, machineInfo.machinePassword, MACHINE_ADDR, MACHINE_PORT);
	}
	else {
		printf("Enter new password for machine: ");
		gets_s(password, BUFF_SIZE);
		snprintf(buff, BUFF_SIZE, "CPWMACHINE %s %s %s", machineInfo.machineCode, machineInfo.machinePassword, password);
	}
	printf("--%s--\n", buff);
	strcat_s(buff, ENDING_DELIMITER);

	CommunicateServer(machineSock, buff);

	// Close socket
	closesocket(machineSock);

	// Terminate WinSock
	WSACleanup();
}

/**
*  @function CommunicateServer: Send and receive data with Server

*  @param connServer [IN]: socket connect with server
*  @param buffSend [IN]: char array contains data to send to server
*/
void CommunicateServer(SOCKET connServer, char *buffSend) {
	char rcvBuff[BUFF_SIZE];
	char streamData[BUFF_SIZE];
	strcpy_s(streamData, "");
	// send buff to server
	int ret = send(connServer, buffSend, strlen(buffSend), 0);
	if (ret == SOCKET_ERROR) {
		printf("Error %d: Cannot send data.\n", WSAGetLastError());
	}

	// Receive buff from server
	ret = recv(connServer, rcvBuff, BUFF_SIZE, 0);
	if (ret == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT)
			printf("Time out!");
		else printf("Error %d: Cannot receive data.", WSAGetLastError());
	}
	else if (strlen(rcvBuff) > 0) {
		rcvBuff[ret] = 0;
		strcat_s(streamData, rcvBuff);
		char responseData[BUFF_SIZE];	// responseData contains data of a reponse

		// Process streamData
		do {
			int indexOfEndChar = CheckEnd(streamData, responseData);

			// Continue receive data if streamData don't contains ending delimiter
			if (indexOfEndChar == NOT_CONTAIN_ENDING)
				break;

			// Process if streamData contains ending delimiter
			PrintResponse(responseData);

			// Clear streamData after processing a request
			strncpy_s(streamData, streamData + indexOfEndChar + 1, strlen(streamData) - indexOfEndChar - 1);
		} while (strlen(streamData) != 0);
	}
}

/**
*  @function PrintResponse: Print response from server

*  @param responseData [IN]: char array contains response data from server 
*/
void PrintResponse(char responseData[]) {
	if (strcmp(responseData, "80") == 0) {
		printf("%s: Sending address to server successfully\n\n", responseData);
	}
	if (strcmp(responseData, "81") == 0) {
		printf("Error code %s: Missing parameters\n\n", responseData);
	}
	else if (strcmp(responseData, "02") == 0) {
		printf("Error code %s: Wrong password!\n\n", responseData);
	}
	else if (strcmp(responseData, "31") == 0) {
		printf("Error code %s: Machine code doesn't exist!\n\n", responseData);
	}
	else if (strcmp(responseData, "21") == 0) {
		printf("Error code %s: Wrong old password!\n\n", responseData);
	}
	else if (strcmp(responseData, "22") == 0) {
		printf("Error code %s: Duplicate password!\n\n", responseData);
	}
	else if (strcmp(responseData, "23") == 0) {
		printf("Error code %s: Mising new password!\n\n", responseData);
	}
	else if (strcmp(responseData, "90") == 0) {
		printf("%s: Change password machine successfully\n\n", responseData);
		strcpy_s(machineInfo.machinePassword, password);
		UpdateFileConfig();
	}
	else if (strcmp(responseData, "99") == 0) {
		printf("Error code %s: Invalid request!\n\n", responseData);
	}
}

/**
*  @function StartNormall: Process if choose normal start
*/
void StartNormally() {
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in machineAddr;
	machineAddr.sin_family = AF_INET;
	machineAddr.sin_port = htons(MACHINE_PORT);
	inet_pton(AF_INET, MACHINE_ADDR, &machineAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&machineAddr, sizeof(machineAddr))) {
		printf("Error! Cannot bind this address.");
		_getch();
		return;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error! Cannot listen.");
		_getch();
		return;
	}

	printf("Server started!\n");

	fd_set readfds, initfds; //use initfds to initiate readfds at the begining of every loop step
	int nEvents;

	InitClientList();

	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);

	//Step 5: Communicate with clients
	while (1) {
		readfds = initfds;		/* structure assignment */
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		//new client connection
		if (FD_ISSET(listenSock, &readfds)) {
			ProcessNewClient(listenSock, initfds);

			if (--nEvents == 0)
				continue; //no more event
		}

		//receive data from clients
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (client[i].connSock == 0)
				continue;

			ProcessReceiveData(i, readfds, initfds);

			if (--nEvents <= 0)
				continue; //no more event
		}

	}

	closesocket(listenSock);
	WSACleanup();
}

/**
*  @function ProcessNewClient: Process if new client connect to machine
*
*  @param listenSock [IN]: listen socket of machine
*  @param initfds [IN]: init fd_set
*/
void ProcessNewClient(SOCKET listenSock, fd_set &initfds) {
	sockaddr_in clientAddr;
	int clientAddrLen;
	SOCKET connSock;

	clientAddrLen = sizeof(clientAddr);
	if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
		printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
	}
	else {
		char clientIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
		printf("You got a connection from %s\n", clientIP); /* prints client's IP */

		int i;
		for (i = 0; i < FD_SETSIZE; i++)
			if (client[i].connSock == 0) {
				client[i].connSock = connSock;
				FD_SET(client[i].connSock, &initfds);
				break;
			}

		if (i == FD_SETSIZE) {
			printf("\nToo many clients.");
			closesocket(connSock);
		}
	}
}

/**
*  @function ProcessReceiveData: Receive data from client
*
*  @param i [IN]: numerical order of client
*  @param readfds [IN]: read fd_set
*  @param initfds [IN]: init fd_set
*/
void ProcessReceiveData(int i, fd_set &readfds, fd_set &initfds) {
	int ret;
	char rcvBuff[BUFF_SIZE], sendBuff[BUFF_SIZE];

	if (FD_ISSET(client[i].connSock, &readfds)) {
		ret = recv(client[i].connSock, rcvBuff, BUFF_SIZE, 0);
		if (ret <= 0 || WSAGetLastError() == 10054) {
			printf("Client disconnects\n");
			strcpy_s(client[i].streamData, "");
			client[i].state = false;

			FD_CLR(client[i].connSock, &initfds);
			closesocket(client[i].connSock);
			client[i].connSock = 0;
			return;
		}
		else if (ret == SOCKET_ERROR) {
			printf("Error: %d", WSAGetLastError());
			return;
		}
		else if (ret > 0) {
			rcvBuff[ret] = 0;
			strcat_s(client[i].streamData, rcvBuff);
			char requestData[BUFF_SIZE];	// requestData contains data of a request

											// Process streamData
			do {
				int indexOfEndChar = CheckEnd(client[i].streamData, requestData);

				// Continue receive data if streamData don't contains ending delimiter
				if (indexOfEndChar == NOT_CONTAIN_ENDING)
					break;

				// Process if streamData contains ending delimiter
				char sendBuff[BUFF_SIZE];
				strcpy_s(sendBuff, ProcessRequestData(requestData, client[i].state));
				strcat_s(sendBuff, ENDING_DELIMITER);
				int retSend = send(client[i].connSock, sendBuff, strlen(sendBuff), 0);
				if (retSend == SOCKET_ERROR) {
					printf("Error %d: Cannot send data\n", WSAGetLastError());
					break;
				}

				// Clear streamData after processing a request
				strncpy_s(client[i].streamData, client[i].streamData + indexOfEndChar + 1, strlen(client[i].streamData) - indexOfEndChar - 1);
			} while (strlen(client[i].streamData) != 0);
		}
	}
}

/**
*  @function InitClientList: Initiate client list
*/
void InitClientList() {
	// Initiate value for list of client
	for (int i = 0; i < FD_SETSIZE; i++) {
		client[i].connSock = 0;
		strcpy_s(client[i].streamData, "");
		client[i].state = false;
	}
}

/**
*  @function CheckEnd: Check if streamData contains ending delimiter

*  @param streamData: [IN] char Array contains stream data
*  @param requestData: [OUT] char Array contains data until ending delimiter is found

*  @return: NOT_CONTAINS_ENDING if streamData contains ending delimiter
Index of the last char of ending delimiter if streamData contains it
*/
int CheckEnd(char streamData[], char requestData[]) {
	int index = NOT_CONTAIN_ENDING;
	int lenOfEnding = strlen(ENDING_DELIMITER);
	int n = strlen(streamData) - lenOfEnding + 1;

	for (int i = 0; i < n; i++) {
		if (streamData[i] == ENDING_DELIMITER[0]) {
			bool flag = true;
			for (int j = 1; j < lenOfEnding; j++) {
				if (streamData[i + j] != ENDING_DELIMITER[j]) {
					flag = false;
					break;
				}
			}

			// if contain ending delimiter
			if (flag) {
				index = (int)(i + lenOfEnding - 1);
				requestData[i] = 0;
				break;
			}
		} // end if (streamData[i] == ENDING_DELIMITER[0])
		requestData[i] = streamData[i];
	}

	return index;
} // end CheckEnd

/**
*  @function CheckValid: Check if value is a double number

*  @param valueOfParam: [IN] value to check

*  @return: false if valueOfParam is not a double number
			true if valueOfParam is a double number
*/
bool CheckValid(char valueOfParam[]) {
	int len = strlen(valueOfParam);
	if (len == 0)
		return false;
	for (int i = 0; i < len; i++) {
		if (!isdigit(valueOfParam[i]) && valueOfParam[i] != '.') {
			return false;
		}
	}

	return true;
}

/**
*  @function CheckValidTime: Check if value is a time string

*  @param valueOfParam: [IN] value to check

*  @return: false if valueOfParam is not a time string
			true if valueOfParam is a time string
*/
bool CheckValidTime(char valueOfParam[]) {
	if (strlen(valueOfParam) != 5)
		return false;

	for (int i = 0; i < 5; i++) {
		if (i != 2) {
			if (!isdigit(valueOfParam[i])) return false;
		}
		else if (valueOfParam[2] != ':') return false;
	}

	if (valueOfParam[0] - '0' > 2) {
		return false;
	}
	else if (valueOfParam[0] - '0' == 2) {
		if (valueOfParam[1] - '0' > 3) return false;
	}

	if (valueOfParam[3] - '0' > 5) return false;

	return true;
}

/**
*  @function ShowMenu: show menu of machine
*/
void ShowMenu() {
	// Show menu 
	printf("\n<-----Choose your selection----->\n");
	printf("1: Send address to server  \n2: Change password  \n3: Start! \n");
	printf("<----------------Please select request--------------->\n");
}

/**
*  @function fRand: random a double number
*
*  @param fMax [IN]: max of return value
*
*  @return: random double value from 0 to fMax;
*/
double fRand(double fMax) {
	srand(time(NULL));
	double f = (double)rand() / RAND_MAX;
	return f * fMax;
}

/**
*  @function ProcessConnect: process if a client conect and send password
*
*  @param password [IN]: char array contains password from client
*  @param state [IN/OUT]: log in state of client
*
*  @return: char array contains reply code: 40 - Connect successfully
*											41 - Wrong password
*											42 - Client is logged in
*/
char *ProcessConnect(char password[], bool &state) {
	char response[BUFF_SIZE];

	if (state) {
		strcpy_s(response, "42");
		return response;
	}

	if (strcmp(password, machineInfo.machinePassword) == 0) {
		state = true;
		strcpy_s(response, "40");
		return response;
	}
	else {
		strcpy_s(response, "41");
		return response;
	}
}

/**
*  @function ProcessSetDeviceParam: process if a client set param of machine an save to file config
*
*  @param nameOfParam [IN]: char array contains name of param
*  @param valueOfParam [IN]: char array contains value of param
*  @param state [IN]: log in state of client
*
*  @return: char array contains reply code: 50 - Set param successfully
*											51 - client is not logged in
*											52 - Name of param is not exist
*											53 - Wrong format value of param
*/
char *ProcessSetDeviceParam(char nameOfParam[], char valueOfParam[], bool state) {
	char response[BUFF_SIZE];
	if (state) {
		if (strcmp(nameOfParam, "sTime") == 0) {
			if (!CheckValidTime(valueOfParam)) {
				strcpy_s(response, "53");
				return response;
			}
			else {
				strcpy_s(machineInfo.startTime, valueOfParam);
				UpdateFileConfig();
				strcpy_s(response, "50");
				return response;
			}
		}
		else if (strcmp(nameOfParam, "lTime") == 0) {
			if (!CheckValidTime(valueOfParam)) {
				strcpy_s(response, "53");
				return response;
			}
			else {
				strcpy_s(machineInfo.longTime, valueOfParam);
				UpdateFileConfig();
				strcpy_s(response, "50");
				return response;
			}
		}
		else if (strcmp(nameOfParam, "P") == 0) {
			if (!CheckValid(valueOfParam)) {
				strcpy_s(response, "53");
				return response;
			}
			else {
				machineInfo.P = atof(valueOfParam);
				UpdateFileConfig();
				strcpy_s(response, "50");
				return response;
			}
		}
		else {
			strcpy_s(response, "52");
			return response;
		}

	}
	else  strcpy_s(response, "51");

	return response;
}

/**
*  @function ProcessGetData: process if a client get data from machine
*
*  @param nameOfParam [IN]: char array contains name of param
*  @param state [IN]: log in state of client
*
*  @return: char array contains reply code: 60 [value of param] - Get param successfully
*											51 - client is not logged in
*											52 - Name of param is not exist
*/
char *ProcessGetData(char nameOfParam[], bool state) {
	char response[BUFF_SIZE];

	if (state) {	// If client logged in
		if (strcmp(nameOfParam, "sTime") == 0) {
			strcpy_s(response, "60 ");
			strcat_s(response, machineInfo.startTime);
		}
		else if (strcmp(nameOfParam, "lTime") == 0) {
			strcpy_s(response, "60 ");
			strcat_s(response, machineInfo.longTime);
		}
		else if (strcmp(nameOfParam, "P") == 0) {
			snprintf(response, 15, "60 %f", machineInfo.P);
		}
		else if (strcmp(nameOfParam, "state") == 0) {
			strcpy_s(response, "60 ");
			if (machineInfo.state) {
				strcat_s(response, "ON");
			}
			else strcat_s(response, "OFF");
		}
		else {
			strcpy_s(response, "52");
			return response;
		}
	}
	else strcpy_s(response, "51");

	return response;
}

/**
*  @function ProcessControl: process if a client control machine's state
*
*  @param nameOfParam [IN]: char array contains name of param
*  @param state [IN]: log in state of client
*
*  @return: char array contains reply code: 70 - Control successfully
*											51 - client is not logged in
*											71 - Wrong state
*/
char *ProcessControl(char machineState[], bool state) {
	char response[BUFF_SIZE];

	if (state) {
		if (strcmp(machineState, "ON") == 0) {
			printf("Start lighting\n");
			machineInfo.state = true;
			strcpy_s(response, "70");
			return response;
		}
		else if (strcmp(machineState, "OFF") == 0) {
			printf("Stop lighting\n");
			machineInfo.state = false;
			strcpy_s(response, "70");
			return response;
		}
		else {
			strcpy_s(response, "71");
			return response;
		}
	}
	else strcpy_s(response, "51");

	return response;
}

/**
*  @function ProcessRequestData: process request data from client

*  @param requestData: [IN] char array contains requestData from client
*  @param curUser: [IN] char array contains username of current user in client

*  @return: char array contains response for request
*/
char *ProcessRequestData(char requestData[], bool &state) {
	printf("buff: --%s--\n", requestData);
	int i = 0;
	char requestType[BUFF_SIZE], response[BUFF_SIZE];

	while (requestData[i] != ' ' && requestData[i] != '\0') {
		i++;
	}

	strncpy_s(requestType, requestData, i);
	strncpy_s(requestData, BUFF_SIZE, requestData + i + 1, strlen(requestData) - i - 1);

	if (strcmp(requestType, "CONN") == 0) {
		strcpy_s(response, ProcessConnect(requestData, state));
	}
	else if (strcmp(requestType, "SET") == 0) {
		int i = 0;
		char nameOfParam[BUFF_SIZE];
		char valueOfParam[BUFF_SIZE];

		while (requestData[i] != ' ' && requestData[i] != '\0') {
			i++;
		}
		if (i == strlen(requestData)) {
			strncpy_s(nameOfParam, requestData, i);
			strcpy_s(valueOfParam, "");
		}
		else {
			strncpy_s(nameOfParam, requestData, i);
			strncpy_s(valueOfParam, BUFF_SIZE, requestData + i + 1, strlen(requestData) - i - 1);
		}

		strcpy_s(response, ProcessSetDeviceParam(nameOfParam, valueOfParam, state));
	}
	else if (strcmp(requestType, "GDATA") == 0) {
		strcpy_s(response, ProcessGetData(requestData, state));
	}
	else if (strcmp(requestType, "CTRL") == 0) {
		strcpy_s(response, ProcessControl(requestData, state));
	}
	else {
		strcpy_s(response, "99");
	}

	printf("res: --%s--", response);
	return response;
}

/**
*  @function GetMachineInfo: Get data from file config when machine start
*/
void GetMachineInfo() {
	FILE *f = NULL;
	errno_t err = fopen_s(&f, "configDen.txt", "r");
	if (err != 0) {
		printf("Error code %d: Cannot open file config", err);
	}
	fgets(machineInfo.machineCode, BUFF_SIZE, f);
	machineInfo.machineCode[strlen(machineInfo.machineCode) - 1] = 0;
	fgets(machineInfo.machinePassword, BUFF_SIZE, f);
	machineInfo.machinePassword[strlen(machineInfo.machinePassword) - 1] = 0;
	fgets(machineInfo.startTime, 10, f);
	machineInfo.startTime[strlen(machineInfo.startTime) - 1] = 0;
	fgets(machineInfo.longTime, 10, f);
	machineInfo.longTime[strlen(machineInfo.longTime) - 1] = 0;
	fscanf_s(f, "%lf", &machineInfo.P);
	machineInfo.state = false;

	printf("code: %s\npass: %s\nstartTime: %s\nendTime: %s\nP: %lf\n", machineInfo.machineCode, machineInfo.machinePassword, machineInfo.startTime, machineInfo.longTime, machineInfo.P);

	fclose(f);
}

/**
*  @function UpdateFileConfig: Update file config of machine
*/
void UpdateFileConfig() {
	FILE *f = NULL;
	errno_t err = fopen_s(&f, "configDen.txt", "w");

	if (err != 0) {
		printf("Error code %d: Cannot open file config", err);
	}

	fprintf(f, "%s\n%s\n%s\n%s\n%lf", machineInfo.machineCode, machineInfo.machinePassword, machineInfo.startTime, machineInfo.longTime, machineInfo.P);

	fclose(f);
}