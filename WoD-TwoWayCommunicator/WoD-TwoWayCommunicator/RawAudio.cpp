// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved.
// Licensed under the BSD 2 - Clause License.
// See License.txt in the project root for license information.

#include "RawAudio.h"
#include "arduino.h"
#include "MCP4921.h"
#include "spi.h"

//return 1 if read failed
#define CHECK_SUCC 	\
if (succ == 0)\
{\
	return 0;\
}

#define WAV_HEADER_SIZE 46

//tweak these numbers if you're finding the playback is too slow or fast
#define DELAY_8KHZ 80
#define DELAY_16KHZ 45
#define SAMPLE_COUNT_16KHZ 16000

//empty constructor
RawAudio::RawAudio()
{

}
//empty destructor
RawAudio::~RawAudio()
{

}

/*
	Sets up the communicator and destination information for the audio stream

	@params:
	serv_hostname - the hostname of the server to use for this instance
	(should be the name of the local pc)
	dest_hostname - the hostname of the machine to receive streamed data

*/
int RawAudio::SetupStream(const char * serv_hostname, const char * dest_hostname)
{
	m_network_communicator = Communicator();
	m_network_communicator.startWindowsConnection();
	m_network_communicator.openUDPSocket();
	m_network_communicator.setupServerAndBind(serv_hostname);
	m_network_communicator.setupDestination(dest_hostname);
	return 0;
}

/*
	Tear down networking data structs
*/
int RawAudio::TeardownStream()
{
	m_network_communicator.closeWindowsConnection();
	return 0;
}

/*
	Prepares the 8 bit samples for the DAC being used

	@params:
	samples - pointer to the 8 bit audio samples
	modified - pointer to the modified array for 12bit data samples
	data - pointer to the storage array to put the converted 8bit bytes
	control - the DAC control bits
	file_size - the number of samples
*/
int RawAudio::prepareSamplesForDac(UINT8 * samples, UINT16 * modified, UINT8 * data, UINT8 control, DWORD file_size)
{
	/*
	Note: for memory/processing efficiency, this could be folded into one loop
	It's left as two to illustrate what's going on with the manipulations
	*/
	for (DWORD n = 0; n < file_size; n++){
		//adjust to 12bit unsigned int for MCP4921
		modified[n] = (UINT16)(((samples[n]) / 255.0) * 4095);
	}

	//add control bits for MCP4921
	prependControlBits(data, modified, control, file_size);
	
	return 0;
}

/*
	Prepends the DAC control bits to the samples modified to the appropriate width for the DAC

	@params:
	modified - pointer to the modified array for 12bit data samples
	data - pointer to the storage array to put the converted 8bit bytes
	control - the DAC control bits
	file_size - the number of samples
*/
int RawAudio::prependControlBits(UINT8 * data, UINT16 * modified, UINT8 control, DWORD file_size)
{
	for (DWORD n = 0, x = 0; x < file_size; x++){
		data[n] = (((control << 4) & 0xF0) | ((modified[x] >> 8) & 0x0F));
		data[n + 1] = modified[x] & 0xFF;
		n += 2;
	}

	return 0;
}

