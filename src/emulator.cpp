#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_set>


#include "emulator.h"

// TODO: Add customization for some variables (font, pixelScale, instPerSecond)
// TODO: Add debug mode
// TODO: Use initializer list
/* Creates a CHIP-8 emulator with default settings.
 * Load a program with the loadProgram function, then start emulation with the start function.
 */
Emulator::Emulator() {
    // Set vars to their default values
    pixelScale = 16;
    instPerSecond = 700;
    awaitingKey = false;
    keyPressed = 0xFF;
    fontStart = 0x050;

    // TODO: Handle setting the default font better (store in file maybe? not sure)
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
    std::copy(font, std::end(font), memory + fontStart);
}

// TODO: Actually implement...
Emulator::~Emulator() {
    //TODO: Delete SDL elements
}

/* Sets the instruction variable to the next instruction (pointed to by the program counter).
 * Increments the instruction counter to point to the next instruction (some instructions will undo this increment).
 */
void Emulator::fetch() {
    instruction = ((uint16_t) memory[programCounter] << 8) 
                +  (uint16_t) memory[programCounter + 1];
    
    programCounter += 2;
}

/* Determines and calls the correct function based on the instruction variable (which is set by fetch).
 */
void Emulator::decode() {
    // Separate the instruction into nibbles
    uint8_t nibble1 = (instruction >> 12) & 0xF;
    uint8_t nibble2 = (instruction >>  8) & 0xF;
    uint8_t nibble3 = (instruction >>  4) & 0xF;
    uint8_t nibble4 =  instruction        & 0xF;

    // TODO: add error messages for illegal opcodes
    // Switch to find what function to execute. Other than 0x8NNN, 0xENNN, and 0xFNNN, nibble1 alone identifies the function.
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

/* Opcode: 00E0
 * Makes the entire screen black.
 * 
 * Rather than changing individual "pixels" to black, the function makes the entire screen black
 * and fills the pixel array with 0 (which indicates a black pixel).
 */
void Emulator::clearScreen() {
    // Set pixel array to 0
    std::fill(&pixels[0][0], &pixels[0][0]+(32*64), 0);

    // Make window black, then update display
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));
    SDL_UpdateWindowSurface(window);
}

/* Opcode: 00EE
 * Pops an address off of the address stack and sets the programCounter to it.
 * Should be used with a previous "call" (2NNN) instruction.
 *
 * Prints an error message if the stack is empty.
 */
void Emulator::ret(){
    // Attempt to update programCounter from address stack, print error if stack is empty
    if (addressStack.size() > 0) {
        programCounter = addressStack.back();
        addressStack.pop_back();
    } else{
        printf("Ret (00EE) called with empty address stack! Program Counter: %d\n", programCounter);
    }
}

/* Opcode: 1NNN
 * Sets the programCounter to the specified address.
 */
void Emulator::jump(uint16_t address) {
    programCounter = address;
}

/* Opcode: 2NNN
 * Pushes the current programCounter to the address stack, then sets the programCounter to the specified address.
 * Should be used with a later "ret" (00EE) instruction.
 */
void Emulator::call(uint16_t address){
    addressStack.push_back(programCounter);
    programCounter = address;
}

/* Opcode: 3XNN
 * Skips an instruction if the value in the specified register equals the passed value.
 */
void Emulator::skipRegEqVal(uint8_t reg, uint8_t value) {
    if (vRegs[reg] == value){
        programCounter += 2;
    }
}

/* Opcode: 4XNN
 * Skips an instruction if the value in the specified register does not equal the passed value.
 */
void Emulator::skipRegNeqVal(uint8_t reg, uint8_t value) {
    if (vRegs[reg] != value){
        programCounter += 2;
    }
}

/* Opcode: 5XY0
 * Skips an instruction if the values in the specified registers are equal.
 */
void Emulator::skipRegEqReg(uint8_t reg1, uint8_t reg2) {
    if (vRegs[reg1] == vRegs[reg2]){
        programCounter += 2;
    }
}

/* Opcode: 9XY0
 * Skips an instruction if the values in the specified registers are not equal.
 */
void Emulator::skipRegNeqReg(uint8_t reg1, uint8_t reg2) {
    if (vRegs[reg1] != vRegs[reg2]){
        programCounter += 2;
    }
}

/* Opcode: 6XNN
 * Sets the value of the destination register to the passed value.
 */
void Emulator::setRegToVal(uint8_t value, uint8_t dstReg){
    vRegs[dstReg] = value;
}

