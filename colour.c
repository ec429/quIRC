#include "colour.h"

int setcolour(colour c)
{
	return(setcol(c.fore, c.back, c.hi, c.ul));
}
