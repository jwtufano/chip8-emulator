#pragma once

#include <cstdint>
#include <fstream>
#include <random>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <vector>

class Emulator {
    private:
        // Debug flag
        bool debug = false;

        // Instruction rate
        int instPerSecond = 700;

        // Memory
        uint8_t memory [4096];
        uint16_t fontStart = 0x50;

        // Display and SDL
        uint8_t pixels [64][32]; //Column major, such that access is pixels[x][y]
        SDL_Window* window = NULL;
        SDL_Surface* screenSurface = NULL;
        const uint8_t windowWidth  = 64;
        const uint8_t windowHeight = 32;
        uint16_t pixelScale = 16;

        // Address-related vars
        uint16_t programCounter;
        uint16_t indexRegister;
        std::vector<uint16_t> addressStack;

        // Timers
        uint8_t delayTimer;
        uint8_t soundTimer;

        // General purpose registers
        uint8_t vRegs [16];

        // Key press vars
        bool awaitingKey = false;
        uint8_t keyPressed = 0xFF;

        // Instruction processing
        uint16_t instruction;
        void fetch();
        void decode();

        // Instructions (and helpers)
        void clearScreen();                                         //00E0
        void ret();                                                 //00EE
        void jump(uint16_t address);                                //1NNN
        void call(uint16_t address);                                //2NNN
        void skipRegEqVal(uint8_t reg, uint8_t value);              //3XNN
        void skipRegNeqVal(uint8_t reg, uint8_t value);             //4XNN
        void skipRegEqReg(uint8_t reg1, uint8_t reg2);              //5XY0
        void setRegToVal(uint8_t value, uint8_t dstReg);            //6XNN
        void addValToReg(uint8_t value, uint8_t dstReg);            //7XNN

        void setRegToReg(uint8_t srcReg, uint8_t dstReg);           //8XY0
        void orRegToReg(uint8_t srcReg, uint8_t dstReg);            //8XY1
        void andRegToReg(uint8_t srcReg, uint8_t dstReg);           //8XY2
        void xorRegToReg(uint8_t srcReg, uint8_t dstReg);           //8XY3
        void addRegToReg(uint8_t srcReg, uint8_t dstReg);           //8XY4
        void subSRegFromDReg(uint8_t srcReg, uint8_t dstReg);       //8XY5
        void rightShift(uint8_t reg);                               //8XY6
        void subDRegFromSReg(uint8_t srcReg, uint8_t dstReg);       //8XY7
        void leftShift(uint8_t reg);                                //8XYE

        void skipRegNeqReg(uint8_t reg1, uint8_t reg2);             //9XY0
        void setIndex(uint16_t address);                            //ANNN
        void jumpWithOffset(uint16_t address);                      //BNNN

        // TODO: Figure out if this random implementation is actually good...
        std::default_random_engine& getRNG();
        void random(uint8_t reg, uint8_t bitMask);                  //CXNN

        void display(uint8_t xReg, uint8_t yReg, uint8_t height);   //DXYN

        bool isPressed(uint8_t reg);
        void skipIfKey(uint8_t reg);                                //EX93
        void skipIfNotKey(uint8_t reg);                             //EXA1

        void setRegFromDTimer(uint8_t reg);                         //FX07
        void getKey(uint8_t reg);                                   //FX0A
        void setDTimerFromReg(uint8_t reg);                         //FX15
        void setSTimerFromReg(uint8_t reg);                         //FX18
        void addToIndex(uint8_t reg);                               //FX1E
        void fontChar(uint8_t reg);                                 //FX29
        void decimalConversion(uint8_t reg);                        //FX33
        void storeRegToMem(uint8_t reg);                            //FX55
        void loadRegFromMem(uint8_t reg);                           //FX65



    public:
        // Constructor and destructor
        Emulator();
        ~Emulator();

        // TODO: Probably change this to a string, filestream reference passing feels weird
        // Load program into memory
        void loadProgram(std::ifstream &filestream);

        // Main loop function
        void start();
};
