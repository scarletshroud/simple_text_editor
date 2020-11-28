/*** includes ***/

#include <sys/ioctl.h>
#include <stdio.h> 
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

/*** defines ***/ 
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/ 

struct config {
    size_t screenRows; 
    size_t screenCols; 
    struct termios canonicalMode;
};

struct config conf;

/*** output ***/ 

void draw_tildes() {
    for (size_t i = 0; i < conf.screenRows; i++) {
        write(STDOUT_FILENO, "~", 1); 

        if (i < conf.screenRows - 1) {
            write(STDOUT_FILENO, "\r\n", 2);
        }
    } 
}

void refresh_screen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3); 

    draw_tildes(); 

    write(STDOUT_FILENO, "\x1b[H", 3); 
}

/*** terminal ***/

void print_error_msg(const char* msg) {
    refresh_screen(); 

    perror(msg);
    exit(1); 
}

char read_keyboard() {
    char c = '\0';

    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
        print_error_msg("read keyboard error");
    }

    return c; 
}

void handle_keypress() {

    char c = read_keyboard(); 

    switch (c) {
        case CTRL_KEY('q') :
            refresh_screen(); 
            exit(0); 
        break;
    } 
}

void disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &conf.canonicalMode) == -1) {
        print_error_msg("tcsetattr error"); 
    } 
}

void enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &conf.canonicalMode) == -1) {
        print_error_msg("tcgetattr error"); 
    } 

    atexit(disable_raw_mode);

    struct termios rawMode = conf.canonicalMode; 
    rawMode.c_iflag &= ~(BRKINT | ICRNL | INPCK | IXON | ISTRIP);
    rawMode.c_oflag &= ~(OPOST);
    rawMode.c_cflag |= (CS8);
    rawMode.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
    rawMode.c_cc[VMIN] = 0;
    rawMode.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawMode) == -1) {
        print_error_msg("tcsetattr error"); 
    }
}

int get_cursor_pos(size_t *rows, size_t *cols) {
    char buf[32]; 

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }
    
    size_t i = 0; 
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            break;
        }

        if (buf[i] == 'R') {
            break;
        }

        i++; 
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') {
        return -1; 
    } 

    if (sscanf(&buf[2],"%zu;%zu", rows, cols) != 2) {
        return -1;
    }

    return 0;
}

int get_win_size(size_t *rows, size_t *cols) {
    struct winsize w; 

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col ==0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) { 
            return -1; 
        }
        return get_cursor_pos(rows, cols); 
    };
    
    *rows = w.ws_row;
    *cols = w.ws_col;

    return 0; 
}
    
void init() {
    if(get_win_size(&conf.screenCols, &conf.screenRows) == -1) {
        print_error_msg("Unable to get a window size.");
    } 
}

/*** main ***/

int main() {
    enable_raw_mode();
    init(); 
    
    while(1) { 
        handle_keypress(); 
    }

    return 0;
}