/*
	Plays 8-bit PCM 8kHz WAV files.

	@params:
	file_name - the WAV file to play
	dac_cs - the GPIO output connected to the dac cs pin
*/
int RawAudio::PlayWavFile(LPCWSTR file_name, int dac_cs)
{
	HANDLE wav_file;
	char * buf;
	int succ;
	DWORD file_size; //size of file in bytes
	UINT8 * samples; //holds the 8 bit samples read from the file
	UINT16 * modified; //holds the 8 bit samples converted into 12 bit samples
	UINT8 * data; //holds the 8 bit words configured to send to the DAC
	UINT8 control = CONFIG_DACA | CONFIG_STANDARD_OUTPUT | CONFIG_1X_GAIN | CONFIG_OUTPUT_ON; //DAC control bits
	DWORD i = 0; //iterator for playback

	wav_file = CreateFile(
		file_name,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);

	buf = (char *)malloc(WAV_HEADER_SIZE);

	//read the header bytes in the WAV file
	succ = ReadFile(
		wav_file,
		buf,
		WAV_HEADER_SIZE,
		NULL,
		NULL
		);

	CHECK_SUCC

	//throw the bytes out -- we are assuming 8kHz 8 bit PCM
	free(buf);

	//read file into memory
	file_size = GetFileSize(wav_file, nullptr);
	file_size = file_size - WAV_HEADER_SIZE; //adjust for header bytes
	samples = (UINT8 *)malloc(file_size);
	succ = ReadFile(wav_file, samples, file_size, NULL, NULL);

	CHECK_SUCC

	//pre-prepare the data to send to the DAC
	data = (UINT8 *)malloc(sizeof(UINT8)* file_size * 2);
	modified = (UINT16 *)malloc(sizeof(UINT16)* file_size);
	prepareSamplesForDac(samples, modified, data, control, file_size);
	
	file_size = file_size * 2; //compensate for control bytes, samples are now 16bits/sample
	
	//prepare pins for SPI
	pinMode(dac_cs, OUTPUT);
	digitalWrite(dac_cs, HIGH);
	SPI.begin();
	while (i < file_size)
	{
		//output a sample
		digitalWrite(dac_cs, LOW);
		SPI.transfer(data[i++]);
		SPI.transfer(data[i++]);
		digitalWrite(dac_cs, HIGH);
		//delay to get ~8kHz
		delayMicroseconds(DELAY_8KHZ);
	}
	SPI.end();

	free(samples);

	return 0;


}

/*
	Streams out an 8-bit PCM 8kHz WAV file. Sends the data in chunks of size defined
	by buf_size

	@params:
	file_name - the WAV file to be streamed
	buf_size - the number of samples to be sent,
	actually ends up sending twice as many bytes as samples
*/
int RawAudio::StreamOutWavFile(LPCWSTR file_name, unsigned int buf_size)
{
	
	HANDLE wav_file;
	char * buf;
	int succ;
	DWORD file_size; //size of file in bytes
	UINT8 * samples; //holds the 8 bit samples read from the file
	UINT16 * modified; //holds the 8 bit samples converted into 12 bit samples
	UINT8 * data; //holds the 8 bit words configured to send to the DAC
	UINT8 control = CONFIG_DACA | CONFIG_STANDARD_OUTPUT | CONFIG_1X_GAIN | CONFIG_OUTPUT_ON; //DAC control
	int i = 0; //iterator for playback

	wav_file = CreateFile(
		file_name,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);

	buf = (char *)malloc(WAV_HEADER_SIZE);

	//read the header bytes
	succ = ReadFile(
		wav_file,
		buf,
		WAV_HEADER_SIZE,
		NULL,
		NULL
		);

	CHECK_SUCC

	free(buf);

	//read file into memory
	file_size = GetFileSize(wav_file, nullptr);
	file_size = file_size - WAV_HEADER_SIZE; //adjust for header bytes
	samples = (UINT8 *)malloc(file_size);
	succ = ReadFile(wav_file, samples, file_size, NULL, NULL);

	CHECK_SUCC

	//pre-prepare the data to send to the DAC
	data = (UINT8 *)malloc(sizeof(UINT8)* file_size * 2);
	modified = (UINT16 *)malloc(sizeof(UINT16)* file_size);
	prepareSamplesForDac(samples, modified, data, control, file_size);

	file_size = file_size * 2; //compensate for control bytes, samples are now 16bit/sample
	for (DWORD n = 0; n < file_size;){
		m_network_communicator.sendUDPChunk((char *)&data[n], min(buf_size, file_size - buf_size));
		n += buf_size;
	}

	free(samples);

	return 0;
}

