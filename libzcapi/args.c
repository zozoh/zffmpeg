/*
 * zargs.c
 *
 *  Created on: Dec 26, 2013
 *      Author: zozoh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "args.h"
#include "z.h"
//----------------------------------------------------------------------
void z_args_m0_parse(int argc,
        char *argv[],
        void (*callback)(int i,
                const char *argnm,
                const char *argval,
                void *userdata),
        void *userdata)
{
    if (NULL == callback) return;

    // 寻找最长的字符串
    int bufsz = 0;
    for (int i = 1; i < argc; i++)
    {
        bufsz = MAX(bufsz, strlen(argv[i]));
    }
    bufsz++;
    // 分配字符串缓冲
    char argnm[bufsz];
    char argval[bufsz];

    // 开始分配字参数吧
    int index = 0;
    int x;

    for (int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        int len = strlen(arg);

        if (len == 0) continue;
        if (*arg != '-') continue;

        x = 1;
        for (; x < len; x++)
            if (*(arg + x) == '=') break;

        if (x == len)
        {
            callback(index++, arg + 1, NULL, userdata);
        }
        else
        {
            memset(argnm, 0, bufsz);
            memset(argval, 0, bufsz);
            strncpy(argnm, arg + 1, x - 1);
            strncpy(argval, arg + x + 1, len - x - 1);
            callback(index++, argnm, argval, userdata);
        }

    }
}
//----------------------------------------------------------------------
