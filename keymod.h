#pragma once

typedef struct
{
	const char *name;
	char *mod;
}
keymod;

#define KEY_BS	0
#define KEY_UP	1
#define KEY_DOWN	2
#define KEY_LEFT	3
#define KEY_RIGHT	4
#define KEY_HOME	5
#define KEY_END	6
#define KEY_DELETE	7
#define KEY_PGUP	8
#define KEY_PGDN	9
#define KEY_CUP	10
#define KEY_AUP	11
#define KEY_CDOWN	12
#define KEY_ADOWN	13
#define KEY_SLEFT	14
#define KEY_CLEFT	15
#define KEY_ALEFT	16
#define KEY_SRIGHT	17
#define KEY_CRIGHT	18
#define KEY_ARIGHT	19
#define KEY_SHOME	20
#define KEY_CHOME	21
#define KEY_AHOME	22
#define KEY_SEND	23
#define KEY_CEND	24
#define KEY_AEND	25
#define KEY_CPGUP	26
#define KEY_APGUP	27
#define KEY_CPGDN	28
#define KEY_APGDN	29
#define KEY_F1	30
#define KEY_F2	31
#define KEY_F3	32
#define KEY_F4	33
#define KEY_F5	34
#define KEY_F6	35
#define KEY_F7	36
#define KEY_F8	37
#define KEY_F9	38
#define KEY_F10	39
#define KEY_F11	40
#define KEY_F12	41
#define KEY_F(n)	((int[12]){KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_F11,KEY_F12,}[n-1])
