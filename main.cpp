#include "SerialPort.h"
#include <stdint.h>
#include <iostream>

#define VERIFY

#define MAX_X 80
#define MAX_Y 40
#define CHARATRIB 0b01110000


char* address = "\\\\.\\COM5";

#define DATA_MASK 0x0001

#define size 4
  
int setchar(SerialPort *MySerial, char c, char atr, int x, int y){
    #define NUMCHARSET 8
    char data[NUMCHARSET];
    data[0] = 'p';
    data[1] = x;
    data[2] = y;
    data[3] = 'l';
    data[4] = 'c';
    data[5] = c;
    data[6] = atr;
    data[7] = 'l';
    
    int TxBytes = MySerial->writeSerialPort(data, NUMCHARSET);
     
    return TxBytes;
    
}

char clearScr(SerialPort *MySerial){        //return 0 if good, 1 if bad transmision
    #define NUMCHARCLEAR 4
    char data[NUMCHARCLEAR];
    data[0] = 'e';
    data[1] = 0;
    data[2] = 0;
    data[3] = 'l';
    while(MySerial->getRxNum()){                                    //clear buffer
        char junk;
        MySerial->readSerialPort(&junk, 1);
    }

    int TxBytes = MySerial->writeSerialPort(data, NUMCHARCLEAR);
    if (TxBytes != NUMCHARCLEAR) 
        return TxBytes;

    int RxReadyBytes = 0;   //number of bytes ready to be read from RX buffer
    DWORD RxMask = 0;
    do{
        WaitCommEvent(MySerial->handler, &RxMask, NULL);
        if (RxMask == DATA_MASK){
            RxReadyBytes = MySerial->getRxNum();
        }
        #ifndef VERIFY
        else{
            std::cout << "ERROR\n";
        }
        #endif
    } while (RxReadyBytes < NUMCHARCLEAR/2);
    char RxData[NUMCHARCLEAR/2];
    int RxBytes = MySerial->readSerialPort(RxData, NUMCHARCLEAR/2);
    return RxData[0];
}

char setCtl(SerialPort *MySerial, bool R, bool G, bool B, bool blinking){        //return 0 if good, 1 if bad transmision
    #define NUMCTLSET 4
    char data[NUMCTLSET];
    data[0] = 's';
    data[1] = 0;
    data[2] = 0;
    data[3] = 'l';
    data[1] = data[1]||(B << 0)||(G << 1)||(R << 2)||(blinking << 5);
    while(MySerial->getRxNum()){                                                        //clear buffer
        char junk;
        MySerial->readSerialPort(&junk, 1);
    }
    int TxBytes = MySerial->writeSerialPort(data, NUMCTLSET);
    if (TxBytes != NUMCTLSET) 
        return TxBytes;
    int RxReadyBytes = 0;   //number of bytes ready to be read from RX buffer
    DWORD RxMask = 0;
    do{
        WaitCommEvent(MySerial->handler, &RxMask, NULL);
        if (RxMask == DATA_MASK){
            RxReadyBytes = MySerial->getRxNum();
        }
        #ifndef VERIFY
        else{
            std::cout << "ERROR\n";
        }
        #endif
    } while (RxReadyBytes < NUMCTLSET/2);
    char RxData[NUMCTLSET/2];
    int RxBytes = MySerial->readSerialPort(RxData, NUMCTLSET/2);
    return RxData[0];
    
}

int main(void){
    SerialPort MySerial(address);
    SetCommMask(MySerial.handler, DATA_MASK);

    setchar(&MySerial, ' ',CHARATRIB,0,0);                         //set char atributes to black background, white cahr
    if (setCtl(&MySerial,1,1,1,0) != 'A') return 1;                //set color atributes of cursor
    if (clearScr(&MySerial) != 'A') return 1;                      //clear whole screen

    for (int i = 0; i < MAX_X; i += 2)
    {
        setchar(&MySerial, ' ',0b00000111,i,0);             //black char, white background
        setchar(&MySerial, ' ',0b00000111,i+1,MAX_Y-1);
    }
    for (int i = 0; i < MAX_Y; i += 2)
    {
        setchar(&MySerial, ' ',0b00000111,0,i);             //black char, white background
        setchar(&MySerial, ' ',0b00000111,MAX_X-1,i+1);
    }
///--------------------------------------------------------------------------------------------------------
//----------------------------boarders-set-----------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------

    char Rx[size] = {'\0'};

    

    setchar(&MySerial, '0', CHARATRIB, 5,5);

    int r;
    char ret[4];
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
        } while (ready_bytes != 4);
    r = MySerial.readSerialPort(ret, ready_bytes);
/*
    int written_bytes = 0;
    #ifndef VERIFY
    SYSTEMTIME st1, st2;
    #endif
    bool one = 1;

   */
    /*
    for(uint64_t i = 0; i < UINT64_MAX; ++i)
    {
        

        //uint8_t randomized = rand()%0xFF;
        #ifndef VERIFY
        GetSystemTime(&st1);
        #endif
        //written_bytes = MySerial.writeSerialPort(buffer, size);
        

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
        }
        #endif
        

    }*/

    return 0;
}


