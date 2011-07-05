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
#define KEY_CLEFT	14
#define KEY_ALEFT	15
#define KEY_CRIGHT	16
#define KEY_ARIGHT	17
#define KEY_CHOME	18
#define KEY_AHOME	19
#define KEY_CEND	20
#define KEY_AEND	21
#define KEY_CPGUP	22
#define KEY_APGUP	23
#define KEY_CPGDN	24
#define KEY_APGDN	25
#define KEY_F1	26
#define KEY_F2	27
#define KEY_F3	28
#define KEY_F4	29
#define KEY_F5	30
#define KEY_F6	31
#define KEY_F7	32
#define KEY_F8	33
#define KEY_F9	34
#define KEY_F10	35
#define KEY_F11	36
#define KEY_F12	37
#define KEY_F(n)	((int[12]){KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_F11,KEY_F12,}[n-1])
