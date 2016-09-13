/*
 * hat_filemng_pub.h
 *
 *  Created on: 2016-9-2
 *      Author: zhaozongpeng
 */




#ifndef HAT_FILEMNG_PUB_H_
#define HAT_FILEMNG_PUB_H_

#define VER_H	0
#define VER_L	1


#ifdef PATH_MAX
#undef PATH_MAX
#define PATH_MAX	1024
#endif
#define BUFFER_SIZE	8192

#define DEF_ITEM_NUM	1024


#pragma pack(push,1)

typedef struct Hat_MNG_FILE_HEADER_ST
{
	unsigned int st_size;
	unsigned int hver:4;
	unsigned int lver:4;
	unsigned int reserved1:8;
	unsigned int reserved2:8;
	unsigned int reserved3:8;
	unsigned long file_size;		/* entire file size , ignore */
	unsigned long file_num;
	unsigned long file_item_header_offset;
}MNG_HEADER, *PMNG_HEADER;

typedef struct Hat_Item_Header_ST
{
	unsigned int st_size;
	unsigned long item_size;
	unsigned int item_order;
	unsigned int file_num;
	unsigned int file_capacity;
	unsigned long next_offset;
}ITEM_HEADER, *PITEM_HEADER;

typedef struct Hat_Item_ST
{
	unsigned int st_size;
	unsigned long filesize;
	unsigned long fileoffset;
	unsigned int fileinfosize;
}ITEM, *PITEM;


typedef struct Hat_Fmng_context_ST
{
	char fmngname[PATH_MAX];
	char addname[PATH_MAX];
	int fmng_fd;
	int	filenum;
	int item_header_order;
	unsigned long filesize;
	unsigned long item_header_offset;
}FMNG_CT, *PFMNG_CT;

#pragma pack(pop)

#ifdef _HAT_LINUX__

#include <sys/types.h>


void listDir(char *path);
ssize_t safe_read(int fd, void *buf, size_t count);
ssize_t safe_write(int fd, const void *buf, size_t count);
int hat_is_file_exit(const char *fileName);
int hat_is_dir(const char *path);


#endif // _HAT_LINUX__


#ifdef _HAT_WIN32__
#endif // _HAT_WIN32__

#endif /* HAT_FILEMNG_PUB_H_ */
