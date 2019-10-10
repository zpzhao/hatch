// OpenSSL.h: interface for the COpenSSL class.
//
//////////////////////////////////////////////////////////////////////
/*
#if !defined(AFX_OPENSSL_H__8F669F84_5538_4BC5_A253_A32C2DEC2B47__INCLUDED_)
#define AFX_OPENSSL_H__8F669F84_5538_4BC5_A253_A32C2DEC2B47__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <strings.h>
#include <openssl/des.h>
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libeay32.lib")
using  std::string;
using namespace std;

class COpenSSL  
{
public:
	void DES_ENCODE(char *input,char *output);
	void DES_DECODE(char *input,char *output);
public:
	void DES3_ENCODE(char *input,char *output);
	void DES3_DECODE(char *input,char *output);
	void My_DES3_Key(char *output);
	CString FileHash(char* str, unsigned int len); 
// 	EVP_CIPHER_CTX en, de;
public:
	COpenSSL();
	virtual ~COpenSSL();
};

#endif // !defined(AFX_OPENSSL_H__8F669F84_5538_4BC5_A253_A32C2DEC2B47__INCLUDED_)
*/