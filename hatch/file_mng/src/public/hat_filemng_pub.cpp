/*
 * hat_filemng_pub.c
 *
 *  Created on: 2016-9-2
 *      Author: zhaozongpeng
 */


#include "hat_filemng_pub.h"


#ifdef _HAT_WIN32__
#include <iostream>
#include <io.h>
#include <string>
using namespace std;



void dir(string path)
{
	long hFile = 0;
	struct _finddata_t fileInfo;
	string pathName, exdName;

	// \\* 代表要遍历所有的类型
	if ((hFile = _findfirst(pathName.assign(path).append("\\*").c_str(), &fileInfo)) == -1)
	{
		return;
	}

	do
	{
		//判断文件的属性是文件夹还是文件
		cout << fileInfo.name << (fileInfo.attrib&_A_SUBDIR? "[folder]":"[file]") << endl;
	} while (_findnext(hFile, &fileInfo) == 0);

	_findclose(hFile);
	return;
}
#endif

#ifdef _HAT_LINUX__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

using namespace std;
void listDir(char *path)
{
	 DIR              *pDir ;
	 struct dirent    *ent  ;
	 int               i=0  ;
	 char              childpath[512];

	 pDir=opendir(path);
	 memset(childpath,0,sizeof(childpath));


	 while((ent=readdir(pDir))!=NULL)
	 {

			 if(ent->d_type & DT_DIR)
			 {

					 if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)
							 continue;

					 sprintf(childpath,"%s/%s",path, ent->d_name);
					 printf("path:%s\n",childpath);

					 listDir(childpath);

			 }
			else
			{
			cout<<ent->d_name<<endl;
			}
	 }
	 closedir(pDir);
}


ssize_t safe_read(int fd, void *buf, size_t count)
{
	size_t cnt = 0;

	while (cnt < count) {
		ssize_t r = read(fd, buf, count - cnt);
		if (r <= 0) {
			if (r == 0) {
				// EOF
				return cnt;
			}
			if (errno == EINTR)
				continue;
			return -errno;
		}
		cnt += r;
		buf = (char *)buf + r;
	}
	return cnt;
}


ssize_t safe_write(int fd, const void *buf, size_t count)
{
	while (count > 0) {
		ssize_t r = write(fd, buf, count);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -errno;
		}
		count -= r;
		buf = (char *)buf + r;
	}
	return 0;
}


int hat_is_file_exit(const char *fileName)
{
	if(NULL == fileName)
		return -1;
	if(0 == access(fileName, F_OK))
		return 0;
	return -1;
}


int hat_is_dir(const char *path)
{
	struct stat info;
	stat(path, &info);
	if(S_ISDIR(info.st_mode))
	    return 1;
	return 0;
}

#endif



