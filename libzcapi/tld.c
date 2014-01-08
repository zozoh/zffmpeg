/*
 * tld.c
 *
 *  Created on: Jan 3, 2014
 *      Author: zozoh
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "z.h"
#include "log.h"
#include "tld.h"
#include "alg/md5.h"
//------------------------------------------------------------
BOOL tld_head_finished(TLD *tld)
{
    return TLD_HEAD_SIZE == tld->head_size;
}
//------------------------------------------------------------
BOOL tld_data_finished(TLD *tld)
{
    return tld->data_size == tld->len;
}
//------------------------------------------------------------
void tld_brief_print(TLD *tld)
{
    printf("TLD:: %02X(%d/%d)", tld->tag, tld->data_size, tld->len);
    tld_brief_print_data(tld->data, tld->data_size);
    printf("\n");
}
//------------------------------------------------------------
void tld_brief_print_data(uint8_t *data, int data_size)
{
    // 计算 TLD data 段的 md5
    char md5[33];
    if (NULL == data)
    {
        sprintf(md5, "--nil data--");
    }
    else
    {
        md5_context md5_ctx;
        md5_uint8(&md5_ctx, data, data_size);
        md5_sprint(&md5_ctx, md5);
    }
    printf("\nMD5: %s", md5);

    // 如果小于 16*10 就全部打印
    if (data_size <= 160)
    {
        for (int i = 0; i < data_size; i++)
        {
            if (i % 16 == 0) printf("\n\t");
            if (i % 2 == 0) printf(" ");
            printf("%02X", *(data + i));

        }
    }
    // 否则首尾各打印 5 行
    else
    {
        for (int i = 0; i < 5 * 16; i++)
        {
            if (i % 16 == 0) printf("\n\t");
            if (i % 2 == 0) printf(" ");
            printf("%02X", *(data + i));

        }
        printf("\n\t ...\n\t ...");
        int off = data_size - (5 * 16);
        for (int i = 0; i < 5 * 16; i++)
        {
            if (i % 16 == 0) printf("\n\t");
            if (i % 2 == 0) printf(" ");
            printf("%02X", *(data + off + i));

        }
    }
    printf("\n\n");
}
//------------------------------------------------------------
int tld_copy_head(TLD *tld, uint8_t *pHead, int in_size)
{
    int should = TLD_HEAD_SIZE - tld->head_size;
    int cpysz = in_size > 0 ? MIN(should, in_size) : should;
    memcpy(tld->head + tld->head_size, pHead, cpysz);
    tld->head_size += cpysz;
    return cpysz;
}
//------------------------------------------------------------
BOOL tld_parse_head(TLD *tld)
{
    if (tld->head_size != TLD_HEAD_SIZE) return FALSE;
    tld->tag = tld->head[0];
    tld->len = tld->head[2] | (tld->head[3] << 8) | (tld->head[4] << 16)
            | (tld->head[5] << 24);
    return TRUE;
}
//------------------------------------------------------------
int tld_copy_data(TLD *tld, uint8_t *pData, int in_size)
{
    // 如果 TLD 的 data 段还未分配内存，分配之
    if (tld->data == NULL)
    {
        tld->data = malloc(tld->len);
        if (NULL == tld->data)
        {
            printf("\n\n !!! no more memory to malloc(%d) !!! \n\n", tld->len);
            exit(0);
        }
    }
    // 如果 TLD 的 data 段已经满了，就自裁
    else if (tld->data_size == tld->len)
    {
        _F("tld:: data is full!!!");
        exit(0);
    }

    // 计算要 copy 的字节数量
    int should = tld->len - tld->data_size;
    int cpysz = in_size > 0 ? MIN(should, in_size) : should;

//    _I("+++ it will copy : %d bytes >> %p + %d/%d ",
//       cpysz,
//       tld->data,
//       tld->data_size,
//       tld->len);
//    tld_brief_print_data(pData, in_size);

    memcpy(tld->data + tld->data_size, pData, cpysz);
    tld->data_size += cpysz;

    // _I("=== after copy: %p + %d/%d", tld->data, tld->data_size, tld->len);
    //tld_brief_print_data(tld->data, tld->data_size);

    return cpysz;
}
//------------------------------------------------------------
TLD *tld_alloc()
{
    TLD *tld = malloc(sizeof(TLD));
    memset(tld, 0, sizeof(TLD));
    return tld;
}
//------------------------------------------------------------
void tld_free(TLD *tld)
{
    if (NULL != tld)
    {
        if (NULL != tld->data)
        {
            free(tld->data);
        }
        free(tld);
    }
}
//------------------------------------------------------------
void tld_freep(TLD **tld)
{
    tld_free(*tld);
    *tld = NULL;
}
//------------------------------------------------------------
