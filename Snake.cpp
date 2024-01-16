#include "Snake.hpp"

//Represents x, y coordinates
union position{
    struct{
        uint8_t x;
        uint8_t y;
    };
    uint16_t posHash;
};

//Initiates connection and board
//Step time in miliseconds
GameBoard::GameBoard(SerialPort& s, int step_time){
    running = false;
    Serial = s;
    //this->Serial.writeSerialPort("e\0\0l", 4);
    if (this->Serial.connected == false){
        return;
    }
    stepTime = step_time;

    if (drawBorder() == false || InitSnake() == false){
        return;
    }
}

GameBoard::~GameBoard(){
    Serial.closeSerial();
}

//Draws first blocks of snake
bool GameBoard::InitSnake(){
    head = singleBlock(CENTER_X, CENTER_Y, HEAD_CHARACTER, DEFAULT_CHAR_ATTRIB);
    if (writeBlock(head) == false){
        return false;
    }

    singleBlock newBlock(CENTER_X - 1, CENTER_Y, HORIZONTAL, DEFAULT_CHAR_ATTRIB);
    snakeBody.push_front(newBlock);
    if (writeBlock(newBlock) == false){
        return false;
    }

    lastDir = right;
    currentDir = right;
    running = true;
    score = 0;
    return true;
}

//Used for all rs232 communication, returns false if transmission fails, true otherwise
bool GameBoard::sendSerial(uint32_t* buffer, int size){
    uint16_t *responseBuf = new uint16_t[size];
    Serial.writeSerialPort((char*)buffer, sizeof(uint32_t) * size);
    Serial.readSerialPort((char*)responseBuf, sizeof(uint16_t) * size);
    bool ret_val = true;
    for (int i = 0; i < size; ++i){
        if ((responseBuf[i] & UINT8_MAX) != RESP_ACK){
            serialPacket req(buffer[i]);
            responsePacket res(responseBuf[i]);
            std::cerr << "Invalid response: \"" << res.packet_to_string() << "\" for request: \"" << req.packet_to_string() << "\"!\n";
            ret_val = false;
        }
    }
    delete[] responseBuf;
    return ret_val;
}

bool GameBoard::sendSerial(serialPacket& packet){
    return sendSerial(&packet.rawPacket, 1);
}

bool GameBoard::writePosChar(writePosPacket& drawPacket){
    //print_char_pos(drawPacket.x, drawPacket.y, drawPacket.character);
    return sendSerial((uint32_t*)&drawPacket.rawPacket, 2);
}

bool GameBoard::writeBlock(singleBlock& block){
    writePosPacket newPacket = writePosCharPacket(block);
    return writePosChar(newPacket);
}

//Writes text from the position, returns false if transmission fails
bool GameBoard::writeText(std::string str, uint8_t x, uint8_t y){
    uint64_t *buffer = new uint64_t[str.length()];
    int iterator = 0;
    for (char c : str){
        buffer[iterator++] = writePosCharPacket(x + iterator, y, c, DEFAULT_CHAR_ATTRIB).rawPacket;
    }
    bool ret_val = sendSerial((uint32_t*)buffer, iterator * 2);
    delete[] buffer;
    return ret_val;
}


