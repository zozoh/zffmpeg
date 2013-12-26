/*
 * datatime.h
 *
 *  Created on: Dec 25, 2013
 *      Author: zozoh
 */

#ifndef DATATIME_H_
#define DATATIME_H_

#include <sys/time.h>

// -------------------------------------------- 关于毫秒的
extern time_t z_now_ms();
extern void z_now_ms_sprint(char *buf);
extern void z_now_ms_print();
extern void z_now_ms_day_sprint(char *buf);
extern void z_now_ms_day_print();

extern time_t z_ms(const struct timeval *t);
extern void z_ms_t(time_t ms, struct timeval *t);

extern void z_ms_sprint(struct timeval *t, char *buf);
extern void z_ms_print(struct timeval *t);
extern void z_ms_day_sprint(struct timeval *t, char *buf);
extern void z_ms_day_print(struct timeval *t);

#endif /* DATATIME_H_ */
