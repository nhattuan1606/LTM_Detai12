#include "ServerHeader.h"

CLIENT_INFO client[FD_SETSIZE];
ACCOUNT account[MAX_ACCOUNT];
MACHINE machine[BUFF_SIZE];
int totalAcc;
int totalMachine;

/**
*  @function CheckEnd: Check if streamData contains ending delimiter

*  @param streamData: [IN] char Array contains stream data
*  @param requestData: [OUT] char Array contains data until ending delimiter is found

*  @return: NOT_CONTAINS_ENDING if streamData contains ending delimiter
Index of the last char of ending delimiter if streamData contains it
*/
int CheckEnd(char streamData[], char requestData[]);

/**
*  @function ProcessLogIn: process logging in request from client

*  @param data: [IN] char Array contains username to login
*  @param curUser: [IN/OUT] char array contains username of current user in client
*  @param id: [IN/OUT] numerical order of current username in account list

*  @return: char array contains reply code
*/
char *ProcessLogIn(char data[], char curUser[], int &id);

/**
*  @function ProcessLogout: process logging out request from client

*  @param id: [IN/OUT] numerical order of current username in account list
*  @param curUser: [OUT] char Array contains username of current user in client

*  @return: char array contains reply code
*/
char *ProcessLogout(int &id, char curUser[]);

/**
*  @function ProcessCPW: process changing password request from client

*  @param data: [IN] char Array contains data
*  @param curUser: [IN/OUT] char array contains username of current user in client
*  @param id: [IN/OUT] numerical order of current username in account list

*  @return: char array contains reply code
*/
char *ProcessCPW(char data[], char curUser[], int &id);

/**
*  @function ProcessGetInfo: process getting information request from client

*  @param data: [IN] char Array contains data
*  @param id: [IN/OUT] numerical order of current username in account list

*  @return: char array contains reply code
*/
char *ProcessGetInfo(char data[], int &id);

/**
*  @function ProcessMachineAddr: process changing/storing the information of machine (IP address, port) from machine

*  @param data: [IN] char Array contains data

*  @return: char array contains reply code
*/
char *ProcessMachineAddr(char data[]);

/**
*  @function ProcessCPWMachine:  process changing password request from machine

*  @param data: [IN] char Array contains data

*  @return: char array contains reply code
*/
char *ProcessCPWMachine(char data[]);

/**
*  @function ProcessRequestData: process request data from client

*  @param requestData: [IN] char array contains requestData from client
*  @param curUser: [IN] char array contains username of current user in client

*  @return: char array contains response for request
*/
char *ProcessRequestData(char requestData[], char curUser[], int &id);

/**
*  @procedure UpdateData: change data from a file at a certain line

*  @param path: [IN] char array contains path to the file needed to change
*  @param line: [IN] the line of the file needed to change
*  @param newline: [IN] a char pointer to the new data replacing in that line
*/
void UpdateData(char *path, int line, char *newline);

/**
*  @function getAccountData: get data about username and password of accounts from file account.txt

*  @return: total accounts
*/
int getAccountData();

/**
*  @function getMachineData: get data about information of machines from file machine.txt

*  @return: total machines
*/
int getMachineData();


