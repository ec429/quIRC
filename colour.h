#include "ttyesc.h"
#pragma once

typedef struct
{
	int fore;
	int back;
	bool hi;
	bool ul;
}
colour;

int setcolour(colour);
