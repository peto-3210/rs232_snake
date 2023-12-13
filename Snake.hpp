//Corners of game board
#define MIN_X 1
#define MAX_X 78
#define MIN_Y 1
#define MAX_Y 38

//Misc
#define MY_CRC (uint8_t)'l'
#define COMMAND_SIZE 4
#define PACKET_DATATYPE uint32_t

//Commands
#define WRITE_CHAR (uint8_t)'C'
#define SET_POS (uint8_t)'P'
#define SET_CURSOR (uint8_t)'S'
#define ERASE_SCR (uint8_t)'E'

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

//Response
#define RESP_ACK (uint8_t)'A'
#define RESP_NO_ACK (uint8_t)'N'
#define RESP_CHANGE_ENTRY (uint8_t)'x'

#include "SerialPort.h"
#include <unordered_set>
#include <deque>


char* address = "\\\\.\\COM5";

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
    writePosPacket(uint8_t x, uint8_t y, uint8_t ch, uint8_t attrib){
        position = serialPacket(SET_POS, x, y);
        character = serialPacket(WRITE_CHAR, ch, attrib);
    }

    writePosPacket(uint64_t packet){
        rawPacket = packet;
    }

    struct{
        serialPacket position;
        serialPacket character;
    };
    uint64_t rawPacket;
};

//Represents response received from device
union responsePacket{
    responsePacket(uint16_t packet){
        rawPacket = packet;
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

enum direction{up, down, right, left};

class GameBoard{
    public:
        //Board
        GameBoard(char* conn);
        ~GameBoard();

        //Snake
        bool setMoveDir(uint8_t dir);
        int moveHead();

        //Treats
        bool generateTreat();

    private:
        //Board
        SerialPort Serial;

        bool sendSerial(serialPacket& packet);
        bool sendSerial(writePosPacket& drawPacket);
        bool sendSerial(uint32_t* buffer, int size);
        bool sendSerial(singleBlock& block);
        bool drawBorder();

        //Snake
        singleBlock head;
        std::deque<singleBlock> snakeBody;
        direction lastDir;
        direction currentDir;

        uint8_t selectChar();
        bool addBlock();
        bool deleteBlock();
        bool detectCollision();
        
        //Treats
        std::unordered_set<uint16_t> treats;

        bool detectTreat();

        static serialPacket getEraseScreenPacket(){return serialPacket(ERASE_SCR, 0, 0);}
        static serialPacket getSetCursorPacket(uint8_t properties){return serialPacket(SET_CURSOR, properties, 0);}
        static serialPacket getSetPositionPacket(uint8_t x, uint8_t y){return serialPacket(SET_POS, x, y);}
        static serialPacket getWriteCharPacket(char c, uint8_t attribute){return serialPacket(WRITE_CHAR, c, attribute);}
        static writePosPacket writeCharacterToPosition(uint8_t x, uint8_t y, char c, uint8_t attrib){return writePosPacket(x, y, c, attrib);}
        static writePosPacket writeCharacterToPosition(singleBlock& block){return writePosPacket(block.x, block.y, block.character, block.attribute);}
        static writePosPacket eraseCharacterAtPosition(uint8_t x, uint8_t y){return writePosPacket(x, y, ' ', ATTRIB_WHITE);}
};