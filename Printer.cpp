#include "Printer.h"

Printer::Printer(char* ad)
{
    s = SerialPort(ad);
    for (size_t i = 0; i < MAX_X; i += 2)
    {
        for (size_t j = 0; j < MAX_Y; j += 2)
        {
            screen[i][j] = 1;
        }
        
    }
    
};