int main(int argc, char* argv[]) {
	const int SERVER_PORT = atoi(argv[1]);

	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot bind this address.");
		_getch();
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error! Cannot listen.");
		_getch();
		return 0;
	}

	printf("Server started!\n");

	// Get accounts and machines from file txt
	totalAcc = getAccountData();
	totalMachine = getMachineData();

	// Initiate value for list of client
	for (int i = 0; i < FD_SETSIZE; i++) {
		client[i].connSock = 0;
		strcpy_s(client[i].curUsername, "");
		client[i].idUsername = -1;
		strcpy_s(client[i].streamData, "");
	}

	SOCKET connSock; // client[FD_SETSIZE];
	fd_set readfds, initfds; // use initfds to initiate readfds at the begining of every loop step
	sockaddr_in clientAddr;
	int ret, nEvents, clientAddrLen;
	char rcvBuff[BUFF_SIZE], sendBuff[BUFF_SIZE];

	for (int i = 0; i < FD_SETSIZE; i++)
		client[i].connSock = 0;	// 0 indicates available entry

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

		// New client connection
		if (FD_ISSET(listenSock, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				char clientIP[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
				int clientPort = ntohs(clientAddr.sin_port);
				printf("You got a connection from [%s:%d]\n", clientIP, clientPort);

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

				if (--nEvents == 0)
					continue; // no more event
			}
		}

		// Receive data from clients
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (client[i].connSock == 0)
				continue;

			if (FD_ISSET(client[i].connSock, &readfds)) {
				ret = recv(client[i].connSock, rcvBuff, BUFF_SIZE, 0);
				if (ret <= 0 || WSAGetLastError() == 10054) {
					printf("Client disconnects\n");
					strcpy_s(client[i].streamData, "");
					if (client[i].idUsername != -1) {
						strcpy_s(client[i].curUsername, "");
						account[client[i].idUsername].state = false;
						client[i].idUsername = -1;
					}
					FD_CLR(client[i].connSock, &initfds);
					closesocket(client[i].connSock);
					client[i].connSock = 0;
					continue;
				}
				else if (ret == SOCKET_ERROR) {
					printf("Error: %d", WSAGetLastError());
					return 0;
				}
				else if (ret > 0) {
					rcvBuff[ret] = 0;
					strcat_s(client[i].streamData, rcvBuff);
					char requestData[BUFF_SIZE];	// requestData contains data of a request

													/* Process streamData */
					do {
						int indexOfEndChar = CheckEnd(client[i].streamData, requestData);

						// Continue receive data if streamData don't contains ending delimiter
						if (indexOfEndChar == NOT_CONTAIN_ENDING)
							break;

						// Process if streamData contains ending delimiter
						strcpy_s(rcvBuff, ProcessRequestData(requestData, client[i].curUsername, client[i].idUsername));
						strcat_s(rcvBuff, ENDING_DELIMITER);
						int retSend = send(client[i].connSock, rcvBuff, strlen(rcvBuff), 0);
						if (retSend == SOCKET_ERROR) {
							printf("Error %d: Cannot send data\n", WSAGetLastError());
							break;
						}

						// Clear streamData after processing a request
						strncpy_s(client[i].streamData, client[i].streamData + indexOfEndChar + 1, strlen(client[i].streamData) - indexOfEndChar - 1);
					} while (strlen(client[i].streamData) != 0);
				}
			}

			if (--nEvents <= 0)
				continue; // no more event
		}

	}

	closesocket(listenSock);
	WSACleanup();
	return 0;
}


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
				index = i + lenOfEnding - 1;
				requestData[i] = 0;
				break;
			}
		} // end if (streamData[i] == ENDING_DELIMITER[0])
		requestData[i] = streamData[i];
	}

	return index;
}

char *ProcessLogIn(char data[], char curUser[], int &id) {
	char response[BUFF_SIZE];

	char *un, *pw;
	un = strtok(data, DELIM);
	pw = strtok(NULL, DELIM);

	if (un == NULL) {
		strcpy_s(response, "01");
		return response;
	}

	if (id != -1) {
		strcpy_s(response, "03");
		return response;
	}

	bool flag = false;
	int i;

	for (i = 0; i < totalAcc; i++) {
		if (strcmp(un, account[i].username) == 0) {
			flag = true;

			if (pw == NULL) {
				strcpy_s(response, "02");
				return response;
			}

			if (strcmp(pw, account[i].password) != 0)
				strcpy_s(response, "02");
			else {
				strcpy_s(response, "00");
				strcpy_s(curUser, USERNAME_SIZE, account[i].username);
				id = i;
				account[i].state = true;
			}
			break;
		}
	}

	if (!flag) strcpy_s(response, "01");

	return response;
}

char *ProcessLogout(int &id, char curUser[]) {
	char response[BUFF_SIZE];

	if (id == -1) {
		strcpy_s(response, "11");
	}
	else {
		strcpy_s(curUser, USERNAME_SIZE, "");
		account[id].state = false;
		id = -1;
		strcpy_s(response, "10");
	}

	return response;
}

char *ProcessCPW(char data[], char curUser[], int &id) {
	char response[BUFF_SIZE];

	char *opw, *npw;
	// Extract data
	opw = strtok(data, DELIM);
	npw = strtok(NULL, DELIM);

	if (id == -1) {
		strcpy_s(response, "11");
		return response;
	}

	if (npw == NULL || opw == NULL) {
		strcpy_s(response, "23");
		return response;
	}


	if (strcmp(opw, account[id].password) == 0) {
		if (strcmp(opw, npw) != 0) {
			// Save to the list
			strcpy_s(account[id].password, npw);

			// Save to the file
			FILE *changeFile;
			changeFile = fopen("account.txt", "w");
			if (changeFile == NULL) {
				printf("Error! Unable to open file to change.");
				exit(1);
			}

			for (int x = 0; x < totalAcc; x++)
				fprintf(changeFile, "%s %s\n", account[x].username, account[x].password);

			fclose(changeFile);
			strcpy_s(response, "20");
		}
		else strcpy_s(response, "22");
	}
	else strcpy_s(response, "21");

	return response;
}

