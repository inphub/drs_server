#ifndef __MYCONIO__
#define __MYCONIO__

#include <ctype.h>

int kbhit(void);
int getch(void);
void clrscr(void);
int gotoxy(int x, int y);
uint32_t GetHexValue(const char * msg, uint32_t * value, uint32_t max);
void GetHexValue_with_correct(const char * msg, uint32_t * value, uint32_t max);
uint32_t GetDecValue(const char * msg, uint32_t * value, uint32_t max);
void GetDecValue_with_correct(const char * msg, uint32_t * value, uint32_t max);
void fmygets(char * st, int szst);

#endif// __MYCONIO__
