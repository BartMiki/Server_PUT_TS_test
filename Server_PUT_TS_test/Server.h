#pragma once

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#pragma comment(lib,"ws2_32.lib") 

enum struct Packet : char {
	//								  reply  | operation
	//								---------+----------
	InviteSend = (char)0x1f,//			none | invite
	InviteAccepted = (char)0x2f,//	  accept | invite
	InviteDecline = (char)0x3f,//	 decline | invite
	MsgSend = (char)0x1e,//				none | sendMsg
	MsgReceived = (char)0x2e,//		recieved | sendMsg
	MsgFailure = (char)0x3e,//		  failed | sendMsg
	MsgServer = (char)0x4e,//			none | sendMsg
	CloseSession = (char)0x1c,//		none | close
	CloseSucces = (char)0x2c,//		 success | close
	CloseFail = (char)0x3c//			fail | close
};

class Server
{
public:
	// --- methods ---
	Server(int PORT, bool broadcastLocally = false);
	bool ListenForNewConnections();
private:
	// --- data fields ---
	static const int connectionsSize = 100;
	SOCKET connections[connectionsSize]; //Array to hold connections
	int TotalConnections = 0;
	WSAData wsaData;
	WORD DllVersion;
	SOCKADDR_IN addr; //Address to bind listening socket to
	SOCKET sListen;
	int addrLen;

	// --- methods send ---
	bool SendPacketType(int ID, Packet packetType);
	bool SendInt64(int ID, u_int64 num);
	bool SendString(int ID, std::string str, Packet packetType = Packet::MsgSend);

	// --- methods recv ---
	bool RecvPacketType(int ID, Packet & packetType);
	bool RecvInt64(int ID, u_int64 & num);
	bool RecvString(int ID, std::string & str);

	// --- methods other ---
	bool ProcessPacket(int ID, Packet packetType);
	static void ClientHandlerThread(int ID);
};

static Server * serverPtr;

