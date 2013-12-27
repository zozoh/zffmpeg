/*
 * 提供了日志写入操作接口
 *
 *  Created on: 2012-12-27
 *      Author: zozoh
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#include "datatime.h"

#define MX_LOG_LV_FATAL   0
#define MX_LOG_LV_WARN    1
#define MX_LOG_LV_INFO    2
#define MX_LOG_LV_DEBUG   3
#define MX_LOG_LV_TRACE   4
//------------------------------------------------------------------------------
#define MX_LOG_LV   MX_LOG_LV_INFO

//------------------------------------------------------------------------------
#define _MSG(lv,fmt,args...)\
    z_now_ms_day_print(); \
	printf(" "lv": "fmt"\n", ##args);

//------------------------------------------------------------------------------
#if MX_LOG_LV == MX_LOG_LV_TRACE
#define _F(fmt,args...)_MSG("F",fmt,##args)
#define _W(fmt,args...)_MSG("W",fmt,##args)
#define _I(fmt,args...)_MSG("I",fmt,##args)
#define _D(fmt,args...)_MSG("D",fmt,##args)
#define _T(fmt,args...)_MSG("T",fmt,##args)
//------------------------------------------------------------------------------
#elif MX_LOG_LV == MX_LOG_LV_DEBUG
#define _F(fmt,args...)_MSG("F",fmt,##args)
#define _W(fmt,args...)_MSG("W",fmt,##args)
#define _I(fmt,args...)_MSG("I",fmt,##args)
#define _D(fmt,args...)_MSG("D",fmt,##args)
#define _T(fmt,args...) ;
//------------------------------------------------------------------------------
#elif MX_LOG_LV == MX_LOG_LV_INFO
#define _F(fmt,args...)_MSG("F",fmt,##args)
#define _W(fmt,args...)_MSG("W",fmt,##args)
#define _I(fmt,args...)_MSG("I",fmt,##args)
#define _D(fmt,args...) ;
#define _T(fmt,args...) ;
//------------------------------------------------------------------------------
#elif MX_LOG_LV == MX_LOG_LV_WARN
#define _F(fmt,args...)_MSG("F",fmt,##args)
#define _W(fmt,args...)_MSG("W",fmt,##args)
#define _I(fmt,args...) ;
#define _D(fmt,args...) ;
#define _T(fmt,args...) ;
//------------------------------------------------------------------------------
#elif MX_LOG_LV == MX_LOG_LV_FATALS
#define _F(fmt,args...)_MSG("F",fmt,##args)
#define _W(fmt,args...) ;
#define _I(fmt,args...) ;
#define _D(fmt,args...) ;
#define _T(fmt,args...) ;
//------------------------------------------------------------------------------
#else
#define _F(fmt,args...) ;
#define _W(fmt,args...) ;
#define _I(fmt,args...) ;
#define _D(fmt,args...) ;
#define _T(fmt,args...) ;
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
#endif /* LOG_H_ */
