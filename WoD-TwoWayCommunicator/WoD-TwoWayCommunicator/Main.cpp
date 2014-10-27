// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved.
// Licensed under the BSD 2 - Clause License.
// See License.txt in the project root for license information.


// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RawAudio.h"
#include "arduino.h"

#define DAC_CS_PIN 2
#define CONTROL_BUTTON 3
#define READY_LED 4
#define MICROPHONE_INPUT A0

#define COMMUNICATOR_ONE_NAME L"CommunicatorOne"
#define COMMUNICATOR_TWO_NAME L"CommunicatorTwo"

void setup()
{
	pinMode(READY_LED, OUTPUT);
	digitalWrite(READY_LED, 0);
	pinMode(CONTROL_BUTTON, INPUT);
}

int _tmain(int argc, _TCHAR* argv[])
{
	setup();

	//Prepare Audio Manager
	RawAudio audio_manager = RawAudio();

	//setup destination location
	//get the computer name
	DWORD name_length = MAX_COMPUTERNAME_LENGTH + 1;
	LPWSTR computer_name = (LPWSTR)malloc(MAX_COMPUTERNAME_LENGTH + 1);
	GetComputerNameEx(ComputerNameDnsHostname, computer_name, &name_length);

	/* Determine the transmitter and receiver by comparing this computer's
	   name with communicator names */
	if (wcscmp(computer_name, COMMUNICATOR_ONE_NAME) == 0)
	{
		audio_manager.SetupStream("CommunicatorOne", "CommunicatorTwo");
	}
	else
	{
		audio_manager.SetupStream("CommunicatorTwo", "CommunicatorOne");
	}

	//Play startup noise, set Ready Light on
	audio_manager.PlayWavFile(L"C:\\Communicator\\aud\\ready.wav", DAC_CS_PIN);
	digitalWrite(READY_LED, 1);

	while (true)
	{
		if (digitalRead(CONTROL_BUTTON) == 1)
		{
			//stream out when the button is pressed, in 1 second clips
			audio_manager.StreamOutAnalog(DAC_CS_PIN, CONTROL_BUTTON, MICROPHONE_INPUT, 1);
		}
		else
		{
			//stream in 1 second clips
			audio_manager.StreamInAnalog(DAC_CS_PIN, CONTROL_BUTTON, 1);
		}
	}



}