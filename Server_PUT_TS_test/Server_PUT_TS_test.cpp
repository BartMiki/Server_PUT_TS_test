// Server_PUT_TS_test.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib") 

// --- Setup ---

const int connectionsSize = 100;
SOCKET connections[connectionsSize]; //Array to hold connections
int connectionCounter = 0;

// --- Thread for client service ---

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

bool ProcessPacket(int ID, Packet packetType)
{
	switch (packetType)
	{
	case Packet::MsgServer:
	{
		u_int64 bufferSize; //size of a buffer
		recv(connections[ID], (char*)&bufferSize, sizeof(u_int64), NULL); //get buffer lenght
		char * buffer = new char[bufferSize + 1]; //Buffer to store message
		buffer[bufferSize] = '\0';
		recv(connections[ID], buffer, bufferSize + 1, NULL); //Receive data form client and store it into buffer
		for (int i = 0; i < connectionCounter; i++)
		{
			if (i == ID) //Clients can't send message to themeselves
				continue;
			packetType = Packet::MsgServer; //Message Send
			send(connections[i], (char*)&packetType, sizeof(Packet), NULL); //Send forward message
			send(connections[i], (char*)&bufferSize, sizeof(u_int64), NULL); //Sending message lenght
			send(connections[i], buffer, bufferSize + 1, NULL); //Sending message
		}
		delete[] buffer;
		break;
	}
	case Packet::MsgSend:
	{
		u_int64 bufferSize; //size of a buffer
		recv(connections[ID], (char*)&bufferSize, sizeof(u_int64), NULL); //get buffer lenght
		char * buffer = new char[bufferSize + 1]; //Buffer to store message
		buffer[bufferSize] = '\0';
		recv(connections[ID], buffer, bufferSize + 1, NULL); //Receive data form client and store it into buffer
		for (int i = 0; i < connectionCounter; i++)
		{
			if (i == ID) //Clients can't send message to themeselves
				continue;
			packetType = Packet::MsgSend; //Message Send
			send(connections[i], (char*)&packetType, sizeof(Packet), NULL); //Send forward message
			send(connections[i], (char*)&bufferSize, sizeof(u_int64), NULL); //Sending message lenght
			send(connections[i], buffer, bufferSize + 1, NULL); //Sending message
		}
		delete[] buffer;
		break;
	}
	case Packet::MsgReceived:
	{
		for (int i = 0; i < connectionCounter; i++)
		{
			if (i == ID) //Clients can't send message to themeselves
				continue;
			packetType = Packet::MsgReceived; //Message recevied
			send(connections[i], (char*)&packetType, sizeof(Packet), NULL); //Send forward that message was recevied
		}
		break;
	}
	default:
		std::cout << "Unrecognized packet: " << std::hex << (int)packetType << std::endl;
		break;
	}
	return true;
}

void ClientHandlerThread(int ID) //Connecions array index
{
	Packet packetType;
	while (true)
	{
		recv(connections[ID], (char*)&packetType, sizeof(Packet), NULL); //Received packet type

		if (!ProcessPacket(ID, packetType)) //If packet is not properly processed
			break; //Break infinite loop	
	}
}

int main()
{
	//WinSock2 startup
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 2);

	//if WSAStartup return anything other than 0, that means an error has occured
	if (WSAStartup(DllVersion, &wsaData) != 0)
	{
		MessageBoxA(NULL, "WinSock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	// --- Address setup ---

	SOCKADDR_IN addr; //Address to bind listening socket to
	int addrLen = sizeof(addr); //Address lenght
	inet_pton(AF_INET, "127.0.0.1",&(addr.sin_addr));//Brodacast locally
	addr.sin_port = htons(5555); //Port ("htons" means "Host TO Network Short")
	addr.sin_family = AF_INET; //IPv4 Socket
	
	// --- Binding address to listening socket ---

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL); //Socket for listening new connections
	bind(sListen, (SOCKADDR*)&addr, addrLen); //Bind address to the socket
	listen(sListen, SOMAXCONN); //sListen socket is listening

	// --- Socket for connection ---

	SOCKET newConnection; //Hold's client connection
	for (int i = 0; i < connectionsSize; i++) {
		newConnection = accept(sListen, (SOCKADDR*)&addr, &addrLen); //Accept a new connection
		if (newConnection == 0) //If accepting the client connection failed
		{
			std::cout << "Failed to accept client's connection." << std::endl;
		}
		else //If client connection accepted
		{
			std::cout << "Client connected!" << std::endl;
			std::string message = "Welcome! Happy to see you here!.\0"; //Buffer with messege
			u_int64 messageSize = message.size();
			Packet packetType = Packet::MsgServer; //Type of packet to be send
			send(newConnection, (char*)&packetType, sizeof(Packet), NULL); //Sending packet type
			send(newConnection, (char*)&messageSize, sizeof(u_int64), NULL); //Send message size
			send(newConnection, message.c_str(), messageSize, NULL); //Send messege buffer
			connections[i] = newConnection;
			connectionCounter ++;
			//Create Thread to handle this client. Value i is the index to connections array
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)i, NULL, NULL);

		}
	}
	getchar();
	return 0;
}
