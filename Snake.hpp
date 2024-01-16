
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <unistd.h>
#include <ncurses\ncurses.h>
#define sleep_s(x) sleep(x)
#define sleep_ms(x) usleep((x) * 1000)
#define is_key_pressed() (getch() != -1)
#define read_input() getch()
#define init_console() initscr(); nodelay(stdscr, TRUE)
#define restore_console() endwin()
#define set_position(x, y) move((y), (x))
#define print_char(x) addch(x)

#elif defined(_WIN32) 
#include <conio.h>
#include <Windows.h>
#define sleep_s(x) Sleep((x) * 1000)
#define sleep_ms(x) Sleep((x))
#define is_key_pressed() _kbhit()
#define read_input() _getch_nolock()
#define init_console() clrscr()
#define restore_console() clrscr()
#define set_position(x, y) gotoxy((x), (y))
#define print_char(x) putch(x)
#endif


//Corners of game board
#define MIN_X 1
#define MAX_X 78
#define MIN_Y 1
#define MAX_Y 38
#define CENTER_X 39
#define CENTER_Y 19

//Game settings
#define STEP_COEFICIENT 1.05

//For console
#define set_default_pos() set_position(0, MAX_Y + 2)
#define print_char_pos(x, y, c) set_position((x),(y)); print_char(c); set_position(0, MAX_Y + 2); read_input()

//Misc
#define MY_CRC (uint8_t)'l'

//Commands
#define WRITE_CHAR (uint8_t)'c'
#define SET_POS (uint8_t)'p'
#define SET_CURSOR (uint8_t)'s'
#define ERASE_SCR (uint8_t)'e'

//Attributes
#define ATTRIB_COLOR_BACK_R 0b00000001
#define ATTRIB_COLOR_BACK_G 0b00000010
#define ATTRIB_COLOR_BACK_B 0b00000100

#define ATTRIB_CHAR_COLOR_TEXT_R 0b00010000
#define ATTRIB_CHAR_COLOR_TEXT_G 0b00100000
#define ATTRIB_CHAR_COLOR_TEXT_B 0b01000000
#define ATTRIB_CHAR_BLINK 0b10000000

#define ATTRIB_CURS_MODE 0b00010000
#define ATTRIB_CURS_BLINK 0b00100000
#define ATTRIB_CURS_ON 0b01000000

//Misc attribs
#define ATTRIB_BLACK 0b00000000
#define ATTRIB_WHITE 0b00000111
#define DEFAULT_CHAR_ATTRIB  (ATTRIB_COLOR_BACK_B | ATTRIB_CHAR_COLOR_TEXT_R)
#define DEFAULT_CURS_PROP (ATTRIB_COLOR_BACK_G | ATTRIB_CURS_ON)

//snake body symbols
#define HORIZONTAL 255-37
#define VERTICAL 255-42
#define TOP_LEFT_CORNER 255-41
#define TOP_RIGHT_CORNER 255-35
#define BOT_LEFT_CORNER 255-44
#define BOT_RIGHT_CORNER 255-38
#define HEAD_CHARACTER 'O'

//Response
#define RESP_ACK (uint8_t)'A'
#define RESP_NO_ACK (uint8_t)'N'
#define RESP_CHANGE_ENTRY (uint8_t)'x'

#include "SerialPort.h"
#include <unordered_set>
#include <deque>
#include <string>
#include <cstring>



//Structure representing one RS232 packet
union serialPacket{
    serialPacket(uint8_t command, uint8_t param1, uint8_t param2){
        genericPacket.commandName = command;
        genericPacket.commandPar1 = param1;
        genericPacket.commandPar2 = param2;
        genericPacket.crc = MY_CRC; 
    }

    serialPacket(uint32_t packet){
        rawPacket = packet;
    }

    serialPacket(){
        memset(this, 0, sizeof(serialPacket));
    }

    std::string packet_to_string(){
        return (char)genericPacket.commandName + " " + 
            std::to_string(genericPacket.commandPar1) + " " +
            std::to_string(genericPacket.commandPar2) + " " +
            (char)genericPacket.crc;
    }

    struct {
        uint8_t commandName;
        uint8_t commandPar1;
        uint8_t commandPar2;
        uint8_t crc;
    } genericPacket;

