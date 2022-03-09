#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_set>


#include "emulator.h"

Emulator::Emulator() {
    // Set font part of memory (at 0x050 by convention)
    uint8_t font[] =
        {0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
         0x20, 0x60, 0x20, 0x20, 0x70,  // 1
         0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
         0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
         0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
         0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
         0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
         0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
         0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
         0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
         0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
         0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
         0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
         0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
         0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
         0xF0, 0x80, 0xF0, 0x80, 0x80}; // F
    std::copy(font, std::end(font), memory + 0x50);
    
    pixelScale = 16;
    instPerSecond = 700;
    awaitingKey = false;
    keyPressed = 0xFF;
    fontStart = 0x50;
}

Emulator::~Emulator() {
    //TODO: Delete SDL elements
}

void Emulator::fetch() {
    instruction = ((uint16_t) memory[programCounter] << 8) 
                +  (uint16_t) memory[programCounter + 1];
    
    programCounter += 2;
}

void Emulator::decode() {
    uint8_t nibble1 = (instruction >> 12) & 0xF;
    uint8_t nibble2 = (instruction >>  8) & 0xF;
    uint8_t nibble3 = (instruction >>  4) & 0xF;
    uint8_t nibble4 =  instruction        & 0xF;

    switch (nibble1) {
    case 0x0:
        switch (nibble4){
        case 0x0:
            clearScreen();
            break;
        case 0xE:
            ret();
            break;
        default:
            break;
        }
        break;
    case 0x1:
        jump(instruction & 0xFFF);
        break;
    case 0x2:
        call(instruction & 0xFFF);
        break;
    case 0x3:
        skipRegEqVal(nibble2, instruction & 0xFF);
        break;
    case 0x4:
        skipRegNeqVal(nibble2, instruction & 0xFF);
        break;
    case 0x5:
        skipRegEqReg(nibble2, nibble3);
        break;
    case 0x6:
        setRegToVal(instruction & 0xFF, nibble2);
        break;
    case 0x7:
        addValToReg(instruction & 0xFF, nibble2);
        break;
    case 0x8:
        switch (nibble4){
        case 0x0:
            setRegToReg(nibble3, nibble2);
            break;
        case 0x1:
            orRegToReg(nibble3, nibble2);
            break;
        case 0x2:
            andRegToReg(nibble3, nibble2);
            break;
        case 0x3:
            xorRegToReg(nibble3, nibble2);
            break;
        case 0x4:
            addRegToReg(nibble3, nibble2);
            break;
        case 0x5:
            subSRegFromDReg(nibble3, nibble2);
            break;
        case 0x6:
            rightShift(nibble2);
            break;
        case 0x7:
            subDRegFromSReg(nibble3, nibble2);
            break;
        case 0xE:
            leftShift(nibble2);
            break;
        default:
            break;
        }
        break;
    case 0x9:
        skipRegNeqReg(nibble2, nibble3);
        break;
    case 0xA:
        setIndex(instruction & 0xFFF);
        break;
    case 0xB:
        jumpWithOffset(instruction & 0xFFF);
        break;
    case 0xC:
        random(nibble2, instruction & 0xFF);
        break;
    case 0xD:
        display(nibble2, nibble3, nibble4);
        break;
    case 0xE:
        switch (nibble4){
        case 0x3:
            skipIfKey(nibble2);
            break;
        case 0x1:
            skipIfNotKey(nibble2);
            break;
        default:
            break;
        }
        break;
    case 0xF:
        switch (instruction & 0xFF){
        case 0x07:
            setRegFromDTimer(nibble2);
            break;
        case 0x15:
            setDTimerFromReg(nibble2);
            break;
        case 0x18:
            setSTimerFromReg(nibble2);
            break;
        case 0x1E:
            addToIndex(nibble2);
            break;
        case 0x0A:
            getKey(nibble2);
            break;
        case 0x29:
            fontChar(nibble2);
            break;
        case 0x33:
            decimalConversion(nibble2);
            break;
        case 0x55:
            storeRegToMem(nibble2);
            break;
        case 0x65:
            loadRegFromMem(nibble2);
            break;
        default:
            break;
        }
        break;
    
    default:
        break;
    }
}

