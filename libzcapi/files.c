/*
 * files.c
 *
 *  Created on: Jan 5, 2014
 *      Author: zozoh
 */

#include <stdio.h>
#include <stdlib.h>

#include "files.h"

int z_fwrite(const char *ph, uint8_t *data, int size)
{
    // Open file
    FILE *pFile = fopen(ph, "wb");
    if (pFile == NULL) return -1;

    // write...
    fwrite(data, sizeof(uint8_t), size, pFile);

    // Close file
    fclose(pFile);

    return 0;
}

int z_fwrite_str(const char *ph, char *cs)
{
    // Open file
    FILE *pFile = fopen(ph, "wb");
    if (pFile == NULL) return -1;

    // write...
    fprintf(pFile, "%s", cs);

    // Close file
    fclose(pFile);

    return 0;
}