//Draws gameboard border, returns false if transmission fails
bool GameBoard::drawBorder(){
    serialPacket eraseScr = getEraseScreenPacket();
    serialPacket dummyChar = getWriteCharPacket(' ', ATTRIB_BLACK);
    if (sendSerial(dummyChar) == false || sendSerial(eraseScr) == false){
        return false;
    }

    int drawnPixels = 2 * (MAX_X + 1) + 2 * (MAX_Y + 1);
    uint64_t buffer[2 * (MAX_X + 1) + 2 * (MAX_Y + 1)];
    int iterator = 0;
    
    const uint8_t default_char = '#';

    for (int i = MIN_X - 1; i <= MAX_X + 1; ++i){
        buffer[iterator++] = writePosCharPacket(i, MIN_Y - 1, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
        //print_char_pos(i, MIN_Y - 1, default_char);
        buffer[iterator++] = writePosCharPacket(i, MAX_Y + 1, default_char, (i % 2 == 0) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
        //print_char_pos(i, MAX_Y + 1, default_char);
    }
    for (int i = MIN_Y; i <= MAX_Y; ++i){
        //print_char_pos(MIN_X - 1, i, default_char);
        buffer[iterator++] = writePosCharPacket(MIN_X - 1, i, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
        //print_char_pos(MAX_X + 1, i, default_char);
        buffer[iterator++] = writePosCharPacket(MAX_X + 1, i, default_char, (i % 2 == 0) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
    }

    return sendSerial((uint32_t*)buffer, iterator * 2);
}




//Selects correct character according to current and last position
uint8_t GameBoard::selectChar(){
    if (currentDir == lastDir){
        switch (currentDir){
            case right: return HORIZONTAL;
            case left:  return HORIZONTAL;
            case up:    return VERTICAL;
            case down:  return VERTICAL;
            default:    return 0;
        }
    }

    else if (currentDir == up){
        return (lastDir == left) ? BOT_LEFT_CORNER : BOT_RIGHT_CORNER;
    }

    else if (currentDir == down) {
        return (lastDir == left) ? TOP_LEFT_CORNER : TOP_RIGHT_CORNER;
    }

    else if (currentDir == left){
        return (lastDir == up) ? TOP_RIGHT_CORNER : BOT_RIGHT_CORNER;
    }

    else {
        return (lastDir == up) ? TOP_LEFT_CORNER : BOT_LEFT_CORNER;
    }
}

//Sets direction of movement according to input from keyboard, returns true if input is valid
bool GameBoard::setMoveDir(uint8_t dir){
    direction newDir = right;
    switch(dir){
        case 'w':   newDir = up;
            break;
        case 's':   newDir = down;
            break;
        case 'a':   newDir = left;
            break;
        case 'd':   newDir = right;
            break;
        default:    return false;
    }

    //No turning from right to left
    if (currentDir != newDir && currentDir % 2 == newDir % 2){
        return false;
    }

    lastDir = currentDir;
    currentDir = newDir;
    return true;
}




//Draws new block to the current position of head and pushes it to the back of queue, returns false if transmission fails
bool GameBoard::addBlock(){
    singleBlock newBlock(head.x, head.y, selectChar(), DEFAULT_CHAR_ATTRIB);
    lastDir = currentDir;
    snakeBody.push_back(newBlock);
    return writeBlock(newBlock);
}

//Removes block from queue and deletes it from screen, returns false if transmission fails
bool GameBoard::deleteBlock(){
    singleBlock p = snakeBody.front();
    writePosPacket eraseChar = erasePosPacket(p.x, p.y);
    bool ret_val = writePosChar(eraseChar);

    snakeBody.pop_front();
    return ret_val;
}





//Generates new treat at random position and renders it onto screen, returns false if transmission fails
bool GameBoard::generateTreat(){
    SYSTEMTIME st1;
    GetSystemTime(&st1);
    srand(st1.wMilliseconds);
    position pos;
    
    do {
        pos.x = (rand() % MAX_X);
        pos.y = (rand() % MAX_Y);
    } while(pos.x < MIN_X || pos.y < MIN_Y || treats.find(pos.posHash) != treats.end());
    
    uint8_t treat;
    treat = (rand() % 127);
    //If reat is non-printable char
    if (!isprint(treat)){
        treat += 32;
    }
    

    treats.insert(pos.posHash);
    writePosPacket newPack = writePosCharPacket(pos.x, pos.y, treat, DEFAULT_CHAR_ATTRIB);
    return writePosChar(newPack);
}

//Detects eating of treat and removes it from screen, no packet is sent because treat gets overwriten by head block
//Also increments score and calculates new step time
bool GameBoard::detectTreat(){
    if (treats.find(head.posHash) != treats.end()){
        treats.erase(head.posHash);
        ++score;
        stepTime = (int) stepTime / STEP_COEFICIENT;
        return true;
    }
    return false;
}





//Returns true in case of collision
bool GameBoard::detectCollision(){
    if (head.x < MIN_X || head.x > MAX_X || head.y < MIN_Y || head.y > MAX_Y){
        return true;
    }

    std::deque<singleBlock>::const_iterator iter;
    for(iter = snakeBody.cbegin(); iter < snakeBody.cend(); ++iter){
        if (iter->posHash == head.posHash){
            return true;
        }
    }
    return false;
}

//Moves head of snake, returns false in case of transmission error, ends game in case of collision
bool GameBoard::moveHead(){

    //Adds block to the position of head
    if (addBlock() == false){
        return false;
    }

    //moves head
    switch(currentDir){
        case up:    --head.y;
            break;
        case down:  ++head.y;
            break;
        case right: ++head.x;
            break;
        case left:  --head.x;
            break;
    }

    //Draws new head block
    if (writeBlock(head) == false){
        return false;
    }

    //If no treat was eaten, removes last block
    if (detectTreat() == false){
        if (deleteBlock() == false){
            return false;
        }
    }

    //If collicion occurs, terminates game
   if (detectCollision() == true){
        running = false;
   }

   return true; 
}




