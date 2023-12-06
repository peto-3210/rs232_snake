/*
* Author: Manash Kumar Mandal
* Modified Library introduced in Arduino Playground which does not work
* This works perfectly
* LICENSE: MIT
*/


#ifndef SERIALPORT_H
#define SERIALPORT_H

#define ARDUINO_WAIT_TIME 2000
#define MAX_DATA_LENGTH 255

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fileapi.h>

class SerialPort
{
	
public:
	HANDLE handler;
	bool connected;
	COMSTAT status;
	DWORD errors;

	SerialPort(char *portName);
	~SerialPort();

	int getTxNum() {ClearCommError(this->handler, &this->errors, &this->status); return this->status.cbOutQue;}
	int getRxNum() {ClearCommError(this->handler, &this->errors, &this->status); return this->status.cbInQue;}

	int readSerialPort(char *buffer, unsigned int buf_size);
	int writeSerialPort(char *buffer, unsigned int buf_size);
	bool isConnected();
	void closeSerial();
};

#endif // SERIALPORT_H
