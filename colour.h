#include "stdbool.h"
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
colour c_mirc(int, int);

#ifdef COLOURS
colour c_msg[2]={{7, 0, false, true}, {7, 0, false, false}};
colour c_notice[2]={{7, 0, true, false}, {7, 0, true, false}};
colour c_join[2]={{2, 0, true, false}, {2, 0, true, false}};
colour c_part[2]={{6, 0, true, false}, {6, 0, true, false}};
colour c_quit[2]={{3, 0, true, false}, {3, 0, true, false}};
colour c_nick[2]={{4, 0, true, false}, {4, 0, true, false}};
colour c_actn[2]={{0, 3, false, true}, {0, 3, false, false}};
colour c_status={5, 0, false, false};
colour c_err={1, 0, true, false};
colour c_unk={3, 4, false, false};
colour c_unn={1, 6, false, true};
#endif // COLOURS