/*
	Stream in 8-bit PCM 8kHz WAV data. Requires the DAC_CS pin to use, and the expected
	buffer size

	@params:
	dac_cs - the dac chip select, used to play alerts
	buf_size - the number of samples to be sent,
	actually ends up receiving twice as many bytes as samples
*/
int RawAudio::StreamAndPlayAudio(int dac_cs, unsigned int buf_size)
{
	UINT8 * data = (UINT8 *)malloc(buf_size * 2);
	pinMode(dac_cs, OUTPUT);
	digitalWrite(dac_cs, HIGH);
	SPI.begin();

	//Currently, this function doesn't return.
	while (true)
	{
		int x = m_network_communicator.receiveUDPChunk((char *)data, buf_size);

		//if no data received, do nothing
		if (x == WSAEWOULDBLOCK)
		{
			continue;
		}

		for (int i = 0; i < x;)
		{
			//output a sample
			digitalWrite(dac_cs, LOW);
			SPI.transfer(data[i++]);
			SPI.transfer(data[i++]);
			digitalWrite(dac_cs, HIGH);

			//delay to get 8kHz
			delayMicroseconds(DELAY_8KHZ);
		}

	}
	SPI.end();
	return 0;
}

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
int RawAudio::StreamOutAnalog(int dac_cs, int input_pin, int control_pin, int buffer_length_in_seconds)
{

	analogReadResolution(12);
	int buf_size = SAMPLE_COUNT_16KHZ * buffer_length_in_seconds;
	UINT16 * samples = (UINT16 *)malloc(sizeof(UINT16)* buf_size);
	UINT8 * data = (UINT8 *)malloc(sizeof(UINT8)* buf_size * 2);
	UINT8 control = CONFIG_DACA | CONFIG_STANDARD_OUTPUT | CONFIG_1X_GAIN | CONFIG_OUTPUT_ON; //DAC control

	PlayWavFile(L"C:\\Communicator\\aud\\record.wav", dac_cs);

	//while the control pin is pressed, record audio clips
	//perform checks at 1s intervals so that we don't get stuttering
	while (digitalRead(control_pin) == 1)
	{
		//record samples at a 16kHz rate
		for (int i = 0; i < buf_size; i++)
		{
			samples[i] = analogRead(input_pin);
			delayMicroseconds(DELAY_16KHZ);
		}

		//add control bits for MCP4921
		prependControlBits(data, samples, control, buf_size);

		//transmit
		m_network_communicator.sendUDPChunk((char *)data, buf_size*2);
	}

	return 0;
}

/*
	Streams in raw analog samples taken from another machine
	Plays audio at a 16kHz rate, streams until the button defined by control_pin is pressed

	@params:
	dac_cs - the dac chip select, used to play alerts
	control_pin - the button that needs to be held to stay in record mode
	buffer_length_in_seconds - defines the size of the buffer to use, in seconds.
	One second of recorded audio has 16k samples
*/
int RawAudio::StreamInAnalog(int dac_cs, int control_pin, int buffer_length_in_seconds)
{
	int buf_size = SAMPLE_COUNT_16KHZ * 2 * buffer_length_in_seconds;
	UINT8 * data = (UINT8 *)malloc(buf_size);

	PlayWavFile(L"C:\\Communicator\\aud\\waiting.wav", dac_cs);

	pinMode(dac_cs, OUTPUT);
	digitalWrite(dac_cs, HIGH);
	SPI.begin();

	//exit when the user presses the transmit button
	while (digitalRead(control_pin) == 0)
	{
		int x = m_network_communicator.receiveUDPChunk((char *)data, buf_size);
		
		//if no data received, do nothing
		if (x == WSAEWOULDBLOCK)
		{
			continue;
		}

		for (int i = 0; i < x;)
		{
			//output a sample
			digitalWrite(dac_cs, LOW);
			SPI.transfer(data[i++]);
			SPI.transfer(data[i++]);
			digitalWrite(dac_cs, HIGH);

			//delay to get 16kHz
			delayMicroseconds(DELAY_16KHZ);
		}

	}
	SPI.end();

	return 0;
}