char *ProcessGetInfo(char data[], int &id) {
	char response[BUFF_SIZE] = "";

	if (id == -1) {
		strcpy_s(response, "11");
		return response;
	}

	bool flag1 = false;
	for (int i = 0; i < totalMachine; i++)
		if (strcmp(data, machine[i].code) == 0) {
			flag1 = true;
			bool flag2 = false;
			for (int x = 0; x <= machine[i].totalUsers; x++) {
				if (strcmp(account[id].username, machine[i].users[x]) == 0) {
					flag2 = true;
					sprintf_s(response, BUFF_SIZE, "30 %s %d %s", machine[i].ipAddr, machine[i].port, machine[i].password);
					break;
				}
				if (!flag2) strcpy_s(response, "32");
			}
			break;
		}

	if (!flag1) strcpy_s(response, "31");

	return response;
}

void UpdateData(char *path, int line, char *newline) {
	FILE *fPtr, *fTemp;

	char buffer[BUFF_SIZE];
	int count;

	//  Open all required files
	fPtr = fopen(path, "r");
	fTemp = fopen("replace.tmp", "w");

	// fopen() return NULL if unable to open file in given mode.
	if (fPtr == NULL || fTemp == NULL) {
		printf("\nUnable to open file.\n");
		exit(EXIT_SUCCESS);
	}

	// Read line from source file and write to destination file after replacing given line.
	count = 0;
	while (fgets(buffer, BUFF_SIZE, fPtr) != NULL)
	{
		count++;
		// If current line is line to replace
		if (count == line)
			fputs(newline, fTemp);
		else
			fputs(buffer, fTemp);
	}

	// Close all files to release resource
	fclose(fPtr);
	fclose(fTemp);

	// Delete original source file
	remove(path);

	// Rename temporary file as original file
	rename("replace.tmp", path);
}

char *ProcessMachineAddr(char data[]) {
	char response[BUFF_SIZE] = "";

	char *code, *pw, *ipAddr, *port;
	// Extract data
	code = strtok(data, DELIM);
	pw = strtok(NULL, DELIM);
	ipAddr = strtok(NULL, DELIM);
	port = strtok(NULL, DELIM);

	if (port == NULL || pw == NULL || ipAddr == NULL || port == NULL) {
		strcpy_s(response, "81");
		return response;
	}

	bool flag = false;
	int position;
	for (int i = 0; i < totalMachine; i++)
		if (strcmp(machine[i].code, code) == 0) {
			flag = true;
			if (strcmp(machine[i].password, pw) != 0) {
				strcpy_s(response, "02");
				return response;
			}

			// update data in list
			strcpy_s(machine[i].ipAddr, INET_ADDRSTRLEN, ipAddr);
			machine[i].port = atoi(port);

			// get position of machine's data in file
			position = i * 4 + 3;
			break;
		}

	if (!flag) {
		strcpy_s(response, "31");
		return response;
	}

	char newline[BUFF_SIZE];
	sprintf_s(newline, BUFF_SIZE, "%s %s\n", ipAddr, port);

	// Update the new data to the file machine.txt
	UpdateData("machine.txt", position, newline);

	strcpy_s(response, "80");
	return response;
}

char *ProcessCPWMachine(char data[]) {
	char response[BUFF_SIZE] = "";

	char *code, *opw, *npw;
	// Extract data
	code = strtok(data, DELIM);
	opw = strtok(NULL, DELIM);
	npw = strtok(NULL, DELIM);

	if (npw == NULL || opw == NULL || code == NULL) {
		strcpy_s(response, "81");
		return response;
	}

	bool flag = false;
	int position;
	for (int i = 0; i < totalMachine; i++)
		if (strcmp(machine[i].code, code) == 0) {
			flag = true;
			if (strcmp(machine[i].password, opw) != 0) {
				strcpy_s(response, "21");
				return response;
			}

			if (strcmp(machine[i].password, npw) == 0) {
				strcpy_s(response, "22");
				return response;
			}
			// update data in list
			strcpy_s(machine[i].password, BUFF_SIZE, npw);

			// get position of machine's data in file
			position = i * 4 + 2;
			break;
		}

	if (!flag) {
		strcpy_s(response, "31");
		return response;
	}

	char newline[BUFF_SIZE];
	sprintf_s(newline, BUFF_SIZE, "%s\n", npw);

	// Update the new data to the file machine.txt
	UpdateData("machine.txt", position, newline);

	strcpy_s(response, "90");
	return response;
}

