//
// Created by Weiju Wang on 8/21/24.
//

#ifndef PARLIAMENT_TIMER_H
#define PARLIAMENT_TIMER_H

#include <time.h>

typedef struct
{
    clock_t start, end;
} ParlTimer;

void parlTimer_start(ParlTimer* t);

void parlTimer_stop(ParlTimer* t);

double parlTimer_secs(const ParlTimer* t);

double parlTimer_ms(const ParlTimer* t);

int parlTimer_microSecs(const ParlTimer* t);

#endif //PARLIAMENT_TIMER_H
