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
#include "tld.h"
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
    printf("TLD:: %02X(%d/%d) : ", tld->tag, tld->data_size, tld->len);
    tld_brief_print_data(tld->data, tld->data_size);
    printf("\n");
}
//------------------------------------------------------------
void tld_brief_print_data(uint8_t *data, int data_size)
{
    int count = MIN(data_size, 128);
    for (int i = 0; i < count; i++)
    {
        if (i % 16 == 0) printf("\n\t");
        printf(" %02X", *(data + i));

    }
    if (count < data_size) printf("\n\t...\n");
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
    tld->data = malloc(tld->len);
    if (NULL == tld->data)
    {
        printf("\n\n !!! no more memory to malloc(%d) !!! \n\n", tld->len);
        exit(0);
    }
    int should = tld->len - tld->data_size;
    int cpysz = in_size > 0 ? MIN(should, in_size) : should;
    memcpy(tld->data + tld->data_size, pData, cpysz);
    tld->data_size += cpysz;
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
