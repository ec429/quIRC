// TTY RAW MODE
// From http://docs.linux.cz/programming/c/unix_examples/raw.html

#include "ttyraw.h"

struct termios oldtermios;

int ttyraw(int fd)
{
	/* Set terminal mode as follows:
	   Noncanonical mode - turn off ICANON.
	   Turn off signal-generation (ISIG)
	   	including BREAK character (BRKINT).
	   Turn off any possible preprocessing of input (IEXTEN).
	   Turn ECHO mode off.
	   Disable input parity detection (INPCK).
	   Disable stripping of eighth bit on input (ISTRIP).
	   Disable flow control (IXON).
	   Use eight bit characters (CS8).
	   Disable parity checking (PARENB).
	   Disable any implementation-dependent output processing (OPOST).
	   One byte at a time input (MIN=1, TIME=0).
	*/
	struct termios newtermios;
	if(tcgetattr(fd, &oldtermios) < 0)
		return(-1);
	newtermios = oldtermios;

	newtermios.c_lflag &= ~(ICANON | ISIG);
	/* OK, why IEXTEN? If IEXTEN is on, the DISCARD character
	   is recognized and is not passed to the process. This 
	   character causes output to be suspended until another
	   DISCARD is received. The DSUSP character for job control,
	   the LNEXT character that removes any special meaning of
	   the following character, the REPRINT character, and some
	   others are also in this category.
	*/

	newtermios.c_iflag &= ~(INPCK | IXON);
	/* If an input character arrives with the wrong parity, then INPCK
	   is checked. If this flag is set, then IGNPAR is checked
	   to see if input bytes with parity errors should be ignored.
	   If it shouldn't be ignored, then PARMRK determines what
	   character sequence the process will actually see.
	   
	   When we turn off IXON, the start and stop characters can be read.
	*/

	newtermios.c_cflag &= ~(CSIZE | PARENB);
	/* CSIZE is a mask that determines the number of bits per byte.
	   PARENB enables parity checking on input and parity generation
	   on output.
	*/

	newtermios.c_cflag |= CS8;
	/* Set 8 bits per character. */
	
	//newtermios.c_oflag &= ~(OPOST);
	/* This includes things like expanding tabs to spaces. */

	newtermios.c_cc[VMIN] = 1;
	newtermios.c_cc[VTIME] = 0;

	/* You tell me why TCSAFLUSH. */
	if(tcsetattr(fd, TCSAFLUSH, &newtermios) < 0)
		return(-1);
	return(0);
}
	
	 
int ttyreset(int fd)
{
	if(tcsetattr(fd, TCSAFLUSH, &oldtermios) < 0)
		return(-1);

	return(0);
}

void sigcatch(int sig)
{
	ttyreset(STDIN_FILENO);
	exit(0);
}
