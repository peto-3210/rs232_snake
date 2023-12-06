#ifndef had
#define had

#include "SerialPort.h"

#define MAX_X 80
#define MAX_Y 40



class Printer
{
private:
    int SnakeLen;
    bool screen[80][40];

    SerialPort s;
public:
    Printer(){};
    Printer(char* address);
    ~Printer(){};
};


#endif