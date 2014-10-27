// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved.
// Licensed under the BSD 2 - Clause License.
// See License.txt in the project root for license information.

// Communicator.cpp : a wrapper for setup and management of UDP communication

#include "Communicator.h"

#include <winsock.h>

SOCKET m_partner_socket;
WSADATA m_wsdata;
sockaddr_in m_server;
sockaddr_in m_dest;
const char * m_serv_hostname;
const char * m_dest_hostname;

Communicator::Communicator(){

}

Communicator::~Communicator(){
}

/*
	Starts the required Windows conection

	Returns 1 for _success, 0 for failure
*/
int Communicator::startWindowsConnection(){
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &m_wsdata) != 0)
	{
		return 0;
	}
	return 1; //1 for _success
}

int Communicator::closeWindowsConnection(){
	closesocket(m_partner_socket);
	return WSACleanup();
}

/*
Opens a UDP Socket

-1 for failure, else returns the sd
*/
int Communicator::openUDPSocket(){
	m_partner_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_partner_socket == INVALID_SOCKET){
		//cleanup windows connection and return 0
		WSACleanup();

		return -1;
	}
	int buffer_size = 100000000;
	setsockopt(m_partner_socket, SOL_SOCKET, SO_RCVBUF, (char *)&buffer_size, sizeof(int));

	//set the socket to be nonblocking
	u_long non_blocking = 1;
	ioctlsocket(m_partner_socket, FIONBIO, &non_blocking);

	return 1; //1 for _success
}

/*
Setup m_server details and bind
*/
int Communicator::setupServerAndBind(const char * serv_hostname){
	hostent * host;
	m_serv_hostname = serv_hostname;
	host = gethostbyname(m_serv_hostname);

	if (host == NULL){
		//failed hostname lookup
		WSACleanup();
		return 0;
	}

	/* Set family and port */
	m_server.sin_family = AF_INET;
	m_server.sin_port = htons(PORT_NUMBER);
	m_server.sin_addr.S_un.S_un_b.s_b1 = host->h_addr_list[0][0];
	m_server.sin_addr.S_un.S_un_b.s_b2 = host->h_addr_list[0][1];
	m_server.sin_addr.S_un.S_un_b.s_b3 = host->h_addr_list[0][2];
	m_server.sin_addr.S_un.S_un_b.s_b4 = host->h_addr_list[0][3];
	
	if (bind(m_partner_socket, (struct sockaddr *)&m_server, sizeof(m_server))){
		int se;
		se = WSAGetLastError();

		WSACleanup();
		closesocket(m_partner_socket);
		return 0;
	}
	
	
	return 1;
};

/*
Setup m_dest details
*/
int Communicator::setupDestination(const char * dest_hostname){
	hostent * host;
	m_dest_hostname = dest_hostname;
	host = gethostbyname(dest_hostname);

	while (host == NULL){
		/*
		  failed hostname lookup
		  keep looking for the receiver
		  we do this because the two communicator boxes won't boot at the same time
		  so one will always be first and not be able to find the other one
		
		  Of course, this means that the application will hang here until the partner
		  is discovered.
		*/
		host = gethostbyname(dest_hostname);
	}

	/* Set family and port */
	m_dest.sin_family = AF_INET;
	m_dest.sin_port = htons(PORT_NUMBER);
	m_dest.sin_addr.S_un.S_un_b.s_b1 = host->h_addr_list[0][0];
	m_dest.sin_addr.S_un.S_un_b.s_b2 = host->h_addr_list[0][1];
	m_dest.sin_addr.S_un.S_un_b.s_b3 = host->h_addr_list[0][2];
	m_dest.sin_addr.S_un.S_un_b.s_b4 = host->h_addr_list[0][3];


	return 1;
};

/*
Receives the 16bit DAC command for the DAC from the sender application
*/
UINT16 Communicator::receiveSample(){
	
	char * bytes = new char[4];
	int bytecount = -1;
	bytecount = recv(m_partner_socket, bytes, 4, 0);
	
	if (bytecount == -1){
		int se;
		se = WSAGetLastError();

		return 0;
	}
	
	UINT16 val = *(UINT16 *)bytes;

	delete bytes;
	return val;
}

/*
Receives the 16bit DAC command chunk for the DAC from the sender application
*/
UINT16 Communicator::receiveUDPChunk(char * recv_data, int chunk_size){

	int bytecount = -1;
	bytecount = recv(m_partner_socket, (char *)recv_data, chunk_size, 0);

	if (bytecount < 0){
		int se;
		se = WSAGetLastError();

		return se;
	}


	return bytecount;
}



/*
Sends bytes to dest
*/
int Communicator::sendUDPChunk(char * chunk, int chunk_size){

	return sendto(m_partner_socket, (char *)chunk, chunk_size, 0, (sockaddr *)&m_dest, sizeof(m_dest));

}