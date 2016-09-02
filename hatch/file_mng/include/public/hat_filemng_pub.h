/*
 * hat_filemng_pub.h
 *
 *  Created on: 2016-9-2
 *      Author: zhaozongpeng
 */




#ifndef HAT_FILEMNG_PUB_H_
#define HAT_FILEMNG_PUB_H_

#define PATH_MAX	1024
#define BUFFER_SIZE	8192


typedef struct Hat_MNG_FILE_HEADER_ST
{
	unsigned int hver:4;
	unsigned int lver:4;
	unsigned int reserved1:8;
	unsigned int reserved2:8;
	unsigned int reserved3:8;
	unsigned long file_size;
	unsigned long file_num;
	unsigned long file_item_header_offset;
}MNG_HEADER, *PMNG_HEADER;

typedef struct Hat_Item_Header_ST
{
	unsigned long item_size;
	unsigned int item_order;
	unsigned int file_num;
	unsigned int file_capacity;
	unsigned long next_offset;
}ITEM_HEADER, *PITEM_HEADER;

typedef struct Hat_Item_ST
{
	unsigned long filesize;
	unsigned long fileoffset;
	unsigned int fileinfosize;
};




#ifdef _HAT_LINUX__

#include <sys/types.h>


void listDir(char *path);
ssize_t safe_read(int fd, void *buf, size_t count);
ssize_t safe_write(int fd, const void *buf, size_t count);

#endif // _HAT_LINUX__


#ifdef _HAT_WIN32__
#endif // _HAT_WIN32__

#endif /* HAT_FILEMNG_PUB_H_ */
