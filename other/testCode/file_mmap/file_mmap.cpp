#include "file_mmap.h"

#include <fcntl.h>  
#include <io.h>  
#include <afxwin.h>  


int test_mmap()
{
    //开始  
    //获得文件句柄  
    HANDLE hFile=CreateFile(  
        "test.txt",   //文件名  
        GENERIC_READ|GENERIC_WRITE, //对文件进行读写操作  
        FILE_SHARE_READ|FILE_SHARE_WRITE,  
        NULL,       
        OPEN_EXISTING,  //打开已存在文件  
        FILE_ATTRIBUTE_NORMAL,     
        0);    

    //返回值size_high,size_low分别表示文件大小的高32位/低32位  
    DWORD size_low,size_high;  
    size_low= GetFileSize(hFile,&size_high);   

    //创建文件的内存映射文件。     
    HANDLE hMapFile=CreateFileMapping(    
        hFile,       
        NULL,     
        PAGE_READWRITE,  //对映射文件进行读写  
        size_high,      
        size_low,   //这两个参数共64位，所以支持的最大文件长度为16EB  
        NULL);     
    if(hMapFile==INVALID_HANDLE_VALUE)     
    {     
        AfxMessageBox("Can't create file mapping.Error%d:\n",   GetLastError());     
        CloseHandle(hFile);  
        return 0;     
    }    

    //把文件数据映射到进程的地址空间  
    void* pvFile=MapViewOfFile(  
        hMapFile,   
        FILE_MAP_READ|FILE_MAP_WRITE,   
        0,  
        0,  
        0);    
    unsigned char *p=(unsigned char*)pvFile;   

    //至此，就获得了外部文件test.dat在内存地址空间的映射，  
    //下面就可以用指针p"折磨"这个文件了  
    CString s;  
    p[size_low-1]='!';   
    p[size_low-2]='X'; //修改该文件的最后两个字节(文件大小<4GB高32位为0)  
    s.Format("%s",p);  
    //读文件的最后3个字节  
    //AfxMessageBox(s);  
    printf("context: %s \n",s.GetBuffer());

    //结束  
    UnmapViewOfFile(pvFile); //撤销映射  
    CloseHandle(hFile); //关闭文件  

    return 0;
}