//
//	Copyright © 2019 by Phillip Chang
//
#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <cstdio>
#include <vector>
#include <stdlib.h>
#include <iostream>

SOCKET socketServer = INVALID_SOCKET;
const char *infoServer[] = {"localhost", "8888"};
/*
	請將 idTeam 改成組別
*/
int idTeam = 11;

class _WSAData
{
public:
	_WSAData()
	{
		if (WSAStartup(MAKEWORD(2, 2), &wsadata_) != 0)
		{
			printf("WSAStartup Fail %d", WSAGetLastError());
		}
	}
	~_WSAData()
	{
		WSACleanup();
	}

private:
	WSADATA wsadata_;
} initializerWSA;

SOCKET _CreateConnectSocket(const char *strIp, const char *strPort)
{
	addrinfo addrinfoHint, *listDestAddrinfo = NULL, *addrinfoIterator = NULL;
	ZeroMemory(&addrinfoHint, sizeof(addrinfo));
	addrinfoHint.ai_family = AF_INET;
	addrinfoHint.ai_socktype = SOCK_STREAM;
	addrinfoHint.ai_protocol = IPPROTO_TCP;
	int errorApi = getaddrinfo(strIp, strPort, &addrinfoHint, &listDestAddrinfo);
	if (errorApi != 0)
		return INVALID_SOCKET;
	//	create socket
	SOCKET socketResult = socket(listDestAddrinfo->ai_family, listDestAddrinfo->ai_socktype, listDestAddrinfo->ai_protocol);
	if (socketResult == INVALID_SOCKET)
	{
		freeaddrinfo(listDestAddrinfo);
		return INVALID_SOCKET;
	}
	//	try connect
	errorApi = SOCKET_ERROR;
	addrinfoIterator = listDestAddrinfo;
	while (addrinfoIterator != NULL)
	{
		errorApi = connect(socketResult, addrinfoIterator->ai_addr, (int)addrinfoIterator->ai_addrlen);
		if (errorApi != SOCKET_ERROR)
			break;
		addrinfoIterator = addrinfoIterator->ai_next;
	}
	freeaddrinfo(listDestAddrinfo);
	if (errorApi == SOCKET_ERROR)
	{
		closesocket(socketResult);
		socketResult = INVALID_SOCKET;
	}
	return socketResult;
}
bool _SendToSocket(SOCKET socketDest, size_t lengthSend, const BYTE *rbSend)
{
	return send(socketDest, (const char *)rbSend, (int)lengthSend, 0) == lengthSend;
}
bool _RecvFromSocket(SOCKET socketSrc, size_t lengthRecv, BYTE *rbRecv)
{
	return recv(socketSrc, (char *)rbRecv, (int)lengthRecv, MSG_WAITALL) == lengthRecv;
}

void _ConnectToServer(int cntRecursive = 0)
{
	if (cntRecursive > 3)
	{
		printf("[Error] : maximum connection try reached!\n");
		return;
	}
	while (socketServer == INVALID_SOCKET)
		socketServer = _CreateConnectSocket(infoServer[0], infoServer[1]);

	if (!_SendToSocket(socketServer, sizeof(int), (BYTE *)&idTeam))
	{
		closesocket(socketServer);
		socketServer = INVALID_SOCKET;
		_ConnectToServer(cntRecursive + 1);
	}
}

void _ReconnectToServer()
{
	if (socketServer != INVALID_SOCKET)
	{
		closesocket(socketServer);
		socketServer = INVALID_SOCKET;
	}

	_ConnectToServer();
}

/*
	取得地圖
	return true/false, true-成功接收
*/

bool GetMap(int parallel_wall[16][17], int vertical_wall[17][16])
{

	if (socketServer == INVALID_SOCKET)
	{
		_ConnectToServer();
		if (socketServer == INVALID_SOCKET)
		{
			return false;
		}
	}
	BYTE rbHeader[8];
	if (!_RecvFromSocket(socketServer, 8, rbHeader))
	{
		printf("[Error] : Connection lose, trying to reconnect...\n");
		closesocket(socketServer);
		socketServer = INVALID_SOCKET;
		return false;
	}
	int codeHeader = *((int *)rbHeader);
	int id_package = *((int *)(rbHeader + 4));
	if (codeHeader == 0)
	{
		return false;
	}

	// parallel_wall
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 17; j++)
		{
			if (!_RecvFromSocket(socketServer, 4, (BYTE *)&(parallel_wall[i][j])))
			{
				printf("[Error] : Connection lose, trying to reconnect...\n");
				closesocket(socketServer);
				socketServer = INVALID_SOCKET;
				return false;
			}
		}
	}

	// vertical_wall
	for (int i = 0; i < 17; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (!_RecvFromSocket(socketServer, 4, (BYTE *)&(vertical_wall[i][j])))
			{
				printf("[Error] : Connection lose, trying to reconnect...\n");
				closesocket(socketServer);
				socketServer = INVALID_SOCKET;
				return false;
			}
		}
	}

	std::vector<BYTE> rbData;
	rbData.resize(4);
	int *rbInsert = (int *)&(rbData[0]);
	*rbInsert++ = 1;

	if (!_SendToSocket(socketServer, rbData.size(), &(rbData[0])))
	{
		printf("[Error] : Connection lose\n");
		return false;
	}

	return true;
}

