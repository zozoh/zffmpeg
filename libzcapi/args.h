/*
 * 本函数集合提供了解析程序 main(int argc, char *argv[]) 函数的函数集合。
 *
 *  Created on: Dec 26, 2013
 *      Author: zozoh
 */

#ifndef ZARGS_H_
#define ZARGS_H_
//-------------------------------------------------------------------: m0
// 如果程序的参数形式如
//
//    myapp -p=3300 -f=/path/to/file
//
// 适合用 z_args_m0_parse 函数来解析
/**
 * 解析 main 函数传入的参数，会针对每个 "-" 开头的参数，回调函数 callback。
 * 如果你需要把解析的结果保存在你的一个结构里，你可以传入 userdata，
 * 在回调的时候，我传回给你，以便你来保存解析结果
 *
 * 解析函数的回调函数：
 *
 * [in] i      : 表示这是第一几个参数 （从第一个 '-' 开头的参数开始计数，0 base）
 * [in] argnm  : 选项名，如果是 -p=3300, 那么这个值是 "p"
 * [in] argval : 选项值，如果是 -p=3300, 那么这个值是 "3300"，如果没有等号("=")，将是 NULL
 * [in] userdata : 你在解析时传入的一个结构指针
 */
extern void z_args_m0_parse(int argc,
        char *argv[],
        void (*callback)(int i,
                const char *argnm,
                const char *argval,
                void *userdata),
        void *userdata);

#endif /* ZARGS_H_ */
