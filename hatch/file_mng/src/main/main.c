/*
 * main.c
 *
 *  Created on: 2016-9-2
 *      Author: zhaozongpeng
 */

#if 0
#include <iostream>
#include <io.h>
#include <string>
using namespace std;

void dir(string path)
{
	long hFile = 0;
	struct _finddata_t fileInfo;
	string pathName, exdName;

	// \\* ����Ҫ�������е�����
	if ((hFile = _findfirst(pathName.assign(path).append("\\*").c_str(), &fileInfo)) == -1)
	{
		return;
	}

	do
	{
		//�ж��ļ����������ļ��л����ļ�
		cout << fileInfo.name << (fileInfo.attrib&_A_SUBDIR? "[folder]":"[file]") << endl;
	} while (_findnext(hFile, &fileInfo) == 0);

	_findclose(hFile);
	return;
}

int main(int argc, char *argv[])
{
	//Ҫ������Ŀ¼
	string path="E:\\work\\zhidao\\test4";
	dir(path);

	return 0;
}
#endif



#include "hat_filemng_pub.h"
#include "hat_add_file.h"

int main(int argc,char *argv[])
{
	safe_add_dir(argv[1], argv[2]);
	return 0;
}