/* Opcode: 7XNN
 * Adds the passed value to the destination register. 
 * Does not set the carry flag (vRegs[0xF]) if there is overflow.
 */
void Emulator::addValToReg(uint8_t value, uint8_t dstReg){
    vRegs[dstReg] += value;
}

/* Opcode: 8XY0
 * Sets the destination register to the value of the source register.
 */
void Emulator::setRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] = vRegs[srcReg];
}

/* Opcode: 8XY1
 * Sets the value of the destination register to the binary OR of the destination and source registers.
 */
void Emulator::orRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] |= vRegs[srcReg];
}

/* Opcode: 8XY2
 * Sets the value of the destination register to the binary AND of the destination and source registers.
 */
void Emulator::andRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] &= vRegs[srcReg];
}

/* Opcode: 8XY3
 * Sets the value of the destination register to the binary XOR of the destination and source registers.
 */
void Emulator::xorRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] ^= vRegs[srcReg];
}

/* Opcode: 8XY4
 * Adds the value of the source register to the destination register.
 * 
 * If the addition overflows, the carry flag (vRegs[0xF]) is set to 1. Otherwise, it is set to 0.
 */
void Emulator::addRegToReg(uint8_t srcReg, uint8_t dstReg){
    vRegs[dstReg] += vRegs[srcReg];

    // Check for overflow, set carry flag accordingly
    if (vRegs[dstReg] < vRegs[srcReg]){
        vRegs[0xF] = 1;
    } else{
        vRegs[0xF] = 0;
    }
}

/* Opcode: 8XY5
 * Subtracts the source register from the destination register.
 * 
 * If the subtraction underflows, the carry flag (vRegs[0xF]) is set to 0. Otherwise, it is set to 1.
 */
void Emulator::subSRegFromDReg(uint8_t srcReg, uint8_t dstReg){
    // If the subtraction doesn't underflow, set carry flag to 1; otherwise, 0.
    if (vRegs[dstReg] >= vRegs[srcReg]){
        vRegs[0xF] = 1;
    } else{
        vRegs[0xF] = 0;
    }

    vRegs[dstReg] = vRegs[dstReg] - vRegs[srcReg];
}

/* Opcode: 8XY7
 * Sets the destination register to the source register minus the destination register.
 * 
 * If the subtraction underflows, the carry flag (vRegs[0xF]) is set to 0. Otherwise, it is set to 1.
 */
void Emulator::subDRegFromSReg(uint8_t srcReg, uint8_t dstReg){
    // If the subtraction doesn't underflow, set carry flag to 1; otherwise, 0.
    if (vRegs[srcReg] >= vRegs[dstReg]){
        vRegs[0xF] = 1;
    } else{
        vRegs[0xF] = 0;
    }

    vRegs[dstReg] = vRegs[srcReg] - vRegs[dstReg];
}

/* Opcode: 8XY6
 * Shifts the value in the specified register once to the right.
 * 
 * Sets the carry flag (vRegs[0xF]) to the value of the bit shifted out.
 */
void Emulator::rightShift(uint8_t reg){
    vRegs[0xF] = vRegs[reg] & 1;
    vRegs[reg] >>= 1;
}

/* Opcode: 8XYE
 * Shifts the value in the specified register once to the left.
 * 
 * Sets the carry flag (vRegs[0xF]) to the value of the bit shifted out.
 */
void Emulator::leftShift(uint8_t reg){
    vRegs[0xF] = (vRegs[reg] & 0x80) >> 7;
    vRegs[reg] <<= 1;
}

/* Opcode: ANNN
 * Sets the index register to the specified address.
 */
void Emulator::setIndex(uint16_t address){
    indexRegister = address;
}

/* Opcode: BNNN
 * Sets the programCounter to the specified address plus the value in the V0 register (vRegs[0x0]).
 */
void Emulator::jumpWithOffset(uint16_t address){
    programCounter = address + vRegs[0x0];
}

/* Helper function for random.
 * Returns the random engine of the emulator to be used with other components of the <random> library.
 */
std::default_random_engine& Emulator::getRNG(){
    static std::default_random_engine u{};
    return u;
}

/* Opcode: CXNN
 * Sets the value of the specified register to a random number between 0 and 255 binary ANDed with the passed bit mask.
 */
void Emulator::random(uint8_t reg, uint8_t bitMask){
    std::uniform_int_distribution<int> dist(0,255);
    vRegs[reg] = (uint8_t) dist(getRNG()) & bitMask;
}

