#include "ClientHeader.h"

int CheckEnd(char streamData[], char responseData[]);
void PrintResponse(char responseData[]);
int Send(SOCKET s, char *buff, int size, int flags);
int Receive(SOCKET s, char *buff, int size, int flags);
void ProcessLogin(SOCKET client);
void ProcessChangePassword(SOCKET client);
void ProcessLogout(SOCKET client);
void ProcessConnDevice(SOCKET client);
void ProcessSetParam(SOCKET clientDev);
void ProcessGetData(SOCKET clientDev);
void ProcessControlDevice(SOCKET clientDev);
void CommunicateDevice(char rcvBuff[]);

int main(int argc, char* argv[]) {
	// Get Server's address
	const char *SERVER_ADDR = argv[1];

	// Get Server's Port
	const int SERVER_PORT = atoi(argv[2]);

	// Inittiate WinSock
	WSADATA wsaData;
	WORD wsaVer = MAKEWORD(2, 2);
	if (WSAStartup(wsaVer, &wsaData)) {
		printf("WinSock 2.2 is not supported!\n");
		return 0;
	}

	// Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket", WSAGetLastError());
		return 0;
	}

	// Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	// Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return 0;
	}
	printf("Connected server!\n");

	// Communicate with server
	char key[BUFF_SIZE];

	while (1) {
		printf(" ----------------------------------\n");
		printf(" |-------------MENU---------------|\n");
		printf(" |   1. Log in.                   |\n");
		printf(" |   2. Change password.          |\n");
		printf(" |   3. Connect to device.        |\n");
		printf(" |   4. Log out.                  |\n");
		printf(" |   #. Exit                      |\n");
		printf(" ----------------------------------\n");
		printf(">> Choose your selection : ");
		gets_s(key, BUFF_SIZE); // Enter selection.

		// User requires login
		if (strcmp(key, "1") == 0) {
			ProcessLogin(client);
		}

		// User request to change password
		else if (strcmp(key, "2") == 0) {
			ProcessChangePassword(client);
		}

		// User request to connect to device
		else if (strcmp(key, "3") == 0) {
			ProcessConnDevice(client);
		}

		// User requested to log out
		else if (strcmp(key, "4") == 0) {
			ProcessLogout(client);
		}

		// User request to end the program: If the user enters a character other than 1-8, the program will be closed
		else {
			printf("Goodbye! See you again!\n");
			break;
		}


	}


	// Close socket
	closesocket(client);

	// Terminate WinSock
	WSACleanup();

	return 0;
}

/*
@function CheckEnd: Check if streamData contains ending delimiter

@param streamData: [IN] char Array contains stream data
@param responseData: [OUT] char Array contains data until ending delimiter is found

@return: NOT_CONTAINS_ENDING if streamData contains ending delimiter
Index of the last char of ending delimiter if streamData contains it
*/
int CheckEnd(char streamData[], char responseData[]) {
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
				responseData[i] = 0;
				break;
			}
		} // end if (streamData[i] == ENDING_DELIMITER[0])
		responseData[i] = streamData[i];
	}

	return index;
} // end CheckEnd

/**
*  @function PrintResponse: Print reply code and message for user

*  @param responseData: [IN] Char array contain reply code from server
*/
void PrintResponse(char responseData[]) {
	if (strcmp(responseData, "00") == 0) {
		printf("%s: Login successfully!\n\n", responseData);
	}
	if (strcmp(responseData, "01") == 0) {
		printf("Error code %s: Account does not exist\n\n", responseData);
	}
	else if (strcmp(responseData, "02") == 0) {
		printf("Error code %s: Password is wrong\n\n", responseData);
	}
	else if (strcmp(responseData, "03") == 0) {
		printf("Error code %s: You are logged in\n\n", responseData);
	}
	else if (strcmp(responseData, "10") == 0) {
		printf("%s: Logout successfully!\n\n", responseData);
	}
	else if (strcmp(responseData, "11") == 0) {
		printf("Error code %s: You are not logged in\n\n", responseData);
	}
	else if (strcmp(responseData, "20") == 0) {
		printf("%s: Change password is successfully!\n\n", responseData);
	}
	else if (strcmp(responseData, "21") == 0) {
		printf("Error code %s: Old password is wrong\n\n", responseData);
	}
	else if (strcmp(responseData, "22") == 0) {
		printf("Error code %s: The new password is the same as the old password\n\n", responseData);
	}
	else if (strcmp(responseData, "23") == 0) {
		printf("Error code %s: You have not entered password\n\n", responseData);
	}
	else if (strcmp(responseData, "31") == 0) {
		printf("Error code %s: Device code does not exist\n\n", responseData);
	}
	else if (strcmp(responseData, "32") == 0) {
		printf("Error code %s: Account does not have permission to connect to the device\n\n", responseData);
	}
	else if (strcmp(responseData, "40") == 0) {
		printf("%s: Connect to device is successfully!\n\n", responseData);
	}
	else if (strcmp(responseData, "41") == 0) {
		printf("Error code %s: Password is wrong\n\n", responseData);
	}
	else if (strcmp(responseData, "50") == 0) {
		printf("%s:  Set parameters is successfully!\n\n", responseData);
	}
	else if (strcmp(responseData, "51") == 0) {
		printf("Error code %s: You are not logged in to device\n\n", responseData);
	}
	else if (strcmp(responseData, "52") == 0) {
		printf("Error code %s: parameter name is not exist\n\n", responseData);
	}
	else if (strcmp(responseData, "53") == 0) {
		printf("Error code %s: parameter fomat is wrong\n\n", responseData);
	}
	else if (responseData[0] == '6' && responseData[1] == '0') {
		printf("Successfully! Value: %s\n\n", responseData + 3);
	}
	else if (strcmp(responseData, "70") == 0) {
		printf("%s: Successfully!\n\n", responseData);
	}
	else if (strcmp(responseData, "71") == 0) {
		printf("Error code %s: Status is wrong\n\n", responseData);
	}
	else if (strcmp(responseData, "99") == 0) {
		printf("Error code %s: Invalid request!\n\n", responseData);
	}
}

