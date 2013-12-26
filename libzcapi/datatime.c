/*
 * datatime.c
 *
 *  Created on: Dec 25, 2013
 *      Author: zozoh
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "datatime.h"
//----------------------------------------------------------------
time_t z_now_ms()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return z_ms(&t);
}
//----------------------------------------------------------------
void z_now_ms_sprint(char *buf)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    z_ms_sprint(&t, buf);
}
//----------------------------------------------------------------
void z_now_ms_print()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    z_ms_print(&t);
}
//----------------------------------------------------------------
void z_now_ms_day_sprint(char *buf)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    z_ms_day_sprint(&t, buf);
}
//----------------------------------------------------------------
void z_now_ms_day_print()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    z_ms_day_print(&t);
}
//----------------------------------------------------------------
time_t z_ms(const struct timeval *t)
{
    return ((time_t) t->tv_sec) * 1000 + ((time_t) t->tv_usec) / 1000;
}
//----------------------------------------------------------------
void z_ms_t(time_t ms, struct timeval *t)
{
    t->tv_sec = ms / 1000;
    t->tv_usec = ms - t->tv_sec * 1000;
}
//----------------------------------------------------------------
void z_ms_sprint(struct timeval *t, char *buf)
{
    struct tm * ti;
    ti = localtime(&t->tv_sec);

    sprintf(buf,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            1900 + ti->tm_year,
            1 + (int) ti->tm_mon,
            (int) ti->tm_mday,
            (int) ti->tm_hour,
            (int) ti->tm_min,
            (int) ti->tm_sec,
            (int) (t->tv_usec / 1000));
}
//----------------------------------------------------------------
void z_ms_print(struct timeval *t)
{
    struct tm * ti;
    ti = localtime(&t->tv_sec);

    printf("%04d-%02d-%02d %02d:%02d:%02d.%03d",
           1900 + (int) ti->tm_year,
           1 + (int) ti->tm_mon,
           (int) ti->tm_mday,
           (int) ti->tm_hour,
           (int) ti->tm_min,
           (int) ti->tm_sec,
           (int) (t->tv_usec / 1000));
}
//----------------------------------------------------------------
void z_ms_day_sprint(struct timeval *t, char *buf)
{
    struct tm * ti;
    ti = localtime(&t->tv_sec);

    sprintf(buf,
            "%02d:%02d:%02d.%03d",
            (int) ti->tm_hour,
            (int) ti->tm_min,
            (int) ti->tm_sec,
            (int) (t->tv_usec / 1000));
}
//----------------------------------------------------------------
void z_ms_day_print(struct timeval *t)
{
    struct tm * ti;
    ti = localtime(&t->tv_sec);

    printf("%02d:%02d:%02d.%03d",
           (int) ti->tm_hour,
           (int) ti->tm_min,
           (int) ti->tm_sec,
           (int) (t->tv_usec / 1000));
}
//----------------------------------------------------------------

