/*
 * files.h
 *
 *  Created on: Jan 5, 2014
 *      Author: zozoh
 */

#ifndef FILES_H_
#define FILES_H_

#include <stdint.h>

extern int z_fwrite(const char *ph, uint8_t *data, int size);

extern int z_fwrite_str(const char *ph, char *cs);

#endif /* FILES_H_ */
