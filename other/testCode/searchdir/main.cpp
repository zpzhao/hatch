#include <iostream>
#include <io.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
void dir(string path)
{
    long hFile = 0;
    struct _finddata_t fileInfo;
    string pathName, exdName;
    int count = 0;
    FILE *fp_sqlldr = NULL;

    fp_sqlldr = fopen("run_sqlldr.sh", "w+");
    if(NULL == fp_sqlldr)
    {
        printf("shell open err\n");
        return ;
    }
    
    // \\* 代表要遍历所有的类型
    if ((hFile = _findfirst(pathName.assign(path).append("\\*").c_str(), &fileInfo)) == -1) 
    {
        return;
    }

    do 
    {
        //判断文件的属性是文件夹还是文件
        cout << fileInfo.name << (fileInfo.attrib&_A_SUBDIR? "[folder]":"[file]") << endl;

        if(!(fileInfo.attrib & _A_SUBDIR))
        {
            // file
            fprintf(fp_sqlldr, "sqlldr userid=cjcl/123456@BTJ control=./imp/%s.ctl\n", fileInfo.name);
            count++;
        }
        
    } while (_findnext(hFile, &fileInfo) == 0);

    cout<<"file :" << count<<endl;

    _findclose(hFile);
    fclose(fp_sqlldr);
    return;
}


int main()
{
 //要遍历的目录
 string path=".\\imp";
 dir(path);
 system("pause");
 return 0;
}