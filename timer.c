//
// Created by Weiju Wang on 8/21/24.
//

#include "timer.h"

void parlTimer_start(ParlTimer* t)
{
    t->start = clock();
}
void parlTimer_stop(ParlTimer* t)
{
    t->end = clock();
}

double parlTimer_secs(const ParlTimer* t)
{
    return (double)(t->end - t->start) / CLOCKS_PER_SEC;
}

double parlTimer_ms(const ParlTimer* t)
{
    return 1000 * parlTimer_secs(t);
}

int parlTimer_microSecs(const ParlTimer* t)
{
    return 1000000 * parlTimer_secs(t);
}