/* 00E0
 * Makes the entire screen black.
 * 
 * Rather than changing individual "pixels" to black, the function makes the entire screen black
 * and fill the pixel array with 0, which indicates a black pixel.
 */
void Emulator::clearScreen() {
    std::fill(&pixels[0][0], &pixels[0][0]+(32*64), 0);
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));
    SDL_UpdateWindowSurface(window);
}

/* 00EE
 * Pop an address off of the address stack and set the programCounter to it.
 */
void Emulator::ret(){
    if (addressStack.size() > 0) {
        programCounter = addressStack.back();
        addressStack.pop_back();
    } else{
        printf("Ret (00EE) called with empty address stack!\n");
    }
}

/* 1NNN
 * Jump to specified address (set programCounter)
 */
void Emulator::jump(uint16_t address) {
    programCounter = address;
}

/* 2NNN
 * Push the current programCounter to the address stack, then set to the specified address.
 * Intended for a later call to ret() to use the address pushed to the stack.
 */
void Emulator::call(uint16_t address){
    addressStack.push_back(programCounter);
    programCounter = address;
}



/* 3XNN
 * Skip an instruction if the value in the specified reg equals the passed value.
 */
void Emulator::skipRegEqVal(uint8_t reg, uint8_t value) {
    if (vRegs[reg] == value){
        programCounter += 2;
    }
}

/* 4XNN
 * Skip an instruction if the value in the specified reg does not equal the passed value.
 */
void Emulator::skipRegNeqVal(uint8_t reg, uint8_t value) {
    if (vRegs[reg] != value){
        programCounter += 2;
    }
}

/* 5XY0
 * Skip an instruction if the values in the specified regs are equal.
 */
void Emulator::skipRegEqReg(uint8_t reg1, uint8_t reg2) {
    if (vRegs[reg1] == vRegs[reg2]){
        programCounter += 2;
    }
}

/* 9XY0
 * Skip an instruction if the values in the specified regs are not equal.
 */
void Emulator::skipRegNeqReg(uint8_t reg1, uint8_t reg2) {
    if (vRegs[reg1] != vRegs[reg2]){
        programCounter += 2;
    }
}

/* 6XNN
 * Sets the value of the specified register to the passed value.
 */
void Emulator::setRegToVal(uint8_t value, uint8_t dstReg){
    vRegs[dstReg] = value;
}

/* 7XNN
 * Adds a value to the specified register. 
 * Does not set the carry flag (VF) if there is overflow.
 */
void Emulator::addValToReg(uint8_t value, uint8_t dstReg){
    vRegs[dstReg] += value;
}

/* 8XY0
 * Sets reg2 to the value of reg1
 */
void Emulator::setRegToReg(uint8_t reg1, uint8_t reg2){
    vRegs[reg2] = vRegs[reg1];
}

void Emulator::orRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] |= vRegs[srcReg];
}

void Emulator::andRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] &= vRegs[srcReg];
}

void Emulator::xorRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] ^= vRegs[srcReg];
}

void Emulator::addRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] += vRegs[srcReg];
    if (vRegs[dstReg] < vRegs[srcReg]){
        vRegs[0xF] = 1;
    } else{
        vRegs[0xF] = 0;
    }
}

void Emulator::subSRegFromDReg(uint8_t srcReg, uint8_t dstReg){
    if (vRegs[dstReg] >= vRegs[srcReg]){
        vRegs[0xF] = 1;
    } else{
        vRegs[0xF] = 0;
    }
    vRegs[dstReg] = vRegs[dstReg] - vRegs[srcReg];
}

void Emulator::subDRegFromSReg(uint8_t srcReg, uint8_t dstReg){
    if (vRegs[srcReg] >= vRegs[dstReg]){
        vRegs[0xF] = 1;
    } else{
        vRegs[0xF] = 0;
    }
    vRegs[dstReg] = vRegs[srcReg] - vRegs[dstReg];
}

void Emulator::rightShift(uint8_t reg){
    vRegs[0xF] = vRegs[reg] & 1;
    vRegs[reg] >>= 1;
}

