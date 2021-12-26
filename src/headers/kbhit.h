#ifndef _KB_HIT_H
#define _KB_HIT_H

#include <stdio.h>
#include <termios.h>
#include <stropts.h>
#define FIONREAD 0x541B

// https://www.flipcode.com/archives/_kbhit_for_Linux.shtml

int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (!initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

int _getch() {
    return getchar();
}

#endif