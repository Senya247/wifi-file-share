#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H
#include <stdio.h>
void p(char c, int times)
{
    for (int i = 0; i < times; i++)
	    putc(c, stdout);
}

void update_bar(int i, int max)
{
    printf("\r");
    int len = i * max/100;
    putc('[', stdout);
    p('#', len);
    p(' ', max - len);
    putc(']', stdout);
    fflush(stdout);
}
#endif // ifdef PROGRESS_BAR_H
