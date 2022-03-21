#include "emulator.h"

// TODO: Implement command line argument for CHIP-8 program file
int main() {
    Emulator* emulator = new Emulator();
    if (std::ifstream is{"chip8_programs/tetris.ch8", std::ios::binary | std::ios::ate}) {
        emulator->loadProgram(is);
    } else {
        printf("Error opening input filestream!\n");
    }
    emulator->start();
    delete emulator;
    return 0;
}