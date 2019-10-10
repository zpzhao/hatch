// OpenSSL.cpp: implementation of the COpenSSL class.
//
//////////////////////////////////////////////////////////////////////

#include "OpenSSL.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//
//COpenSSL::COpenSSL()
//{
//}
//
//COpenSSL::~COpenSSL()
//{	
//}
//void COpenSSL::DES_ENCODE(char *input,char *output)
//{
//    const_DES_cblock key;
//	DES_cblock ivec;
//	DES_key_schedule key_schedule;
//	char *str = "1q@W3e$5";
//	int len = strlen(str);
//	memcpy((char *)key,str,len);
//    memcpy((char *)ivec,str,len);
//	DES_set_key_unchecked(&key,&key_schedule);
//	int slen = strlen(input);
//
//	DES_ncbc_encrypt((const unsigned char*)input,(unsigned char*)output,len,&key_schedule,&ivec,DES_ENCRYPT );
//}
//void COpenSSL::DES_DECODE(char *input,char *output)
//{
//    const_DES_cblock key;
//	DES_cblock ivec;
//	DES_key_schedule key_schedule;
//	char *str = "1q@W3e$5";
//	int len = strlen(str);
//	memcpy((char *)key,str,len);
//    memcpy((char *)ivec,str,len);
//	DES_set_key_unchecked(&key,&key_schedule);
//	int slen = strlen(input);
//
//	DES_ncbc_encrypt((const unsigned char*)input,(unsigned char*)output,len,&key_schedule,&ivec,DES_DECRYPT );
//}
//void COpenSSL::DES3_ENCODE(char *input,char *output)
//{
//	char *k = (char *)malloc(24);
//	memset(k,0,24);
//	My_DES3_Key(k); /* 原始密钥 */ 
//	int key_len; 
//	#define LEN_OF_KEY 24 
//	unsigned char key[LEN_OF_KEY]; /* 补齐后的密钥 */ 
//	unsigned char block_key[9]; 
//	DES_key_schedule ks,ks2,ks3;
//    /* 构造补齐后的密钥 */ 
//    key_len = strlen(k); 
//    memcpy(key, k, key_len);
//    memset(key + key_len ,0 ,LEN_OF_KEY - key_len);
//    /* 密钥置换 */
//	memset(block_key, 0, sizeof(block_key)); 
//	memcpy(block_key, key + 0, 8); 
//	DES_set_key_unchecked((const_DES_cblock*)block_key, &ks); 
//	memcpy(block_key, key + 8, 8); 
//	DES_set_key_unchecked((const_DES_cblock*)block_key, &ks2); 
//	memcpy(block_key, key + 16, 8); 
//	DES_set_key_unchecked((const_DES_cblock*)block_key, &ks3);
//
//    DES_ecb3_encrypt((const_DES_cblock*)input, (DES_cblock*)output, &ks, &ks2, &ks3, DES_ENCRYPT);
//	free(k);
//	k = NULL;
//
//}
//void COpenSSL::DES3_DECODE(char *input,char *output)
//{
//	char *k = (char *)malloc(24);
//	memset(k,0,24);
//	My_DES3_Key(k); /* 原始密钥 */ 
//	int key_len; 
//	#define LEN_OF_KEY 24 
//	unsigned char key[LEN_OF_KEY]; /* 补齐后的密钥 */ 
//	unsigned char block_key[9]; 
//	DES_key_schedule ks,ks2,ks3;
//    /* 构造补齐后的密钥 */ 
//    key_len = strlen(k); 
//    memcpy(key, k, key_len);
//    memset(key + key_len ,0 ,LEN_OF_KEY - key_len);
//    /* 密钥置换 */
//	memset(block_key, 0, sizeof(block_key)); 
//	memcpy(block_key, key + 0, 8); 
//	DES_set_key_unchecked((const_DES_cblock*)block_key, &ks); 
//	memcpy(block_key, key + 8, 8); 
//	DES_set_key_unchecked((const_DES_cblock*)block_key, &ks2); 
//	memcpy(block_key, key + 16, 8); 
//	DES_set_key_unchecked((const_DES_cblock*)block_key, &ks3);
//
//    DES_ecb3_encrypt((const_DES_cblock*)input, (DES_cblock*)output, &ks, &ks2, &ks3, DES_DECRYPT);
//
//	free(k);
//	k = NULL;
//}
//void COpenSSL::My_DES3_Key(char *output)
//{
//    CMainFrame   *pFrame = (CMainFrame*)(AfxGetApp()->m_pMainWnd);
//	CString strIPAddr = pFrame->server_linkIP;
//	char *ip = (LPSTR)(LPCTSTR)strIPAddr;
//	int len = strlen(ip)+1;
//	CString MD5_KEY = FileHash(ip,len);
//
//	char *key = (LPSTR)(LPCTSTR)MD5_KEY;
//	int keylen = strlen(key)+1;
//
//	memcpy(output,key,keylen);
//}
//CString COpenSSL::FileHash(char* str, unsigned int len)  //数据MD5加密
//{  
//   unsigned int hash = 1315423911;  
//   unsigned int i    = 0;  
//   for(i = 0; i < len; str++, i++)  
//   {  
//      hash ^= ((hash << 5) + (*str) + (hash >> 2));  
//   }  
//   CString s_hash;
//   s_hash.Format("%ud",hash); //将无符号整形转换为CString类型
//   return s_hash;  
//}