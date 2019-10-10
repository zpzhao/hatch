#include "file_mmap.h"

#include <fcntl.h>  
#include <io.h>  
#include <afxwin.h>  


int test_mmap()
{
    //��ʼ  
    //����ļ����  
    HANDLE hFile=CreateFile(  
        "test.txt",   //�ļ���  
        GENERIC_READ|GENERIC_WRITE, //���ļ����ж�д����  
        FILE_SHARE_READ|FILE_SHARE_WRITE,  
        NULL,       
        OPEN_EXISTING,  //���Ѵ����ļ�  
        FILE_ATTRIBUTE_NORMAL,     
        0);    

    //����ֵsize_high,size_low�ֱ��ʾ�ļ���С�ĸ�32λ/��32λ  
    DWORD size_low,size_high;  
    size_low= GetFileSize(hFile,&size_high);   

    //�����ļ����ڴ�ӳ���ļ���     
    HANDLE hMapFile=CreateFileMapping(    
        hFile,       
        NULL,     
        PAGE_READWRITE,  //��ӳ���ļ����ж�д  
        size_high,      
        size_low,   //������������64λ������֧�ֵ�����ļ�����Ϊ16EB  
        NULL);     
    if(hMapFile==INVALID_HANDLE_VALUE)     
    {     
        AfxMessageBox("Can't create file mapping.Error%d:\n",   GetLastError());     
        CloseHandle(hFile);  
        return 0;     
    }    

    //���ļ�����ӳ�䵽���̵ĵ�ַ�ռ�  
    void* pvFile=MapViewOfFile(  
        hMapFile,   
        FILE_MAP_READ|FILE_MAP_WRITE,   
        0,  
        0,  
        0);    
    unsigned char *p=(unsigned char*)pvFile;   

    //���ˣ��ͻ�����ⲿ�ļ�test.dat���ڴ��ַ�ռ��ӳ�䣬  
    //����Ϳ�����ָ��p"��ĥ"����ļ���  
    CString s;  
    p[size_low-1]='!';   
    p[size_low-2]='X'; //�޸ĸ��ļ�����������ֽ�(�ļ���С<4GB��32λΪ0)  
    s.Format("%s",p);  
    //���ļ������3���ֽ�  
    //AfxMessageBox(s);  
    printf("context: %s \n",s.GetBuffer());

    //����  
    UnmapViewOfFile(pvFile); //����ӳ��  
    CloseHandle(hFile); //�ر��ļ�  

    return 0;
}