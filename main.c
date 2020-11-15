#include <stdio.h> 
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

struct termios canonicalMode; 

void readKeyboard() {
    char c; 
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q'); 
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonicalMode); 
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &canonicalMode); 
    atexit(disableRawMode);

    struct termios rawMode = canonicalMode; 
    rawMode.c_lflag &= (~ECHO | ICANON); 

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawMode); 
}

int main() {
    enableRawMode(); 
    readKeyboard(); 
    return 0;
}
