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


#include "hat_filemng_pub.h"
#include "hat_add_file.h"

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
	DIR              *pDir ;
	struct dirent    *ent  ;
	int               i=0  ;
	char              childpath[PATH_MAX];
	int 			ret = 0;
	int				fd = 0;

	pDir=opendir(adddir);
	memset(childpath,0,sizeof(childpath));

	while((ent=readdir(pDir))!=NULL)
	{

		 if(ent->d_type & DT_DIR)
		 {
			 if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)
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
