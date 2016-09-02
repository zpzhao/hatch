/*
 * hat_add_file.h
 *
 *  Created on: 2016-9-2
 *      Author: zhaozongpeng
 */

#ifndef HAT_ADD_FILE_H_
#define HAT_ADD_FILE_H_

#include <sys/types.h>

int safe_write_file(const char *base, const char *file,
		    const char *val, size_t vallen);
int safe_read_file(const char *base, const char *file,
		   char *val, size_t vallen);

int safe_add_dir(const char *adddir, const char *mngfile);
int safe_copy(int fdsrc, unsigned long offset1, int fddes, unsigned long offset2);


#endif /* HAT_ADD_FILE_H_ */
