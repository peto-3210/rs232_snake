#include "Snake.hpp"

//times in seconds
//Default step time (miliseconds)
#define INIT_STEP_TIME 1000
//How often the treat is spawned (in steps)
#define TREAT_TIME 20
char* address = "\\\\.\\COM3";

int get_difference_ms(SYSTEMTIME &s1, SYSTEMTIME &s2);
int main(){
    init_console();

    SerialPort s1(address);
    GameBoard b1(s1, INIT_STEP_TIME);
    if (b1.isRunning() == false){
        return 1;
    }

    SYSTEMTIME reference;
    SYSTEMTIME current_time;
    GetSystemTime(&reference);
    int treat_counter = 0;

    while(b1.isRunning()){
        //Getting input
        do {
            if (b1.setMoveDir(read_input())){
                break;
            }
            sleep_ms(1);
            GetSystemTime(&current_time);
            int i = b1.getStepTime();
        } while (get_difference_ms(reference, current_time) < b1.getStepTime());

        //Moving 1 block
        if (b1.moveHead() == false){
            return 1;
        }
        GetSystemTime(&reference);
        ++treat_counter;

        //Adding treat
        if (treat_counter == TREAT_TIME){
            if (b1.generateTreat() == false){
                return 1;
            }
            treat_counter = 0;
        }

    }
    restore_console();
    b1.drawBorder();
    std::string msg = "Game over! Score: " + std::to_string(b1.getScore());
    std::cout << msg + "\n";
    b1.writeText(msg, CENTER_X - msg.length() / 2, CENTER_Y);
    int c = getchar();
    return 0;
}

int get_difference_ms(SYSTEMTIME &reference, SYSTEMTIME &current_time){
    return (current_time.wMinute * 60 + current_time.wSecond * 1000 + current_time.wMilliseconds) - 
    (reference.wMinute * 60 + reference.wSecond * 1000 + reference.wMilliseconds);
}