/* Opcode: DXYN
 * Displays a sprite to the screen. The sprite is displayed at the (x,y) coordinate contained in xReg and yReg, respectively.
 * When calculating the (x,y) coordinate, the screen wraps -- the x value is modulo the screen width, and the y value is 
 * modulo the screen height. However, after the initial calculation, the sprites clip at the edge of the screen rather than
 * wrap.
 * 
 * Sprites are 8 bits wide, with the height specified by parameter. The address of the sprite data is stored in the index
 * register. Each byte represents a row of 8 pixels, starting from the top of the sprite.
 * 
 * Drawing a sprite is done bit by bit. If a bit is 0, the screen is not changed. If a bit is 1, the corresponding pixel is
 * flipped (turned on if it is off, and vice versa).
 * 
 * The carry flag (vRegs[0xF]) is set to 0 if no pixels are turned off by the instruction. If a pixel is turned off, it is 
 * set to 1.
 */
void Emulator::display(uint8_t xReg, uint8_t yReg, uint8_t height){
    // Get the x and y coordinate where the sprite will be drawn
    uint8_t x = vRegs[xReg] % windowWidth;
    uint8_t y = vRegs[yReg] % windowHeight;

    // Set the carry flag to 0 initially
    vRegs[0xF] = 0;

    // Loop over each row of the sprite, and draw row by row
    SDL_Rect pixelRect;
    for (uint8_t yOff = 0; (y + yOff) < windowHeight && yOff < height; ++yOff){
        // Get the sprite data for the row
        uint8_t spriteData = memory[indexRegister + yOff];

        // Loop over each bit of the row
        for (uint8_t xOff = 0; xOff < 8 && (x + xOff) < windowWidth; ++xOff){
            // If the bit is 1, flip the pixel
            if (spriteData >> (7 - xOff) & 1){
                // If the bit is getting turned off, set the carry flag
                if (pixels[y+yOff][x+xOff]){
                    vRegs[0xF] = 1;
                }

                // Flip the pixel
                pixels[y+yOff][x+xOff] ^= 0xFF;

                // Actually draw the flipped pixel
                pixelRect.x = ((uint16_t) (x+xOff))*pixelScale;
                pixelRect.y = ((uint16_t) (y+yOff))*pixelScale;
                pixelRect.w = pixelScale;
                pixelRect.h = pixelScale;
                uint8_t color = pixels[y+yOff][x+xOff]; // The pixel array doubles as color (0x00 for black, 0xFF for white)
                SDL_FillRect(screenSurface, &pixelRect, SDL_MapRGB(screenSurface->format, color, color, color));
            }
        }
    }

    // Update the window after all of the new pixels are drawn
    SDL_UpdateWindowSurface(window);
}

// TODO: Create separate function to handle switch for skipIfKey and skipIfNotKey
/* Opcode: EX93
 * If the key contained in the specified register is pressed, skip the next instruction.
 * The key is a value between 0x0 and 0xF.
 */