/* The send() wrapper function*/
int Send(SOCKET s, char *buff, int size, int flags) {
	int n;

	n = send(s, buff, size, flags);
	if (n == SOCKET_ERROR)
		printf("Error %d: Cannot send data.\n", WSAGetLastError());

	return n;
}

/* The recv() wrapper function */
int Receive(SOCKET s, char *buff, int size, int flags) {
	int n;

	n = recv(s, buff, size, flags);
	if (n == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT)
			printf("Time out!");
		else printf("Error %d: Cannot receive data.", WSAGetLastError());
	}
	else if (strlen(buff) > 0) {
		buff[n] = 0;
		char streamData[BUFF_SIZE];
		strcpy_s(streamData, "");
		strcat_s(streamData, buff);
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
	return n;
}

/*
@function ProcessLogin: Login to server

@param client[IN]: socket connect with server
*/
void ProcessLogin(SOCKET client) {
	char buff[BUFF_SIZE], username[BUFF_SIZE], password[BUFF_SIZE];
	printf("Enter your username: ");
	gets_s(username, BUFF_SIZE);
	printf("Enter your password: ");
	gets_s(password, BUFF_SIZE);

	snprintf(buff, BUFF_SIZE, "USER %s %s", username, password);
	strcat_s(buff, ENDING_DELIMITER);
	Send(client, buff, strlen(buff), 0);
	Receive(client, buff, BUFF_SIZE, 0);
}

/*
@function ProcessChangePassword: Change password in server

@param client[IN]: socket connect with server
*/
void ProcessChangePassword(SOCKET client) {
	char buff[BUFF_SIZE], oldPassword[BUFF_SIZE], newPassword[BUFF_SIZE];
	printf("Enter your old password: ");
	gets_s(oldPassword, BUFF_SIZE);
	printf("Enter your new password: ");
	gets_s(newPassword, BUFF_SIZE);

	snprintf(buff, BUFF_SIZE, "CPW %s %s", oldPassword, newPassword);
	strcat_s(buff, ENDING_DELIMITER);
	Send(client, buff, strlen(buff), 0);
	Receive(client, buff, BUFF_SIZE, 0);
}

/*
@function ProcessLogout: Logout

@param client[IN]: socket connect with server
*/
void ProcessLogout(SOCKET client) {
	char buff[BUFF_SIZE];
	strcpy_s(buff, "BYE");
	strcat_s(buff, ENDING_DELIMITER);
	Send(client, buff, strlen(buff), 0);
	Receive(client, buff, BUFF_SIZE, 0);
}

/*
@function ProcessChangePassword: Get infomation of device from server and connect with device

@param client[IN]: socket connect with server
*/
void ProcessConnDevice(SOCKET client) {
	char buff[BUFF_SIZE], deviceCode[BUFF_SIZE];

	// Get device information
	printf("Enter your device code: ");
	gets_s(deviceCode, BUFF_SIZE);
	strcpy_s(buff, "GINFO ");
	strcat_s(buff, deviceCode);
	strcat_s(buff, ENDING_DELIMITER);
	Send(client, buff, strlen(buff), 0);

	char rcvBuff[BUFF_SIZE];
	Receive(client, rcvBuff, BUFF_SIZE, 0);
	if (rcvBuff[0] == '3' && rcvBuff[1] == '0') {
		CommunicateDevice(rcvBuff);
	}
}

/*
@function ProcessSetParam: Set Param in device

@param clientDev[IN]: socket connect with device
*/
void ProcessSetParam(SOCKET clientDev) {
	char buff[BUFF_SIZE], parameterName[BUFF_SIZE], parameterValue[BUFF_SIZE];
	printf("Enter your parameter name: ");
	gets_s(parameterName, BUFF_SIZE);
	printf("Enter your parameter value: ");
	gets_s(parameterValue, BUFF_SIZE);

	snprintf(buff, BUFF_SIZE, "SET %s %s", parameterName, parameterValue);
	strcat_s(buff, ENDING_DELIMITER);
	Send(clientDev, buff, strlen(buff), 0);
	Receive(clientDev, buff, BUFF_SIZE, 0);
}

