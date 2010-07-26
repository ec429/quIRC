#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>

int ttyraw(int fd);
int ttyreset(int fd);
void sigcatch(int sig);
