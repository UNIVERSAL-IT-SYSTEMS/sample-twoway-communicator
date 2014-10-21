/**
 * Communicator is a wrapper for setup and management of UDP communication
 **/

#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "windows.h"

#define PORT_NUMBER  10001

class Communicator{
public:
	Communicator();
	~Communicator();
	/*
	Starts the required Windows conection
	*/
	int startWindowsConnection();

	/*
	Close the windows connection
	*/
	int closeWindowsConnection();

	/*
	Opens a UDP Socket
	*/
	int openUDPSocket();

	/*
	Setup m_server details and bind
	*/
	int setupServerAndBind(const char * serv_hostname);

	/*
	Setup m_dest details
	*/
	int setupDestination(const char * dest_hostname);

	/*
	Receives the 12 bit (formatted as uint16) sample for the ADC from the sender application
	*/
	UINT16 receiveSample();

	/*
	Generates the 12bit sample (formatted as uint 16) from the passed integer and sends it off
	*/
	int sendSample(UINT8 p_sample);

	/*
	Generates the 16bit DAC code (formatted as uint 16) from a passed array of samples and sends it
	*/
	int sendUDPChunk(char * p_chunk, int p_chunk_size);

	/*
	Receives the 16bit DAC command chunk for the DAC from the sender application
	*/
	UINT16 receiveUDPChunk(char * recv_data, int p_chunk_size);
};


#endif