// Server_PUT_TS_test.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib") 

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
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrLen); //Accept a new connection
	if (newConnection == 0) //If accepting the client connection failed
	{
		std::cout << "Failed to accept client's connection." << std::endl;
	}
	else //If client connection accepted
	{
		std::cout << "Client connected!" << std::endl;
		char messege[128] = "Welcome! Happy to see you here!."; //Buffer with messege
		send(newConnection, messege, sizeof(messege), NULL); //Send messege buffer
	}

	getchar();
	return 0;
}
