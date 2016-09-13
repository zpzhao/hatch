/*
 * hat_add_file.h
 *
 *  Created on: 2016-9-2
 *      Author: zhaozongpeng
 */

#ifndef HAT_ADD_FILE_H_
#define HAT_ADD_FILE_H_

#include <sys/types.h>
#include "hat_filemng_pub.h"

int safe_write_file(const char *base, const char *file,
		    const char *val, size_t vallen);
int safe_read_file(const char *base, const char *file,
		   char *val, size_t vallen);

int safe_add_dir(const char *adddir, const char *mngfile);
int safe_copy(int fdsrc, unsigned long offset1, int fddes, unsigned long offset2);

int hat_init_file_header(const char *mngfile);
int hat_update_num_file_header(const char *mngfile);
int hat_update_file_header(const char *mngfile);
int hat_init_item_header(int mngfd, unsigned long offset, int order);


int hat_add(const char *mngfile, const char *addfile);
int hat_add_dir(const char *mngfile, const char *addfile);
int hat_add_file(const char *mngfile, const char *addfile);

int hat_mv_context(FMNG_CT *psrc, FMNG_CT **pdes);
FMNG_CT * hat_switch_context(const char *mngfile, const char *addfile);

#endif /* HAT_ADD_FILE_H_ */
