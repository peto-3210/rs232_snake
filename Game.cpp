#include "Snake.hpp"

//times in seconds
#define STEP_TIME 1
#define TREAT_TIME 5
char* address = "\\\\.\\COM5";

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <unistd.h>
#include <ncurses/ncurses.h>

#elif defined(_WIN32)
#include <ncurses.h>

#elif defined(__linux__)
#include <unistd.h>
#endif


int main(){
    GameBoard b1(address);

    if (b1.isRunning() == false){
        return 1;
    }
    initscr();
    timeout(STEP_TIME);

    while(b1.isRunning()){
        for(int i = 0; i < TREAT_TIME; ++i){
            b1.setMoveDir(getch());
            if (b1.moveHead() == false){
                return 1;
            }
        }
    }
    return 0;
}