/*
* Author: Manash Kumar Mandal
* Modified Library introduced in Arduino Playground which does not work
* This works perfectly
* LICENSE: MIT
*/

#include "SerialPort.h"



/*void write_routine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped){
	int a = 1;
	written_bytes = 0;
}
OVERLAPPED lpOverlapped_w = {0};

void read_routine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped){
	int a = 1;
}
OVERLAPPED lpOverlapped_r = {0};*/

SerialPort::SerialPort(char *portName)
{
	this->connected = false;

	this->handler = CreateFileA(static_cast<LPCSTR>(portName),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL/* | FILE_FLAG_OVERLAPPED*/,
		NULL);
	if (this->handler == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			printf("ERROR: Handle was not attached. Reason: %s not available\n", portName);
		}
		else
		{
			printf("ERROR!!!");
		}
	}
	else {
		DCB dcbSerialParameters = { 0 };

		if (!GetCommState(this->handler, &dcbSerialParameters)) {
			printf("failed to get current serial parameters");
		}
		else {
			//dcbSerialParameters.BaudRate = 921600;
			dcbSerialParameters.BaudRate = CBR_115200;
			dcbSerialParameters.ByteSize = 8;
			dcbSerialParameters.StopBits = ONESTOPBIT;
			dcbSerialParameters.Parity = NOPARITY;
			dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

			if (!SetCommState(handler, &dcbSerialParameters))
			{
				printf("ALERT: could not set Serial port parameters\n");
			}
			else {
				this->connected = true;
				PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
				Sleep(ARDUINO_WAIT_TIME);
			}
		}
	}
}

SerialPort::~SerialPort()
{
	if (this->connected) {
		this->connected = false;
		CloseHandle(this->handler);
	}
}


int SerialPort::readSerialPort(char *buffer, unsigned int buf_size)
{
	DWORD bytesRead=0;
	unsigned int toRead = 0;

	ClearCommError(this->handler, &this->errors, &this->status);

	if (this->status.cbInQue > 0) {
		if (this->status.cbInQue > buf_size) {
			toRead = buf_size;
		}
		else toRead = this->status.cbInQue;
	}

	memset(buffer, 0, buf_size);

	if (ReadFile(this->handler, buffer, toRead, &bytesRead, NULL)) return bytesRead;
	
	return 0;
}

int SerialPort::writeSerialPort(char *buffer, unsigned int buf_size)
{
	DWORD bytesSend;


	if (!WriteFile(this->handler, (void*)buffer, buf_size, &bytesSend, NULL)) {
		ClearCommError(this->handler, &this->errors, &this->status);
		return 0;
	}
	else return bytesSend;
}

bool SerialPort::isConnected()
{
	if (!ClearCommError(this->handler, &this->errors, &this->status))
		this->connected = false;

	return this->connected;
}
// Close Connection
void SerialPort::closeSerial()
{
	CloseHandle(this->handler);
}


