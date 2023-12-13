#include "Snake.hpp"

//times in seconds
#define STEP_TIME 1
#define TREAT_TIME 5
char* address = "\\\\.\\COM5";

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <unistd.h>
#include <ncurses/ncurses.h>
#define readInput() getch()
#define initBoard() initscr();timeout(STEP_TIME)

#elif defined(_WIN32)
#include <conio.h>
#define readInput() _getch()
#define initBoard()

#elif defined(__linux__)
#include <unistd.h>
#include <ncurses.h>
#define readInput() getch()
#define initBoard() initscr();timeout(STEP_TIME)
#endif


int main(){
    SerialPort s1(address);
    GameBoard b1(s1);

    if (b1.isRunning() == false){
        return 1;
    }
    initBoard();

    while(b1.isRunning()){
        for(int i = 0; i < TREAT_TIME; ++i){
            b1.setMoveDir(readInput());
            if (b1.moveHead() == false){
                return 1;
            }
            sleep(STEP_TIME);
        }
    }
    return 0;
}