char *ProcessRequestData(char requestData[], char curUser[], int &id) {
	int i = 0;

	printf("Received data: %s\n", requestData);

	char requestType[BUFF_SIZE], response[BUFF_SIZE];

	while (requestData[i] != ' ' && requestData[i] != '\0') i++;

	strncpy_s(requestType, requestData, i);
	strncpy_s(requestData, BUFF_SIZE, requestData + i + 1, strlen(requestData) - i - 1);

	if (strcmp(requestType, "USER") == 0)
		strcpy_s(response, ProcessLogIn(requestData, curUser, id));

	else if (strcmp(requestType, "BYE") == 0)
		strcpy_s(response, ProcessLogout(id, curUser));

	else if (strcmp(requestType, "CPW") == 0)
		strcpy_s(response, ProcessCPW(requestData, curUser, id));

	else if (strcmp(requestType, "GINFO") == 0)
		strcpy_s(response, ProcessGetInfo(requestData, id));

	else if (strcmp(requestType, "SENDADDR") == 0)
		strcpy_s(response, ProcessMachineAddr(requestData));

	else if (strcmp(requestType, "CPWMACHINE") == 0)
		strcpy_s(response, ProcessCPWMachine(requestData));

	else strcpy_s(response, "99");

	return response;
}

int getAccountData() {
	FILE *f = NULL;
	errno_t err = fopen_s(&f, "account.txt", "r");
	if (err != 0) {
		printf("Error code %d: Cannot open file account.txt", err);
	}
	char *user, *pass;
	int i = 0;
	char tmpAcc[BUFF_SIZE];
	while (fgets(tmpAcc, 100, f) != NULL) {
		int len = strlen(tmpAcc);
		if (tmpAcc[len - 1] == '\n')
			strncpy_s(tmpAcc, tmpAcc, len - 1);

		user = strtok(tmpAcc, DELIM);
		strcpy_s(account[i].username, user);
		pass = strtok(NULL, DELIM);
		strcpy_s(account[i].password, pass);

		i++;
	}

	fclose(f);
	return i;
}

int getMachineData() {
	FILE *f = NULL;
	errno_t err = fopen_s(&f, "machine.txt", "r");
	if (err != 0) {
		printf("Error code %d: Cannot open file machine.txt", err);
	}
	char *ipaddr, *port, *user;
	int i = 0;
	char tmpMachine[BUFF_SIZE];
	while (fgets(tmpMachine, BUFF_SIZE, f) != NULL) {

		// get machine code
		strncpy_s(machine[i].code, tmpMachine, strlen(tmpMachine) - 1);

		// get machine password
		if (fgets(tmpMachine, BUFF_SIZE, f) != NULL)
			strncpy_s(machine[i].password, tmpMachine, strlen(tmpMachine) - 1);
		else break;

		// get machine IP and port
		if (fgets(tmpMachine, BUFF_SIZE, f) != NULL) {
			ipaddr = strtok(tmpMachine, DELIM);
			if (ipaddr != NULL && ipaddr[strlen(ipaddr) - 1] != '\n')
				strcpy_s(machine[i].ipAddr, ipaddr);
			else
				strcpy_s(machine[i].ipAddr, "");
			port = strtok(NULL, DELIM);
			if (port != NULL)
				machine[i].port = atoi(port);
		}
		else break;

		// get machine user
		if (fgets(tmpMachine, BUFF_SIZE, f) != NULL) {
			int len = strlen(tmpMachine);
			if (tmpMachine[len - 1] == '\n')
				strncpy_s(tmpMachine, tmpMachine, len - 1);
			user = strtok(tmpMachine, DELIM);
			strcpy_s(machine[i].users[0], BUFF_SIZE, user);
			int x = 1;
			user = strtok(NULL, DELIM);
			while (user != NULL) {
				strcpy_s(machine[i].users[x], BUFF_SIZE, user);
				x++;
				user = strtok(NULL, DELIM);
			}
			machine[i].totalUsers = x;
		}
		else break;

		i++;
	}

	fclose(f);
	return i;
}