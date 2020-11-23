/*** includes ***/

#include <stdio.h> 
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

/*** data ***/ 

struct termios canonicalMode; 

/*** terminal ***/

void printErrorMsg(const char* msg) {
    perror(msg);
    exit(1); 
}

void readKeyboard() {
    while (1) {
        char c = '\0';

        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
            printErrorMsg("read keyboard error");
        }

        if(iscntrl(c)) {
            printf("%d\r\n", c); 
        } else {
            printf("%d ('%c')\r\n", c, c); 
        }

        if (c == 'q') break; 
    } 
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonicalMode) == -1) {
        printErrorMsg("tcsetattr error"); 
    } 
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &canonicalMode) == -1) {
        printErrorMsg("tcgetattr error"); 
    } 

    atexit(disableRawMode);

    struct termios rawMode = canonicalMode; 
    rawMode.c_iflag &= ~(BRKINT | ICRNL | INPCK | IXON | ISTRIP);
    rawMode.c_oflag &= ~(OPOST);
    rawMode.c_cflag |= (CS8);
    rawMode.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
    rawMode.c_cc[VMIN] = 0;
    rawMode.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawMode) == -1) {
        printErrorMsg("tcsetattr error"); 
    } 
}

/*** main ***/

int main() {
    enableRawMode(); 
    readKeyboard(); 
    return 0;
}
