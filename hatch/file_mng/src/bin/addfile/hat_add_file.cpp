/*
 * hat_add_file.cpp
 *
 *  Created on: 2016-9-2
 *      Author: zhaozongpeng
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include <dirent.h>
#include <sys/stat.h>



#include "hat_add_file.h"

static PFMNG_CT *s_pfmng_ct = NULL;

int safe_write_file(const char *base, const char *file,
		    const char *val, size_t vallen)
{
  int ret;
  char fn[PATH_MAX];
  char tmp[PATH_MAX];
  int fd;

  // does the file already have correct content?
  char oldval[80];
  ret = safe_read_file(base, file, oldval, sizeof(oldval));
  if (ret == (int)vallen && memcmp(oldval, val, vallen) == 0)
    return 0;  // yes.

  snprintf(fn, sizeof(fn), "%s/%s", base, file);
  snprintf(tmp, sizeof(tmp), "%s/%s.tmp", base, file);
  fd = open(tmp, O_WRONLY|O_CREAT|O_TRUNC, 0644);
  if (fd < 0)
  {
    ret = errno;
    return -ret;
  }
  ret = safe_write(fd, val, vallen);
  if (ret)
  {
    close(fd);
    return ret;
  }

  ret = fsync(fd);
  if (ret < 0)
	  ret = -errno;
  close(fd);

  if (ret < 0)
  {
    unlink(tmp);
    return ret;
  }
  ret = rename(tmp, fn);
  if (ret < 0) {
    ret = -errno;
    unlink(tmp);
    return ret;
  }

  fd = open(base, O_RDONLY);
  if (fd < 0)
  {
    ret = -errno;
    return ret;
  }
  ret = fsync(fd);
  if (ret < 0)
	  ret = -errno;
  close(fd);

  return ret;
}

int safe_read_file(const char *base, const char *file,
		   char *val, size_t vallen)
{
  char fn[PATH_MAX];
  int fd, len;

  snprintf(fn, sizeof(fn), "%s/%s", base, file);

  fd = open(fn, O_RDONLY);
  if (fd < 0)
  {
    return -errno;
  }

  len = safe_read(fd, val, vallen);
  if (len < 0)
  {
    close(fd);
    return len;
  }
  // close sometimes returns errors, but only after write()
  close(fd);

  return len;
}


int safe_add_file(const char *addfile, const char *mngfile)
{
	int fdadd = -1;
	int fdmng = -1;
	int ret = 0;
	unsigned long pos = 0;

	fdadd = open(addfile, O_RDONLY);
	if(fdadd < 0)
	{
		return -errno;
	}

	fdmng = open(mngfile, O_WRONLY | O_CREAT, 0644);
	if(fdmng < 0)
	{
		ret = errno;
		close(fdadd);
		return -ret;
	}

	pos = lseek(fdmng, 0, SEEK_END);

	ret = safe_copy(fdadd, 0, fdmng, pos);

	ret = fsync(fdmng);
	if(ret < 0)
	{
		ret = errno;
		close(fdadd);
		close(fdmng);
		return -errno;
	}

	close(fdadd);
	close(fdmng);

	return ret;
}


int safe_copy(int fdsrc, unsigned long offset1, int fddes, unsigned long offset2)
{
	int ret = 0;
	int len = 0;
	char buffer[BUFFER_SIZE] = {0};

	lseek(fdsrc, offset1, SEEK_SET);
	lseek(fddes, offset2, SEEK_SET);

	len = safe_read(fdsrc, buffer, BUFFER_SIZE);
	while(len >= BUFFER_SIZE)
	{
		// write
		ret = safe_write(fddes, buffer, len);
		if(ret != len)
		{
			return errno;
		}

		// read
		len = safe_read(fdsrc, buffer, BUFFER_SIZE);
	}

	// end write
	if(len > 0)
	{
		ret = safe_write(fddes, buffer, len);
		if(ret != len)
		{
			return errno;
		}
	}

	return 0;
}

int safe_add_dir(const char *adddir, const char *mngfile)
{
	DIR              *pDir = NULL;
	struct dirent    *ent  ;
	int               i=0  ;
	char              childpath[PATH_MAX];
	int 			ret = 0;
	int				fd = 0;

	pDir=opendir(adddir);
	memset(childpath,0,sizeof(childpath));

	if(NULL == pDir)
	{
		return -1;
	}

	while((ent=readdir(pDir))!=NULL)
	{

		 if(ent->d_type & DT_DIR)
		 {
			 if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..") == 0)
					 continue;

			 sprintf(childpath,"%s/%s",adddir, ent->d_name);
			 safe_add_dir(childpath, mngfile);
		 }
		else
		{
			sprintf(childpath,"%s/%s",adddir, ent->d_name);
			ret = safe_add_file(childpath, mngfile);
			// cout<<ent->d_name<<endl;
		}
	}
	closedir(pDir);
	return 0;
}


int hat_update_file_header(const char *mngfile)
{
	int mngfd = -1;
	int ret = -1;

	if(NULL == mngfile)
		return -1;

	if(0 != hat_is_file_exit(mngfile))
	{
		ret = hat_init_file_header(mngfile);
	}

	return ret;
}

int hat_update_num_file_header(const char *mngfile)
{
	int mngfd = -1;
	MNG_HEADER stMngHeader = {0};

	// update
	mngfd = open(mngfile, O_RDWR | O_CREAT, 0664);
	if(mngfd < 0)
		return -1;

	lseek(fdmng, 0, SEEK_SET);
	read(mngfd, &stMngHeader,sizeof(MNG_HEADER));

	// simple check
	if(stMngHeader.st_size != sizeof(MNG_HEADER))
	{
		close(mngfd);
		return -1;
	}

	stMngHeader.file_num += 1;

	lseek(fdmng, 0, SEEK_SET);
	write(mngfd, &stMngHeader, sizeof(MNG_HEADER));

	close(mngfd);

	return 0;
}

int hat_init_file_header(const char *mngfile)
{
	int ret = 0;
	int mngfd = -1;
	MNG_HEADER stMngHeader = {0};

	// init
	mngfd = open(mngfile, O_RDWR | O_CREAT, 0664);
	if(mngfd < 0)
		return -1;

	// file header
	stMngHeader.st_size = sizeof(MNG_HEADER);
	stMngHeader.hver = VER_H;
	stMngHeader.lver = VER_L;
	stMngHeader.file_size = sizeof(MNG_HEADER);
	stMngHeader.file_num = 0;
	stMngHeader.file_item_header_offset = sizeof(MNG_HEADER);

	lseek(mngfd, 0, SEEK_SET);
	write(mngfd, &stMngHeader, sizeof(MNG_HEADER));

	s_pfmng_ct->item_header_offset = stMngHeader.file_item_header_offset;
	ret = hat_init_item_header(mngfd, stMngHeader.file_item_header_offset, 0);

	close(mngfd);
	return ret;
}


int hat_init_item_header(int mngfd, unsigned long offset, int order)
{
	ITEM_HEADER stItemHeader = {0};
	ITEM		stItem[DEF_ITEM_NUM] = {0};

	if(mngfd <= 0)
	{
		return -1;
	}

	stItemHeader.st_size = sizeof(ITEM_HEADER);
	stItemHeader.file_capacity = DEF_ITEM_NUM;
	stItemHeader.file_num = 0;
	stItemHeader.item_order = order;
	stItemHeader.item_size = stItemHeader.st_size + sizeof(stItem);
	stItemHeader.next_offset = 0;

	lseek(mngfd, offset, SEEK_SET);
	write(mngfd, &stItemHeader, sizeof(stItemHeader));
	write(mngfd, stItem, sizeof(stItem));
	fsync(mngfd);

	return 0;
}


int hat_add(const char *mngfile, const char *addfile)
{
	int ret = 0;
	FMNG_CT *pOld_ct = NULL;

	pOld_ct = hat_switch_context(mngfile, addfile);

	ret = hat_update_file_header(mngfile);
	if(ret < 0)
	{
		return ret;
	}

	if(hat_is_dir(addfile))
	{
		ret = hat_add_dir(mngfile, addfile);
	}
	else
	{
		ret = hat_add_file(mngfile, addfile);
	}

	if(ret < 0)
	{
		return ret;
	}

	// init fmng_ct
	return ret;
}


int hat_add_dir(const char *mngfile, const char *addfile)
{
	DIR              *pDir = NULL;
	struct dirent    *ent  ;
	int               i=0  ;
	char              childpath[PATH_MAX];
	int 			ret = 0;
	int				fd = 0;

	pDir=opendir(adddir);
	memset(childpath,0,sizeof(childpath));

	if(NULL == pDir)
	{
		return -1;
	}

	while((ent=readdir(pDir)) != NULL)
	{

		 if(ent->d_type & DT_DIR)
		 {
			 if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..") == 0)
					 continue;

			 sprintf(childpath,"%s/%s",adddir, ent->d_name);
			 safe_add_dir(childpath, mngfile);
		 }
		else
		{
			sprintf(childpath,"%s/%s",adddir, ent->d_name);
			ret = hat_add_file(mngfile, childpath);
			if(ret < 0)
			{
				closedir(pDir);
				return ret;
			}
		}
	}
	closedir(pDir);
	return 0;
}

int hat_add_file(const char *mngfile, const char *addfile)
{
	// update three parts

	return 0;
}


FMNG_CT * hat_switch_context(const char *mngfile, const char *addfile)
{
	FMNG_CT *pOld_ct = NULL;
	int ret = 0;

	if(NULL != s_pfmng_ct)
	{
		ret = hat_mv_context(s_pfmng_ct, &pOld_ct);
		free(s_pfmng_ct);
		s_pfmng_ct = NULL;
	}

	s_pfmng_ct = (FMNG_CT *)malloc(sizeof(FMNG_CT));
	if(NULL == s_pfmng_ct)
	{
		return -1;
	}

	memset(s_pfmng_ct, 0x00, sizeof(FMNG_CT));
	strcpy(s_pfmng_ct->fmngname, mngfile);
	strcpy(s_pfmng_ct->addname, addfile);
	s_pfmng_ct->filenum = 0;
	s_pfmng_ct->item_header_order = 0;
	s_pfmng_ct->filesize = 0;
	s_pfmng_ct->item_header_offset = 0;

	return pOld_ct;
}


int hat_mv_context(FMNG_CT *psrc, FMNG_CT **pdes)
{
	FMNG_CT *ptmp = *pdes;
	if(NULL == psrc)
		return -1;

	if(NULL != ptmp)
	{
		free(ptmp);
		ptmp = NULL;
	}

	ptmp = (FMNG_CT*)malloc(sizeof(FMNG_CT));
	if(NULL == ptmp)
	{
		return -1;
	}

	memcpy(ptmp, psrc, sizeof(FMNG_CT));
	*pdes = ptmp;

	return 0;
}