/*
@function ProcessGetData: Get data from device

@param clientDev[IN]: socket connect with device
*/
void ProcessGetData(SOCKET clientDev) {
	char buff[BUFF_SIZE], parameterName[BUFF_SIZE];
	printf("Enter your parameter name: ");
	gets_s(parameterName, BUFF_SIZE);

	snprintf(buff, BUFF_SIZE, "GDATA %s", parameterName);
	strcat_s(buff, ENDING_DELIMITER);
	Send(clientDev, buff, strlen(buff), 0);
	Receive(clientDev, buff, BUFF_SIZE, 0);
}

/*
@function ProcessSetParam: Set Param in device

@param clientDev[IN]: socket connect with device
*/
void ProcessControlDevice(SOCKET clientDev) {
	char buff[BUFF_SIZE], status[BUFF_SIZE];
	printf("Enter your status (ON/OFF): ");
	gets_s(status, BUFF_SIZE);

	snprintf(buff, BUFF_SIZE, "CTRL %s", status);
	strcat_s(buff, ENDING_DELIMITER);
	Send(clientDev, buff, strlen(buff), 0);
	Receive(clientDev, buff, BUFF_SIZE, 0);
}

/*
@function CommunicateDevice: Communicata with device

@param rcvBuff[IN]: char array contains infomation of device
*/
void CommunicateDevice(char rcvBuff[]) {
	printf("Get device information is successfully!\n\n");

	char deviceAdd[BUFF_SIZE], devicePort[BUFF_SIZE], devicePassword[BUFF_SIZE];

	int i = 0;
	while (rcvBuff[i] != ' ' && rcvBuff[i] != '\0') i++;
	strncpy_s(rcvBuff, BUFF_SIZE, rcvBuff + i + 1, strlen(rcvBuff) - i - 1);

	i = 0;
	while (rcvBuff[i] != ' ' && rcvBuff[i] != '\0') i++;
	strncpy_s(deviceAdd, rcvBuff, i);
	strncpy_s(rcvBuff, BUFF_SIZE, rcvBuff + i + 1, strlen(rcvBuff) - i - 1);

	i = 0;
	while (rcvBuff[i] != ' ' && rcvBuff[i] != '\0') i++;
	strncpy_s(devicePort, rcvBuff, i);
	strncpy_s(rcvBuff, BUFF_SIZE, rcvBuff + i + 1, strlen(rcvBuff) - i - 1);

	i = 0;
	while (rcvBuff[i] != ' ' && rcvBuff[i] != '\0') i++;
	strncpy_s(devicePassword, rcvBuff, i);
	devicePassword[strlen(devicePassword) - 2] = 0;
	strncpy_s(rcvBuff, BUFF_SIZE, rcvBuff + i + 1, strlen(rcvBuff) - i - 1);

	SOCKET clientDev;
	clientDev = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientDev == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket", WSAGetLastError());
		return;
	}

	// Specify server address
	const int DEVICE_PORT = atoi(devicePort);
	const char *DEVICE_ADDR = deviceAdd;
	sockaddr_in deviceAddr;
	deviceAddr.sin_family = AF_INET;
	deviceAddr.sin_port = htons(DEVICE_PORT);
	inet_pton(AF_INET, DEVICE_ADDR, &deviceAddr.sin_addr);
	// Request to connect device server
	if (connect(clientDev, (sockaddr *)&deviceAddr, sizeof(deviceAddr))) {
		printf("Error %d: Cannot connect device server.", WSAGetLastError());
		return;
	}
	printf("Connect device server!\n");

	char buff[BUFF_SIZE];
	// Login to device
	strcpy_s(buff, "CONN ");
	strcat_s(buff, devicePassword);
	strcat_s(buff, ENDING_DELIMITER);
	Send(clientDev, buff, strlen(buff), 0);
	Receive(clientDev, rcvBuff, BUFF_SIZE, 0);

	if (rcvBuff[0] == '4' && rcvBuff[1] == '0') {
		char key[BUFF_SIZE];

		while (1) {
			printf(" ----------------------------------\n");
			printf(" |-------------MENU---------------|\n");
			printf(" |   1. Set the parameters.       |\n");
			printf(" |   2. Get data.                 |\n");
			printf(" |   3. Control device status.    |\n");
			printf(" |   #. Disconnect to device.     |\n");
			printf(" ----------------------------------\n");
			printf(">> Choose your selection : ");
			gets_s(key, BUFF_SIZE); // Enter selection.

									// User request to set the parameters
			if (strcmp(key, "1") == 0) {
				ProcessSetParam(clientDev);
			}

			// User request to get data
			else if (strcmp(key, "2") == 0) {
				ProcessGetData(clientDev);
			}

			// User request to control device status
			else if (strcmp(key, "3") == 0) {
				ProcessControlDevice(clientDev);
			}

			// User requested to disconnect to device
			else {
				closesocket(clientDev);
				printf("You have disconnected from the machine!\n");
				return;
			}
		} // end while
	} // end if (rcvBuff[0] == '4' && rcvBuff[1] == '0')
	else closesocket(clientDev);
}