void Emulator::leftShift(uint8_t reg){
    vRegs[0xF] = (vRegs[reg] & 128) >> 7;
    vRegs[reg] <<= 1;
}

/* ANNN
 * Sets the index register to the specified value.
 */
void Emulator::setIndex(uint16_t value){
    indexRegister = value;
}

void Emulator::jumpWithOffset(uint16_t address){
    programCounter = address + vRegs[0x0];
}

std::default_random_engine& Emulator::getRNG(){
    static std::default_random_engine u{};
    return u;
}

void Emulator::random(uint8_t reg, uint8_t bitMask){
    std::uniform_int_distribution<int> dist(0,255);
    vRegs[reg] = (uint8_t) dist(getRNG()) & bitMask;
}

/* DXYN
 * TODO: Write description
 */
void Emulator::display(uint8_t xReg, uint8_t yReg, uint8_t height){
    uint8_t x = vRegs[xReg] % windowWidth;
    uint8_t y = vRegs[yReg] % windowHeight;

    vRegs[0xF] = 0;

    SDL_Rect pixelRect;
    for (uint8_t yOff = 0; (y + yOff) < windowHeight && yOff < height; ++yOff){
        uint8_t spriteData = memory[indexRegister + yOff];
        for (uint8_t xOff = 0; xOff < 8 && (x + xOff) < windowWidth; ++xOff){
            if (spriteData >> (7 - xOff) & 1){
                if (pixels[y+yOff][x+xOff]){
                    vRegs[0xF] = 1;
                }

                pixels[y+yOff][x+xOff] ^=1;

                pixelRect.x = ((uint16_t) (x+xOff))*pixelScale;
                pixelRect.y = ((uint16_t) (y+yOff))*pixelScale;
                pixelRect.w = pixelScale;
                pixelRect.h = pixelScale;
                uint8_t color = pixels[y+yOff][x+xOff] * 0xFF;
                SDL_FillRect(screenSurface, &pixelRect, SDL_MapRGB(screenSurface->format, color, color, color));
            }
        }
    }

    SDL_UpdateWindowSurface(window);
}

void Emulator::skipIfKey(uint8_t reg){
    const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);
    switch (vRegs[reg] & 0xF){
    case 0x0:
        if (currentKeyStates[SDL_SCANCODE_X]){
            programCounter += 2;
        }
        break;
    case 0x1:
        if (currentKeyStates[SDL_SCANCODE_1]){
            programCounter += 2;
        }
        break;
    case 0x2:
        if (currentKeyStates[SDL_SCANCODE_2]){
            programCounter += 2;
        }
        break;
    case 0x3:
        if (currentKeyStates[SDL_SCANCODE_3]){
            programCounter += 2;
        }
        break;
    case 0x4:
        if (currentKeyStates[SDL_SCANCODE_Q]){
            programCounter += 2;
        }
        break;
    case 0x5:
        if (currentKeyStates[SDL_SCANCODE_W]){
            programCounter += 2;
        }
        break;
    case 0x6:
        if (currentKeyStates[SDL_SCANCODE_E]){
            programCounter += 2;
        }
        break;
    case 0x7:
        if (currentKeyStates[SDL_SCANCODE_A]){
            programCounter += 2;
        }
        break;
    case 0x8:
        if (currentKeyStates[SDL_SCANCODE_S]){
            programCounter += 2;
        }
        break;
    case 0x9:
        if (currentKeyStates[SDL_SCANCODE_D]){
            programCounter += 2;
        }
        break;
    case 0xA:
        if (currentKeyStates[SDL_SCANCODE_Z]){
            programCounter += 2;
        }
        break;
    case 0xB:
        if (currentKeyStates[SDL_SCANCODE_C]){
            programCounter += 2;
        }
        break;
    case 0xC:
        if (currentKeyStates[SDL_SCANCODE_4]){
            programCounter += 2;
        }
        break;
    case 0xD:
        if (currentKeyStates[SDL_SCANCODE_R]){
            programCounter += 2;
        }
        break;
    case 0xE:
        if (currentKeyStates[SDL_SCANCODE_F]){
            programCounter += 2;
        }
        break;
    case 0xF:
        if (currentKeyStates[SDL_SCANCODE_V]){
            programCounter += 2;
        }
        break;
    
    default:
        break;
    }
}