    struct {
        uint8_t command;
        uint8_t character;
        uint8_t attrib;
        uint8_t crc;
    } writeChar;

    struct {
        uint8_t command;
        uint8_t x;
        uint8_t y;
        uint8_t crc;
    } moveCursor;

    struct {
        uint8_t command;
        uint8_t prop;
        uint8_t dummyByte1;
        uint8_t crc;
    } setCursor;

    struct {
        uint8_t command;
        uint8_t dummyByte1;
        uint8_t dummyByte2;
        uint8_t crc;
    } clearScreen;
    uint32_t rawPacket;

};

//Concatenated moveCursor and writeChar packets
union writePosPacket{
    writePosPacket(uint8_t x_cor, uint8_t y_cor, uint8_t ch, uint8_t attrib){
        set_pos = SET_POS;
        x = x_cor;
        y = y_cor;
        crc1 = MY_CRC;
        write_char = WRITE_CHAR;
        character = ch;
        attribute = attrib;
        crc2 = MY_CRC;
    }

    writePosPacket(uint64_t packet){
        rawPacket = packet;
    }

    struct{
        uint8_t set_pos;
        uint8_t x;
        uint8_t y;
        uint8_t crc1;
        uint8_t write_char;
        uint8_t character;
        uint8_t attribute;
        uint8_t crc2;
    };
    uint64_t rawPacket;
};

//Represents response received from device
union responsePacket{
    responsePacket(uint16_t packet){
        rawPacket = packet;
    }
    std::string packet_to_string(){
        return (char)response + " " + (char)command;
    }
    struct {
        uint8_t response;
        uint8_t command;
    };
    uint16_t rawPacket;
};

//Represents one block of snake
struct singleBlock{
    singleBlock(){
        memset(this, 0, sizeof(singleBlock));
    }
    singleBlock(uint8_t x_cor, uint8_t y_cor, uint8_t c, uint8_t attrib){
        character = c;
        attribute = attrib;
        x = x_cor;
        y = y_cor;
    };
    
    uint8_t character;
    uint8_t attribute;

    union {
        struct{
            uint8_t x;
            uint8_t y;
        };
        uint16_t posHash;

    };
};

enum direction{up, left, down, right};

class GameBoard{
    public:
        //Board
        GameBoard(SerialPort& s, int step_time);
        ~GameBoard();
        bool drawBorder();
        bool writeText(std::string str, uint8_t x, uint8_t y);
        bool isRunning(){return running;};
        int getStepTime(){return stepTime;};

        //Snake
        bool setMoveDir(uint8_t dir);
        bool moveHead();

        //Treats
        bool generateTreat();
        int getScore(){return score;};

    private:
        //Board
        SerialPort Serial;
        bool running;
        int stepTime;

        bool sendSerial(uint32_t* buffer, int size);
        bool sendSerial(serialPacket& packet);
        bool writePosChar(writePosPacket& drawPacket);
        bool writeBlock(singleBlock& block);
        
        //Snake
        singleBlock head;
        std::deque<singleBlock> snakeBody;
        direction lastDir;
        direction currentDir;

        bool InitSnake();
        uint8_t selectChar();
        bool addBlock();
        bool deleteBlock();
        bool detectCollision();
        
        //Treats
        std::unordered_set<uint16_t> treats;
        int score;
        bool detectTreat();

        static serialPacket getEraseScreenPacket(){return serialPacket(ERASE_SCR, 0, 0);}
        static serialPacket getSetCursorPacket(uint8_t properties){return serialPacket(SET_CURSOR, properties, 0);}
        static serialPacket getSetPositionPacket(uint8_t x, uint8_t y){return serialPacket(SET_POS, x, y);}
        static serialPacket getWriteCharPacket(char c, uint8_t attribute){return serialPacket(WRITE_CHAR, c, attribute);}
        static writePosPacket writePosCharPacket(uint8_t x, uint8_t y, char c, uint8_t attrib){return writePosPacket(x, y, c, attrib);}
        static writePosPacket writePosCharPacket(singleBlock& block){return writePosPacket(block.x, block.y, block.character, block.attribute);}
        static writePosPacket erasePosPacket(uint8_t x, uint8_t y){return writePosPacket(x, y, ' ', ATTRIB_BLACK);}
};