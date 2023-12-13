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
#define DATA_MASK 0x0001

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fileapi.h>
#include <stdint.h>
#include <iostream>


class SerialPort
{
	
public:
	HANDLE handler;
	bool connected;
	COMSTAT status;
	DWORD errors;

	SerialPort(char *portName);
	SerialPort();
	~SerialPort();

	int getTxNum() {ClearCommError(this->handler, &this->errors, &this->status); return this->status.cbOutQue;}
	int getRxNum() {ClearCommError(this->handler, &this->errors, &this->status); return this->status.cbInQue;}

	//Reads specified number of bytes to buffer, waits until all bytes arrive
	int readSerialPort(char *buffer, unsigned int buf_size);

	//Write function for buffer, one packet and 2 packets
	int writeSerialPort(char *buffer, unsigned int buf_size);
	int writeSerialPort(uint32_t value);
	int writeSerialPort(uint64_t value);

	bool isConnected();
	void closeSerial();
};

#endif // SERIALPORT_H