void Emulator::skipIfNotKey(uint8_t reg){
    programCounter += 2;

    const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);
    switch (vRegs[reg] & 0xF){
    case 0x0:
        if (currentKeyStates[SDL_SCANCODE_X]){
            programCounter -= 2;
        }
        break;
    case 0x1:
        if (currentKeyStates[SDL_SCANCODE_1]){
            programCounter -= 2;
        }
        break;
    case 0x2:
        if (currentKeyStates[SDL_SCANCODE_2]){
            programCounter -= 2;
        }
        break;
    case 0x3:
        if (currentKeyStates[SDL_SCANCODE_3]){
            programCounter -= 2;
        }
        break;
    case 0x4:
        if (currentKeyStates[SDL_SCANCODE_Q]){
            programCounter -= 2;
        }
        break;
    case 0x5:
        if (currentKeyStates[SDL_SCANCODE_W]){
            programCounter -= 2;
        }
        break;
    case 0x6:
        if (currentKeyStates[SDL_SCANCODE_E]){
            programCounter -= 2;
        }
        break;
    case 0x7:
        if (currentKeyStates[SDL_SCANCODE_A]){
            programCounter -= 2;
        }
        break;
    case 0x8:
        if (currentKeyStates[SDL_SCANCODE_S]){
            programCounter -= 2;
        }
        break;
    case 0x9:
        if (currentKeyStates[SDL_SCANCODE_D]){
            programCounter -= 2;
        }
        break;
    case 0xA:
        if (currentKeyStates[SDL_SCANCODE_Z]){
            programCounter -= 2;
        }
        break;
    case 0xB:
        if (currentKeyStates[SDL_SCANCODE_C]){
            programCounter -= 2;
        }
        break;
    case 0xC:
        if (currentKeyStates[SDL_SCANCODE_4]){
            programCounter -= 2;
        }
        break;
    case 0xD:
        if (currentKeyStates[SDL_SCANCODE_R]){
            programCounter -= 2;
        }
        break;
    case 0xE:
        if (currentKeyStates[SDL_SCANCODE_F]){
            programCounter -= 2;
        }
        break;
    case 0xF:
        if (currentKeyStates[SDL_SCANCODE_V]){
            programCounter -= 2;
        }
        break;
    
    default:
        break;
    }
}

void Emulator::setRegFromDTimer(uint8_t reg){
    vRegs[reg] = delayTimer;
}

void Emulator::setDTimerFromReg(uint8_t reg){
    delayTimer = vRegs[reg];
}

void Emulator::setSTimerFromReg(uint8_t reg){
    soundTimer = vRegs[reg];
}

void Emulator::addToIndex(uint8_t reg){
    indexRegister += vRegs[reg];
    if (indexRegister >= 0x1000 || indexRegister < vRegs[reg]){
        vRegs[0xF] = 1;
    }
}

void Emulator::getKey(uint8_t reg){
    if (awaitingKey && keyPressed != 0xFF){
        vRegs[reg] = keyPressed;
        awaitingKey = false;
        keyPressed = 0xFF;
        programCounter += 2;
    } else if (!awaitingKey) {
        awaitingKey = true;
    }

    programCounter -= 2;
}

void Emulator::fontChar(uint8_t reg){
    indexRegister = fontStart + 5*(vRegs[reg] & 0xF);
}

void Emulator::decimalConversion(uint8_t reg){
    memory[indexRegister]     = (vRegs[reg] / 100) % 10;
    memory[indexRegister + 1] = (vRegs[reg] / 10 ) % 10;
    memory[indexRegister + 2] = (vRegs[reg] / 1  ) % 10;
}

void Emulator::storeRegToMem(uint8_t reg){
    for (uint8_t i = 0; i <= reg && indexRegister + i < 0x1000; ++i){
        memory[indexRegister + i] = vRegs[i];
    }
}

void Emulator::loadRegFromMem(uint8_t reg){
    for (uint8_t i = 0; i <= reg && indexRegister + i < 0x1000; ++i){
        vRegs[i] = memory[indexRegister + i];
    }
}

