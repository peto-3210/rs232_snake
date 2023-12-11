#include "SerialPort.h"
#include <stdint.h>
#include <iostream>

//#define VERIFY

char* address = "\\\\.\\COM5";

#define DATA_MASK 0x0001

#define size 4094


int main(void){
    SerialPort MySerial(address);
    SetCommMask(MySerial.handler, DATA_MASK);

    char buffer[size];
    char Rx[size] = {'\0'};

    int written_bytes = 0;
    #ifndef VERIFY
    SYSTEMTIME st1, st2;
    #endif
    bool one = 1;

    for (int j = 0; j < size; ++j){
            buffer[j] = j % UINT8_MAX + 1;
    }
    
    for(uint64_t i = 0; i < UINT64_MAX; ++i)
    {
        

        //uint8_t randomized = rand()%0xFF;
        #ifndef VERIFY
        GetSystemTime(&st1);
        #endif
        written_bytes = MySerial.writeSerialPort(buffer, size);
        

        int read_num = 0;
        int ready_bytes = 0;
        DWORD ret_mask = 0;
        do{

            WaitCommEvent(MySerial.handler, &ret_mask, NULL);
            if (ret_mask == DATA_MASK){
                ready_bytes = MySerial.getRxNum();
            }
            #ifndef VERIFY
            else{
                std::cout << "ERROR\n";
            }
            #endif
        } while (ready_bytes < size);

        read_num = MySerial.readSerialPort(Rx, ready_bytes);

        #ifndef VERIFY
        GetSystemTime(&st2);

        int differ = size - read_num; 
        if (memcmp(buffer, Rx, size)){
            std::cout<<"Fail: differ: " << differ << " read_num: " << read_num << " i: " << i << std::endl;
            while(MySerial.readSerialPort(Rx, 1));
        }
        /*else { 
            int diff = (st2.wSecond * 1000 + st2.wMilliseconds) - (st1.wSecond * 1000 + st1.wMilliseconds);           
            std::cout << "Success: i: " << i <<" difference in ms: " << diff <<  std::endl;
            if (diff < 0){
                std::cout << "Fatal error\n";
            }
        }*/
        #endif
        

    }

    return 0;
}


