// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved.
// Licensed under the BSD 2 - Clause License.
// See License.txt in the project root for license information.

/**
* Communicator is a wrapper for setup and management of UDP communication
**/

#ifndef RAWAUDIO_H
#define RAWAUDIO_H

#include "windows.h"
#include "Communicator.h"

class RawAudio{
	Communicator m_network_communicator;
public:

	RawAudio();
	~RawAudio();

	/*
		Sets up the communicator and destination information for the audio stream

		@params:
		serv_hostname - the hostname of the server to use for this instance
		(should be the name of the local pc)
		dest_hostname - the hostname of the machine to receive streamed data

	*/
	int SetupStream(const char * serv_hostname, const char * dest_hostname);

	/*
		Tear down networking data structs
	*/
	int TeardownStream();

	/*
		Prepares the 8 bit samples for the DAC being used
		
		@params:
		samples - pointer to the 8 bit audio samples
		modified - pointer to the modified array for 12bit data samples
		data - pointer to the storage array to put the converted 8bit bytes
		control - the DAC control bits
		file_size - the number of samples
	*/
	int prepareSamplesForDac(UINT8 * samples, UINT16 * modified, UINT8 * data, UINT8 control, DWORD file_size);

	/*
		Prepends the DAC control bits to the samples modified to the appropriate width for the DAC

		@params:
		modified - pointer to the modified array for 12bit data samples
		data - pointer to the storage array to put the converted 8bit bytes
		control - the DAC control bits
		file_size - the number of samples
	*/
	int prependControlBits(UINT8 * data, UINT16 * modified, UINT8 control, DWORD file_size);

	/*
		Plays 8-bit PCM 8kHz WAV files.

		@params:
		file_name - the WAV file to play
		dac_cs - the GPIO output connected to the dac cs pin
	*/
	int PlayWavFile(LPCWSTR file_name, int dac_cs);

	/*
		Streams out an 8-bit PCM 8kHz WAV file. Sends the data in chunks of size defined
		by buf_size

		@params:
		file_name - the WAV file to be streamed
		buf_size - the number of samples to be sent,
		actually ends up sending twice as many bytes as samples
	*/
	int StreamOutWavFile(LPCWSTR file_name, unsigned int buf_size);
	
	/*
		Stream in 8-bit PCM 8kHz WAV data. Requires the DAC_CS pin to use, and the expected 
		buffer size

		@params:
		dac_cs - the dac chip select, used to play alerts
		buf_size - the number of samples to be sent, 
			actually ends up receiving twice as many bytes as samples
	*/
	int StreamAndPlayAudio(int dac_cs, unsigned int buf_size);

	/*
		Streams out raw analog samples taken from the analog microphone feeding it's input 
		to input_pin. Records audio at a 16kHz rate, streams until the button defined by 
		control_pin is released

		@params:
		dac_cs - the dac chip select, used to play alerts
		input_pin - the pin being fed analog audio data
		control_pin - the button that needs to be held to stay in record mode
		buffer_length_in_seconds - defines the size of the buffer to use, in seconds. 
			One second of recorded audio has 16k samples
	*/
	int StreamOutAnalog(int dac_cs, int input_pin, int control_pin, int buffer_length_in_seconds);

	/*
		Streams in raw analog samples taken from another machine
		Plays audio at a 16kHz rate, streams until the button defined by control_pin is pressed

		@params:
		dac_cs - the dac chip select, used to play alerts
		control_pin - the button that needs to be held to stay in record mode
		buffer_length_in_seconds - defines the size of the buffer to use, in seconds.
			One second of recorded audio has 16k samples
	*/
	int StreamInAnalog(int dac_cs, int control_pin, int buffer_length_in_seconds);



};


#endif