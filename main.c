/*** includes ***/

#include <sys/ioctl.h>
#include <stdio.h> 
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h> 
#include <ctype.h>
#include <errno.h>

/*** defines ***/
#define VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)

/*** abuf ***/ 

struct abuf {
    char *b; 
    size_t length; 
};

#define ABUF_INIT {NULL, 0}

void ab_append(struct abuf *ab, const char *s, int len) {
    char *new_ab = realloc(ab->b, ab->length + len); 

    if (new_ab == NULL)
        return;

    memcpy(&new_ab[ab->length], s, len);
    ab->b = new_ab;
    ab->length += len;  
}

void ab_free(struct abuf *ab) {
    free(ab->b); 
}

/*** data ***/ 

struct config {
    size_t cx, cy; 
    size_t screenRows; 
    size_t screenCols; 
    struct termios canonicalMode;
};

enum keys {
    ARROW_UP = 1000,
    ARROW_LEFT = 1001,
    ARROW_DOWN = 1002, 
    ARROW_RIGHT = 1003, 
    PAGE_UP = 1004, 
    PAGE_DOWN = 1005, 
    END = 1006, 
    HOME = 1007,
    DELETE = 1008
};

struct config conf;

/*** output ***/ 

void draw_tildes(struct abuf *ab) {
    for (size_t i = 0; i < conf.screenRows; i++) {
        if (i == conf.screenRows / 3) {
            char welcomeMsg[50]; 
            size_t welcomeLen = snprintf(welcomeMsg, sizeof(welcomeMsg), 
                    "Text editor by slayer404 version %s", VERSION);

            if (welcomeLen < conf.screenCols) {
                welcomeLen = conf.screenCols;
            }
            
            size_t padding = (conf.screenCols - welcomeLen) / 2;
            for (size_t j = 1; j <= padding; j++) {
                ab_append(ab, " ", 1); 
            }

            ab_append(ab, welcomeMsg, welcomeLen);
        } else {
            ab_append(ab, "~", 1); 
        }

        ab_append(ab, "\x1b[K", 3);
        if (i < conf.screenRows - 1) {
            ab_append(ab, "\r\n", 2);
        }
    } 
}

void refresh_screen() {
    struct abuf ab = ABUF_INIT; 

    ab_append(&ab, "\x1b[?25l", 6); //hide cursor 
    ab_append(&ab, "\x1b[H", 3); //move cursor to the top-left corner of the screen

    draw_tildes(&ab); 
    
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%zu;%zuH", conf.cx + 1, conf.cy + 1); 
    ab_append(&ab, buf, strlen(buf)); 

    ab_append(&ab, "\x1b[?25h", 6); //show cursor 

    write(STDOUT_FILENO, ab.b, ab.length);
    ab_free(&ab); 
}

/*** input ***/
void move_cursor(int key) { //correct: handle arrows not wasd
    switch(key) {
        case ARROW_UP:
            if (conf.cy > 1)   
                conf.cy--;  
            break;

        case ARROW_LEFT:
            if (conf.cx > 1) 
                conf.cx--;     
            break;
    
        case ARROW_DOWN:
            if (conf.cy < conf.screenRows - 1) 
                conf.cy++;
            break;
 
        case ARROW_RIGHT:
            if (conf.cx < conf.screenCols - 1)
                conf.cx++;
            break;
    }
}

/*** terminal ***/

void print_error_msg(const char* msg) {
    refresh_screen(); 

    perror(msg);
    exit(1); 
}

size_t read_key() {
    int readStatus = 0;
    char c = '\0';

    while ((readStatus = read(STDIN_FILENO, &c, 1)) != 1) {
        if (readStatus == -1 && errno != EAGAIN) {
            print_error_msg("read keyboard error");
        }
    }

    if (c == '\x1b') {
        char seq[3]; 

        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }

        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }

        if (seq[0] == '[') {
            if (seq[1] > '0' && seq[1] < '9') {

                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return '\x1b';
                } 

                if (seq[2] == '~') {     
                    switch (seq[1]) {
                        case '1': 
                            return HOME;
                        
                        case '3':
                            return DELETE; 
                        
                        case '4':
                            return END; 

                        case '5':  
                            return PAGE_UP;

                        case '6':  
                            return PAGE_DOWN;

                        case '7':
                            return HOME;

                        case '8':
                            return END;    
                    }
                }
            }

            switch (seq[1]) {
                case 'A':
                    return ARROW_UP;

                case 'B':
                    return ARROW_DOWN;

                case 'C':
                    return ARROW_RIGHT;

                case 'D':
                    return ARROW_LEFT;

                case 'H':
                    return HOME;
                
                case 'F':
                    return END;        
            }
        }

        if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return HOME;
                
                case 'F':
                    return END;        
            }
        }

        return '\x1b'; 
    }

    return c; 
}

void handle_keypress() {
    size_t c = read_key(); 

    switch (c) {
        case CTRL_KEY('q') :
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3); 
            exit(0); 
        break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:   
        case ARROW_LEFT:
            move_cursor(c);
            break;
     
        case PAGE_UP:
        case PAGE_DOWN: {
                size_t i = conf.screenRows;
                size_t dir;

                if (c == PAGE_UP) {
                    dir = ARROW_UP; 
                }
                else {
                    dir = ARROW_DOWN;
                }

                while (i > 0) {
                    move_cursor(dir);
                    i--;    
                }
            }
            break;

        case HOME:
            conf.cx = 0;
            break;

        case END:
            conf.cx = conf.screenCols - 1;
            break;
        
        case DELETE: 
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
    conf.cx = 0;
    conf.cy = 0;

    if(get_win_size(&conf.screenCols, &conf.screenRows) == -1) {
        print_error_msg("Unable to get a window size.");
    } 
}

/*** main ***/

int main() {
    enable_raw_mode();
    init(); 
    
    while(1) { 
        refresh_screen(); 
        handle_keypress();
    }

    return 0;
}
