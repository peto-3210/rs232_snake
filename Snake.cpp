#include "Snake.hpp"

//Represents x, y coordinates
union position{
    struct{
        uint8_t x;
        uint8_t y;
    };
    uint16_t posHash;
};


GameBoard::GameBoard(char* conn){
    this->running = false;
    this->Serial = SerialPort(conn);
    if (this->Serial.connected == false){
        return;
    }

    if (this->drawBorder() == false){
        return;
    }

    this->head = singleBlock(MAX_X/2, MAX_Y/2, '5', DEFAULT_CHAR_ATTRIB);
    if (this->sendSerial(this->head) == false){
        return;
    }

    singleBlock newBlock(MAX_X/2 - 1, MAX_Y/2 - 2, '6', DEFAULT_CHAR_ATTRIB);
    this->snakeBody.push_front(newBlock);
    if (this->sendSerial(newBlock)){
        return;
    }

    this->lastDir = right;
    this->currentDir = right;
    this->running = true;
}

GameBoard::~GameBoard(){
    this->Serial.closeSerial();
}




//Used for all rs232 communication, returns false if transmission fails, true otherwise
bool GameBoard::sendSerial(uint32_t* buffer, int size){
    uint16_t responseBuf[size] = {0};
    this->Serial.writeSerialPort((char*)buffer, sizeof(uint32_t) * size);
    this->Serial.readSerialPort((char*)buffer, size);
    bool ret_val = true;
    for (int i = 0; i < size; ++i){
        if (responseBuf[i] & UINT8_MAX != RESP_ACK){
            serialPacket req(buffer[i]);
            responsePacket res(responseBuf[i]);
            std::cerr << "Invalid response: \"" << res.packet_to_string() << "\" for request: \"" << req.packet_to_string() << "\"!\n";
            ret_val = false;
        }
    }
    return ret_val;
}

bool GameBoard::sendSerial(serialPacket& packet){
    return this->sendSerial(&packet.rawPacket, 1);
}

bool GameBoard::sendSerial(writePosPacket& drawPacket){
    return this->sendSerial((uint32_t*)&drawPacket.rawPacket, 2);
}

bool GameBoard::sendSerial(singleBlock& block){
    writePosPacket newPacket = this->writeCharacterToPosition(block);
    return this->sendSerial(newPacket);
}




//Draws gameboard border, returns false if transmission fails
bool GameBoard::drawBorder(){
    serialPacket eraseScr = this->getEraseScreenPacket();
    if (this->sendSerial(eraseScr) == false){
        return false;
    }

    int drawnPixels = 2 * (MAX_X + 1) + 2 * (MAX_Y + 1);
    uint64_t buffer[COMMAND_SIZE * drawnPixels];
    int iterator = 0;
    
    const uint8_t default_char = ' ';

    for (int i = MIN_X - 1; i <= MAX_X + 1; ++i){
        buffer[iterator++] = this->writeCharacterToPosition(i, 0, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
        buffer[iterator++] = this->writeCharacterToPosition(i, MAX_Y + 1, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
    }
    for (int i = MIN_Y; i <= MAX_Y; ++i){
        buffer[iterator++] = this->writeCharacterToPosition(MIN_X, i, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
        buffer[iterator++] = this->writeCharacterToPosition(MAX_X, i, default_char, (i % 2 == 1) ? ATTRIB_WHITE : ATTRIB_BLACK).rawPacket;
    }

    return this->sendSerial((uint32_t*)buffer, iterator * COMMAND_SIZE * 2);
}




//Selects correct character according to current and last position
uint8_t GameBoard::selectChar(){
    if (this->currentDir == this->lastDir){
        switch (this->currentDir){
            case right: return '6';
            case left:  return '4';
            case up:    return '8';
            case down:  return '2';
            default:    return 0;
        }
    }

    else if (this->currentDir == up){
        return (this->lastDir == left) ? '7' : '9';
    }

    else {
        return (this->lastDir == left) ? '1' : '3';
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
    this->lastDir = this->currentDir;
    this->currentDir = newDir;
    return true;
}




//Draws new block to the current position of head and pushes it to the back of queue, returns false if transmission fails
bool GameBoard::addBlock(){
    singleBlock newBlock(this->head.x, this->head.y, this->selectChar(), DEFAULT_CHAR_ATTRIB);
    snakeBody.push_back(newBlock);
    return this->sendSerial(newBlock);
}

//Removes block from queue and deletes it from screen, returns false if transmission fails
bool GameBoard::deleteBlock(){
    singleBlock p = this->snakeBody.front();
    writePosPacket eraseChar = this->eraseCharacterAtPosition(p.x, p.y);
    bool ret_val = this->sendSerial(eraseChar);

    this->snakeBody.pop_front();
    return ret_val;
}





//Generates new treat at random position and renders it onto screen, returns false if transmission fails
bool GameBoard::generateTreat(){
    srand(time(NULL));
    position pos;
    
    do {
        pos.posHash = rand() % UINT16_MAX;
    } while(pos.x < MIN_X || pos.x > MAX_X || pos.y < MIN_Y || pos.y > MAX_Y ||
        this->treats.find(pos.posHash) != this->treats.end());
    
    uint8_t treat;
    do {
        treat = rand() % UINT8_MAX;
    } while(!isprint(treat));

    this->treats.insert(pos.posHash);
    writePosPacket newPack = this->writeCharacterToPosition(pos.x, pos.y, treat, DEFAULT_CHAR_ATTRIB);
    return this->sendSerial(newPack);
}

//Detects eating of treat and removes it from screen, no packet is sent because treat gets overwriten by head block
bool GameBoard::detectTreat(){
    if (this->treats.find(this->head.posHash) != this->treats.end()){
        this->treats.extract(this->head.posHash);
        return true;
    }
    return false;
}





//Returns true in case of collision
bool GameBoard::detectCollision(){
    if (this->head.x < MIN_X || this->head.x > MAX_X || this->head.y < MIN_Y || this->head.y > MAX_Y){
        return true;
    }

    std::deque<singleBlock>::const_iterator iter;
    for(iter = this->snakeBody.cbegin(); iter < this->snakeBody.cend(); ++iter){
        if (iter->posHash == this->head.posHash){
            return true;
        }
    }
    return false;
}

//Moves head of snake, returns false in case of transmission error, ends game in case of collision
bool GameBoard::moveHead(){

    //Adds block to the position of head
    if (this->addBlock() == false){
        return false;
    }

    //moves head
    switch(currentDir){
        case up:    ++this->head.y;
            break;
        case down:  --this->head.y;
            break;
        case right: ++this->head.x;
            break;
        case left:  --this->head.x;
            break;
    }

    //Draws new head block
    if (this->sendSerial(this->head) == false){
        return false;
    }

    //If no treat was eaten, removes last block
    if (this->detectTreat() == false && this->deleteBlock() == false){
        return false;
    }

    //If collicion occurs, terminates game
   if (this->detectCollision() == true){
        this->running = false;
   }

   return false; 
}




