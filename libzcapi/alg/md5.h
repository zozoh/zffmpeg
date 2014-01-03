/*
 * md5.h
 *
 *  Created on: Jan 3, 2014
 *      Author: zozoh
 */

#ifndef MD5_H_
#define MD5_H_

/*
 **********************************************************************
 ** md5.h -- Header file for implementation of MD5                   **
 ** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
 ** Created: 2/17/90 RLR                                             **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version              **
 ** Revised (for MD5): RLR 4/27/91                                   **
 **   -- G modified to have y&~z instead of y&z                      **
 **   -- FF, GG, HH modified to add in last register done            **
 **   -- Access pattern: round 2 works mod 5, round 3 works mod 3    **
 **   -- distinct additive constant for each step                    **
 **   -- round 4 added, working mod 7                                **
 **********************************************************************
 */

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */
#include <stdint.h>

/* typedef a 32 bit type */
typedef unsigned long int MD5_UINT4;

/* Data structure for MD5 (Message Digest) computation */
typedef struct
{
    MD5_UINT4 i[2]; /* number of _bits_ handled mod 2^64 */
    MD5_UINT4 buf[4]; /* scratch buffer */
    unsigned char in[64]; /* input buffer */
    unsigned char digest[16]; /* actual digest after MD5Final call */
} md5_context;

extern void md5_init(md5_context *mdContext);
extern void md5_update(md5_context *mdContext,
        unsigned char *inBuf,
        unsigned int inLen);
extern void md5_final(md5_context *mdContext);

extern void md5_transform(MD5_UINT4 *buf, MD5_UINT4 *in);
extern void md5_sprint(md5_context *mdContext, char *buf);
extern void md5_print(md5_context *mdContext);
extern void md5_string(md5_context *mdContext, char *inString);
extern void md5_uint8(md5_context *mdContext, uint8_t *data, int size);
extern void md5_file(md5_context *mdContext, char *filename);
extern void md5_filter(md5_context *mdContext);
#endif /* MD5_H_ */