void Emulator::loadProgram(std::ifstream &filestream){
    auto size = filestream.tellg();
    filestream.seekg(0);
    if (!filestream.read((char*) &memory[0x200], size)){
        printf("Error reading from filestream into memory array!\n");
    }
}

void Emulator::start() {
    // Attempt to initialize SDL Window
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    } else {
        window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth*pixelScale, windowHeight*pixelScale, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            return;
        } else {
            //Get window surface
            screenSurface = SDL_GetWindowSurface(window);

            //Fill the surface white
            // SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
            
            //Update the surface
            // SDL_UpdateWindowSurface(window);
        }
    }

    bool quit = false;
    SDL_Event e;
    programCounter = 0x200;

    typedef std::chrono::high_resolution_clock Clock;
    auto last_inst_time = Clock::now();
    auto last_timer_decrement = Clock::now();
    auto time_start = Clock::now();
    int instExecuted = 0;

    while (!quit) {
    // while (std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - time_start).count() < 20) {
        // Process any events
        while (SDL_PollEvent(&e) != 0){
            switch (e.type){
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                if (!awaitingKey){
                    break;
                }
                switch (((SDL_KeyboardEvent*) &e)->keysym.scancode){
                case SDL_SCANCODE_X:
                    keyPressed = 0x0;
                    break;
                case SDL_SCANCODE_1:
                    keyPressed = 0x1;
                    break;
                case SDL_SCANCODE_2:
                    keyPressed = 0x2;
                    break;
                case SDL_SCANCODE_3:
                    keyPressed = 0x3;
                    break;
                case SDL_SCANCODE_Q:
                    keyPressed = 0x4;
                    break;
                case SDL_SCANCODE_W:
                    keyPressed = 0x5;
                    break;
                case SDL_SCANCODE_E:
                    keyPressed = 0x6;
                    break;
                case SDL_SCANCODE_A:
                    keyPressed = 0x7;
                    break;
                case SDL_SCANCODE_S:
                    keyPressed = 0x8;
                    break;
                case SDL_SCANCODE_D:
                    keyPressed = 0x9;
                    break;
                case SDL_SCANCODE_Z:
                    keyPressed = 0xA;
                    break;
                case SDL_SCANCODE_C:
                    keyPressed = 0xB;
                    break;
                case SDL_SCANCODE_4:
                    keyPressed = 0xC;
                    break;
                case SDL_SCANCODE_R:
                    keyPressed = 0xD;
                    break;
                case SDL_SCANCODE_F:
                    keyPressed = 0xE;
                    break;
                case SDL_SCANCODE_V:
                    keyPressed = 0xF;
                    break;
                
                default:
                    break;
                }
                break;

            default:
                break;
            }
        }



        // If enough time has passed, process another instruction
        if ((Clock::now() - last_inst_time) >= (std::chrono::nanoseconds(1000000000) / instPerSecond)){
            // SDL_Rect r;
            // r.x = x * pixelScale;
            // r.y = y * pixelScale;
            // r.w = pixelScale;
            // r.h = pixelScale;
            // SDL_FillRect(screenSurface, &r, SDL_MapRGB(screenSurface->format, color, color, color));
            // SDL_UpdateWindowSurface(window);
            
            // if (x == 63){
            //     if (y == 31){
            //         color ^= 0xFF;
            //     }
            //     y = (y + 1) % 32;
            // }
            // x = (x + 1) % 64;
            

            last_inst_time = Clock::now();
            fetch();
            // std::cout << std::hex << instruction << std::endl;
            decode();
            // std::uniform_int_distribution<int> dist(0,255);
            // printf("Random number: %d\n", dist(getRNG()));
            ++instExecuted;
        }

        // If enough time has passed, process another instruction
        if ((Clock::now() - last_timer_decrement) >= (std::chrono::nanoseconds(1000000000) / 60)){
            last_timer_decrement += (std::chrono::nanoseconds(1000000000) / 60);
            if (delayTimer > 0){
                --delayTimer;
            }
            if (soundTimer > 0){
                --soundTimer;
            }
        }
    }

    printf("Instructions executed: %d\n", instExecuted);
}
