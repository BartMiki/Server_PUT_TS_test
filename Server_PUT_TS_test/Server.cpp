#include "stdafx.h"
#include "Server.h"


Server::Server(int PORT, bool broadcastLocally)
{
	DllVersion = MAKEWORD(2, 2);

	//if WSAStartup return anything other than 0, that means an error has occured
	if (WSAStartup(DllVersion, &wsaData) != 0)
	{
		MessageBoxA(NULL, "WinSock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	// --- Address setup ---
	addrLen = sizeof(addr); //Address lenght
	if (broadcastLocally) //Server open to public
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	else //Server only for our router
		inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));//Brodacast locally
	addr.sin_port = htons(PORT); //Port ("htons" means "Host TO Network Short")
	addr.sin_family = AF_INET; //IPv4 Socket

	// --- Binding address to listening socket ---
	sListen = socket(AF_INET, SOCK_STREAM, NULL); //Socket for listening new connections
	if (bind(sListen, (SOCKADDR*)&addr, addrLen) == SOCKET_ERROR) //Bind address to the socket
	{
		std::string error = "Failed to bind address to listetnin socket: " + std::to_string(WSAGetLastError());
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
	}
	if (listen(sListen, SOMAXCONN) == SOCKET_ERROR) //sListen socket is listening
	{
		std::string error = "Failed to listen on listetnin socket: " + std::to_string(WSAGetLastError());
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
	}
	serverPtr = this;
}

bool Server::ListenForNewConnections()
{
	SOCKET newConnection; //Hold's client connection
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrLen); //Accept a new connection
	if (newConnection == 0) //If accepting the client connection failed
	{
		std::cout << "Failed to accept client's connection." << std::endl;
		return false;
	}
	else //If client connection accepted
	{
		std::cout << "Client connected!" << std::endl;
		connections[TotalConnections] = newConnection;
		//Create Thread to handle this client. Value i is the index to connections array
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)TotalConnections, NULL, NULL);
		std::string message = "Welcome! Happy to see you here!.\0"; //Buffer with messege
		SendString(TotalConnections, message, Packet::MsgServer);
		TotalConnections++;
		return true;
	}
}

bool Server::ProcessPacket(int ID, Packet packetType)
{
	switch (packetType)
	{
	case Packet::MsgServer:
	{
		std::string message;
		if (!RecvString(ID, message))
			return false;
		for (int i = 0; i < TotalConnections; i++)
		{
			if (i == ID) //Clients can't send message to themeselves
				continue;
			if (!SendString(i, message, Packet::MsgServer))
			{
				std::cout << "Failed to send message from client ID: " << ID << " to client ID: " << i << "." << std::endl;
			}
		}
		std::cout << "Processed message packet from client ID: " << ID << "." << std::endl;
		break;
	}
	case Packet::MsgSend:
	{
		std::string message;
		if (!RecvString(ID, message))
			return false;
		for (int i = 0; i < TotalConnections; i++)
		{
			if (i == ID) //Clients can't send message to themeselves
				continue;
			if (!SendString(i, message))
			{
				std::cout << "Failed to send message from client ID: " << ID << " to client ID: " << i << "." << std::endl;
			}
		}
		std::cout << "Processed message packet from client ID: " << ID << "." << std::endl;
		break;
	}
	case Packet::MsgReceived:
	{
		for (int i = 0; i < TotalConnections; i++)
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

void Server::ClientHandlerThread(int ID) //Connecions array index
{
	Packet packetType;
	while (true)
	{
		if (!serverPtr->RecvPacketType(ID, packetType)) //If cannot receive packet properly
			break; //Break infinite loop
		if (!serverPtr->ProcessPacket(ID, packetType)) //If packet is not properly processed
			break; //Break infinite loop	
	}
	std::cout << "Lost connection to client ID: " << ID << "." << std::endl;
	closesocket(serverPtr->connections[ID]); //Close socked used by client after error occured 
}

bool Server::SendInt64(int ID, u_int64 num)
{
	int check = send(connections[ID], (char*)&num, sizeof(u_int64), NULL); //Send num in 8 bytes
	if (check == SOCKET_ERROR) //If sending failed
		return false;
	return true;
}

bool Server::RecvInt64(int ID, u_int64 & num)
{
	int check = recv(connections[ID], (char*)&num, sizeof(u_int64), NULL); //Receive 8 bytes (64 bits) num
	if (check == SOCKET_ERROR) //If receiving failed
		return false;
	return true;
}

bool Server::SendPacketType(int ID, Packet packetType)
{
	int check = send(connections[ID], (char*)&packetType, sizeof(Packet), NULL); //Send packetType in 1 byte
	if (check == SOCKET_ERROR) //If sending failed
		return false;
	return true;
}

bool Server::RecvPacketType(int ID, Packet & packetType)
{
	int check = recv(connections[ID], (char*)&packetType, sizeof(Packet), NULL); //Receive 1 byte (8 bit) packetType
	if (check == SOCKET_ERROR) //If receiving failed
		return false;
	return true;
}

bool Server::SendString(int ID, std::string str, Packet packetType)
{
	if (!SendPacketType(ID, packetType)) //If failed to send packet header
		return false;

	//str += "\0";
	u_int64 strSize = str.size();
	if (!SendInt64(ID, strSize)) //If failed to send size of string
		return false;

	int check = send(connections[ID], str.c_str(), strSize, NULL); //Send string in strSize bytes
	if (check == SOCKET_ERROR) //If sending failed
		return false;
	return true;
}

bool Server::RecvString(int ID, std::string & str)
{
	u_int64 bufferSize;
	if (!RecvInt64(ID, bufferSize)) //If failed to receive size of upcomming string
		return false;

	char * buffer = new char[bufferSize + 1];
	buffer[bufferSize] = '\0'; //adding terminator

	int check = recv(connections[ID], buffer, bufferSize, NULL); //recv string in bufferSize bytes
	str = buffer;
	delete[] buffer;
	if (check == SOCKET_ERROR) //If receiving failed
		return false;
	return true;
}