void Emulator::skipIfKey(uint8_t reg){
    // Switch on the value in the specified register, increment programCounter if corresponding key is pressed
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

/* Opcode: EXA1
 * If the key contained in the specified register is not pressed, skip the next instruction.
 * The key is a value between 0x0 and 0xF.
 */
void Emulator::skipIfNotKey(uint8_t reg){
    // Increment programCounter assuming key is not held
    programCounter += 2;

    // Switch on the value in the specified register, undo the assumed increment if corresponding key is pressed
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

/* Opcode: FX07
 * Sets the specified register to the value of the delay timer.
 */
void Emulator::setRegFromDTimer(uint8_t reg){
    vRegs[reg] = delayTimer;
}

/* Opcode: FX15
 * Sets the delay timer to the value of the specified register.
 */
void Emulator::setDTimerFromReg(uint8_t reg){
    delayTimer = vRegs[reg];
}

/* Opcode: FX18
 * Sets the sound timer to the value of the specified register.
 */
void Emulator::setSTimerFromReg(uint8_t reg){
    soundTimer = vRegs[reg];
}

/* Opcode: FX1E
 * Adds the value in the specified register to the index register.
 *
 * The carry flag (vRegs[0xF]) is set to 1 if the index register "overflows" (by exceeding the addressing range).
 */
void Emulator::addToIndex(uint8_t reg){
    indexRegister += vRegs[reg];
    if (indexRegister >= 0x1000){
        vRegs[0xF] = 1;
    }
}

/* Opcode: FX0A
 * Halts execution until a key is pressed. Once a key is pressed, it is stored in the specified register.
 */
void Emulator::getKey(uint8_t reg){
    // If a key has been pressed, set register and increment program counter
    // If we aren't waiting for a key yet, set the awaitingKey flag
    if (awaitingKey && keyPressed != 0xFF){
        vRegs[reg] = keyPressed;
        awaitingKey = false;
        keyPressed = 0xFF;
        programCounter += 2;
    } else if (!awaitingKey) {
        awaitingKey = true;
    }

    // Decrement programCounter to halt execution (the above conditional offsets this once a key is pressed)
    programCounter -= 2;
}

/* Opcode: FX29
 * Sets the index register to the sprite for the character (0x0 - 0xF) contained in the specified register.
 */
void Emulator::fontChar(uint8_t reg){
    indexRegister = fontStart + 5*(vRegs[reg] & 0xF);
}

/* Opcode: FX33
 * Converts the value in the specified register to decimal.
 * Each digit is stored in a byte in memory where the index register points (from most to least significant).
 */
void Emulator::decimalConversion(uint8_t reg){
    memory[indexRegister]     = (vRegs[reg] / 100) % 10;
    memory[indexRegister + 1] = (vRegs[reg] /  10) % 10;
    memory[indexRegister + 2] = (vRegs[reg] /   1) % 10;
}

/* Opcode: FX55
 * Stores all of the registers up to (and including) the specified register to memory pointed to by the index register.
 */
void Emulator::storeRegToMem(uint8_t reg){
    for (uint8_t i = 0; i <= reg && indexRegister + i < 0x1000; ++i){
        memory[indexRegister + i] = vRegs[i];
    }
}

/* Opcode: FX65
 * Loads all of the registers up to (and including) the specified register from memory pointed to by the index register.
 */
void Emulator::loadRegFromMem(uint8_t reg){
    for (uint8_t i = 0; i <= reg && indexRegister + i < 0x1000; ++i){
        vRegs[i] = memory[indexRegister + i];
    }
}

/* Loads the program contained in the filestream into memory.
 * By convention, the program is loaded to location 0x200.
 */
void Emulator::loadProgram(std::ifstream &filestream){
    auto size = filestream.tellg();
    filestream.seekg(0);
    if (!filestream.read((char*) &memory[0x200], size)){
        printf("Error reading from filestream into memory array!\n");
    }
}

/* The main emulation loop.
 * Creates the display, sets the program counter to 0x200, and begins execution of whatever program is loaded into memory.
 */
void Emulator::start() {
    // Attempt to initialize SDL window, setting the window and screenSurface vars
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
        }
    }

    // Set programCounter to the start of the program
    programCounter = 0x200;

    // Set up some vars for the loop
    bool quit = false;
    SDL_Event e;
    uint32_t instExecuted = 0;
    uint32_t timerDecrements = 0;
    
    // Set up all time vars for the loop
    typedef std::chrono::high_resolution_clock Clock;
    auto last_inst_time = Clock::now();
    auto last_timer_decrement = Clock::now();
    auto time_start = Clock::now();

    while (!quit) {
        // Process any SDL events
        while (SDL_PollEvent(&e) != 0){
            switch (e.type){
            case SDL_QUIT:
                // End the loop
                quit = true;
                break;
            case SDL_KEYDOWN:
                // If not waiting for a key, ignore keypress
                if (!awaitingKey){
                    break;
                }

                // If waiting for a key, set the keyPressed variable
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
            last_inst_time += (std::chrono::nanoseconds(1000000000) / instPerSecond);
            fetch();
            decode();
            ++instExecuted;
        }

        // If enough time has passed, decrement the timers as needed
        if ((Clock::now() - last_timer_decrement) >= (std::chrono::nanoseconds(1000000000) / 60)){
            last_timer_decrement += (std::chrono::nanoseconds(1000000000) / 60);
            if (delayTimer > 0){
                --delayTimer;
            }
            if (soundTimer > 0){
                --soundTimer;
            }
            ++timerDecrements;
        }
    }

    // TODO: lock debug messages behind a flag
    // Print some info about the execution (mostly for debug purposes)
    double totalTime = (Clock::now() - time_start) / (std::chrono::nanoseconds(1000000000));

    printf("Instructions executed: %d\n", instExecuted);
    printf("Timer decrements: %d\n", timerDecrements);
    printf("Total time: %f seconds\n", totalTime);
    printf("Instructions per second: %f\n", ((double) instExecuted)/totalTime);
    printf("Timer decrements per second: %f\n", ((double) timerDecrements)/totalTime);
}
