/*
 * z.h
 *
 * 定义一些本库全局都会用到的宏或者函数
 *
 *  Created on: Dec 26, 2013
 *      Author: zozoh
 */

#ifndef ZSTD_H_
#define ZSTD_H_
//-----------------------------------------------------------
#ifndef BOOL
#define BOOL short
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
//-----------------------------------------------------------
#ifndef NULL
#define NULL 0
#endif
//-----------------------------------------------------------
/*
 * 这个宏用来判断一个位码，是否在给定的遮罩码都是 1
 */
#define Z_BON(v,mask)((v & mask) == mask)
//-----------------------------------------------------------
#ifndef MAX
#define MAX(x, y)(((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y)(((x) < (y)) ? (x) : (y))
#endif
//-----------------------------------------------------------
#define FREEP(p)\
    if(NULL!=p)\
        free(p);\
        p=NULL
//-----------------------------------------------------------
// 这里定义了一些常用的结构
typedef struct
{
    int num;    // 分子
    int den;    // 分母
} ZFract;
//-----------------------------------------------------------
#endif /* ZSTD_H_ */