/*
	取得當前遊戲狀態

	return True/False: True-end Game

	id_package : 當前棋盤狀態的 id，回傳移動訊息時需要使用
	playerStat: [x,y,n_landmine,super_time]
	ghostStat: [[x,y],[x,y],[x,y],[x,y]]
	propsStat: n_props*<type,x,y>
*/
bool GetGameStat(int &id_package,
				 int playerStat[5],
				 int otherPlayerStat[3][5],
				 int ghostStat[4][2],
				 std::vector<std::vector<int>> &propsStat)
{
	if (socketServer == INVALID_SOCKET)
	{
		_ConnectToServer();
		if (socketServer == INVALID_SOCKET)
		{
			return true;
		}
	}
	BYTE rbHeader[8];
	if (!_RecvFromSocket(socketServer, 8, rbHeader))
	{
		printf("[Error] : Connection lose\n");
		closesocket(socketServer);
		socketServer = INVALID_SOCKET;
		return true;
	}
	int codeHeader = *((int *)rbHeader);
	id_package = *((int *)(rbHeader + 4));
	if (codeHeader == 0)
	{
		// game end
		return true;
	}

	// playerStat
	for (int i = 0; i < 5; i++)
	{
		if (!_RecvFromSocket(socketServer, 4, (BYTE *)&(playerStat[i])))
		{
			printf("[Error] : Connection lose\n");
			closesocket(socketServer);
			socketServer = INVALID_SOCKET;
			return true;
		}
	}

	// otherPlayerStat
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			if (!_RecvFromSocket(socketServer, 4, (BYTE *)&(otherPlayerStat[i][j])))
			{
				printf("[Error] : Connection lose\n");
				closesocket(socketServer);
				socketServer = INVALID_SOCKET;
				return true;
			}
		}
	}

	// ghostStat
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			if (!_RecvFromSocket(socketServer, 4, (BYTE *)&(ghostStat[i][j])))
			{
				printf("[Error] : Connection lose\n");
				closesocket(socketServer);
				socketServer = INVALID_SOCKET;
				return true;
			}
		}
	}

	int n_props = 0;
	if (!_RecvFromSocket(socketServer, 4, (BYTE *)&(n_props)))
	{
		printf("[Error] : Connection lose\n");
		closesocket(socketServer);
		socketServer = INVALID_SOCKET;
		return true;
	}

	// propsStat
	propsStat.resize(n_props);
	for (int i = 0; i < n_props; i++)
	{
		propsStat[i].resize(3);
		for (int j = 0; j < 3; j++)
		{
			if (!_RecvFromSocket(socketServer, 4, (BYTE *)&(propsStat[i][j])))
			{
				printf("[Error] : Connection lose\n");
				closesocket(socketServer);
				socketServer = INVALID_SOCKET;
				return true;
			}
		}
	}

	return false;
}
/*
	向 server 傳達移動訊息
	id_package : 想要回復的訊息的 id_package
	Step : vector, <dir, is_throwLandmine>
			dir= 0: left, 1:right, 2: up, 3: down
			is_throwLandmine= True/False
*/
void SendStep(int id_package, std::vector<int> Step)
{
	if (socketServer == INVALID_SOCKET)
	{
		std::cout << "[Error] : trying to send step before connection is established\n"
				  << std::endl;

		return;
	}
	std::vector<BYTE> rbData;
	rbData.resize(8);
	int *rbInsert = (int *)&(rbData[0]);
	//*rbInsert++ = 1;
	//*rbInsert++ = id_package;
	*rbInsert++ = Step[0];
	*rbInsert++ = Step[1];

	if (!_SendToSocket(socketServer, rbData.size(), &(rbData[0])))
	{
		std::cout << "[Error] : Connection lose\n"
				  << std::endl;
	}
}
