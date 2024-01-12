#include "Snake.hpp"

//Represents x, y coordinates
union position{
    struct{
        uint8_t x;
        uint8_t y;
    };
    uint16_t posHash;
};


GameBoard::GameBoard(SerialPort& s){
    running = false;
    Serial = s;
    //this->Serial.writeSerialPort("e\0\0l", 4);
    if (this->Serial.connected == false){
        return;
    }

    if (drawBorder() == false){
        return;
    }

    head = singleBlock(MAX_X/2, MAX_Y/2, '5', DEFAULT_CHAR_ATTRIB);
    if (sendSerial(head) == false){
        return;
    }

    singleBlock newBlock(MAX_X/2 - 1, MAX_Y/2, '6', DEFAULT_CHAR_ATTRIB);
    snakeBody.push_front(newBlock);
    if (sendSerial(newBlock) == false){
        return;
    }

    lastDir = right;
    currentDir = right;
    running = true;
}

GameBoard::~GameBoard(){
    Serial.closeSerial();
}




//Used for all rs232 communication, returns false if transmission fails, true otherwise
bool GameBoard::sendSerial(uint32_t* buffer, int size){
    uint16_t responseBuf[size];
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
    return ret_val;
}

bool GameBoard::sendSerial(serialPacket& packet){
    return sendSerial(&packet.rawPacket, 1);
}

bool GameBoard::sendSerial(writePosPacket& drawPacket){
    return sendSerial((uint32_t*)&drawPacket.rawPacket, 2);
}

bool GameBoard::sendSerial(singleBlock& block){
    writePosPacket newPacket = writeCharacterToPosition(block);
    return sendSerial(newPacket);
}




//Draws gameboard border, returns false if transmission fails
bool GameBoard::drawBorder(){
    serialPacket eraseScr = getEraseScreenPacket();
    if (sendSerial(eraseScr) == false){
        return false;
    }

    int drawnPixels = 2 * (MAX_X + 1) + 2 * (MAX_Y + 1);
    uint64_t buffer[2 * (MAX_X + 1) + 2 * (MAX_Y + 1)];
    int iterator = 0;
    
    const uint8_t default_char = ' ';

    for (int i = MIN_X - 1; i <= MAX_X + 1; ++i){
        buffer[iterator++] = writeCharacterToPosition(i, MIN_Y - 1, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
        buffer[iterator++] = writeCharacterToPosition(i, MAX_Y + 1, default_char, (i % 2 == 0) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
    }
    for (int i = MIN_Y; i <= MAX_Y; ++i){
        buffer[iterator++] = writeCharacterToPosition(MIN_X - 1, i, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
        buffer[iterator++] = writeCharacterToPosition(MAX_X + 1, i, default_char, (i % 2 == 0) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
    }

    return sendSerial((uint32_t*)buffer, iterator * 2);
}




//Selects correct character according to current and last position
uint8_t GameBoard::selectChar(){
    if (currentDir == lastDir){
        switch (currentDir){
            case right: return '6';
            case left:  return '4';
            case up:    return '8';
            case down:  return '2';
            default:    return 0;
        }
    }

    else if (currentDir == up){
        return (lastDir == left) ? '7' : '9';
    }

    else {
        return (lastDir == left) ? '1' : '3';
    }
}

//Sets direction of movement according to input from keyboard
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
    lastDir = currentDir;
    currentDir = newDir;
    return true;
}




//Draws new block to the current position of head and pushes it to the back of queue, returns false if transmission fails
bool GameBoard::addBlock(){
    singleBlock newBlock(head.x, head.y, selectChar(), DEFAULT_CHAR_ATTRIB);
    snakeBody.push_back(newBlock);
    return sendSerial(newBlock);
}

//Removes block from queue and deletes it from screen, returns false if transmission fails
bool GameBoard::deleteBlock(){
    singleBlock p = snakeBody.front();
    writePosPacket eraseChar = eraseCharacterAtPosition(p.x, p.y);
    bool ret_val = sendSerial(eraseChar);

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
        pos.posHash = rand() % UINT16_MAX;
    } while(pos.x < MIN_X || pos.x > MAX_X || pos.y < MIN_Y || pos.y > MAX_Y ||
        treats.find(pos.posHash) != treats.end());
    
    uint8_t treat;
    do {
        treat = rand() % UINT8_MAX;
    } while(!isprint(treat));

    treats.insert(pos.posHash);
    writePosPacket newPack = writeCharacterToPosition(pos.x, pos.y, treat, DEFAULT_CHAR_ATTRIB);
    return sendSerial(newPack);
}

//Detects eating of treat and removes it from screen, no packet is sent because treat gets overwriten by head block
bool GameBoard::detectTreat(){
    if (treats.find(head.posHash) != treats.end()){
        treats.erase(head.posHash);
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
        case up:    ++head.y;
            break;
        case down:  --head.y;
            break;
        case right: ++head.x;
            break;
        case left:  --head.x;
            break;
    }

    //Draws new head block
    if (sendSerial(head) == false){
        return false;
    }

    //If no treat was eaten, removes last block
    if (detectTreat() == false && deleteBlock() == false){
        return false;
    }

    //If collicion occurs, terminates game
   if (detectCollision() == true){
        running = false;
   }

   return false; 
}




