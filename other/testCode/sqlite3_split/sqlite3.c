/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
static int sqlite3StrAccumEnlarge(StrAccum *p, int N){
  char *zNew;
  assert( p->nChar+(i64)N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==STRACCUM_TOOBIG);
    testcase(p->accError==STRACCUM_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    N = p->nAlloc - p->nChar - 1;
    setStrAccumError(p, STRACCUM_TOOBIG);
    return N;
  }else{
    char *zOld = (p->zText==p->zBase ? 0 : p->zText);
    i64 szNew = p->nChar;
    szNew += N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3_realloc64(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( zOld==0 && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
    }else{
      sqlite3StrAccumReset(p);
      setStrAccumError(p, STRACCUM_NOMEM);
      return 0;
    }
  }
  return N;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_PRIVATE void sqlite3AppendChar(StrAccum *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3StrAccumAppend() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3StrAccumAppend() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppend(StrAccum *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else{
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}

/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_PRIVATE void sqlite3StrAccumAppendAll(StrAccum *p, const char *z){
  sqlite3StrAccumAppend(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && p->zText==p->zBase ){
      p->zText = sqlite3DbMallocRaw(p->db, p->nChar+1 );
      if( p->zText ){
        memcpy(p->zText, p->zBase, p->nChar+1);
      }else{
        setStrAccumError(p, STRACCUM_NOMEM);
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_PRIVATE void sqlite3StrAccumReset(StrAccum *p){
  if( p->zText!=p->zBase ){
    sqlite3DbFree(p->db, p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = p->zBase = zBase;
  p->db = db;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->accError = 0;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  sqlite3VXPrintf(&acc, SQLITE_PRINTF_INTERNAL, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==STRACCUM_NOMEM ){
    db->mallocFailed = 1;
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR  
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *SQLITE_CDECL sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *SQLITE_STDCALL sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  return sqlite3StrAccumFinish(&acc);
}
SQLITE_API char *SQLITE_CDECL sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap,zFormat);
  z = sqlite3_vsnprintf(n, zBuf, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3VXPrintf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_PRINT_BUF_SIZE*3];    /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void SQLITE_CDECL sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[500];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3VXPrintf(&acc, 0, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}
#endif


/*
** variable-argument wrapper around sqlite3VXPrintf().  The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_PRIVATE void sqlite3XPrintf(StrAccum *p, u32 bFlags, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3VXPrintf(p, bFlags, zFormat, ap);
  va_end(ap);
}

/************** End of printf.c **********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
******************************************************************************/

/******************************************************************************
****************************** No-op Locking **********************************
**
** Of the various locking implementations available, this is by far the
** simplest:  locking is ignored.  No attempt is made to lock the database
** file for reading or writing.
**
** This locking mode is appropriate for use on read-only databases
** (ex: databases that are burned into CD-ROM, for example.)  It can
** also be used if the application employs some external mechanism to
** prevent simultaneous access of the same database by two or more
** database connections.  But there is a serious risk of database
** corruption if this locking mode is used in situations where multiple
** database connections are accessing the same database file at the same
** time and one or more of those connections are writing.
*/

static int nolockCheckReservedLock(sqlite3_file *NotUsed, int *pResOut){
  UNUSED_PARAMETER(NotUsed);
  *pResOut = 0;
  return SQLITE_OK;
}
static int nolockLock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}
static int nolockUnlock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}

/*
** Close the file.
*/
static int nolockClose(sqlite3_file *id) {
  return closeUnixFile(id);
}

/******************* End of the no-op lock implementation *********************
******************************************************************************/

/******************************************************************************
************************* Begin dot-file Locking ******************************
**
** The dotfile locking implementation uses the existence of separate lock
** files (really a directory) to control access to the database.  This works
** on just about every filesystem imaginable.  But there are serious downsides:
**
**    (1)  There is zero concurrency.  A single reader blocks all other
**         connections from reading or writing the database.
**
**    (2)  An application crash or power loss can leave stale lock files
**         sitting around that need to be cleared manually.
**
** Nevertheless, a dotlock is an appropriate locking mode for use if no
** other locking strategy is available.
**
** Dotfile locking works by creating a subdirectory in the same directory as
** the database and with the same name but with a ".lock" extension added.
** The existence of a lock directory implies an EXCLUSIVE lock.  All other
** lock types (SHARED, RESERVED, PENDING) are mapped into EXCLUSIVE.
*/

/*
** The file suffix added to the data base filename in order to create the
** lock directory.
*/
#define DOTLOCK_SUFFIX ".lock"

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
**
** In dotfile locking, either a lock exists or it does not.  So in this
** variation of CheckReservedLock(), *pResOut is set to true if any lock
** is held on the file and false if the file is unlocked.
*/
static int dotlockCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    /* Either this connection or some other connection in the same process
    ** holds a lock on the file.  No need to check further. */
    reserved = 1;
  }else{
    /* The lock is held if and only if the lockfile exists */
    const char *zLockFile = (const char*)pFile->lockingContext;
    reserved = osAccess(zLockFile, 0)==0;
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (dotlock)\n", pFile->h, rc, reserved));
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
**
** With dotfile locking, we really only support state (4): EXCLUSIVE.
** But we track the other locking levels internally.
*/
static int dotlockLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc = SQLITE_OK;


  /* If we have any lock, then the lock file already exists.  All we have
  ** to do is adjust our internal record of the lock level.
  */
  if( pFile->eFileLock > NO_LOCK ){
    pFile->eFileLock = eFileLock;
    /* Always update the timestamp on the old file */
#ifdef HAVE_UTIME
    utime(zLockFile, NULL);
#else
    utimes(zLockFile, NULL);
#endif
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  rc = osMkdir(zLockFile, 0777);
  if( rc<0 ){
    /* failed to open/create the lock directory */
    int tErrno = errno;
    if( EEXIST == tErrno ){
      rc = SQLITE_BUSY;
    } else {
      rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
      if( IS_LOCK_ERROR(rc) ){
        storeLastErrno(pFile, tErrno);
      }
    }
    return rc;
  } 
  
  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
**
** When the locking level reaches NO_LOCK, delete the lock file.
*/
static int dotlockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc;

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (dotlock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }

  /* To downgrade to shared, simply update our internal notion of the
  ** lock state.  No need to mess with the file on disk.
  */
  if( eFileLock==SHARED_LOCK ){
    pFile->eFileLock = SHARED_LOCK;
    return SQLITE_OK;
  }
  
  /* To fully unlock the database, delete the lock file */
  assert( eFileLock==NO_LOCK );
  rc = osRmdir(zLockFile);
  if( rc<0 && errno==ENOTDIR ) rc = osUnlink(zLockFile);
  if( rc<0 ){
    int tErrno = errno;
    rc = 0;
    if( ENOENT != tErrno ){
      rc = SQLITE_IOERR_UNLOCK;
    }
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
** Close a file.  Make sure the lock has been released before closing.
*/
static int dotlockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    dotlockUnlock(id, NO_LOCK);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
  }
  return rc;
}
/****************** End of the dot-file lock implementation *******************
******************************************************************************/

/******************************************************************************
************************** Begin flock Locking ********************************
**
** Use the flock() system call to do file locking.
**
** flock() locking is like dot-file locking in that the various
** fine-grain locking levels supported by SQLite are collapsed into
** a single exclusive lock.  In other words, SHARED, RESERVED, and
** PENDING locks are the same thing as an EXCLUSIVE lock.  SQLite
** still works when you do this, but concurrency is reduced since
** only a single process can be reading the database at a time.
**
** Omit this section if SQLITE_ENABLE_LOCKING_STYLE is turned off
*/
#if SQLITE_ENABLE_LOCKING_STYLE

/*
** Retry flock() calls that fail with EINTR
*/
#ifdef EINTR
static int robust_flock(int fd, int op){
  int rc;
  do{ rc = flock(fd,op); }while( rc<0 && errno==EINTR );
  return rc;
}
#else
# define robust_flock(a,b) flock(a,b)
#endif
     

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int flockCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    /* attempt to get the lock */
    int lrc = robust_flock(pFile->h, LOCK_EX | LOCK_NB);
    if( !lrc ){
      /* got the lock, unlock it */
      lrc = robust_flock(pFile->h, LOCK_UN);
      if ( lrc ) {
        int tErrno = errno;
        /* unlock failed with an error */
        lrc = SQLITE_IOERR_UNLOCK; 
        if( IS_LOCK_ERROR(lrc) ){
          storeLastErrno(pFile, tErrno);
          rc = lrc;
        }
      }
    } else {
      int tErrno = errno;
      reserved = 1;
      /* someone else might have it reserved */
      lrc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK); 
      if( IS_LOCK_ERROR(lrc) ){
        storeLastErrno(pFile, tErrno);
        rc = lrc;
      }
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (flock)\n", pFile->h, rc, reserved));

#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_OK;
    reserved=1;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** flock() only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int flockLock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;

  assert( pFile );

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  
  if (robust_flock(pFile->h, LOCK_EX | LOCK_NB)) {
    int tErrno = errno;
    /* didn't get, must be busy */
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
  } else {
    /* got it, set the type and return ok */
    pFile->eFileLock = eFileLock;
  }
  OSTRACE(("LOCK    %d %s %s (flock)\n", pFile->h, azFileLock(eFileLock), 
           rc==SQLITE_OK ? "ok" : "failed"));
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_BUSY;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int flockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  
  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (flock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really, unlock. */
  if( robust_flock(pFile->h, LOCK_UN) ){
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
    return SQLITE_OK;
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
    return SQLITE_IOERR_UNLOCK;
  }else{
    pFile->eFileLock = NO_LOCK;
    return SQLITE_OK;
  }
}

/*
** Close a file.
*/
static int flockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    flockUnlock(id, NO_LOCK);
    rc = closeUnixFile(id);
  }
  return rc;
}

#endif /* SQLITE_ENABLE_LOCKING_STYLE && !OS_VXWORK */

/******************* End of the flock lock implementation *********************
******************************************************************************/

/******************************************************************************
************************ Begin Named Semaphore Locking ************************
**
** Named semaphore locking is only supported on VxWorks.
**
** Semaphore locking is like dot-lock and flock in that it really only
** supports EXCLUSIVE locking.  Only a single process can read or write
** the database file at a time.  This reduces potential concurrency, but
** makes the lock implementation much easier.
*/
#if OS_VXWORKS

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int semXCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    sem_t *pSem = pFile->pInode->pSem;

    if( sem_trywait(pSem)==-1 ){
      int tErrno = errno;
      if( EAGAIN != tErrno ){
        rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_CHECKRESERVEDLOCK);
        storeLastErrno(pFile, tErrno);
      } else {
        /* someone else has the lock when we are in NO_LOCK */
        reserved = (pFile->eFileLock < SHARED_LOCK);
      }
    }else{
      /* we could have it if we want it */
      sem_post(pSem);
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (sem)\n", pFile->h, rc, reserved));

  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** Semaphore locks only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int semXLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;
  int rc = SQLITE_OK;

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    rc = SQLITE_OK;
    goto sem_end_lock;
  }
  
  /* lock semaphore now but bail out when already locked. */
  if( sem_trywait(pSem)==-1 ){
    rc = SQLITE_BUSY;
    goto sem_end_lock;
  }

  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;

 sem_end_lock:
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int semXUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;

  assert( pFile );
  assert( pSem );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (sem)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really unlock. */
  if ( sem_post(pSem)==-1 ) {
    int rc, tErrno = errno;
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_UNLOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
 ** Close a file.
 */
static int semXClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    semXUnlock(id, NO_LOCK);
    assert( pFile );
    unixEnterMutex();
    releaseInodeInfo(pFile);
    unixLeaveMutex();
    closeUnixFile(id);
  }
  return SQLITE_OK;
}

#endif /* OS_VXWORKS */
/*
** Named semaphore locking is only available on VxWorks.
**
*************** End of the named semaphore lock implementation ****************
******************************************************************************/


/******************************************************************************
*************************** Begin AFP Locking *********************************
**
** AFP is the Apple Filing Protocol.  AFP is a network filesystem found
** on Apple Macintosh computers - both OS9 and OSX.
**
** Third-party implementations of AFP are available.  But this code here
** only works on OSX.
*/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
** The afpLockingContext structure contains all afp lock specific state
*/
typedef struct afpLockingContext afpLockingContext;
struct afpLockingContext {
  int reserved;
  const char *dbPath;             /* Name of the open file */
};

struct ByteRangeLockPB2
{
  unsigned long long offset;        /* offset to first byte to lock */
  unsigned long long length;        /* nbr of bytes to lock */
  unsigned long long retRangeStart; /* nbr of 1st byte locked if successful */
  unsigned char unLockFlag;         /* 1 = unlock, 0 = lock */
  unsigned char startEndFlag;       /* 1=rel to end of fork, 0=rel to start */
  int fd;                           /* file desc to assoc this lock with */
};

#define afpfsByteRangeLock2FSCTL        _IOWR('z', 23, struct ByteRangeLockPB2)

/*
** This is a utility for setting or clearing a bit-range lock on an
** AFP filesystem.
** 
** Return SQLITE_OK on success, SQLITE_BUSY on failure.
*/
static int afpSetLock(
  const char *path,              /* Name of the file to be locked or unlocked */
  unixFile *pFile,               /* Open file descriptor on path */
  unsigned long long offset,     /* First byte to be locked */
  unsigned long long length,     /* Number of bytes to lock */
  int setLockFlag                /* True to set lock.  False to clear lock */
){
  struct ByteRangeLockPB2 pb;
  int err;
  
  pb.unLockFlag = setLockFlag ? 0 : 1;
  pb.startEndFlag = 0;
  pb.offset = offset;
  pb.length = length; 
  pb.fd = pFile->h;
  
  OSTRACE(("AFPSETLOCK [%s] for %d%s in range %llx:%llx\n", 
    (setLockFlag?"ON":"OFF"), pFile->h, (pb.fd==-1?"[testval-1]":""),
    offset, length));
  err = fsctl(path, afpfsByteRangeLock2FSCTL, &pb, 0);
  if ( err==-1 ) {
    int rc;
    int tErrno = errno;
    OSTRACE(("AFPSETLOCK failed to fsctl() '%s' %d %s\n",
             path, tErrno, strerror(tErrno)));
#ifdef SQLITE_IGNORE_AFP_LOCK_ERRORS
    rc = SQLITE_BUSY;
#else
    rc = sqliteErrorFromPosixError(tErrno,
                    setLockFlag ? SQLITE_IOERR_LOCK : SQLITE_IOERR_UNLOCK);
#endif /* SQLITE_IGNORE_AFP_LOCK_ERRORS */
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc;
  } else {
    return SQLITE_OK;
  }
}

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int afpCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  afpLockingContext *context;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  context = (afpLockingContext *) pFile->lockingContext;
  if( context->reserved ){
    *pResOut = 1;
    return SQLITE_OK;
  }
  unixEnterMutex(); /* Because pFile->pInode is shared across threads */
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->pInode->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it.
   */
  if( !reserved ){
    /* lock the RESERVED byte */
    int lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);  
    if( SQLITE_OK==lrc ){
      /* if we succeeded in taking the reserved lock, unlock it to restore
      ** the original state */
      lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
    } else {
      /* if we failed to get the lock then someone else must have it */
      reserved = 1;
    }
    if( IS_LOCK_ERROR(lrc) ){
      rc=lrc;
    }
  }
  
  unixLeaveMutex();
  OSTRACE(("TEST WR-LOCK %d %d %d (afp)\n", pFile->h, rc, reserved));
  
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int afpLock(sqlite3_file *id, int eFileLock){
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode = pFile->pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  
  assert( pFile );
  OSTRACE(("LOCK    %d %s was %s(%s,%d) pid=%d (afp)\n", pFile->h,
           azFileLock(eFileLock), azFileLock(pFile->eFileLock),
           azFileLock(pInode->eFileLock), pInode->nShared , osGetpid(0)));

  /* If there is already a lock of this type or more restrictive on the
  ** unixFile, do nothing. Don't use the afp_end_lock: exit path, as
  ** unixEnterMutex() hasn't been called yet.
  */
  if( pFile->eFileLock>=eFileLock ){
    OSTRACE(("LOCK    %d %s ok (already held) (afp)\n", pFile->h,
           azFileLock(eFileLock)));
    return SQLITE_OK;
  }

  /* Make sure the locking sequence is correct
  **  (1) We never move from unlocked to anything higher than shared lock.
  **  (2) SQLite never explicitly requests a pendig lock.
  **  (3) A shared lock is always held when a reserve lock is requested.
  */
  assert( pFile->eFileLock!=NO_LOCK || eFileLock==SHARED_LOCK );
  assert( eFileLock!=PENDING_LOCK );
  assert( eFileLock!=RESERVED_LOCK || pFile->eFileLock==SHARED_LOCK );
  
  /* This mutex is needed because pFile->pInode is shared across threads
  */
  unixEnterMutex();
  pInode = pFile->pInode;

  /* If some thread using this PID has a lock via a different unixFile*
  ** handle that precludes the requested lock, return BUSY.
  */
  if( (pFile->eFileLock!=pInode->eFileLock && 
       (pInode->eFileLock>=PENDING_LOCK || eFileLock>SHARED_LOCK))
     ){
    rc = SQLITE_BUSY;
    goto afp_end_lock;
  }
  
  /* If a SHARED lock is requested, and some thread using this PID already
  ** has a SHARED or RESERVED lock, then increment reference counts and
  ** return SQLITE_OK.
  */
  if( eFileLock==SHARED_LOCK && 
     (pInode->eFileLock==SHARED_LOCK || pInode->eFileLock==RESERVED_LOCK) ){
    assert( eFileLock==SHARED_LOCK );
    assert( pFile->eFileLock==0 );
    assert( pInode->nShared>0 );
    pFile->eFileLock = SHARED_LOCK;
    pInode->nShared++;
    pInode->nLock++;
    goto afp_end_lock;
  }
    
  /* A PENDING lock is needed before acquiring a SHARED lock and before
  ** acquiring an EXCLUSIVE lock.  For the SHARED lock, the PENDING will
  ** be released.
  */
  if( eFileLock==SHARED_LOCK 
      || (eFileLock==EXCLUSIVE_LOCK && pFile->eFileLock<PENDING_LOCK)
  ){
    int failed;
    failed = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 1);
    if (failed) {
      rc = failed;
      goto afp_end_lock;
    }
  }
  
  /* If control gets to this point, then actually go ahead and make
  ** operating system calls for the specified lock.
  */
  if( eFileLock==SHARED_LOCK ){
    int lrc1, lrc2, lrc1Errno = 0;
    long lk, mask;
    
    assert( pInode->nShared==0 );
    assert( pInode->eFileLock==0 );
        
    mask = (sizeof(long)==8) ? LARGEST_INT64 : 0x7fffffff;
    /* Now get the read-lock SHARED_LOCK */
    /* note that the quality of the randomness doesn't matter that much */
    lk = random(); 
    pInode->sharedByte = (lk & mask)%(SHARED_SIZE - 1);
    lrc1 = afpSetLock(context->dbPath, pFile, 
          SHARED_FIRST+pInode->sharedByte, 1, 1);
    if( IS_LOCK_ERROR(lrc1) ){
      lrc1Errno = pFile->lastErrno;
    }
    /* Drop the temporary PENDING lock */
    lrc2 = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    
    if( IS_LOCK_ERROR(lrc1) ) {
      storeLastErrno(pFile, lrc1Errno);
      rc = lrc1;
      goto afp_end_lock;
    } else if( IS_LOCK_ERROR(lrc2) ){
      rc = lrc2;
      goto afp_end_lock;
    } else if( lrc1 != SQLITE_OK ) {
      rc = lrc1;
    } else {
      pFile->eFileLock = SHARED_LOCK;
      pInode->nLock++;
      pInode->nShared = 1;
    }
  }else if( eFileLock==EXCLUSIVE_LOCK && pInode->nShared>1 ){
    /* We are trying for an exclusive lock but another thread in this
     ** same process is still holding a shared lock. */
    rc = SQLITE_BUSY;
  }else{
    /* The request was for a RESERVED or EXCLUSIVE lock.  It is
    ** assumed that there is a SHARED or greater lock on the file
    ** already.
    */
    int failed = 0;
    assert( 0!=pFile->eFileLock );
    if (eFileLock >= RESERVED_LOCK && pFile->eFileLock < RESERVED_LOCK) {
        /* Acquire a RESERVED lock */
        failed = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);
      if( !failed ){
        context->reserved = 1;
      }
    }
    if (!failed && eFileLock == EXCLUSIVE_LOCK) {
      /* Acquire an EXCLUSIVE lock */
        
      /* Remove the shared lock before trying the range.  we'll need to 
      ** reestablish the shared lock if we can't get the  afpUnlock
      */
      if( !(failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST +
                         pInode->sharedByte, 1, 0)) ){
        int failed2 = SQLITE_OK;
        /* now attemmpt to get the exclusive lock range */
        failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST, 
                               SHARED_SIZE, 1);
        if( failed && (failed2 = afpSetLock(context->dbPath, pFile, 
                       SHARED_FIRST + pInode->sharedByte, 1, 1)) ){
          /* Can't reestablish the shared lock.  Sqlite can't deal, this is
          ** a critical I/O error
          */
          rc = ((failed & SQLITE_IOERR) == SQLITE_IOERR) ? failed2 : 
               SQLITE_IOERR_LOCK;
          goto afp_end_lock;
        } 
      }else{
        rc = failed; 
      }
    }
    if( failed ){
      rc = failed;
    }
  }
  
  if( rc==SQLITE_OK ){
    pFile->eFileLock = eFileLock;
    pInode->eFileLock = eFileLock;
  }else if( eFileLock==EXCLUSIVE_LOCK ){
    pFile->eFileLock = PENDING_LOCK;
    pInode->eFileLock = PENDING_LOCK;
  }
  
afp_end_lock:
  unixLeaveMutex();
  OSTRACE(("LOCK    %d %s %s (afp)\n", pFile->h, azFileLock(eFileLock), 
         rc==SQLITE_OK ? "ok" : "failed"));
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int afpUnlock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  int skipShared = 0;
#ifdef SQLITE_TEST
  int h = pFile->h;
#endif

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d(%d,%d) pid=%d (afp)\n", pFile->h, eFileLock,
           pFile->eFileLock, pFile->pInode->eFileLock, pFile->pInode->nShared,
           osGetpid(0)));

  assert( eFileLock<=SHARED_LOCK );
  if( pFile->eFileLock<=eFileLock ){
    return SQLITE_OK;
  }
  unixEnterMutex();
  pInode = pFile->pInode;
  assert( pInode->nShared!=0 );
  if( pFile->eFileLock>SHARED_LOCK ){
    assert( pInode->eFileLock==pFile->eFileLock );
    SimulateIOErrorBenign(1);
    SimulateIOError( h=(-1) )
    SimulateIOErrorBenign(0);
    
#ifdef SQLITE_DEBUG
    /* When reducing a lock such that other processes can start
    ** reading the database file again, make sure that the
    ** transaction counter was updated if any part of the database
    ** file changed.  If the transaction counter is not updated,
    ** other connections to the same file might not realize that
    ** the file has changed and hence might not know to flush their
    ** cache.  The use of a stale cache can lead to database corruption.
    */
    assert( pFile->inNormalWrite==0
           || pFile->dbUpdate==0
           || pFile->transCntrChng==1 );
    pFile->inNormalWrite = 0;
#endif
    
    if( pFile->eFileLock==EXCLUSIVE_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, SHARED_FIRST, SHARED_SIZE, 0);
      if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1) ){
        /* only re-establish the shared lock if necessary */
        int sharedLockByte = SHARED_FIRST+pInode->sharedByte;
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 1);
      } else {
        skipShared = 1;
      }
    }
    if( rc==SQLITE_OK && pFile->eFileLock>=PENDING_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    } 
    if( rc==SQLITE_OK && pFile->eFileLock>=RESERVED_LOCK && context->reserved ){
      rc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
      if( !rc ){ 
        context->reserved = 0; 
      }
    }
    if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1)){
      pInode->eFileLock = SHARED_LOCK;
    }
  }
  if( rc==SQLITE_OK && eFileLock==NO_LOCK ){

    /* Decrement the shared lock counter.  Release the lock using an
    ** OS call only when all threads in this same process have released
    ** the lock.
    */
    unsigned long long sharedLockByte = SHARED_FIRST+pInode->sharedByte;
    pInode->nShared--;
    if( pInode->nShared==0 ){
      SimulateIOErrorBenign(1);
      SimulateIOError( h=(-1) )
      SimulateIOErrorBenign(0);
      if( !skipShared ){
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 0);
      }
      if( !rc ){
        pInode->eFileLock = NO_LOCK;
        pFile->eFileLock = NO_LOCK;
      }
    }
    if( rc==SQLITE_OK ){
      pInode->nLock--;
      assert( pInode->nLock>=0 );
      if( pInode->nLock==0 ){
        closePendingFds(pFile);
      }
    }
  }
  
  unixLeaveMutex();
  if( rc==SQLITE_OK ) pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Close a file & cleanup AFP specific locking context 
*/
static int afpClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    afpUnlock(id, NO_LOCK);
    unixEnterMutex();
    if( pFile->pInode && pFile->pInode->nLock ){
      /* If there are outstanding locks, do not actually close the file just
      ** yet because that would clear those locks.  Instead, add the file
      ** descriptor to pInode->aPending.  It will be automatically closed when
      ** the last lock is cleared.
      */
      setPendingFd(pFile);
    }
    releaseInodeInfo(pFile);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
    unixLeaveMutex();
  }
  return rc;
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the AFP lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  If you don't compile for a mac, then the "unix-afp"
** VFS is not available.
**
********************* End of the AFP lock implementation **********************
******************************************************************************/

/******************************************************************************
*************************** Begin NFS Locking ********************************/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
 ** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
 ** must be either NO_LOCK or SHARED_LOCK.
 **
 ** If the locking level of the file descriptor is already at or below
 ** the requested locking level, this routine is a no-op.
 */
static int nfsUnlock(sqlite3_file *id, int eFileLock){
  return posixUnlock(id, eFileLock, 1);
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the NFS lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  
**
********************* End of the NFS lock implementation **********************
******************************************************************************/

/******************************************************************************
**************** Non-locking sqlite3_file methods *****************************
**
** The next division contains implementations for all methods of the 
** sqlite3_file object other than the locking methods.  The locking
** methods were defined in divisions above (one locking method per
** division).  Those methods that are common to all locking modes
** are gather together into this division.
*/

/*
** Seek to the offset passed as the second argument, then read cnt 
** bytes into pBuf. Return the number of bytes actually read.
**
** NB:  If you define USE_PREAD or USE_PREAD64, then it might also
** be necessary to define _XOPEN_SOURCE to be 500.  This varies from
** one system to another.  Since SQLite does not define USE_PREAD
** in any form by default, we will not attempt to define _XOPEN_SOURCE.
** See tickets #2741 and #2681.
**
** To avoid stomping the errno value on a failed read the lastErrno value
** is set before returning.
*/
static int seekAndRead(unixFile *id, sqlite3_int64 offset, void *pBuf, int cnt){
  int got;
  int prior = 0;
#if (!defined(USE_PREAD) && !defined(USE_PREAD64))
  i64 newOffset;
#endif
  TIMER_START;
  assert( cnt==(cnt&0x1ffff) );
  assert( id->h>2 );
  do{
#if defined(USE_PREAD)
    got = osPread(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#elif defined(USE_PREAD64)
    got = osPread64(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#else
    newOffset = lseek(id->h, offset, SEEK_SET);
    SimulateIOError( newOffset-- );
    if( newOffset!=offset ){
      if( newOffset == -1 ){
        storeLastErrno((unixFile*)id, errno);
      }else{
        storeLastErrno((unixFile*)id, 0);
      }
      return -1;
    }
    got = osRead(id->h, pBuf, cnt);
#endif
    if( got==cnt ) break;
    if( got<0 ){
      if( errno==EINTR ){ got = 1; continue; }
      prior = 0;
      storeLastErrno((unixFile*)id,  errno);
      break;
    }else if( got>0 ){
      cnt -= got;
      offset += got;
      prior += got;
      pBuf = (void*)(got + (char*)pBuf);
    }
  }while( got>0 );
  TIMER_END;
  OSTRACE(("READ    %-3d %5d %7lld %llu\n",
            id->h, got+prior, offset-prior, TIMER_ELAPSED));
  return got+prior;
}

/*
** Read data from a file into a buffer.  Return SQLITE_OK if all
** bytes were read successfully and SQLITE_IOERR if anything goes
** wrong.
*/
static int unixRead(
  sqlite3_file *id, 
  void *pBuf, 
  int amt,
  sqlite3_int64 offset
){
  unixFile *pFile = (unixFile *)id;
  int got;
  assert( id );
  assert( offset>=0 );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this read request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif

  got = seekAndRead(pFile, offset, pBuf, amt);
  if( got==amt ){
    return SQLITE_OK;
  }else if( got<0 ){
    /* lastErrno set by seekAndRead */
    return SQLITE_IOERR_READ;
  }else{
    storeLastErrno(pFile, 0);   /* not a system error */
    /* Unread parts of the buffer must be zero-filled */
    memset(&((char*)pBuf)[got], 0, amt-got);
    return SQLITE_IOERR_SHORT_READ;
  }
}

/*
** Attempt to seek the file-descriptor passed as the first argument to
** absolute offset iOff, then attempt to write nBuf bytes of data from
** pBuf to it. If an error occurs, return -1 and set *piErrno. Otherwise, 
** return the actual number of bytes written (which may be less than
** nBuf).
*/
static int seekAndWriteFd(
  int fd,                         /* File descriptor to write to */
  i64 iOff,                       /* File offset to begin writing at */
  const void *pBuf,               /* Copy data from this buffer to the file */
  int nBuf,                       /* Size of buffer pBuf in bytes */
  int *piErrno                    /* OUT: Error number if error occurs */
){
  int rc = 0;                     /* Value returned by system call */

  assert( nBuf==(nBuf&0x1ffff) );
  assert( fd>2 );
  nBuf &= 0x1ffff;
  TIMER_START;

#if defined(USE_PREAD)
  do{ rc = (int)osPwrite(fd, pBuf, nBuf, iOff); }while( rc<0 && errno==EINTR );
#elif defined(USE_PREAD64)
  do{ rc = (int)osPwrite64(fd, pBuf, nBuf, iOff);}while( rc<0 && errno==EINTR);
#else
  do{
    i64 iSeek = lseek(fd, iOff, SEEK_SET);
    SimulateIOError( iSeek-- );

    if( iSeek!=iOff ){
      if( piErrno ) *piErrno = (iSeek==-1 ? errno : 0);
      return -1;
    }
    rc = osWrite(fd, pBuf, nBuf);
  }while( rc<0 && errno==EINTR );
#endif

  TIMER_END;
  OSTRACE(("WRITE   %-3d %5d %7lld %llu\n", fd, rc, iOff, TIMER_ELAPSED));

  if( rc<0 && piErrno ) *piErrno = errno;
  return rc;
}


/*
** Seek to the offset in id->offset then read cnt bytes into pBuf.
** Return the number of bytes actually read.  Update the offset.
**
** To avoid stomping the errno value on a failed write the lastErrno value
** is set before returning.
*/
static int seekAndWrite(unixFile *id, i64 offset, const void *pBuf, int cnt){
  return seekAndWriteFd(id->h, offset, pBuf, cnt, &id->lastErrno);
}


/*
** Write data from a buffer into a file.  Return SQLITE_OK on success
** or some other error code on failure.
*/
static int unixWrite(
  sqlite3_file *id, 
  const void *pBuf, 
  int amt,
  sqlite3_int64 offset 
){
  unixFile *pFile = (unixFile*)id;
  int wrote = 0;
  assert( id );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#ifdef SQLITE_DEBUG
  /* If we are doing a normal write to a database file (as opposed to
  ** doing a hot-journal rollback or a write to some file other than a
  ** normal database file) then record the fact that the database
  ** has changed.  If the transaction counter is modified, record that
  ** fact too.
  */
  if( pFile->inNormalWrite ){
    pFile->dbUpdate = 1;  /* The database has been modified */
    if( offset<=24 && offset+amt>=27 ){
      int rc;
      char oldCntr[4];
      SimulateIOErrorBenign(1);
      rc = seekAndRead(pFile, 24, oldCntr, 4);
      SimulateIOErrorBenign(0);
      if( rc!=4 || memcmp(oldCntr, &((char*)pBuf)[24-offset], 4)!=0 ){
        pFile->transCntrChng = 1;  /* The transaction counter has changed */
      }
    }
  }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this write request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif
 
  while( (wrote = seekAndWrite(pFile, offset, pBuf, amt))<amt && wrote>0 ){
    amt -= wrote;
    offset += wrote;
    pBuf = &((char*)pBuf)[wrote];
  }
  SimulateIOError(( wrote=(-1), amt=1 ));
  SimulateDiskfullError(( wrote=0, amt=1 ));

  if( amt>wrote ){
    if( wrote<0 && pFile->lastErrno!=ENOSPC ){
      /* lastErrno set by seekAndWrite */
      return SQLITE_IOERR_WRITE;
    }else{
      storeLastErrno(pFile, 0); /* not a system error */
      return SQLITE_FULL;
    }
  }

  return SQLITE_OK;
}

#ifdef SQLITE_TEST
/*
** Count the number of fullsyncs and normal syncs.  This is used to test
** that syncs and fullsyncs are occurring at the right times.
*/
SQLITE_API int sqlite3_sync_count = 0;
SQLITE_API int sqlite3_fullsync_count = 0;
#endif

/*
** We do not trust systems to provide a working fdatasync().  Some do.
** Others do no.  To be safe, we will stick with the (slightly slower)
** fsync(). If you know that your system does support fdatasync() correctly,
** then simply compile with -Dfdatasync=fdatasync or -DHAVE_FDATASYNC
*/
#if !defined(fdatasync) && !HAVE_FDATASYNC
# define fdatasync fsync
#endif

/*
** Define HAVE_FULLFSYNC to 0 or 1 depending on whether or not
** the F_FULLFSYNC macro is defined.  F_FULLFSYNC is currently
** only available on Mac OS X.  But that could change.
*/
#ifdef F_FULLFSYNC
# define HAVE_FULLFSYNC 1
#else
# define HAVE_FULLFSYNC 0
#endif


/*
** The fsync() system call does not work as advertised on many
** unix systems.  The following procedure is an attempt to make
** it work better.
**
** The SQLITE_NO_SYNC macro disables all fsync()s.  This is useful
** for testing when we want to run through the test suite quickly.
** You are strongly advised *not* to deploy with SQLITE_NO_SYNC
** enabled, however, since with SQLITE_NO_SYNC enabled, an OS crash
** or power failure will likely corrupt the database file.
**
** SQLite sets the dataOnly flag if the size of the file is unchanged.
** The idea behind dataOnly is that it should only write the file content
** to disk, not the inode.  We only set dataOnly if the file size is 
** unchanged since the file size is part of the inode.  However, 
** Ted Ts'o tells us that fdatasync() will also write the inode if the
** file size has changed.  The only real difference between fdatasync()
** and fsync(), Ted tells us, is that fdatasync() will not flush the
** inode if the mtime or owner or other inode attributes have changed.
** We only care about the file size, not the other file attributes, so
** as far as SQLite is concerned, an fdatasync() is always adequate.
** So, we always use fdatasync() if it is available, regardless of
** the value of the dataOnly flag.
*/
static int full_fsync(int fd, int fullSync, int dataOnly){
  int rc;

  /* The following "ifdef/elif/else/" block has the same structure as
  ** the one below. It is replicated here solely to avoid cluttering 
  ** up the real code with the UNUSED_PARAMETER() macros.
  */
#ifdef SQLITE_NO_SYNC
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#elif HAVE_FULLFSYNC
  UNUSED_PARAMETER(dataOnly);
#else
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#endif

  /* Record the number of times that we do a normal fsync() and 
  ** FULLSYNC.  This is used during testing to verify that this procedure
  ** gets called with the correct arguments.
  */
#ifdef SQLITE_TEST
  if( fullSync ) sqlite3_fullsync_count++;
  sqlite3_sync_count++;
#endif

  /* If we compiled with the SQLITE_NO_SYNC flag, then syncing is a
  ** no-op
  */
#ifdef SQLITE_NO_SYNC
  rc = SQLITE_OK;
#elif HAVE_FULLFSYNC
  if( fullSync ){
    rc = osFcntl(fd, F_FULLFSYNC, 0);
  }else{
    rc = 1;
  }
  /* If the FULLFSYNC failed, fall back to attempting an fsync().
  ** It shouldn't be possible for fullfsync to fail on the local 
  ** file system (on OSX), so failure indicates that FULLFSYNC
  ** isn't supported for this file system. So, attempt an fsync 
  ** and (for now) ignore the overhead of a superfluous fcntl call.  
  ** It'd be better to detect fullfsync support once and avoid 
  ** the fcntl call every time sync is called.
  */
  if( rc ) rc = fsync(fd);

#elif defined(__APPLE__)
  /* fdatasync() on HFS+ doesn't yet flush the file size if it changed correctly
  ** so currently we default to the macro that redefines fdatasync to fsync
  */
  rc = fsync(fd);
#else 
  rc = fdatasync(fd);
#if OS_VXWORKS
  if( rc==-1 && errno==ENOTSUP ){
    rc = fsync(fd);
  }
#endif /* OS_VXWORKS */
#endif /* ifdef SQLITE_NO_SYNC elif HAVE_FULLFSYNC */

  if( OS_VXWORKS && rc!= -1 ){
    rc = 0;
  }
  return rc;
}

/*
** Open a file descriptor to the directory containing file zFilename.
** If successful, *pFd is set to the opened file descriptor and
** SQLITE_OK is returned. If an error occurs, either SQLITE_NOMEM
** or SQLITE_CANTOPEN is returned and *pFd is set to an undefined
** value.
**
** The directory file descriptor is used for only one thing - to
** fsync() a directory to make sure file creation and deletion events
** are flushed to disk.  Such fsyncs are not needed on newer
** journaling filesystems, but are required on older filesystems.
**
** This routine can be overridden using the xSetSysCall interface.
** The ability to override this routine was added in support of the
** chromium sandbox.  Opening a directory is a security risk (we are
** told) so making it overrideable allows the chromium sandbox to
** replace this routine with a harmless no-op.  To make this routine
** a no-op, replace it with a stub that returns SQLITE_OK but leaves
** *pFd set to a negative number.
**
** If SQLITE_OK is returned, the caller is responsible for closing
** the file descriptor *pFd using close().
*/
static int openDirectory(const char *zFilename, int *pFd){
  int ii;
  int fd = -1;
  char zDirname[MAX_PATHNAME+1];

  sqlite3_snprintf(MAX_PATHNAME, zDirname, "%s", zFilename);
  for(ii=(int)strlen(zDirname); ii>1 && zDirname[ii]!='/'; ii--);
  if( ii>0 ){
    zDirname[ii] = '\0';
    fd = robust_open(zDirname, O_RDONLY|O_BINARY, 0);
    if( fd>=0 ){
      OSTRACE(("OPENDIR %-3d %s\n", fd, zDirname));
    }
  }
  *pFd = fd;
  return (fd>=0?SQLITE_OK:unixLogError(SQLITE_CANTOPEN_BKPT, "open", zDirname));
}

/*
** Make sure all writes to a particular file are committed to disk.
**
** If dataOnly==0 then both the file itself and its metadata (file
** size, access time, etc) are synced.  If dataOnly!=0 then only the
** file data is synced.
**
** Under Unix, also make sure that the directory entry for the file
** has been created by fsync-ing the directory that contains the file.
** If we do not do this and we encounter a power failure, the directory
** entry for the journal might not exist after we reboot.  The next
** SQLite to access the file will not know that the journal exists (because
** the directory entry for the journal was never created) and the transaction
** will not roll back - possibly leading to database corruption.
*/
static int unixSync(sqlite3_file *id, int flags){
  int rc;
  unixFile *pFile = (unixFile*)id;

  int isDataOnly = (flags&SQLITE_SYNC_DATAONLY);
  int isFullsync = (flags&0x0F)==SQLITE_SYNC_FULL;

  /* Check that one of SQLITE_SYNC_NORMAL or FULL was passed */
  assert((flags&0x0F)==SQLITE_SYNC_NORMAL
      || (flags&0x0F)==SQLITE_SYNC_FULL
  );

  /* Unix cannot, but some systems may return SQLITE_FULL from here. This
  ** line is to test that doing so does not cause any problems.
  */
  SimulateDiskfullError( return SQLITE_FULL );

  assert( pFile );
  OSTRACE(("SYNC    %-3d\n", pFile->h));
  rc = full_fsync(pFile->h, isFullsync, isDataOnly);
  SimulateIOError( rc=1 );
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_FSYNC, "full_fsync", pFile->zPath);
  }

  /* Also fsync the directory containing the file if the DIRSYNC flag
  ** is set.  This is a one-time occurrence.  Many systems (examples: AIX)
  ** are unable to fsync a directory, so ignore errors on the fsync.
  */
  if( pFile->ctrlFlags & UNIXFILE_DIRSYNC ){
    int dirfd;
    OSTRACE(("DIRSYNC %s (have_fullfsync=%d fullsync=%d)\n", pFile->zPath,
            HAVE_FULLFSYNC, isFullsync));
    rc = osOpenDirectory(pFile->zPath, &dirfd);
    if( rc==SQLITE_OK && dirfd>=0 ){
      full_fsync(dirfd, 0, 0);
      robust_close(pFile, dirfd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
    pFile->ctrlFlags &= ~UNIXFILE_DIRSYNC;
  }
  return rc;
}

/*
** Truncate an open file to a specified size
*/
static int unixTruncate(sqlite3_file *id, i64 nByte){
  unixFile *pFile = (unixFile *)id;
  int rc;
  assert( pFile );
  SimulateIOError( return SQLITE_IOERR_TRUNCATE );

  /* If the user has configured a chunk-size for this file, truncate the
  ** file so that it consists of an integer number of chunks (i.e. the
  ** actual file size after the operation may be larger than the requested
  ** size).
  */
  if( pFile->szChunk>0 ){
    nByte = ((nByte + pFile->szChunk - 1)/pFile->szChunk) * pFile->szChunk;
  }

  rc = robust_ftruncate(pFile->h, nByte);
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
  }else{
#ifdef SQLITE_DEBUG
    /* If we are doing a normal write to a database file (as opposed to
    ** doing a hot-journal rollback or a write to some file other than a
    ** normal database file) and we truncate the file to zero length,
    ** that effectively updates the change counter.  This might happen
    ** when restoring a database using the backup API from a zero-length
    ** source.
    */
    if( pFile->inNormalWrite && nByte==0 ){
      pFile->transCntrChng = 1;
    }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
    /* If the file was just truncated to a size smaller than the currently
    ** mapped region, reduce the effective mapping size as well. SQLite will
    ** use read() and write() to access data beyond this point from now on.  
    */
    if( nByte<pFile->mmapSize ){
      pFile->mmapSize = nByte;
    }
#endif

    return SQLITE_OK;
  }
}

/*
** Determine the current size of a file in bytes
*/
static int unixFileSize(sqlite3_file *id, i64 *pSize){
  int rc;
  struct stat buf;
  assert( id );
  rc = osFstat(((unixFile*)id)->h, &buf);
  SimulateIOError( rc=1 );
  if( rc!=0 ){
    storeLastErrno((unixFile*)id, errno);
    return SQLITE_IOERR_FSTAT;
  }
  *pSize = buf.st_size;

  /* When opening a zero-size database, the findInodeInfo() procedure
  ** writes a single byte into that file in order to work around a bug
  ** in the OS-X msdos filesystem.  In order to avoid problems with upper
  ** layers, we need to report this file size as zero even though it is
  ** really 1.   Ticket #3260.
  */
  if( *pSize==1 ) *pSize = 0;


  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Handler for proxy-locking file-control verbs.  Defined below in the
** proxying locking division.
*/
static int proxyFileControl(sqlite3_file*,int,void*);
#endif

/* 
** This function is called to handle the SQLITE_FCNTL_SIZE_HINT 
** file-control operation.  Enlarge the database to nBytes in size
** (rounded up to the next chunk-size).  If the database is already
** nBytes or larger, this routine is a no-op.
*/
static int fcntlSizeHint(unixFile *pFile, i64 nByte){
  if( pFile->szChunk>0 ){
    i64 nSize;                    /* Required file size */
    struct stat buf;              /* Used to hold return values of fstat() */
   
    if( osFstat(pFile->h, &buf) ){
      return SQLITE_IOERR_FSTAT;
    }

    nSize = ((nByte+pFile->szChunk-1) / pFile->szChunk) * pFile->szChunk;
    if( nSize>(i64)buf.st_size ){

#if defined(HAVE_POSIX_FALLOCATE) && HAVE_POSIX_FALLOCATE
      /* The code below is handling the return value of osFallocate() 
      ** correctly. posix_fallocate() is defined to "returns zero on success, 
      ** or an error number on  failure". See the manpage for details. */
      int err;
      do{
        err = osFallocate(pFile->h, buf.st_size, nSize-buf.st_size);
      }while( err==EINTR );
      if( err ) return SQLITE_IOERR_WRITE;
#else
      /* If the OS does not have posix_fallocate(), fake it. Write a 
      ** single byte to the last byte in each block that falls entirely
      ** within the extended region. Then, if required, a single byte
      ** at offset (nSize-1), to set the size of the file correctly.
      ** This is a similar technique to that used by glibc on systems
      ** that do not have a real fallocate() call.
      */
      int nBlk = buf.st_blksize;  /* File-system block size */
      int nWrite = 0;             /* Number of bytes written by seekAndWrite */
      i64 iWrite;                 /* Next offset to write to */

      iWrite = ((buf.st_size + 2*nBlk - 1)/nBlk)*nBlk-1;
      assert( iWrite>=buf.st_size );
      assert( (iWrite/nBlk)==((buf.st_size+nBlk-1)/nBlk) );
      assert( ((iWrite+1)%nBlk)==0 );
      for(/*no-op*/; iWrite<nSize; iWrite+=nBlk ){
        nWrite = seekAndWrite(pFile, iWrite, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
      if( nWrite==0 || (nSize%nBlk) ){
        nWrite = seekAndWrite(pFile, nSize-1, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
#endif
    }
  }

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFile->mmapSizeMax>0 && nByte>pFile->mmapSize ){
    int rc;
    if( pFile->szChunk<=0 ){
      if( robust_ftruncate(pFile->h, nByte) ){
        storeLastErrno(pFile, errno);
        return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
      }
    }

    rc = unixMapfile(pFile, nByte);
    return rc;
  }
#endif

  return SQLITE_OK;
}

/*
** If *pArg is initially negative then this is a query.  Set *pArg to
** 1 or 0 depending on whether or not bit mask of pFile->ctrlFlags is set.
**
** If *pArg is 0 or 1, then clear or set the mask bit of pFile->ctrlFlags.
*/
static void unixModeBit(unixFile *pFile, unsigned char mask, int *pArg){
  if( *pArg<0 ){
    *pArg = (pFile->ctrlFlags & mask)!=0;
  }else if( (*pArg)==0 ){
    pFile->ctrlFlags &= ~mask;
  }else{
    pFile->ctrlFlags |= mask;
  }
}

/* Forward declaration */
static int unixGetTempname(int nBuf, char *zBuf);

/*
** Information and control of an open file handle.
*/
static int unixFileControl(sqlite3_file *id, int op, void *pArg){
  unixFile *pFile = (unixFile*)id;
  switch( op ){
    case SQLITE_FCNTL_WAL_BLOCK: {
      /* pFile->ctrlFlags |= UNIXFILE_BLOCK; // Deferred feature */
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LOCKSTATE: {
      *(int*)pArg = pFile->eFileLock;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LAST_ERRNO: {
      *(int*)pArg = pFile->lastErrno;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_CHUNK_SIZE: {
      pFile->szChunk = *(int *)pArg;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SIZE_HINT: {
      int rc;
      SimulateIOErrorBenign(1);
      rc = fcntlSizeHint(pFile, *(i64 *)pArg);
      SimulateIOErrorBenign(0);
      return rc;
    }
    case SQLITE_FCNTL_PERSIST_WAL: {
      unixModeBit(pFile, UNIXFILE_PERSIST_WAL, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_POWERSAFE_OVERWRITE: {
      unixModeBit(pFile, UNIXFILE_PSOW, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_VFSNAME: {
      *(char**)pArg = sqlite3_mprintf("%s", pFile->pVfs->zName);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_TEMPFILENAME: {
      char *zTFile = sqlite3_malloc64( pFile->pVfs->mxPathname );
      if( zTFile ){
        unixGetTempname(pFile->pVfs->mxPathname, zTFile);
        *(char**)pArg = zTFile;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_HAS_MOVED: {
      *(int*)pArg = fileHasMoved(pFile);
      return SQLITE_OK;
    }
#if SQLITE_MAX_MMAP_SIZE>0
    case SQLITE_FCNTL_MMAP_SIZE: {
      i64 newLimit = *(i64*)pArg;
      int rc = SQLITE_OK;
      if( newLimit>sqlite3GlobalConfig.mxMmap ){
        newLimit = sqlite3GlobalConfig.mxMmap;
      }
      *(i64*)pArg = pFile->mmapSizeMax;
      if( newLimit>=0 && newLimit!=pFile->mmapSizeMax && pFile->nFetchOut==0 ){
        pFile->mmapSizeMax = newLimit;
        if( pFile->mmapSize>0 ){
          unixUnmapfile(pFile);
          rc = unixMapfile(pFile, -1);
        }
      }
      return rc;
    }
#endif
#ifdef SQLITE_DEBUG
    /* The pager calls this method to signal that it has done
    ** a rollback and that the database is therefore unchanged and
    ** it hence it is OK for the transaction change counter to be
    ** unchanged.
    */
    case SQLITE_FCNTL_DB_UNCHANGED: {
      ((unixFile*)id)->dbUpdate = 0;
      return SQLITE_OK;
    }
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    case SQLITE_FCNTL_SET_LOCKPROXYFILE:
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      return proxyFileControl(id,op,pArg);
    }
#endif /* SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__) */
  }
  return SQLITE_NOTFOUND;
}

/*
** Return the sector size in bytes of the underlying block device for
** the specified file. This is almost always 512 bytes, but may be
** larger for some devices.
**
** SQLite code assumes this function cannot fail. It also assumes that
** if two files are created in the same file-system directory (i.e.
** a database and its journal file) that the sector size will be the
** same for both.
*/
#ifndef __QNXNTO__ 
static int unixSectorSize(sqlite3_file *NotUsed){
  UNUSED_PARAMETER(NotUsed);
  return SQLITE_DEFAULT_SECTOR_SIZE;
}
#endif

/*
** The following version of unixSectorSize() is optimized for QNX.
*/
#ifdef __QNXNTO__
#include <sys/dcmd_blk.h>
#include <sys/statvfs.h>
static int unixSectorSize(sqlite3_file *id){
  unixFile *pFile = (unixFile*)id;
  if( pFile->sectorSize == 0 ){
    struct statvfs fsInfo;
       
    /* Set defaults for non-supported filesystems */
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
    pFile->deviceCharacteristics = 0;
    if( fstatvfs(pFile->h, &fsInfo) == -1 ) {
      return pFile->sectorSize;
    }

    if( !strcmp(fsInfo.f_basetype, "tmp") ) {
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC4K |       /* All ram filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "etfs") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* etfs cluster size writes are atomic */
        (pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) |
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx6") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC |         /* All filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx4") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "dos") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else{
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC512 |      /* blocks are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        0;
    }
  }
  /* Last chance verification.  If the sector size isn't a multiple of 512
  ** then it isn't valid.*/
  if( pFile->sectorSize % 512 != 0 ){
    pFile->deviceCharacteristics = 0;
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
  }
  return pFile->sectorSize;
}
#endif /* __QNXNTO__ */

/*
** Return the device characteristics for the file.
**
** This VFS is set up to return SQLITE_IOCAP_POWERSAFE_OVERWRITE by default.
** However, that choice is controversial since technically the underlying
** file system does not always provide powersafe overwrites.  (In other
** words, after a power-loss event, parts of the file that were never
** written might end up being altered.)  However, non-PSOW behavior is very,
** very rare.  And asserting PSOW makes a large reduction in the amount
** of required I/O for journaling, since a lot of padding is eliminated.
**  Hence, while POWERSAFE_OVERWRITE is on by default, there is a file-control
** available to turn it off and URI query parameter available to turn it off.
*/
static int unixDeviceCharacteristics(sqlite3_file *id){
  unixFile *p = (unixFile*)id;
  int rc = 0;
#ifdef __QNXNTO__
  if( p->sectorSize==0 ) unixSectorSize(id);
  rc = p->deviceCharacteristics;
#endif
  if( p->ctrlFlags & UNIXFILE_PSOW ){
    rc |= SQLITE_IOCAP_POWERSAFE_OVERWRITE;
  }
  return rc;
}

#if !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0

/*
** Return the system page size.
**
** This function should not be called directly by other code in this file. 
** Instead, it should be called via macro osGetpagesize().
*/
static int unixGetpagesize(void){
#if OS_VXWORKS
  return 1024;
#elif defined(_BSD_SOURCE)
  return getpagesize();
#else
  return (int)sysconf(_SC_PAGESIZE);
#endif
}

#endif /* !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0 */

#ifndef SQLITE_OMIT_WAL

/*
** Object used to represent an shared memory buffer.  
**
** When multiple threads all reference the same wal-index, each thread
** has its own unixShm object, but they all point to a single instance
** of this unixShmNode object.  In other words, each wal-index is opened
** only once per process.
**
** Each unixShmNode object is connected to a single unixInodeInfo object.
** We could coalesce this object into unixInodeInfo, but that would mean
** every open file that does not use shared memory (in other words, most
** open files) would have to carry around this extra information.  So
** the unixInodeInfo object contains a pointer to this unixShmNode object
** and the unixShmNode object is created only when needed.
**
** unixMutexHeld() must be true when creating or destroying
** this object or while reading or writing the following fields:
**
**      nRef
**
** The following fields are read-only after the object is created:
** 
**      fid
**      zFilename
**
** Either unixShmNode.mutex must be held or unixShmNode.nRef==0 and
** unixMutexHeld() is true when reading or writing any other field
** in this structure.
*/
struct unixShmNode {
  unixInodeInfo *pInode;     /* unixInodeInfo that owns this SHM node */
  sqlite3_mutex *mutex;      /* Mutex to access this object */
  char *zFilename;           /* Name of the mmapped file */
  int h;                     /* Open file descriptor */
  int szRegion;              /* Size of shared-memory regions */
  u16 nRegion;               /* Size of array apRegion */
  u8 isReadonly;             /* True if read-only */
  char **apRegion;           /* Array of mapped shared-memory regions */
  int nRef;                  /* Number of unixShm objects pointing to this */
  unixShm *pFirst;           /* All unixShm objects pointing to this */
#ifdef SQLITE_DEBUG
  u8 exclMask;               /* Mask of exclusive locks held */
  u8 sharedMask;             /* Mask of shared locks held */
  u8 nextShmId;              /* Next available unixShm.id value */
#endif
};

/*
** Structure used internally by this VFS to record the state of an
** open shared memory connection.
**
** The following fields are initialized when this object is created and
** are read-only thereafter:
**
**    unixShm.pFile
**    unixShm.id
**
** All other fields are read/write.  The unixShm.pFile->mutex must be held
** while accessing any read/write fields.
*/
struct unixShm {
  unixShmNode *pShmNode;     /* The underlying unixShmNode object */
  unixShm *pNext;            /* Next unixShm with the same unixShmNode */
  u8 hasMutex;               /* True if holding the unixShmNode mutex */
  u8 id;                     /* Id of this connection within its unixShmNode */
  u16 sharedMask;            /* Mask of shared locks held */
  u16 exclMask;              /* Mask of exclusive locks held */
};

/*
** Constants used for locking
*/
#define UNIX_SHM_BASE   ((22+SQLITE_SHM_NLOCK)*4)         /* first lock byte */
#define UNIX_SHM_DMS    (UNIX_SHM_BASE+SQLITE_SHM_NLOCK)  /* deadman switch */

/*
** Apply posix advisory locks for all bytes from ofst through ofst+n-1.
**
** Locks block if the mask is exactly UNIX_SHM_C and are non-blocking
** otherwise.
*/
static int unixShmSystemLock(
  unixFile *pFile,       /* Open connection to the WAL file */
  int lockType,          /* F_UNLCK, F_RDLCK, or F_WRLCK */
  int ofst,              /* First byte of the locking range */
  int n                  /* Number of bytes to lock */
){
  unixShmNode *pShmNode; /* Apply locks to this open shared-memory segment */
  struct flock f;        /* The posix advisory locking structure */
  int rc = SQLITE_OK;    /* Result code form fcntl() */

  /* Access to the unixShmNode object is serialized by the caller */
  pShmNode = pFile->pInode->pShmNode;
  assert( sqlite3_mutex_held(pShmNode->mutex) || pShmNode->nRef==0 );

  /* Shared locks never span more than one byte */
  assert( n==1 || lockType!=F_RDLCK );

  /* Locks are within range */
  assert( n>=1 && n<SQLITE_SHM_NLOCK );

  if( pShmNode->h>=0 ){
    int lkType;
    /* Initialize the locking parameters */
    memset(&f, 0, sizeof(f));
    f.l_type = lockType;
    f.l_whence = SEEK_SET;
    f.l_start = ofst;
    f.l_len = n;

    lkType = (pFile->ctrlFlags & UNIXFILE_BLOCK)!=0 ? F_SETLKW : F_SETLK;
    rc = osFcntl(pShmNode->h, lkType, &f);
    rc = (rc!=(-1)) ? SQLITE_OK : SQLITE_BUSY;
    pFile->ctrlFlags &= ~UNIXFILE_BLOCK;
  }

  /* Update the global lock state and do debug tracing */
#ifdef SQLITE_DEBUG
  { u16 mask;
  OSTRACE(("SHM-LOCK "));
  mask = ofst>31 ? 0xffff : (1<<(ofst+n)) - (1<<ofst);
  if( rc==SQLITE_OK ){
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask &= ~mask;
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask |= mask;
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d ok", ofst));
      pShmNode->exclMask |= mask;
      pShmNode->sharedMask &= ~mask;
    }
  }else{
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d failed", ofst));
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock failed"));
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d failed", ofst));
    }
  }
  OSTRACE((" - afterwards %03x,%03x\n",
           pShmNode->sharedMask, pShmNode->exclMask));
  }
#endif

  return rc;        
}

/*
** Return the minimum number of 32KB shm regions that should be mapped at
** a time, assuming that each mapping must be an integer multiple of the
** current system page-size.
**
** Usually, this is 1. The exception seems to be systems that are configured
** to use 64KB pages - in this case each mapping must cover at least two
** shm regions.
*/
static int unixShmRegionPerMap(void){
  int shmsz = 32*1024;            /* SHM region size */
  int pgsz = osGetpagesize();   /* System page size */
  assert( ((pgsz-1)&pgsz)==0 );   /* Page size must be a power of 2 */
  if( pgsz<shmsz ) return 1;
  return pgsz/shmsz;
}

/*
** Purge the unixShmNodeList list of all entries with unixShmNode.nRef==0.
**
** This is not a VFS shared-memory method; it is a utility function called
** by VFS shared-memory methods.
*/
static void unixShmPurge(unixFile *pFd){
  unixShmNode *p = pFd->pInode->pShmNode;
  assert( unixMutexHeld() );
  if( p && p->nRef==0 ){
    int nShmPerMap = unixShmRegionPerMap();
    int i;
    assert( p->pInode==pFd->pInode );
    sqlite3_mutex_free(p->mutex);
    for(i=0; i<p->nRegion; i+=nShmPerMap){
      if( p->h>=0 ){
        osMunmap(p->apRegion[i], p->szRegion);
      }else{
        sqlite3_free(p->apRegion[i]);
      }
    }
    sqlite3_free(p->apRegion);
    if( p->h>=0 ){
      robust_close(pFd, p->h, __LINE__);
      p->h = -1;
    }
    p->pInode->pShmNode = 0;
    sqlite3_free(p);
  }
}

/*
** Open a shared-memory area associated with open database file pDbFd.  
** This particular implementation uses mmapped files.
**
** The file used to implement shared-memory is in the same directory
** as the open database file and has the same name as the open database
** file with the "-shm" suffix added.  For example, if the database file
** is "/home/user1/config.db" then the file that is created and mmapped
** for shared memory will be called "/home/user1/config.db-shm".  
**
** Another approach to is to use files in /dev/shm or /dev/tmp or an
** some other tmpfs mount. But if a file in a different directory
** from the database file is used, then differing access permissions
** or a chroot() might cause two different processes on the same
** database to end up using different files for shared memory - 
** meaning that their memory would not really be shared - resulting
** in database corruption.  Nevertheless, this tmpfs file usage
** can be enabled at compile-time using -DSQLITE_SHM_DIRECTORY="/dev/shm"
** or the equivalent.  The use of the SQLITE_SHM_DIRECTORY compile-time
** option results in an incompatible build of SQLite;  builds of SQLite
** that with differing SQLITE_SHM_DIRECTORY settings attempt to use the
** same database file at the same time, database corruption will likely
** result. The SQLITE_SHM_DIRECTORY compile-time option is considered
** "unsupported" and may go away in a future SQLite release.
**
** When opening a new shared-memory file, if no other instances of that
** file are currently open, in this process or in other processes, then
** the file must be truncated to zero length or have its header cleared.
**
** If the original database file (pDbFd) is using the "unix-excl" VFS
** that means that an exclusive lock is held on the database file and
** that no other processes are able to read or write the database.  In
** that case, we do not really need shared memory.  No shared memory
** file is created.  The shared memory will be simulated with heap memory.
*/
static int unixOpenSharedMemory(unixFile *pDbFd){
  struct unixShm *p = 0;          /* The connection to be opened */
  struct unixShmNode *pShmNode;   /* The underlying mmapped file */
  int rc;                         /* Result code */
  unixInodeInfo *pInode;          /* The inode of fd */
  char *zShmFilename;             /* Name of the file used for SHM */
  int nShmFilename;               /* Size of the SHM filename in bytes */

  /* Allocate space for the new unixShm object. */
  p = sqlite3_malloc64( sizeof(*p) );
  if( p==0 ) return SQLITE_NOMEM;
  memset(p, 0, sizeof(*p));
  assert( pDbFd->pShm==0 );

  /* Check to see if a unixShmNode object already exists. Reuse an existing
  ** one if present. Create a new one if necessary.
  */
  unixEnterMutex();
  pInode = pDbFd->pInode;
  pShmNode = pInode->pShmNode;
  if( pShmNode==0 ){
    struct stat sStat;                 /* fstat() info for database file */
#ifndef SQLITE_SHM_DIRECTORY
    const char *zBasePath = pDbFd->zPath;
#endif

    /* Call fstat() to figure out the permissions on the database file. If
    ** a new *-shm file is created, an attempt will be made to create it
    ** with the same permissions.
    */
    if( osFstat(pDbFd->h, &sStat) && pInode->bProcessLock==0 ){
      rc = SQLITE_IOERR_FSTAT;
      goto shm_open_err;
    }

#ifdef SQLITE_SHM_DIRECTORY
    nShmFilename = sizeof(SQLITE_SHM_DIRECTORY) + 31;
#else
    nShmFilename = 6 + (int)strlen(zBasePath);
#endif
    pShmNode = sqlite3_malloc64( sizeof(*pShmNode) + nShmFilename );
    if( pShmNode==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }
    memset(pShmNode, 0, sizeof(*pShmNode)+nShmFilename);
    zShmFilename = pShmNode->zFilename = (char*)&pShmNode[1];
#ifdef SQLITE_SHM_DIRECTORY
    sqlite3_snprintf(nShmFilename, zShmFilename, 
                     SQLITE_SHM_DIRECTORY "/sqlite-shm-%x-%x",
                     (u32)sStat.st_ino, (u32)sStat.st_dev);
#else
    sqlite3_snprintf(nShmFilename, zShmFilename, "%s-shm", zBasePath);
    sqlite3FileSuffix3(pDbFd->zPath, zShmFilename);
#endif
    pShmNode->h = -1;
    pDbFd->pInode->pShmNode = pShmNode;
    pShmNode->pInode = pDbFd->pInode;
    pShmNode->mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
    if( pShmNode->mutex==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }

    if( pInode->bProcessLock==0 ){
      int openFlags = O_RDWR | O_CREAT;
      if( sqlite3_uri_boolean(pDbFd->zPath, "readonly_shm", 0) ){
        openFlags = O_RDONLY;
        pShmNode->isReadonly = 1;
      }
      pShmNode->h = robust_open(zShmFilename, openFlags, (sStat.st_mode&0777));
      if( pShmNode->h<0 ){
        rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zShmFilename);
        goto shm_open_err;
      }

      /* If this process is running as root, make sure that the SHM file
      ** is owned by the same user that owns the original database.  Otherwise,
      ** the original owner will not be able to connect.
      */
      osFchown(pShmNode->h, sStat.st_uid, sStat.st_gid);
  
      /* Check to see if another process is holding the dead-man switch.
      ** If not, truncate the file to zero length. 
      */
      rc = SQLITE_OK;
      if( unixShmSystemLock(pDbFd, F_WRLCK, UNIX_SHM_DMS, 1)==SQLITE_OK ){
        if( robust_ftruncate(pShmNode->h, 0) ){
          rc = unixLogError(SQLITE_IOERR_SHMOPEN, "ftruncate", zShmFilename);
        }
      }
      if( rc==SQLITE_OK ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, UNIX_SHM_DMS, 1);
      }
      if( rc ) goto shm_open_err;
    }
  }

  /* Make the new connection a child of the unixShmNode */
  p->pShmNode = pShmNode;
#ifdef SQLITE_DEBUG
  p->id = pShmNode->nextShmId++;
#endif
  pShmNode->nRef++;
  pDbFd->pShm = p;
  unixLeaveMutex();

  /* The reference count on pShmNode has already been incremented under
  ** the cover of the unixEnterMutex() mutex and the pointer from the
  ** new (struct unixShm) object to the pShmNode has been set. All that is
  ** left to do is to link the new object into the linked list starting
  ** at pShmNode->pFirst. This must be done while holding the pShmNode->mutex 
  ** mutex.
  */
  sqlite3_mutex_enter(pShmNode->mutex);
  p->pNext = pShmNode->pFirst;
  pShmNode->pFirst = p;
  sqlite3_mutex_leave(pShmNode->mutex);
  return SQLITE_OK;

  /* Jump here on any error */
shm_open_err:
  unixShmPurge(pDbFd);       /* This call frees pShmNode if required */
  sqlite3_free(p);
  unixLeaveMutex();
  return rc;
}

/*
** This function is called to obtain a pointer to region iRegion of the 
** shared-memory associated with the database file fd. Shared-memory regions 
** are numbered starting from zero. Each shared-memory region is szRegion 
** bytes in size.
**
** If an error occurs, an error code is returned and *pp is set to NULL.
**
** Otherwise, if the bExtend parameter is 0 and the requested shared-memory
** region has not been allocated (by any client, including one running in a
** separate process), then *pp is set to NULL and SQLITE_OK returned. If 
** bExtend is non-zero and the requested shared-memory region has not yet 
** been allocated, it is allocated by this function.
**
** If the shared-memory region has already been allocated or is allocated by
** this call as described above, then it is mapped into this processes 
** address space (if it is not already), *pp is set to point to the mapped 
** memory and SQLITE_OK returned.
*/
static int unixShmMap(
  sqlite3_file *fd,               /* Handle open on database file */
  int iRegion,                    /* Region to retrieve */
  int szRegion,                   /* Size of regions */
  int bExtend,                    /* True to extend file if necessary */
  void volatile **pp              /* OUT: Mapped memory */
){
  unixFile *pDbFd = (unixFile*)fd;
  unixShm *p;
  unixShmNode *pShmNode;
  int rc = SQLITE_OK;
  int nShmPerMap = unixShmRegionPerMap();
  int nReqRegion;

  /* If the shared-memory file has not yet been opened, open it now. */
  if( pDbFd->pShm==0 ){
    rc = unixOpenSharedMemory(pDbFd);
    if( rc!=SQLITE_OK ) return rc;
  }

  p = pDbFd->pShm;
  pShmNode = p->pShmNode;
  sqlite3_mutex_enter(pShmNode->mutex);
  assert( szRegion==pShmNode->szRegion || pShmNode->nRegion==0 );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  /* Minimum number of regions required to be mapped. */
  nReqRegion = ((iRegion+nShmPerMap) / nShmPerMap) * nShmPerMap;

  if( pShmNode->nRegion<nReqRegion ){
    char **apNew;                      /* New apRegion[] array */
    int nByte = nReqRegion*szRegion;   /* Minimum required file size */
    struct stat sStat;                 /* Used by fstat() */

    pShmNode->szRegion = szRegion;

    if( pShmNode->h>=0 ){
      /* The requested region is not mapped into this processes address space.
      ** Check to see if it has been allocated (i.e. if the wal-index file is
      ** large enough to contain the requested region).
      */
      if( osFstat(pShmNode->h, &sStat) ){
        rc = SQLITE_IOERR_SHMSIZE;
        goto shmpage_out;
      }
  
      if( sStat.st_size<nByte ){
        /* The requested memory region does not exist. If bExtend is set to
        ** false, exit early. *pp will be set to NULL and SQLITE_OK returned.
        */
        if( !bExtend ){
          goto shmpage_out;
        }

        /* Alternatively, if bExtend is true, extend the file. Do this by
        ** writing a single byte to the end of each (OS) page being
        ** allocated or extended. Technically, we need only write to the
        ** last page in order to extend the file. But writing to all new
        ** pages forces the OS to allocate them immediately, which reduces
        ** the chances of SIGBUS while accessing the mapped region later on.
        */
        else{
          static const int pgsz = 4096;
          int iPg;

          /* Write to the last byte of each newly allocated or extended page */
          assert( (nByte % pgsz)==0 );
          for(iPg=(sStat.st_size/pgsz); iPg<(nByte/pgsz); iPg++){
            if( seekAndWriteFd(pShmNode->h, iPg*pgsz + pgsz-1, "", 1, 0)!=1 ){
              const char *zFile = pShmNode->zFilename;
              rc = unixLogError(SQLITE_IOERR_SHMSIZE, "write", zFile);
              goto shmpage_out;
            }
          }
        }
      }
    }

    /* Map the requested memory region into this processes address space. */
    apNew = (char **)sqlite3_realloc(
        pShmNode->apRegion, nReqRegion*sizeof(char *)
    );
    if( !apNew ){
      rc = SQLITE_IOERR_NOMEM;
      goto shmpage_out;
    }
    pShmNode->apRegion = apNew;
    while( pShmNode->nRegion<nReqRegion ){
      int nMap = szRegion*nShmPerMap;
      int i;
      void *pMem;
      if( pShmNode->h>=0 ){
        pMem = osMmap(0, nMap,
            pShmNode->isReadonly ? PROT_READ : PROT_READ|PROT_WRITE, 
            MAP_SHARED, pShmNode->h, szRegion*(i64)pShmNode->nRegion
        );
        if( pMem==MAP_FAILED ){
          rc = unixLogError(SQLITE_IOERR_SHMMAP, "mmap", pShmNode->zFilename);
          goto shmpage_out;
        }
      }else{
        pMem = sqlite3_malloc64(szRegion);
        if( pMem==0 ){
          rc = SQLITE_NOMEM;
          goto shmpage_out;
        }
        memset(pMem, 0, szRegion);
      }

      for(i=0; i<nShmPerMap; i++){
        pShmNode->apRegion[pShmNode->nRegion+i] = &((char*)pMem)[szRegion*i];
      }
      pShmNode->nRegion += nShmPerMap;
    }
  }

shmpage_out:
  if( pShmNode->nRegion>iRegion ){
    *pp = pShmNode->apRegion[iRegion];
  }else{
    *pp = 0;
  }
  if( pShmNode->isReadonly && rc==SQLITE_OK ) rc = SQLITE_READONLY;
  sqlite3_mutex_leave(pShmNode->mutex);
  return rc;
}

/*
** Change the lock state for a shared-memory segment.
**
** Note that the relationship between SHAREd and EXCLUSIVE locks is a little
** different here than in posix.  In xShmLock(), one can go from unlocked
** to shared and back or from unlocked to exclusive and back.  But one may
** not go from shared to exclusive or from exclusive to shared.
*/
static int unixShmLock(
  sqlite3_file *fd,          /* Database file holding the shared memory */
  int ofst,                  /* First lock to acquire or release */
  int n,                     /* Number of locks to acquire or release */
  int flags                  /* What to do with the lock */
){
  unixFile *pDbFd = (unixFile*)fd;      /* Connection holding shared memory */
  unixShm *p = pDbFd->pShm;             /* The shared memory being locked */
  unixShm *pX;                          /* For looping over all siblings */
  unixShmNode *pShmNode = p->pShmNode;  /* The underlying file iNode */
  int rc = SQLITE_OK;                   /* Result code */
  u16 mask;                             /* Mask of locks to take or release */

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( ofst>=0 && ofst+n<=SQLITE_SHM_NLOCK );
  assert( n>=1 );
  assert( flags==(SQLITE_SHM_LOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_LOCK | SQLITE_SHM_EXCLUSIVE)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_EXCLUSIVE) );
  assert( n==1 || (flags & SQLITE_SHM_EXCLUSIVE)!=0 );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  mask = (1<<(ofst+n)) - (1<<ofst);
  assert( n>1 || mask==(1<<ofst) );
  sqlite3_mutex_enter(pShmNode->mutex);
  if( flags & SQLITE_SHM_UNLOCK ){
    u16 allMask = 0; /* Mask of locks held by siblings */

    /* See if any siblings hold this same lock */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( pX==p ) continue;
      assert( (pX->exclMask & (p->exclMask|p->sharedMask))==0 );
      allMask |= pX->sharedMask;
    }

    /* Unlock the system-level locks */
    if( (mask & allMask)==0 ){
      rc = unixShmSystemLock(pDbFd, F_UNLCK, ofst+UNIX_SHM_BASE, n);
    }else{
      rc = SQLITE_OK;
    }

    /* Undo the local locks */
    if( rc==SQLITE_OK ){
      p->exclMask &= ~mask;
      p->sharedMask &= ~mask;
    } 
  }else if( flags & SQLITE_SHM_SHARED ){
    u16 allShared = 0;  /* Union of locks held by connections other than "p" */

    /* Find out which shared locks are already held by sibling connections.
    ** If any sibling already holds an exclusive lock, go ahead and return
    ** SQLITE_BUSY.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
      allShared |= pX->sharedMask;
    }

    /* Get shared locks at the system level, if necessary */
    if( rc==SQLITE_OK ){
      if( (allShared & mask)==0 ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, ofst+UNIX_SHM_BASE, n);
      }else{
        rc = SQLITE_OK;
      }
    }

    /* Get the local shared locks */
    if( rc==SQLITE_OK ){
      p->sharedMask |= mask;
    }
  }else{
    /* Make sure no sibling connections hold locks that will block this
    ** lock.  If any do, return SQLITE_BUSY right away.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 || (pX->sharedMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
    }
  
    /* Get the exclusive locks at the system level.  Then if successful
    ** also mark the local connection as being locked.
    */
    if( rc==SQLITE_OK ){
      rc = unixShmSystemLock(pDbFd, F_WRLCK, ofst+UNIX_SHM_BASE, n);
      if( rc==SQLITE_OK ){
        assert( (p->sharedMask & mask)==0 );
        p->exclMask |= mask;
      }
    }
  }
  sqlite3_mutex_leave(pShmNode->mutex);
  OSTRACE(("SHM-LOCK shmid-%d, pid-%d got %03x,%03x\n",
           p->id, osGetpid(0), p->sharedMask, p->exclMask));
  return rc;
}

/*
** Implement a memory barrier or memory fence on shared memory.  
**
** All loads and stores begun before the barrier must complete before
** any load or store begun after the barrier.
*/
static void unixShmBarrier(
  sqlite3_file *fd                /* Database file holding the shared memory */
){
  UNUSED_PARAMETER(fd);
  sqlite3MemoryBarrier();         /* compiler-defined memory barrier */
  unixEnterMutex();               /* Also mutex, for redundancy */
  unixLeaveMutex();
}

/*
** Close a connection to shared-memory.  Delete the underlying 
** storage if deleteFlag is true.
**
** If there is no shared memory associated with the connection then this
** routine is a harmless no-op.
*/
static int unixShmUnmap(
  sqlite3_file *fd,               /* The underlying database file */
  int deleteFlag                  /* Delete shared-memory if true */
){
  unixShm *p;                     /* The connection to be closed */
  unixShmNode *pShmNode;          /* The underlying shared-memory file */
  unixShm **pp;                   /* For looping over sibling connections */
  unixFile *pDbFd;                /* The underlying database file */

  pDbFd = (unixFile*)fd;
  p = pDbFd->pShm;
  if( p==0 ) return SQLITE_OK;
  pShmNode = p->pShmNode;

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );

  /* Remove connection p from the set of connections associated
  ** with pShmNode */
  sqlite3_mutex_enter(pShmNode->mutex);
  for(pp=&pShmNode->pFirst; (*pp)!=p; pp = &(*pp)->pNext){}
  *pp = p->pNext;

  /* Free the connection p */
  sqlite3_free(p);
  pDbFd->pShm = 0;
  sqlite3_mutex_leave(pShmNode->mutex);

  /* If pShmNode->nRef has reached 0, then close the underlying
  ** shared-memory file, too */
  unixEnterMutex();
  assert( pShmNode->nRef>0 );
  pShmNode->nRef--;
  if( pShmNode->nRef==0 ){
    if( deleteFlag && pShmNode->h>=0 ){
      osUnlink(pShmNode->zFilename);
    }
    unixShmPurge(pDbFd);
  }
  unixLeaveMutex();

  return SQLITE_OK;
}


#else
# define unixShmMap     0
# define unixShmLock    0
# define unixShmBarrier 0
# define unixShmUnmap   0
#endif /* #ifndef SQLITE_OMIT_WAL */

#if SQLITE_MAX_MMAP_SIZE>0
/*
** If it is currently memory mapped, unmap file pFd.
*/
static void unixUnmapfile(unixFile *pFd){
  assert( pFd->nFetchOut==0 );
  if( pFd->pMapRegion ){
    osMunmap(pFd->pMapRegion, pFd->mmapSizeActual);
    pFd->pMapRegion = 0;
    pFd->mmapSize = 0;
    pFd->mmapSizeActual = 0;
  }
}

/*
** Attempt to set the size of the memory mapping maintained by file 
** descriptor pFd to nNew bytes. Any existing mapping is discarded.
**
** If successful, this function sets the following variables:
**
**       unixFile.pMapRegion
**       unixFile.mmapSize
**       unixFile.mmapSizeActual
**
** If unsuccessful, an error message is logged via sqlite3_log() and
** the three variables above are zeroed. In this case SQLite should
** continue accessing the database using the xRead() and xWrite()
** methods.
*/
static void unixRemapfile(
  unixFile *pFd,                  /* File descriptor object */
  i64 nNew                        /* Required mapping size */
){
  const char *zErr = "mmap";
  int h = pFd->h;                      /* File descriptor open on db file */
  u8 *pOrig = (u8 *)pFd->pMapRegion;   /* Pointer to current file mapping */
  i64 nOrig = pFd->mmapSizeActual;     /* Size of pOrig region in bytes */
  u8 *pNew = 0;                        /* Location of new mapping */
  int flags = PROT_READ;               /* Flags to pass to mmap() */

  assert( pFd->nFetchOut==0 );
  assert( nNew>pFd->mmapSize );
  assert( nNew<=pFd->mmapSizeMax );
  assert( nNew>0 );
  assert( pFd->mmapSizeActual>=pFd->mmapSize );
  assert( MAP_FAILED!=0 );

  if( (pFd->ctrlFlags & UNIXFILE_RDONLY)==0 ) flags |= PROT_WRITE;

  if( pOrig ){
#if HAVE_MREMAP
    i64 nReuse = pFd->mmapSize;
#else
    const int szSyspage = osGetpagesize();
    i64 nReuse = (pFd->mmapSize & ~(szSyspage-1));
#endif
    u8 *pReq = &pOrig[nReuse];

    /* Unmap any pages of the existing mapping that cannot be reused. */
    if( nReuse!=nOrig ){
      osMunmap(pReq, nOrig-nReuse);
    }

#if HAVE_MREMAP
    pNew = osMremap(pOrig, nReuse, nNew, MREMAP_MAYMOVE);
    zErr = "mremap";
#else
    pNew = osMmap(pReq, nNew-nReuse, flags, MAP_SHARED, h, nReuse);
    if( pNew!=MAP_FAILED ){
      if( pNew!=pReq ){
        osMunmap(pNew, nNew - nReuse);
        pNew = 0;
      }else{
        pNew = pOrig;
      }
    }
#endif

    /* The attempt to extend the existing mapping failed. Free it. */
    if( pNew==MAP_FAILED || pNew==0 ){
      osMunmap(pOrig, nReuse);
    }
  }

  /* If pNew is still NULL, try to create an entirely new mapping. */
  if( pNew==0 ){
    pNew = osMmap(0, nNew, flags, MAP_SHARED, h, 0);
  }

  if( pNew==MAP_FAILED ){
    pNew = 0;
    nNew = 0;
    unixLogError(SQLITE_OK, zErr, pFd->zPath);

    /* If the mmap() above failed, assume that all subsequent mmap() calls
    ** will probably fail too. Fall back to using xRead/xWrite exclusively
    ** in this case.  */
    pFd->mmapSizeMax = 0;
  }
  pFd->pMapRegion = (void *)pNew;
  pFd->mmapSize = pFd->mmapSizeActual = nNew;
}

/*
** Memory map or remap the file opened by file-descriptor pFd (if the file
** is already mapped, the existing mapping is replaced by the new). Or, if 
** there already exists a mapping for this file, and there are still 
** outstanding xFetch() references to it, this function is a no-op.
**
** If parameter nByte is non-negative, then it is the requested size of 
** the mapping to create. Otherwise, if nByte is less than zero, then the 
** requested size is the size of the file on disk. The actual size of the
** created mapping is either the requested size or the value configured 
** using SQLITE_FCNTL_MMAP_LIMIT, whichever is smaller.
**
** SQLITE_OK is returned if no error occurs (even if the mapping is not
** recreated as a result of outstanding references) or an SQLite error
** code otherwise.
*/
static int unixMapfile(unixFile *pFd, i64 nByte){
  i64 nMap = nByte;
  int rc;

  assert( nMap>=0 || pFd->nFetchOut==0 );
  if( pFd->nFetchOut>0 ) return SQLITE_OK;

  if( nMap<0 ){
    struct stat statbuf;          /* Low-level file information */
    rc = osFstat(pFd->h, &statbuf);
    if( rc!=SQLITE_OK ){
      return SQLITE_IOERR_FSTAT;
    }
    nMap = statbuf.st_size;
  }
  if( nMap>pFd->mmapSizeMax ){
    nMap = pFd->mmapSizeMax;
  }

  if( nMap!=pFd->mmapSize ){
    if( nMap>0 ){
      unixRemapfile(pFd, nMap);
    }else{
      unixUnmapfile(pFd);
    }
  }

  return SQLITE_OK;
}
#endif /* SQLITE_MAX_MMAP_SIZE>0 */

/*
** If possible, return a pointer to a mapping of file fd starting at offset
** iOff. The mapping must be valid for at least nAmt bytes.
**
** If such a pointer can be obtained, store it in *pp and return SQLITE_OK.
** Or, if one cannot but no error occurs, set *pp to 0 and return SQLITE_OK.
** Finally, if an error does occur, return an SQLite error code. The final
** value of *pp is undefined in this case.
**
** If this function does return a pointer, the caller must eventually 
** release the reference by calling unixUnfetch().
*/
static int unixFetch(sqlite3_file *fd, i64 iOff, int nAmt, void **pp){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
#endif
  *pp = 0;

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFd->mmapSizeMax>0 ){
    if( pFd->pMapRegion==0 ){
      int rc = unixMapfile(pFd, -1);
      if( rc!=SQLITE_OK ) return rc;
    }
    if( pFd->mmapSize >= iOff+nAmt ){
      *pp = &((u8 *)pFd->pMapRegion)[iOff];
      pFd->nFetchOut++;
    }
  }
#endif
  return SQLITE_OK;
}

/*
** If the third argument is non-NULL, then this function releases a 
** reference obtained by an earlier call to unixFetch(). The second
** argument passed to this function must be the same as the corresponding
** argument that was passed to the unixFetch() invocation. 
**
** Or, if the third argument is NULL, then this function is being called 
** to inform the VFS layer that, according to POSIX, any existing mapping 
** may now be invalid and should be unmapped.
*/
static int unixUnfetch(sqlite3_file *fd, i64 iOff, void *p){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
  UNUSED_PARAMETER(iOff);

  /* If p==0 (unmap the entire file) then there must be no outstanding 
  ** xFetch references. Or, if p!=0 (meaning it is an xFetch reference),
  ** then there must be at least one outstanding.  */
  assert( (p==0)==(pFd->nFetchOut==0) );

  /* If p!=0, it must match the iOff value. */
  assert( p==0 || p==&((u8 *)pFd->pMapRegion)[iOff] );

  if( p ){
    pFd->nFetchOut--;
  }else{
    unixUnmapfile(pFd);
  }

  assert( pFd->nFetchOut>=0 );
#else
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(p);
  UNUSED_PARAMETER(iOff);
#endif
  return SQLITE_OK;
}

/*
** Here ends the implementation of all sqlite3_file methods.
**
********************** End sqlite3_file Methods *******************************
******************************************************************************/

/*
** This division contains definitions of sqlite3_io_methods objects that
** implement various file locking strategies.  It also contains definitions
** of "finder" functions.  A finder-function is used to locate the appropriate
** sqlite3_io_methods object for a particular database file.  The pAppData
** field of the sqlite3_vfs VFS objects are initialized to be pointers to
** the correct finder-function for that VFS.
**
** Most finder functions return a pointer to a fixed sqlite3_io_methods
** object.  The only interesting finder-function is autolockIoFinder, which
** looks at the filesystem type and tries to guess the best locking
** strategy from that.
**
** For finder-function F, two objects are created:
**
**    (1) The real finder-function named "FImpt()".
**
**    (2) A constant pointer to this function named just "F".
**
**
** A pointer to the F pointer is used as the pAppData value for VFS
** objects.  We have to do this instead of letting pAppData point
** directly at the finder-function since C90 rules prevent a void*
** from be cast into a function pointer.
**
**
** Each instance of this macro generates two objects:
**
**   *  A constant sqlite3_io_methods object call METHOD that has locking
**      methods CLOSE, LOCK, UNLOCK, CKRESLOCK.
**
**   *  An I/O method finder function called FINDER that returns a pointer
**      to the METHOD object in the previous bullet.
*/
#define IOMETHODS(FINDER,METHOD,VERSION,CLOSE,LOCK,UNLOCK,CKLOCK,SHMMAP)     \
static const sqlite3_io_methods METHOD = {                                   \
   VERSION,                    /* iVersion */                                \
   CLOSE,                      /* xClose */                                  \
   unixRead,                   /* xRead */                                   \
   unixWrite,                  /* xWrite */                                  \
   unixTruncate,               /* xTruncate */                               \
   unixSync,                   /* xSync */                                   \
   unixFileSize,               /* xFileSize */                               \
   LOCK,                       /* xLock */                                   \
   UNLOCK,                     /* xUnlock */                                 \
   CKLOCK,                     /* xCheckReservedLock */                      \
   unixFileControl,            /* xFileControl */                            \
   unixSectorSize,             /* xSectorSize */                             \
   unixDeviceCharacteristics,  /* xDeviceCapabilities */                     \
   SHMMAP,                     /* xShmMap */                                 \
   unixShmLock,                /* xShmLock */                                \
   unixShmBarrier,             /* xShmBarrier */                             \
   unixShmUnmap,               /* xShmUnmap */                               \
   unixFetch,                  /* xFetch */                                  \
   unixUnfetch,                /* xUnfetch */                                \
};                                                                           \
static const sqlite3_io_methods *FINDER##Impl(const char *z, unixFile *p){   \
  UNUSED_PARAMETER(z); UNUSED_PARAMETER(p);                                  \
  return &METHOD;                                                            \
}                                                                            \
static const sqlite3_io_methods *(*const FINDER)(const char*,unixFile *p)    \
    = FINDER##Impl;

/*
** Here are all of the sqlite3_io_methods objects for each of the
** locking strategies.  Functions that return pointers to these methods
** are also created.
*/
IOMETHODS(
  posixIoFinder,            /* Finder function name */
  posixIoMethods,           /* sqlite3_io_methods object name */
  3,                        /* shared memory and mmap are enabled */
  unixClose,                /* xClose method */
  unixLock,                 /* xLock method */
  unixUnlock,               /* xUnlock method */
  unixCheckReservedLock,    /* xCheckReservedLock method */
  unixShmMap                /* xShmMap method */
)
IOMETHODS(
  nolockIoFinder,           /* Finder function name */
  nolockIoMethods,          /* sqlite3_io_methods object name */
  3,                        /* shared memory is disabled */
  nolockClose,              /* xClose method */
  nolockLock,               /* xLock method */
  nolockUnlock,             /* xUnlock method */
  nolockCheckReservedLock,  /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
IOMETHODS(
  dotlockIoFinder,          /* Finder function name */
  dotlockIoMethods,         /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  dotlockClose,             /* xClose method */
  dotlockLock,              /* xLock method */
  dotlockUnlock,            /* xUnlock method */
  dotlockCheckReservedLock, /* xCheckReservedLock method */
  0                         /* xShmMap method */
)

#if SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  flockIoFinder,            /* Finder function name */
  flockIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  flockClose,               /* xClose method */
  flockLock,                /* xLock method */
  flockUnlock,              /* xUnlock method */
  flockCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if OS_VXWORKS
IOMETHODS(
  semIoFinder,              /* Finder function name */
  semIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  semXClose,                /* xClose method */
  semXLock,                 /* xLock method */
  semXUnlock,               /* xUnlock method */
  semXCheckReservedLock,    /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  afpIoFinder,              /* Finder function name */
  afpIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  afpClose,                 /* xClose method */
  afpLock,                  /* xLock method */
  afpUnlock,                /* xUnlock method */
  afpCheckReservedLock,     /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/*
** The proxy locking method is a "super-method" in the sense that it
** opens secondary file descriptors for the conch and lock files and
** it uses proxy, dot-file, AFP, and flock() locking methods on those
** secondary files.  For this reason, the division that implements
** proxy locking is located much further down in the file.  But we need
** to go ahead and define the sqlite3_io_methods and finder function
** for proxy locking here.  So we forward declare the I/O methods.
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
static int proxyClose(sqlite3_file*);
static int proxyLock(sqlite3_file*, int);
static int proxyUnlock(sqlite3_file*, int);
static int proxyCheckReservedLock(sqlite3_file*, int*);
IOMETHODS(
  proxyIoFinder,            /* Finder function name */
  proxyIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  proxyClose,               /* xClose method */
  proxyLock,                /* xLock method */
  proxyUnlock,              /* xUnlock method */
  proxyCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/* nfs lockd on OSX 10.3+ doesn't clear write locks when a read lock is set */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  nfsIoFinder,               /* Finder function name */
  nfsIoMethods,              /* sqlite3_io_methods object name */
  1,                         /* shared memory is disabled */
  unixClose,                 /* xClose method */
  unixLock,                  /* xLock method */
  nfsUnlock,                 /* xUnlock method */
  unixCheckReservedLock,     /* xCheckReservedLock method */
  0                          /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/* 
** This "finder" function attempts to determine the best locking strategy 
** for the database file "filePath".  It then returns the sqlite3_io_methods
** object that implements that strategy.
**
** This is for MacOSX only.
*/
static const sqlite3_io_methods *autolockIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* open file object for the database file */
){
  static const struct Mapping {
    const char *zFilesystem;              /* Filesystem type name */
    const sqlite3_io_methods *pMethods;   /* Appropriate locking method */
  } aMap[] = {
    { "hfs",    &posixIoMethods },
    { "ufs",    &posixIoMethods },
    { "afpfs",  &afpIoMethods },
    { "smbfs",  &afpIoMethods },
    { "webdav", &nolockIoMethods },
    { 0, 0 }
  };
  int i;
  struct statfs fsInfo;
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }
  if( statfs(filePath, &fsInfo) != -1 ){
    if( fsInfo.f_flags & MNT_RDONLY ){
      return &nolockIoMethods;
    }
    for(i=0; aMap[i].zFilesystem; i++){
      if( strcmp(fsInfo.f_fstypename, aMap[i].zFilesystem)==0 ){
        return aMap[i].pMethods;
      }
    }
  }

  /* Default case. Handles, amongst others, "nfs".
  ** Test byte-range lock using fcntl(). If the call succeeds, 
  ** assume that the file-system supports POSIX style locks. 
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    if( strcmp(fsInfo.f_fstypename, "nfs")==0 ){
      return &nfsIoMethods;
    } else {
      return &posixIoMethods;
    }
  }else{
    return &dotlockIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const autolockIoFinder)(const char*,unixFile*) = autolockIoFinderImpl;

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */

#if OS_VXWORKS
/*
** This "finder" function for VxWorks checks to see if posix advisory
** locking works.  If it does, then that is what is used.  If it does not
** work, then fallback to named semaphore locking.
*/
static const sqlite3_io_methods *vxworksIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* the open file object */
){
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }

  /* Test if fcntl() is supported and use POSIX style locks.
  ** Otherwise fall back to the named semaphore method.
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    return &posixIoMethods;
  }else{
    return &semIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const vxworksIoFinder)(const char*,unixFile*) = vxworksIoFinderImpl;

#endif /* OS_VXWORKS */

/*
** An abstract type for a pointer to an IO method finder function:
*/
typedef const sqlite3_io_methods *(*finder_type)(const char*,unixFile*);


/****************************************************************************
**************************** sqlite3_vfs methods ****************************
**
** This division contains the implementation of methods on the
** sqlite3_vfs object.
*/

/*
** Initialize the contents of the unixFile structure pointed to by pId.
*/
static int fillInUnixFile(
  sqlite3_vfs *pVfs,      /* Pointer to vfs object */
  int h,                  /* Open file descriptor of file being opened */
  sqlite3_file *pId,      /* Write to the unixFile structure here */
  const char *zFilename,  /* Name of the file being opened */
  int ctrlFlags           /* Zero or more UNIXFILE_* values */
){
  const sqlite3_io_methods *pLockingStyle;
  unixFile *pNew = (unixFile *)pId;
  int rc = SQLITE_OK;

  assert( pNew->pInode==NULL );

  /* Usually the path zFilename should not be a relative pathname. The
  ** exception is when opening the proxy "conch" file in builds that
  ** include the special Apple locking styles.
  */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
  assert( zFilename==0 || zFilename[0]=='/' 
    || pVfs->pAppData==(void*)&autolockIoFinder );
#else
  assert( zFilename==0 || zFilename[0]=='/' );
#endif

  /* No locking occurs in temporary files */
  assert( zFilename!=0 || (ctrlFlags & UNIXFILE_NOLOCK)!=0 );

  OSTRACE(("OPEN    %-3d %s\n", h, zFilename));
  pNew->h = h;
  pNew->pVfs = pVfs;
  pNew->zPath = zFilename;
  pNew->ctrlFlags = (u8)ctrlFlags;
#if SQLITE_MAX_MMAP_SIZE>0
  pNew->mmapSizeMax = sqlite3GlobalConfig.szMmap;
#endif
  if( sqlite3_uri_boolean(((ctrlFlags & UNIXFILE_URI) ? zFilename : 0),
                           "psow", SQLITE_POWERSAFE_OVERWRITE) ){
    pNew->ctrlFlags |= UNIXFILE_PSOW;
  }
  if( strcmp(pVfs->zName,"unix-excl")==0 ){
    pNew->ctrlFlags |= UNIXFILE_EXCL;
  }

#if OS_VXWORKS
  pNew->pId = vxworksFindFileId(zFilename);
  if( pNew->pId==0 ){
    ctrlFlags |= UNIXFILE_NOLOCK;
    rc = SQLITE_NOMEM;
  }
#endif

  if( ctrlFlags & UNIXFILE_NOLOCK ){
    pLockingStyle = &nolockIoMethods;
  }else{
    pLockingStyle = (**(finder_type*)pVfs->pAppData)(zFilename, pNew);
#if SQLITE_ENABLE_LOCKING_STYLE
    /* Cache zFilename in the locking context (AFP and dotlock override) for
    ** proxyLock activation is possible (remote proxy is based on db name)
    ** zFilename remains valid until file is closed, to support */
    pNew->lockingContext = (void*)zFilename;
#endif
  }

  if( pLockingStyle == &posixIoMethods
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
    || pLockingStyle == &nfsIoMethods
#endif
  ){
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( rc!=SQLITE_OK ){
      /* If an error occurred in findInodeInfo(), close the file descriptor
      ** immediately, before releasing the mutex. findInodeInfo() may fail
      ** in two scenarios:
      **
      **   (a) A call to fstat() failed.
      **   (b) A malloc failed.
      **
      ** Scenario (b) may only occur if the process is holding no other
      ** file descriptors open on the same file. If there were other file
      ** descriptors on this file, then no malloc would be required by
      ** findInodeInfo(). If this is the case, it is quite safe to close
      ** handle h - as it is guaranteed that no posix locks will be released
      ** by doing so.
      **
      ** If scenario (a) caused the error then things are not so safe. The
      ** implicit assumption here is that if fstat() fails, things are in
      ** such bad shape that dropping a lock or two doesn't matter much.
      */
      robust_close(pNew, h, __LINE__);
      h = -1;
    }
    unixLeaveMutex();
  }

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
  else if( pLockingStyle == &afpIoMethods ){
    /* AFP locking uses the file path so it needs to be included in
    ** the afpLockingContext.
    */
    afpLockingContext *pCtx;
    pNew->lockingContext = pCtx = sqlite3_malloc64( sizeof(*pCtx) );
    if( pCtx==0 ){
      rc = SQLITE_NOMEM;
    }else{
      /* NB: zFilename exists and remains valid until the file is closed
      ** according to requirement F11141.  So we do not need to make a
      ** copy of the filename. */
      pCtx->dbPath = zFilename;
      pCtx->reserved = 0;
      srandomdev();
      unixEnterMutex();
      rc = findInodeInfo(pNew, &pNew->pInode);
      if( rc!=SQLITE_OK ){
        sqlite3_free(pNew->lockingContext);
        robust_close(pNew, h, __LINE__);
        h = -1;
      }
      unixLeaveMutex();        
    }
  }
#endif

  else if( pLockingStyle == &dotlockIoMethods ){
    /* Dotfile locking uses the file path so it needs to be included in
    ** the dotlockLockingContext 
    */
    char *zLockFile;
    int nFilename;
    assert( zFilename!=0 );
    nFilename = (int)strlen(zFilename) + 6;
    zLockFile = (char *)sqlite3_malloc64(nFilename);
    if( zLockFile==0 ){
      rc = SQLITE_NOMEM;
    }else{
      sqlite3_snprintf(nFilename, zLockFile, "%s" DOTLOCK_SUFFIX, zFilename);
    }
    pNew->lockingContext = zLockFile;
  }

#if OS_VXWORKS
  else if( pLockingStyle == &semIoMethods ){
    /* Named semaphore locking uses the file path so it needs to be
    ** included in the semLockingContext
    */
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( (rc==SQLITE_OK) && (pNew->pInode->pSem==NULL) ){
      char *zSemName = pNew->pInode->aSemName;
      int n;
      sqlite3_snprintf(MAX_PATHNAME, zSemName, "/%s.sem",
                       pNew->pId->zCanonicalName);
      for( n=1; zSemName[n]; n++ )
        if( zSemName[n]=='/' ) zSemName[n] = '_';
      pNew->pInode->pSem = sem_open(zSemName, O_CREAT, 0666, 1);
      if( pNew->pInode->pSem == SEM_FAILED ){
        rc = SQLITE_NOMEM;
        pNew->pInode->aSemName[0] = '\0';
      }
    }
    unixLeaveMutex();
  }
#endif
  
  storeLastErrno(pNew, 0);
#if OS_VXWORKS
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
    h = -1;
    osUnlink(zFilename);
    pNew->ctrlFlags |= UNIXFILE_DELETE;
  }
#endif
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
  }else{
    pNew->pMethod = pLockingStyle;
    OpenCounter(+1);
    verifyDbFile(pNew);
  }
  return rc;
}

/*
** Return the name of a directory in which to put temporary files.
** If no suitable temporary file directory can be found, return NULL.
*/
static const char *unixTempFileDir(void){
  static const char *azDirs[] = {
     0,
     0,
     0,
     "/var/tmp",
     "/usr/tmp",
     "/tmp",
     0        /* List terminator */
  };
  unsigned int i;
  struct stat buf;
  const char *zDir = 0;

  azDirs[0] = sqlite3_temp_directory;
  if( !azDirs[1] ) azDirs[1] = getenv("SQLITE_TMPDIR");
  if( !azDirs[2] ) azDirs[2] = getenv("TMPDIR");
  for(i=0; i<sizeof(azDirs)/sizeof(azDirs[0]); zDir=azDirs[i++]){
    if( zDir==0 ) continue;
    if( osStat(zDir, &buf) ) continue;
    if( !S_ISDIR(buf.st_mode) ) continue;
    if( osAccess(zDir, 07) ) continue;
    break;
  }
  return zDir;
}

/*
** Create a temporary file name in zBuf.  zBuf must be allocated
** by the calling process and must be big enough to hold at least
** pVfs->mxPathname bytes.
*/
static int unixGetTempname(int nBuf, char *zBuf){
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  unsigned int i, j;
  const char *zDir;

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. 
  */
  SimulateIOError( return SQLITE_IOERR );

  zDir = unixTempFileDir();
  if( zDir==0 ) zDir = ".";

  /* Check that the output buffer is large enough for the temporary file 
  ** name. If it is not, return SQLITE_ERROR.
  */
  if( (strlen(zDir) + strlen(SQLITE_TEMP_FILE_PREFIX) + 18) >= (size_t)nBuf ){
    return SQLITE_ERROR;
  }

  do{
    sqlite3_snprintf(nBuf-18, zBuf, "%s/"SQLITE_TEMP_FILE_PREFIX, zDir);
    j = (int)strlen(zBuf);
    sqlite3_randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
    zBuf[j+1] = 0;
  }while( osAccess(zBuf,0)==0 );
  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Routine to transform a unixFile into a proxy-locking unixFile.
** Implementation in the proxy-lock division, but used by unixOpen()
** if SQLITE_PREFER_PROXY_LOCKING is defined.
*/
static int proxyTransformUnixFile(unixFile*, const char*);
#endif

/*
** Search for an unused file descriptor that was opened on the database 
** file (not a journal or master-journal file) identified by pathname
** zPath with SQLITE_OPEN_XXX flags matching those passed as the second
** argument to this function.
**
** Such a file descriptor may exist if a database connection was closed
** but the associated file descriptor could not be closed because some
** other file descriptor open on the same file is holding a file-lock.
** Refer to comments in the unixClose() function and the lengthy comment
** describing "Posix Advisory Locking" at the start of this file for 
** further details. Also, ticket #4018.
**
** If a suitable file descriptor is found, then it is returned. If no
** such file descriptor is located, -1 is returned.
*/
static UnixUnusedFd *findReusableFd(const char *zPath, int flags){
  UnixUnusedFd *pUnused = 0;

  /* Do not search for an unused file descriptor on vxworks. Not because
  ** vxworks would not benefit from the change (it might, we're not sure),
  ** but because no way to test it is currently available. It is better 
  ** not to risk breaking vxworks support for the sake of such an obscure 
  ** feature.  */
#if !OS_VXWORKS
  struct stat sStat;                   /* Results of stat() call */

  /* A stat() call may fail for various reasons. If this happens, it is
  ** almost certain that an open() call on the same path will also fail.
  ** For this reason, if an error occurs in the stat() call here, it is
  ** ignored and -1 is returned. The caller will try to open a new file
  ** descriptor on the same path, fail, and return an error to SQLite.
  **
  ** Even if a subsequent open() call does succeed, the consequences of
  ** not searching for a reusable file descriptor are not dire.  */
  if( 0==osStat(zPath, &sStat) ){
    unixInodeInfo *pInode;

    unixEnterMutex();
    pInode = inodeList;
    while( pInode && (pInode->fileId.dev!=sStat.st_dev
                     || pInode->fileId.ino!=sStat.st_ino) ){
       pInode = pInode->pNext;
    }
    if( pInode ){
      UnixUnusedFd **pp;
      for(pp=&pInode->pUnused; *pp && (*pp)->flags!=flags; pp=&((*pp)->pNext));
      pUnused = *pp;
      if( pUnused ){
        *pp = pUnused->pNext;
      }
    }
    unixLeaveMutex();
  }
#endif    /* if !OS_VXWORKS */
  return pUnused;
}

/*
** This function is called by unixOpen() to determine the unix permissions
** to create new files with. If no error occurs, then SQLITE_OK is returned
** and a value suitable for passing as the third argument to open(2) is
** written to *pMode. If an IO error occurs, an SQLite error code is 
** returned and the value of *pMode is not modified.
**
** In most cases, this routine sets *pMode to 0, which will become
** an indication to robust_open() to create the file using
** SQLITE_DEFAULT_FILE_PERMISSIONS adjusted by the umask.
** But if the file being opened is a WAL or regular journal file, then 
** this function queries the file-system for the permissions on the 
** corresponding database file and sets *pMode to this value. Whenever 
** possible, WAL and journal files are created using the same permissions 
** as the associated database file.
**
** If the SQLITE_ENABLE_8_3_NAMES option is enabled, then the
** original filename is unavailable.  But 8_3_NAMES is only used for
** FAT filesystems and permissions do not matter there, so just use
** the default permissions.
*/
static int findCreateFileMode(
  const char *zPath,              /* Path of file (possibly) being created */
  int flags,                      /* Flags passed as 4th argument to xOpen() */
  mode_t *pMode,                  /* OUT: Permissions to open file with */
  uid_t *pUid,                    /* OUT: uid to set on the file */
  gid_t *pGid                     /* OUT: gid to set on the file */
){
  int rc = SQLITE_OK;             /* Return Code */
  *pMode = 0;
  *pUid = 0;
  *pGid = 0;
  if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
    char zDb[MAX_PATHNAME+1];     /* Database file path */
    int nDb;                      /* Number of valid bytes in zDb */
    struct stat sStat;            /* Output of stat() on database file */

    /* zPath is a path to a WAL or journal file. The following block derives
    ** the path to the associated database file from zPath. This block handles
    ** the following naming conventions:
    **
    **   "<path to db>-journal"
    **   "<path to db>-wal"
    **   "<path to db>-journalNN"
    **   "<path to db>-walNN"
    **
    ** where NN is a decimal number. The NN naming schemes are 
    ** used by the test_multiplex.c module.
    */
    nDb = sqlite3Strlen30(zPath) - 1; 
#ifdef SQLITE_ENABLE_8_3_NAMES
    while( nDb>0 && sqlite3Isalnum(zPath[nDb]) ) nDb--;
    if( nDb==0 || zPath[nDb]!='-' ) return SQLITE_OK;
#else
    while( zPath[nDb]!='-' ){
      assert( nDb>0 );
      assert( zPath[nDb]!='\n' );
      nDb--;
    }
#endif
    memcpy(zDb, zPath, nDb);
    zDb[nDb] = '\0';

    if( 0==osStat(zDb, &sStat) ){
      *pMode = sStat.st_mode & 0777;
      *pUid = sStat.st_uid;
      *pGid = sStat.st_gid;
    }else{
      rc = SQLITE_IOERR_FSTAT;
    }
  }else if( flags & SQLITE_OPEN_DELETEONCLOSE ){
    *pMode = 0600;
  }
  return rc;
}

/*
** Open the file zPath.
** 
** Previously, the SQLite OS layer used three functions in place of this
** one:
**
**     sqlite3OsOpenReadWrite();
**     sqlite3OsOpenReadOnly();
**     sqlite3OsOpenExclusive();
**
** These calls correspond to the following combinations of flags:
**
**     ReadWrite() ->     (READWRITE | CREATE)
**     ReadOnly()  ->     (READONLY) 
**     OpenExclusive() -> (READWRITE | CREATE | EXCLUSIVE)
**
** The old OpenExclusive() accepted a boolean argument - "delFlag". If
** true, the file was configured to be automatically deleted when the
** file handle closed. To achieve the same effect using this new 
** interface, add the DELETEONCLOSE flag to those specified above for 
** OpenExclusive().
*/
static int unixOpen(
  sqlite3_vfs *pVfs,           /* The VFS for which this is the xOpen method */
  const char *zPath,           /* Pathname of file to be opened */
  sqlite3_file *pFile,         /* The file descriptor to be filled in */
  int flags,                   /* Input flags to control the opening */
  int *pOutFlags               /* Output flags returned to SQLite core */
){
  unixFile *p = (unixFile *)pFile;
  int fd = -1;                   /* File descriptor returned by open() */
  int openFlags = 0;             /* Flags to pass to open() */
  int eType = flags&0xFFFFFF00;  /* Type of file to open */
  int noLock;                    /* True to omit locking primitives */
  int rc = SQLITE_OK;            /* Function Return Code */
  int ctrlFlags = 0;             /* UNIXFILE_* flags */

  int isExclusive  = (flags & SQLITE_OPEN_EXCLUSIVE);
  int isDelete     = (flags & SQLITE_OPEN_DELETEONCLOSE);
  int isCreate     = (flags & SQLITE_OPEN_CREATE);
  int isReadonly   = (flags & SQLITE_OPEN_READONLY);
  int isReadWrite  = (flags & SQLITE_OPEN_READWRITE);
#if SQLITE_ENABLE_LOCKING_STYLE
  int isAutoProxy  = (flags & SQLITE_OPEN_AUTOPROXY);
#endif
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  struct statfs fsInfo;
#endif

  /* If creating a master or main-file journal, this function will open
  ** a file-descriptor on the directory too. The first time unixSync()
  ** is called the directory file descriptor will be fsync()ed and close()d.
  */
  int syncDir = (isCreate && (
        eType==SQLITE_OPEN_MASTER_JOURNAL 
     || eType==SQLITE_OPEN_MAIN_JOURNAL 
     || eType==SQLITE_OPEN_WAL
  ));

  /* If argument zPath is a NULL pointer, this function is required to open
  ** a temporary file. Use this buffer to store the file name in.
  */
  char zTmpname[MAX_PATHNAME+2];
  const char *zName = zPath;

  /* Check the following statements are true: 
  **
  **   (a) Exactly one of the READWRITE and READONLY flags must be set, and 
  **   (b) if CREATE is set, then READWRITE must also be set, and
  **   (c) if EXCLUSIVE is set, then CREATE must also be set.
  **   (d) if DELETEONCLOSE is set, then CREATE must also be set.
  */
  assert((isReadonly==0 || isReadWrite==0) && (isReadWrite || isReadonly));
  assert(isCreate==0 || isReadWrite);
  assert(isExclusive==0 || isCreate);
  assert(isDelete==0 || isCreate);

  /* The main DB, main journal, WAL file and master journal are never 
  ** automatically deleted. Nor are they ever temporary files.  */
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_DB );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MASTER_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_WAL );

  /* Assert that the upper layer has set one of the "file-type" flags. */
  assert( eType==SQLITE_OPEN_MAIN_DB      || eType==SQLITE_OPEN_TEMP_DB 
       || eType==SQLITE_OPEN_MAIN_JOURNAL || eType==SQLITE_OPEN_TEMP_JOURNAL 
       || eType==SQLITE_OPEN_SUBJOURNAL   || eType==SQLITE_OPEN_MASTER_JOURNAL 
       || eType==SQLITE_OPEN_TRANSIENT_DB || eType==SQLITE_OPEN_WAL
  );

  /* Detect a pid change and reset the PRNG.  There is a race condition
  ** here such that two or more threads all trying to open databases at
  ** the same instant might all reset the PRNG.  But multiple resets
  ** are harmless.
  */
  if( randomnessPid!=osGetpid(0) ){
    randomnessPid = osGetpid(0);
    sqlite3_randomness(0,0);
  }

  memset(p, 0, sizeof(unixFile));

  if( eType==SQLITE_OPEN_MAIN_DB ){
    UnixUnusedFd *pUnused;
    pUnused = findReusableFd(zName, flags);
    if( pUnused ){
      fd = pUnused->fd;
    }else{
      pUnused = sqlite3_malloc64(sizeof(*pUnused));
      if( !pUnused ){
        return SQLITE_NOMEM;
      }
    }
    p->pUnused = pUnused;

    /* Database filenames are double-zero terminated if they are not
    ** URIs with parameters.  Hence, they can always be passed into
    ** sqlite3_uri_parameter(). */
    assert( (flags & SQLITE_OPEN_URI) || zName[strlen(zName)+1]==0 );

  }else if( !zName ){
    /* If zName is NULL, the upper layer is requesting a temp file. */
    assert(isDelete && !syncDir);
    rc = unixGetTempname(MAX_PATHNAME+2, zTmpname);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    zName = zTmpname;

    /* Generated temporary filenames are always double-zero terminated
    ** for use by sqlite3_uri_parameter(). */
    assert( zName[strlen(zName)+1]==0 );
  }

  /* Determine the value of the flags parameter passed to POSIX function
  ** open(). These must be calculated even if open() is not called, as
  ** they may be stored as part of the file handle and used by the 
  ** 'conch file' locking functions later on.  */
  if( isReadonly )  openFlags |= O_RDONLY;
  if( isReadWrite ) openFlags |= O_RDWR;
  if( isCreate )    openFlags |= O_CREAT;
  if( isExclusive ) openFlags |= (O_EXCL|O_NOFOLLOW);
  openFlags |= (O_LARGEFILE|O_BINARY);

  if( fd<0 ){
    mode_t openMode;              /* Permissions to create file with */
    uid_t uid;                    /* Userid for the file */
    gid_t gid;                    /* Groupid for the file */
    rc = findCreateFileMode(zName, flags, &openMode, &uid, &gid);
    if( rc!=SQLITE_OK ){
      assert( !p->pUnused );
      assert( eType==SQLITE_OPEN_WAL || eType==SQLITE_OPEN_MAIN_JOURNAL );
      return rc;
    }
    fd = robust_open(zName, openFlags, openMode);
    OSTRACE(("OPENX   %-3d %s 0%o\n", fd, zName, openFlags));
    if( fd<0 && errno!=EISDIR && isReadWrite && !isExclusive ){
      /* Failed to open the file for read/write access. Try read-only. */
      flags &= ~(SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
      openFlags &= ~(O_RDWR|O_CREAT);
      flags |= SQLITE_OPEN_READONLY;
      openFlags |= O_RDONLY;
      isReadonly = 1;
      fd = robust_open(zName, openFlags, openMode);
    }
    if( fd<0 ){
      rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zName);
      goto open_finished;
    }

    /* If this process is running as root and if creating a new rollback
    ** journal or WAL file, set the ownership of the journal or WAL to be
    ** the same as the original database.
    */
    if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
      osFchown(fd, uid, gid);
    }
  }
  assert( fd>=0 );
  if( pOutFlags ){
    *pOutFlags = flags;
  }

  if( p->pUnused ){
    p->pUnused->fd = fd;
    p->pUnused->flags = flags;
  }

  if( isDelete ){
#if OS_VXWORKS
    zPath = zName;
#elif defined(SQLITE_UNLINK_AFTER_CLOSE)
    zPath = sqlite3_mprintf("%s", zName);
    if( zPath==0 ){
      robust_close(p, fd, __LINE__);
      return SQLITE_NOMEM;
    }
#else
    osUnlink(zName);
#endif
  }
#if SQLITE_ENABLE_LOCKING_STYLE
  else{
    p->openFlags = openFlags;
  }
#endif

  noLock = eType!=SQLITE_OPEN_MAIN_DB;

  
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  if( fstatfs(fd, &fsInfo) == -1 ){
    storeLastErrno(p, errno);
    robust_close(p, fd, __LINE__);
    return SQLITE_IOERR_ACCESS;
  }
  if (0 == strncmp("msdos", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
  if (0 == strncmp("exfat", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
#endif

  /* Set up appropriate ctrlFlags */
  if( isDelete )                ctrlFlags |= UNIXFILE_DELETE;
  if( isReadonly )              ctrlFlags |= UNIXFILE_RDONLY;
  if( noLock )                  ctrlFlags |= UNIXFILE_NOLOCK;
  if( syncDir )                 ctrlFlags |= UNIXFILE_DIRSYNC;
  if( flags & SQLITE_OPEN_URI ) ctrlFlags |= UNIXFILE_URI;

#if SQLITE_ENABLE_LOCKING_STYLE
#if SQLITE_PREFER_PROXY_LOCKING
  isAutoProxy = 1;
#endif
  if( isAutoProxy && (zPath!=NULL) && (!noLock) && pVfs->xOpen ){
    char *envforce = getenv("SQLITE_FORCE_PROXY_LOCKING");
    int useProxy = 0;

    /* SQLITE_FORCE_PROXY_LOCKING==1 means force always use proxy, 0 means 
    ** never use proxy, NULL means use proxy for non-local files only.  */
    if( envforce!=NULL ){
      useProxy = atoi(envforce)>0;
    }else{
      useProxy = !(fsInfo.f_flags&MNT_LOCAL);
    }
    if( useProxy ){
      rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);
      if( rc==SQLITE_OK ){
        rc = proxyTransformUnixFile((unixFile*)pFile, ":auto:");
        if( rc!=SQLITE_OK ){
          /* Use unixClose to clean up the resources added in fillInUnixFile 
          ** and clear all the structure's references.  Specifically, 
          ** pFile->pMethods will be NULL so sqlite3OsClose will be a no-op 
          */
          unixClose(pFile);
          return rc;
        }
      }
      goto open_finished;
    }
  }
#endif
  
  rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);

open_finished:
  if( rc!=SQLITE_OK ){
    sqlite3_free(p->pUnused);
  }
  return rc;
}


/*
** Delete the file at zPath. If the dirSync argument is true, fsync()
** the directory after deleting the file.
*/
static int unixDelete(
  sqlite3_vfs *NotUsed,     /* VFS containing this as the xDelete method */
  const char *zPath,        /* Name of file to be deleted */
  int dirSync               /* If true, fsync() directory after deleting file */
){
  int rc = SQLITE_OK;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError(return SQLITE_IOERR_DELETE);
  if( osUnlink(zPath)==(-1) ){
    if( errno==ENOENT
#if OS_VXWORKS
        || osAccess(zPath,0)!=0
#endif
    ){
      rc = SQLITE_IOERR_DELETE_NOENT;
    }else{
      rc = unixLogError(SQLITE_IOERR_DELETE, "unlink", zPath);
    }
    return rc;
  }
#ifndef SQLITE_DISABLE_DIRSYNC
  if( (dirSync & 1)!=0 ){
    int fd;
    rc = osOpenDirectory(zPath, &fd);
    if( rc==SQLITE_OK ){
#if OS_VXWORKS
      if( fsync(fd)==-1 )
#else
      if( fsync(fd) )
#endif
      {
        rc = unixLogError(SQLITE_IOERR_DIR_FSYNC, "fsync", zPath);
      }
      robust_close(0, fd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
  }
#endif
  return rc;
}

/*
** Test the existence of or access permissions of file zPath. The
** test performed depends on the value of flags:
**
**     SQLITE_ACCESS_EXISTS: Return 1 if the file exists
**     SQLITE_ACCESS_READWRITE: Return 1 if the file is read and writable.
**     SQLITE_ACCESS_READONLY: Return 1 if the file is readable.
**
** Otherwise return 0.
*/
static int unixAccess(
  sqlite3_vfs *NotUsed,   /* The VFS containing this xAccess method */
  const char *zPath,      /* Path of the file to examine */
  int flags,              /* What do we want to learn about the zPath file? */
  int *pResOut            /* Write result boolean here */
){
  int amode = 0;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError( return SQLITE_IOERR_ACCESS; );
  switch( flags ){
    case SQLITE_ACCESS_EXISTS:
      amode = F_OK;
      break;
    case SQLITE_ACCESS_READWRITE:
      amode = W_OK|R_OK;
      break;
    case SQLITE_ACCESS_READ:
      amode = R_OK;
      break;

    default:
      assert(!"Invalid flags argument");
  }
  *pResOut = (osAccess(zPath, amode)==0);
  if( flags==SQLITE_ACCESS_EXISTS && *pResOut ){
    struct stat buf;
    if( 0==osStat(zPath, &buf) && buf.st_size==0 ){
      *pResOut = 0;
    }
  }
  return SQLITE_OK;
}


/*
** Turn a relative pathname into a full pathname. The relative path
** is stored as a nul-terminated string in the buffer pointed to by
** zPath. 
**
** zOut points to a buffer of at least sqlite3_vfs.mxPathname bytes 
** (in this case, MAX_PATHNAME bytes). The full-path is written to
** this buffer before returning.
*/
static int unixFullPathname(
  sqlite3_vfs *pVfs,            /* Pointer to vfs object */
  const char *zPath,            /* Possibly relative input path */
  int nOut,                     /* Size of output buffer in bytes */
  char *zOut                    /* Output buffer */
){

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. This function could fail if, for example, the
  ** current working directory has been unlinked.
  */
  SimulateIOError( return SQLITE_ERROR );

  assert( pVfs->mxPathname==MAX_PATHNAME );
  UNUSED_PARAMETER(pVfs);

  zOut[nOut-1] = '\0';
  if( zPath[0]=='/' ){
    sqlite3_snprintf(nOut, zOut, "%s", zPath);
  }else{
    int nCwd;
    if( osGetcwd(zOut, nOut-1)==0 ){
      return unixLogError(SQLITE_CANTOPEN_BKPT, "getcwd", zPath);
    }
    nCwd = (int)strlen(zOut);
    sqlite3_snprintf(nOut-nCwd, &zOut[nCwd], "/%s", zPath);
  }
  return SQLITE_OK;
}


#ifndef SQLITE_OMIT_LOAD_EXTENSION
/*
** Interfaces for opening a shared library, finding entry points
** within the shared library, and closing the shared library.
*/
#include <dlfcn.h>
static void *unixDlOpen(sqlite3_vfs *NotUsed, const char *zFilename){
  UNUSED_PARAMETER(NotUsed);
  return dlopen(zFilename, RTLD_NOW | RTLD_GLOBAL);
}

/*
** SQLite calls this function immediately after a call to unixDlSym() or
** unixDlOpen() fails (returns a null pointer). If a more detailed error
** message is available, it is written to zBufOut. If no error message
** is available, zBufOut is left unmodified and SQLite uses a default
** error message.
*/
static void unixDlError(sqlite3_vfs *NotUsed, int nBuf, char *zBufOut){
  const char *zErr;
  UNUSED_PARAMETER(NotUsed);
  unixEnterMutex();
  zErr = dlerror();
  if( zErr ){
    sqlite3_snprintf(nBuf, zBufOut, "%s", zErr);
  }
  unixLeaveMutex();
}
static void (*unixDlSym(sqlite3_vfs *NotUsed, void *p, const char*zSym))(void){
  /* 
  ** GCC with -pedantic-errors says that C90 does not allow a void* to be
  ** cast into a pointer to a function.  And yet the library dlsym() routine
  ** returns a void* which is really a pointer to a function.  So how do we
  ** use dlsym() with -pedantic-errors?
  **
  ** Variable x below is defined to be a pointer to a function taking
  ** parameters void* and const char* and returning a pointer to a function.
  ** We initialize x by assigning it a pointer to the dlsym() function.
  ** (That assignment requires a cast.)  Then we call the function that
  ** x points to.  
  **
  ** This work-around is unlikely to work correctly on any system where
  ** you really cannot cast a function pointer into void*.  But then, on the
  ** other hand, dlsym() will not work on such a system either, so we have
  ** not really lost anything.
  */
  void (*(*x)(void*,const char*))(void);
  UNUSED_PARAMETER(NotUsed);
  x = (void(*(*)(void*,const char*))(void))dlsym;
  return (*x)(p, zSym);
}
static void unixDlClose(sqlite3_vfs *NotUsed, void *pHandle){
  UNUSED_PARAMETER(NotUsed);
  dlclose(pHandle);
}
#else /* if SQLITE_OMIT_LOAD_EXTENSION is defined: */
  #define unixDlOpen  0
  #define unixDlError 0
  #define unixDlSym   0
  #define unixDlClose 0
#endif

/*
** Write nBuf bytes of random data to the supplied buffer zBuf.
*/
static int unixRandomness(sqlite3_vfs *NotUsed, int nBuf, char *zBuf){
  UNUSED_PARAMETER(NotUsed);
  assert((size_t)nBuf>=(sizeof(time_t)+sizeof(int)));

  /* We have to initialize zBuf to prevent valgrind from reporting
  ** errors.  The reports issued by valgrind are incorrect - we would
  ** prefer that the randomness be increased by making use of the
  ** uninitialized space in zBuf - but valgrind errors tend to worry
  ** some users.  Rather than argue, it seems easier just to initialize
  ** the whole array and silence valgrind, even if that means less randomness
  ** in the random seed.
  **
  ** When testing, initializing zBuf[] to zero is all we do.  That means
  ** that we always use the same random number sequence.  This makes the
  ** tests repeatable.
  */
  memset(zBuf, 0, nBuf);
  randomnessPid = osGetpid(0);  
#if !defined(SQLITE_TEST) && !defined(SQLITE_OMIT_RANDOMNESS)
  {
    int fd, got;
    fd = robust_open("/dev/urandom", O_RDONLY, 0);
    if( fd<0 ){
      time_t t;
      time(&t);
      memcpy(zBuf, &t, sizeof(t));
      memcpy(&zBuf[sizeof(t)], &randomnessPid, sizeof(randomnessPid));
      assert( sizeof(t)+sizeof(randomnessPid)<=(size_t)nBuf );
      nBuf = sizeof(t) + sizeof(randomnessPid);
    }else{
      do{ got = osRead(fd, zBuf, nBuf); }while( got<0 && errno==EINTR );
      robust_close(0, fd, __LINE__);
    }
  }
#endif
  return nBuf;
}


/*
** Sleep for a little while.  Return the amount of time slept.
** The argument is the number of microseconds we want to sleep.
** The return value is the number of microseconds of sleep actually
** requested from the underlying operating system, a number which
** might be greater than or equal to the argument, but not less
** than the argument.
*/
static int unixSleep(sqlite3_vfs *NotUsed, int microseconds){
#if OS_VXWORKS
  struct timespec sp;

  sp.tv_sec = microseconds / 1000000;
  sp.tv_nsec = (microseconds % 1000000) * 1000;
  nanosleep(&sp, NULL);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#elif defined(HAVE_USLEEP) && HAVE_USLEEP
  usleep(microseconds);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#else
  int seconds = (microseconds+999999)/1000000;
  sleep(seconds);
  UNUSED_PARAMETER(NotUsed);
  return seconds*1000000;
#endif
}

/*
** The following variable, if set to a non-zero value, is interpreted as
** the number of seconds since 1970 and is used to set the result of
** sqlite3OsCurrentTime() during testing.
*/
#ifdef SQLITE_TEST
SQLITE_API int sqlite3_current_time = 0;  /* Fake system time in seconds since 1970. */
#endif

/*
** Find the current time (in Universal Coordinated Time).  Write into *piNow
** the current time and date as a Julian Day number times 86_400_000.  In
** other words, write into *piNow the number of milliseconds since the Julian
** epoch of noon in Greenwich on November 24, 4714 B.C according to the
** proleptic Gregorian calendar.
**
** On success, return SQLITE_OK.  Return SQLITE_ERROR if the time and date 
** cannot be found.
*/
static int unixCurrentTimeInt64(sqlite3_vfs *NotUsed, sqlite3_int64 *piNow){
  static const sqlite3_int64 unixEpoch = 24405875*(sqlite3_int64)8640000;
  int rc = SQLITE_OK;
#if defined(NO_GETTOD)
  time_t t;
  time(&t);
  *piNow = ((sqlite3_int64)t)*1000 + unixEpoch;
#elif OS_VXWORKS
  struct timespec sNow;
  clock_gettime(CLOCK_REALTIME, &sNow);
  *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_nsec/1000000;
#else
  struct timeval sNow;
  if( gettimeofday(&sNow, 0)==0 ){
    *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_usec/1000;
  }else{
    rc = SQLITE_ERROR;
  }
#endif

#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *piNow = 1000*(sqlite3_int64)sqlite3_current_time + unixEpoch;
  }
#endif
  UNUSED_PARAMETER(NotUsed);
  return rc;
}

/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
static int unixCurrentTime(sqlite3_vfs *NotUsed, double *prNow){
  sqlite3_int64 i = 0;
  int rc;
  UNUSED_PARAMETER(NotUsed);
  rc = unixCurrentTimeInt64(0, &i);
  *prNow = i/86400000.0;
  return rc;
}

/*
** We added the xGetLastError() method with the intention of providing
** better low-level error messages when operating-system problems come up
** during SQLite operation.  But so far, none of that has been implemented
** in the core.  So this routine is never called.  For now, it is merely
** a place-holder.
*/
static int unixGetLastError(sqlite3_vfs *NotUsed, int NotUsed2, char *NotUsed3){
  UNUSED_PARAMETER(NotUsed);
  UNUSED_PARAMETER(NotUsed2);
  UNUSED_PARAMETER(NotUsed3);
  return 0;
}


/*
************************ End of sqlite3_vfs methods ***************************
******************************************************************************/

/******************************************************************************
************************** Begin Proxy Locking ********************************
**
** Proxy locking is a "uber-locking-method" in this sense:  It uses the
** other locking methods on secondary lock files.  Proxy locking is a
** meta-layer over top of the primitive locking implemented above.  For
** this reason, the division that implements of proxy locking is deferred
** until late in the file (here) after all of the other I/O methods have
** been defined - so that the primitive locking methods are available
** as services to help with the implementation of proxy locking.
**
****
**
** The default locking schemes in SQLite use byte-range locks on the
** database file to coordinate safe, concurrent access by multiple readers
** and writers [http://sqlite.org/lockingv3.html].  The five file locking
** states (UNLOCKED, PENDING, SHARED, RESERVED, EXCLUSIVE) are implemented
** as POSIX read & write locks over fixed set of locations (via fsctl),
** on AFP and SMB only exclusive byte-range locks are available via fsctl
** with _IOWR('z', 23, struct ByteRangeLockPB2) to track the same 5 states.
** To simulate a F_RDLCK on the shared range, on AFP a randomly selected
** address in the shared range is taken for a SHARED lock, the entire
** shared range is taken for an EXCLUSIVE lock):
**
**      PENDING_BYTE        0x40000000
**      RESERVED_BYTE       0x40000001
**      SHARED_RANGE        0x40000002 -> 0x40000200
**
** This works well on the local file system, but shows a nearly 100x
** slowdown in read performance on AFP because the AFP client disables
** the read cache when byte-range locks are present.  Enabling the read
** cache exposes a cache coherency problem that is present on all OS X
** supported network file systems.  NFS and AFP both observe the
** close-to-open semantics for ensuring cache coherency
** [http://nfs.sourceforge.net/#faq_a8], which does not effectively
** address the requirements for concurrent database access by multiple
** readers and writers
** [http://www.nabble.com/SQLite-on-NFS-cache-coherency-td15655701.html].
**
** To address the performance and cache coherency issues, proxy file locking
** changes the way database access is controlled by limiting access to a
** single host at a time and moving file locks off of the database file
** and onto a proxy file on the local file system.  
**
**
** Using proxy locks
** -----------------
**
** C APIs
**
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_SET_LOCKPROXYFILE,
**                       <proxy_path> | ":auto:");
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_GET_LOCKPROXYFILE,
**                       &<proxy_path>);
**
**
** SQL pragmas
**
**  PRAGMA [database.]lock_proxy_file=<proxy_path> | :auto:
**  PRAGMA [database.]lock_proxy_file
**
** Specifying ":auto:" means that if there is a conch file with a matching
** host ID in it, the proxy path in the conch file will be used, otherwise
** a proxy path based on the user's temp dir
** (via confstr(_CS_DARWIN_USER_TEMP_DIR,...)) will be used and the
** actual proxy file name is generated from the name and path of the
** database file.  For example:
**
**       For database path "/Users/me/foo.db" 
**       The lock path will be "<tmpdir>/sqliteplocks/_Users_me_foo.db:auto:")
**
** Once a lock proxy is configured for a database connection, it can not
** be removed, however it may be switched to a different proxy path via
** the above APIs (assuming the conch file is not being held by another
** connection or process). 
**
**
** How proxy locking works
** -----------------------
**
** Proxy file locking relies primarily on two new supporting files: 
**
**   *  conch file to limit access to the database file to a single host
**      at a time
**
**   *  proxy file to act as a proxy for the advisory locks normally
**      taken on the database
**
** The conch file - to use a proxy file, sqlite must first "hold the conch"
** by taking an sqlite-style shared lock on the conch file, reading the
** contents and comparing the host's unique host ID (see below) and lock
** proxy path against the values stored in the conch.  The conch file is
** stored in the same directory as the database file and the file name
** is patterned after the database file name as ".<databasename>-conch".
** If the conch file does not exist, or its contents do not match the
** host ID and/or proxy path, then the lock is escalated to an exclusive
** lock and the conch file contents is updated with the host ID and proxy
** path and the lock is downgraded to a shared lock again.  If the conch
** is held by another process (with a shared lock), the exclusive lock
** will fail and SQLITE_BUSY is returned.
**
** The proxy file - a single-byte file used for all advisory file locks
** normally taken on the database file.   This allows for safe sharing
** of the database file for multiple readers and writers on the same
** host (the conch ensures that they all use the same local lock file).
**
** Requesting the lock proxy does not immediately take the conch, it is
** only taken when the first request to lock database file is made.  
** This matches the semantics of the traditional locking behavior, where
** opening a connection to a database file does not take a lock on it.
** The shared lock and an open file descriptor are maintained until 
** the connection to the database is closed. 
**
** The proxy file and the lock file are never deleted so they only need
** to be created the first time they are used.
**
** Configuration options
** ---------------------
**
**  SQLITE_PREFER_PROXY_LOCKING
**
**       Database files accessed on non-local file systems are
**       automatically configured for proxy locking, lock files are
**       named automatically using the same logic as
**       PRAGMA lock_proxy_file=":auto:"
**    
**  SQLITE_PROXY_DEBUG
**
**       Enables the logging of error messages during host id file
**       retrieval and creation
**
**  LOCKPROXYDIR
**
**       Overrides the default directory used for lock proxy files that
**       are named automatically via the ":auto:" setting
**
**  SQLITE_DEFAULT_PROXYDIR_PERMISSIONS
**
**       Permissions to use when creating a directory for storing the
**       lock proxy files, only used when LOCKPROXYDIR is not set.
**    
**    
** As mentioned above, when compiled with SQLITE_PREFER_PROXY_LOCKING,
** setting the environment variable SQLITE_FORCE_PROXY_LOCKING to 1 will
** force proxy locking to be used for every database file opened, and 0
** will force automatic proxy locking to be disabled for all database
** files (explicitly calling the SQLITE_FCNTL_SET_LOCKPROXYFILE pragma or
** sqlite_file_control API is not affected by SQLITE_FORCE_PROXY_LOCKING).
*/

/*
** Proxy locking is only available on MacOSX 
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE

/*
** The proxyLockingContext has the path and file structures for the remote 
** and local proxy files in it
*/
typedef struct proxyLockingContext proxyLockingContext;
struct proxyLockingContext {
  unixFile *conchFile;         /* Open conch file */
  char *conchFilePath;         /* Name of the conch file */
  unixFile *lockProxy;         /* Open proxy lock file */
  char *lockProxyPath;         /* Name of the proxy lock file */
  char *dbPath;                /* Name of the open file */
  int conchHeld;               /* 1 if the conch is held, -1 if lockless */
  int nFails;                  /* Number of conch taking failures */
  void *oldLockingContext;     /* Original lockingcontext to restore on close */
  sqlite3_io_methods const *pOldMethod;     /* Original I/O methods for close */
};

/* 
** The proxy lock file path for the database at dbPath is written into lPath, 
** which must point to valid, writable memory large enough for a maxLen length
** file path. 
*/
static int proxyGetLockPath(const char *dbPath, char *lPath, size_t maxLen){
  int len;
  int dbLen;
  int i;

#ifdef LOCKPROXYDIR
  len = strlcpy(lPath, LOCKPROXYDIR, maxLen);
#else
# ifdef _CS_DARWIN_USER_TEMP_DIR
  {
    if( !confstr(_CS_DARWIN_USER_TEMP_DIR, lPath, maxLen) ){
      OSTRACE(("GETLOCKPATH  failed %s errno=%d pid=%d\n",
               lPath, errno, osGetpid(0)));
      return SQLITE_IOERR_LOCK;
    }
    len = strlcat(lPath, "sqliteplocks", maxLen);    
  }
# else
  len = strlcpy(lPath, "/tmp/", maxLen);
# endif
#endif

  if( lPath[len-1]!='/' ){
    len = strlcat(lPath, "/", maxLen);
  }
  
  /* transform the db path to a unique cache name */
  dbLen = (int)strlen(dbPath);
  for( i=0; i<dbLen && (i+len+7)<(int)maxLen; i++){
    char c = dbPath[i];
    lPath[i+len] = (c=='/')?'_':c;
  }
  lPath[i+len]='\0';
  strlcat(lPath, ":auto:", maxLen);
  OSTRACE(("GETLOCKPATH  proxy lock path=%s pid=%d\n", lPath, osGetpid(0)));
  return SQLITE_OK;
}

/* 
 ** Creates the lock file and any missing directories in lockPath
 */
static int proxyCreateLockPath(const char *lockPath){
  int i, len;
  char buf[MAXPATHLEN];
  int start = 0;
  
  assert(lockPath!=NULL);
  /* try to create all the intermediate directories */
  len = (int)strlen(lockPath);
  buf[0] = lockPath[0];
  for( i=1; i<len; i++ ){
    if( lockPath[i] == '/' && (i - start > 0) ){
      /* only mkdir if leaf dir != "." or "/" or ".." */
      if( i-start>2 || (i-start==1 && buf[start] != '.' && buf[start] != '/') 
         || (i-start==2 && buf[start] != '.' && buf[start+1] != '.') ){
        buf[i]='\0';
        if( osMkdir(buf, SQLITE_DEFAULT_PROXYDIR_PERMISSIONS) ){
          int err=errno;
          if( err!=EEXIST ) {
            OSTRACE(("CREATELOCKPATH  FAILED creating %s, "
                     "'%s' proxy lock path=%s pid=%d\n",
                     buf, strerror(err), lockPath, osGetpid(0)));
            return err;
          }
        }
      }
      start=i+1;
    }
    buf[i] = lockPath[i];
  }
  OSTRACE(("CREATELOCKPATH  proxy lock path=%s pid=%d\n", lockPath, osGetpid(0)));
  return 0;
}

/*
** Create a new VFS file descriptor (stored in memory obtained from
** sqlite3_malloc) and open the file named "path" in the file descriptor.
**
** The caller is responsible not only for closing the file descriptor
** but also for freeing the memory associated with the file descriptor.
*/
static int proxyCreateUnixFile(
    const char *path,        /* path for the new unixFile */
    unixFile **ppFile,       /* unixFile created and returned by ref */
    int islockfile           /* if non zero missing dirs will be created */
) {
  int fd = -1;
  unixFile *pNew;
  int rc = SQLITE_OK;
  int openFlags = O_RDWR | O_CREAT;
  sqlite3_vfs dummyVfs;
  int terrno = 0;
  UnixUnusedFd *pUnused = NULL;

  /* 1. first try to open/create the file
  ** 2. if that fails, and this is a lock file (not-conch), try creating
  ** the parent directories and then try again.
  ** 3. if that fails, try to open the file read-only
  ** otherwise return BUSY (if lock file) or CANTOPEN for the conch file
  */
  pUnused = findReusableFd(path, openFlags);
  if( pUnused ){
    fd = pUnused->fd;
  }else{
    pUnused = sqlite3_malloc64(sizeof(*pUnused));
    if( !pUnused ){
      return SQLITE_NOMEM;
    }
  }
  if( fd<0 ){
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
    if( fd<0 && errno==ENOENT && islockfile ){
      if( proxyCreateLockPath(path) == SQLITE_OK ){
        fd = robust_open(path, openFlags, 0);
      }
    }
  }
  if( fd<0 ){
    openFlags = O_RDONLY;
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
  }
  if( fd<0 ){
    if( islockfile ){
      return SQLITE_BUSY;
    }
    switch (terrno) {
      case EACCES:
        return SQLITE_PERM;
      case EIO: 
        return SQLITE_IOERR_LOCK; /* even though it is the conch */
      default:
        return SQLITE_CANTOPEN_BKPT;
    }
  }
  
  pNew = (unixFile *)sqlite3_malloc64(sizeof(*pNew));
  if( pNew==NULL ){
    rc = SQLITE_NOMEM;
    goto end_create_proxy;
  }
  memset(pNew, 0, sizeof(unixFile));
  pNew->openFlags = openFlags;
  memset(&dummyVfs, 0, sizeof(dummyVfs));
  dummyVfs.pAppData = (void*)&autolockIoFinder;
  dummyVfs.zName = "dummy";
  pUnused->fd = fd;
  pUnused->flags = openFlags;
  pNew->pUnused = pUnused;
  
  rc = fillInUnixFile(&dummyVfs, fd, (sqlite3_file*)pNew, path, 0);
  if( rc==SQLITE_OK ){
    *ppFile = pNew;
    return SQLITE_OK;
  }
end_create_proxy:    
  robust_close(pNew, fd, __LINE__);
  sqlite3_free(pNew);
  sqlite3_free(pUnused);
  return rc;
}

#ifdef SQLITE_TEST
/* simulate multiple hosts by creating unique hostid file paths */
SQLITE_API int sqlite3_hostid_num = 0;
#endif

#define PROXY_HOSTIDLEN    16  /* conch file host id length */

#ifdef HAVE_GETHOSTUUID
/* Not always defined in the headers as it ought to be */
extern int gethostuuid(uuid_t id, const struct timespec *wait);
#endif

/* get the host ID via gethostuuid(), pHostID must point to PROXY_HOSTIDLEN 
** bytes of writable memory.
*/
static int proxyGetHostID(unsigned char *pHostID, int *pError){
  assert(PROXY_HOSTIDLEN == sizeof(uuid_t));
  memset(pHostID, 0, PROXY_HOSTIDLEN);
#ifdef HAVE_GETHOSTUUID
  {
    struct timespec timeout = {1, 0}; /* 1 sec timeout */
    if( gethostuuid(pHostID, &timeout) ){
      int err = errno;
      if( pError ){
        *pError = err;
      }
      return SQLITE_IOERR;
    }
  }
#else
  UNUSED_PARAMETER(pError);
#endif
#ifdef SQLITE_TEST
  /* simulate multiple hosts by creating unique hostid file paths */
  if( sqlite3_hostid_num != 0){
    pHostID[0] = (char)(pHostID[0] + (char)(sqlite3_hostid_num & 0xFF));
  }
#endif
  
  return SQLITE_OK;
}

/* The conch file contains the header, host id and lock file path
 */
#define PROXY_CONCHVERSION 2   /* 1-byte header, 16-byte host id, path */
#define PROXY_HEADERLEN    1   /* conch file header length */
#define PROXY_PATHINDEX    (PROXY_HEADERLEN+PROXY_HOSTIDLEN)
#define PROXY_MAXCONCHLEN  (PROXY_HEADERLEN+PROXY_HOSTIDLEN+MAXPATHLEN)

/* 
** Takes an open conch file, copies the contents to a new path and then moves 
** it back.  The newly created file's file descriptor is assigned to the
** conch file structure and finally the original conch file descriptor is 
** closed.  Returns zero if successful.
*/
static int proxyBreakConchLock(unixFile *pFile, uuid_t myHostID){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  char tPath[MAXPATHLEN];
  char buf[PROXY_MAXCONCHLEN];
  char *cPath = pCtx->conchFilePath;
  size_t readLen = 0;
  size_t pathLen = 0;
  char errmsg[64] = "";
  int fd = -1;
  int rc = -1;
  UNUSED_PARAMETER(myHostID);

  /* create a new path by replace the trailing '-conch' with '-break' */
  pathLen = strlcpy(tPath, cPath, MAXPATHLEN);
  if( pathLen>MAXPATHLEN || pathLen<6 || 
     (strlcpy(&tPath[pathLen-5], "break", 6) != 5) ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"path error (len %d)",(int)pathLen);
    goto end_breaklock;
  }
  /* read the conch content */
  readLen = osPread(conchFile->h, buf, PROXY_MAXCONCHLEN, 0);
  if( readLen<PROXY_PATHINDEX ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"read error (len %d)",(int)readLen);
    goto end_breaklock;
  }
  /* write it out to the temporary break file */
  fd = robust_open(tPath, (O_RDWR|O_CREAT|O_EXCL), 0);
  if( fd<0 ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "create failed (%d)", errno);
    goto end_breaklock;
  }
  if( osPwrite(fd, buf, readLen, 0) != (ssize_t)readLen ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "write failed (%d)", errno);
    goto end_breaklock;
  }
  if( rename(tPath, cPath) ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "rename failed (%d)", errno);
    goto end_breaklock;
  }
  rc = 0;
  fprintf(stderr, "broke stale lock on %s\n", cPath);
  robust_close(pFile, conchFile->h, __LINE__);
  conchFile->h = fd;
  conchFile->openFlags = O_RDWR | O_CREAT;

end_breaklock:
  if( rc ){
    if( fd>=0 ){
      osUnlink(tPath);
      robust_close(pFile, fd, __LINE__);
    }
    fprintf(stderr, "failed to break stale lock on %s, %s\n", cPath, errmsg);
  }
  return rc;
}

/* Take the requested lock on the conch file and break a stale lock if the 
** host id matches.
*/
static int proxyConchLock(unixFile *pFile, uuid_t myHostID, int lockType){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  int rc = SQLITE_OK;
  int nTries = 0;
  struct timespec conchModTime;
  
  memset(&conchModTime, 0, sizeof(conchModTime));
  do {
    rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
    nTries ++;
    if( rc==SQLITE_BUSY ){
      /* If the lock failed (busy):
       * 1st try: get the mod time of the conch, wait 0.5s and try again. 
       * 2nd try: fail if the mod time changed or host id is different, wait 
       *           10 sec and try again
       * 3rd try: break the lock unless the mod time has changed.
       */
      struct stat buf;
      if( osFstat(conchFile->h, &buf) ){
        storeLastErrno(pFile, errno);
        return SQLITE_IOERR_LOCK;
      }
      
      if( nTries==1 ){
        conchModTime = buf.st_mtimespec;
        usleep(500000); /* wait 0.5 sec and try the lock again*/
        continue;  
      }

      assert( nTries>1 );
      if( conchModTime.tv_sec != buf.st_mtimespec.tv_sec || 
         conchModTime.tv_nsec != buf.st_mtimespec.tv_nsec ){
        return SQLITE_BUSY;
      }
      
      if( nTries==2 ){  
        char tBuf[PROXY_MAXCONCHLEN];
        int len = osPread(conchFile->h, tBuf, PROXY_MAXCONCHLEN, 0);
        if( len<0 ){
          storeLastErrno(pFile, errno);
          return SQLITE_IOERR_LOCK;
        }
        if( len>PROXY_PATHINDEX && tBuf[0]==(char)PROXY_CONCHVERSION){
          /* don't break the lock if the host id doesn't match */
          if( 0!=memcmp(&tBuf[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN) ){
            return SQLITE_BUSY;
          }
        }else{
          /* don't break the lock on short read or a version mismatch */
          return SQLITE_BUSY;
        }
        usleep(10000000); /* wait 10 sec and try the lock again */
        continue; 
      }
      
      assert( nTries==3 );
      if( 0==proxyBreakConchLock(pFile, myHostID) ){
        rc = SQLITE_OK;
        if( lockType==EXCLUSIVE_LOCK ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, SHARED_LOCK);
        }
        if( !rc ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
        }
      }
    }
  } while( rc==SQLITE_BUSY && nTries<3 );
  
  return rc;
}

/* Takes the conch by taking a shared lock and read the contents conch, if 
** lockPath is non-NULL, the host ID and lock file path must match.  A NULL 
** lockPath means that the lockPath in the conch file will be used if the 
** host IDs match, or a new lock path will be generated automatically 
** and written to the conch file.
*/
static int proxyTakeConch(unixFile *pFile){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  
  if( pCtx->conchHeld!=0 ){
    return SQLITE_OK;
  }else{
    unixFile *conchFile = pCtx->conchFile;
    uuid_t myHostID;
    int pError = 0;
    char readBuf[PROXY_MAXCONCHLEN];
    char lockPath[MAXPATHLEN];
    char *tempLockPath = NULL;
    int rc = SQLITE_OK;
    int createConch = 0;
    int hostIdMatch = 0;
    int readLen = 0;
    int tryOldLockPath = 0;
    int forceNewLockPath = 0;
    
    OSTRACE(("TAKECONCH  %d for %s pid=%d\n", conchFile->h,
             (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"),
             osGetpid(0)));

    rc = proxyGetHostID(myHostID, &pError);
    if( (rc&0xff)==SQLITE_IOERR ){
      storeLastErrno(pFile, pError);
      goto end_takeconch;
    }
    rc = proxyConchLock(pFile, myHostID, SHARED_LOCK);
    if( rc!=SQLITE_OK ){
      goto end_takeconch;
    }
    /* read the existing conch file */
    readLen = seekAndRead((unixFile*)conchFile, 0, readBuf, PROXY_MAXCONCHLEN);
    if( readLen<0 ){
      /* I/O error: lastErrno set by seekAndRead */
      storeLastErrno(pFile, conchFile->lastErrno);
      rc = SQLITE_IOERR_READ;
      goto end_takeconch;
    }else if( readLen<=(PROXY_HEADERLEN+PROXY_HOSTIDLEN) || 
             readBuf[0]!=(char)PROXY_CONCHVERSION ){
      /* a short read or version format mismatch means we need to create a new 
      ** conch file. 
      */
      createConch = 1;
    }
    /* if the host id matches and the lock path already exists in the conch
    ** we'll try to use the path there, if we can't open that path, we'll 
    ** retry with a new auto-generated path 
    */
    do { /* in case we need to try again for an :auto: named lock file */

      if( !createConch && !forceNewLockPath ){
        hostIdMatch = !memcmp(&readBuf[PROXY_HEADERLEN], myHostID, 
                                  PROXY_HOSTIDLEN);
        /* if the conch has data compare the contents */
        if( !pCtx->lockProxyPath ){
          /* for auto-named local lock file, just check the host ID and we'll
           ** use the local lock file path that's already in there
           */
          if( hostIdMatch ){
            size_t pathLen = (readLen - PROXY_PATHINDEX);
            
            if( pathLen>=MAXPATHLEN ){
              pathLen=MAXPATHLEN-1;
            }
            memcpy(lockPath, &readBuf[PROXY_PATHINDEX], pathLen);
            lockPath[pathLen] = 0;
            tempLockPath = lockPath;
            tryOldLockPath = 1;
            /* create a copy of the lock path if the conch is taken */
            goto end_takeconch;
          }
        }else if( hostIdMatch
               && !strncmp(pCtx->lockProxyPath, &readBuf[PROXY_PATHINDEX],
                           readLen-PROXY_PATHINDEX)
        ){
          /* conch host and lock path match */
          goto end_takeconch; 
        }
      }
      
      /* if the conch isn't writable and doesn't match, we can't take it */
      if( (conchFile->openFlags&O_RDWR) == 0 ){
        rc = SQLITE_BUSY;
        goto end_takeconch;
      }
      
      /* either the conch didn't match or we need to create a new one */
      if( !pCtx->lockProxyPath ){
        proxyGetLockPath(pCtx->dbPath, lockPath, MAXPATHLEN);
        tempLockPath = lockPath;
        /* create a copy of the lock path _only_ if the conch is taken */
      }
      
      /* update conch with host and path (this will fail if other process
      ** has a shared lock already), if the host id matches, use the big
      ** stick.
      */
      futimes(conchFile->h, NULL);
      if( hostIdMatch && !createConch ){
        if( conchFile->pInode && conchFile->pInode->nShared>1 ){
          /* We are trying for an exclusive lock but another thread in this
           ** same process is still holding a shared lock. */
          rc = SQLITE_BUSY;
        } else {          
          rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
        }
      }else{
        rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
      }
      if( rc==SQLITE_OK ){
        char writeBuffer[PROXY_MAXCONCHLEN];
        int writeSize = 0;
        
        writeBuffer[0] = (char)PROXY_CONCHVERSION;
        memcpy(&writeBuffer[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN);
        if( pCtx->lockProxyPath!=NULL ){
          strlcpy(&writeBuffer[PROXY_PATHINDEX], pCtx->lockProxyPath,
                  MAXPATHLEN);
        }else{
          strlcpy(&writeBuffer[PROXY_PATHINDEX], tempLockPath, MAXPATHLEN);
        }
        writeSize = PROXY_PATHINDEX + strlen(&writeBuffer[PROXY_PATHINDEX]);
        robust_ftruncate(conchFile->h, writeSize);
        rc = unixWrite((sqlite3_file *)conchFile, writeBuffer, writeSize, 0);
        fsync(conchFile->h);
        /* If we created a new conch file (not just updated the contents of a 
         ** valid conch file), try to match the permissions of the database 
         */
        if( rc==SQLITE_OK && createConch ){
          struct stat buf;
          int err = osFstat(pFile->h, &buf);
          if( err==0 ){
            mode_t cmode = buf.st_mode&(S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP |
                                        S_IROTH|S_IWOTH);
            /* try to match the database file R/W permissions, ignore failure */
#ifndef SQLITE_PROXY_DEBUG
            osFchmod(conchFile->h, cmode);
#else
            do{
              rc = osFchmod(conchFile->h, cmode);
            }while( rc==(-1) && errno==EINTR );
            if( rc!=0 ){
              int code = errno;
              fprintf(stderr, "fchmod %o FAILED with %d %s\n",
                      cmode, code, strerror(code));
            } else {
              fprintf(stderr, "fchmod %o SUCCEDED\n",cmode);
            }
          }else{
            int code = errno;
            fprintf(stderr, "STAT FAILED[%d] with %d %s\n", 
                    err, code, strerror(code));
#endif
          }
        }
      }
      conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, SHARED_LOCK);
      
    end_takeconch:
      OSTRACE(("TRANSPROXY: CLOSE  %d\n", pFile->h));
      if( rc==SQLITE_OK && pFile->openFlags ){
        int fd;
        if( pFile->h>=0 ){
          robust_close(pFile, pFile->h, __LINE__);
        }
        pFile->h = -1;
        fd = robust_open(pCtx->dbPath, pFile->openFlags, 0);
        OSTRACE(("TRANSPROXY: OPEN  %d\n", fd));
        if( fd>=0 ){
          pFile->h = fd;
        }else{
          rc=SQLITE_CANTOPEN_BKPT; /* SQLITE_BUSY? proxyTakeConch called
           during locking */
        }
      }
      if( rc==SQLITE_OK && !pCtx->lockProxy ){
        char *path = tempLockPath ? tempLockPath : pCtx->lockProxyPath;
        rc = proxyCreateUnixFile(path, &pCtx->lockProxy, 1);
        if( rc!=SQLITE_OK && rc!=SQLITE_NOMEM && tryOldLockPath ){
          /* we couldn't create the proxy lock file with the old lock file path
           ** so try again via auto-naming 
           */
          forceNewLockPath = 1;
          tryOldLockPath = 0;
          continue; /* go back to the do {} while start point, try again */
        }
      }
      if( rc==SQLITE_OK ){
        /* Need to make a copy of path if we extracted the value
         ** from the conch file or the path was allocated on the stack
         */
        if( tempLockPath ){
          pCtx->lockProxyPath = sqlite3DbStrDup(0, tempLockPath);
          if( !pCtx->lockProxyPath ){
            rc = SQLITE_NOMEM;
          }
        }
      }
      if( rc==SQLITE_OK ){
        pCtx->conchHeld = 1;
        
        if( pCtx->lockProxy->pMethod == &afpIoMethods ){
          afpLockingContext *afpCtx;
          afpCtx = (afpLockingContext *)pCtx->lockProxy->lockingContext;
          afpCtx->dbPath = pCtx->lockProxyPath;
        }
      } else {
        conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
      }
      OSTRACE(("TAKECONCH  %d %s\n", conchFile->h,
               rc==SQLITE_OK?"ok":"failed"));
      return rc;
    } while (1); /* in case we need to retry the :auto: lock file - 
                 ** we should never get here except via the 'continue' call. */
  }
}

/*
** If pFile holds a lock on a conch file, then release that lock.
*/
static int proxyReleaseConch(unixFile *pFile){
  int rc = SQLITE_OK;         /* Subroutine return code */
  proxyLockingContext *pCtx;  /* The locking context for the proxy lock */
  unixFile *conchFile;        /* Name of the conch file */

  pCtx = (proxyLockingContext *)pFile->lockingContext;
  conchFile = pCtx->conchFile;
  OSTRACE(("RELEASECONCH  %d for %s pid=%d\n", conchFile->h,
           (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"), 
           osGetpid(0)));
  if( pCtx->conchHeld>0 ){
    rc = conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
  }
  pCtx->conchHeld = 0;
  OSTRACE(("RELEASECONCH  %d %s\n", conchFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}

/*
** Given the name of a database file, compute the name of its conch file.
** Store the conch filename in memory obtained from sqlite3_malloc64().
** Make *pConchPath point to the new name.  Return SQLITE_OK on success
** or SQLITE_NOMEM if unable to obtain memory.
**
** The caller is responsible for ensuring that the allocated memory
** space is eventually freed.
**
** *pConchPath is set to NULL if a memory allocation error occurs.
*/
static int proxyCreateConchPathname(char *dbPath, char **pConchPath){
  int i;                        /* Loop counter */
  int len = (int)strlen(dbPath); /* Length of database filename - dbPath */
  char *conchPath;              /* buffer in which to construct conch name */

  /* Allocate space for the conch filename and initialize the name to
  ** the name of the original database file. */  
  *pConchPath = conchPath = (char *)sqlite3_malloc64(len + 8);
  if( conchPath==0 ){
    return SQLITE_NOMEM;
  }
  memcpy(conchPath, dbPath, len+1);
  
  /* now insert a "." before the last / character */
  for( i=(len-1); i>=0; i-- ){
    if( conchPath[i]=='/' ){
      i++;
      break;
    }
  }
  conchPath[i]='.';
  while ( i<len ){
    conchPath[i+1]=dbPath[i];
    i++;
  }

  /* append the "-conch" suffix to the file */
  memcpy(&conchPath[i+1], "-conch", 7);
  assert( (int)strlen(conchPath) == len+7 );

  return SQLITE_OK;
}


/* Takes a fully configured proxy locking-style unix file and switches
** the local lock file path 
*/
static int switchLockProxyPath(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
  char *oldPath = pCtx->lockProxyPath;
  int rc = SQLITE_OK;

  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }  

  /* nothing to do if the path is NULL, :auto: or matches the existing path */
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ||
    (oldPath && !strncmp(oldPath, path, MAXPATHLEN)) ){
    return SQLITE_OK;
  }else{
    unixFile *lockProxy = pCtx->lockProxy;
    pCtx->lockProxy=NULL;
    pCtx->conchHeld = 0;
    if( lockProxy!=NULL ){
      rc=lockProxy->pMethod->xClose((sqlite3_file *)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
    }
    sqlite3_free(oldPath);
    pCtx->lockProxyPath = sqlite3DbStrDup(0, path);
  }
  
  return rc;
}

/*
** pFile is a file that has been opened by a prior xOpen call.  dbPath
** is a string buffer at least MAXPATHLEN+1 characters in size.
**
** This routine find the filename associated with pFile and writes it
** int dbPath.
*/
static int proxyGetDbPathForUnixFile(unixFile *pFile, char *dbPath){
#if defined(__APPLE__)
  if( pFile->pMethod == &afpIoMethods ){
    /* afp style keeps a reference to the db path in the filePath field 
    ** of the struct */
    assert( (int)strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, ((afpLockingContext *)pFile->lockingContext)->dbPath,
            MAXPATHLEN);
  } else
#endif
  if( pFile->pMethod == &dotlockIoMethods ){
    /* dot lock style uses the locking context to store the dot lock
    ** file path */
    int len = strlen((char *)pFile->lockingContext) - strlen(DOTLOCK_SUFFIX);
    memcpy(dbPath, (char *)pFile->lockingContext, len + 1);
  }else{
    /* all other styles use the locking context to store the db file path */
    assert( strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, (char *)pFile->lockingContext, MAXPATHLEN);
  }
  return SQLITE_OK;
}

/*
** Takes an already filled in unix file and alters it so all file locking 
** will be performed on the local proxy lock file.  The following fields
** are preserved in the locking context so that they can be restored and 
** the unix structure properly cleaned up at close time:
**  ->lockingContext
**  ->pMethod
*/
static int proxyTransformUnixFile(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx;
  char dbPath[MAXPATHLEN+1];       /* Name of the database file */
  char *lockPath=NULL;
  int rc = SQLITE_OK;
  
  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }
  proxyGetDbPathForUnixFile(pFile, dbPath);
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ){
    lockPath=NULL;
  }else{
    lockPath=(char *)path;
  }
  
  OSTRACE(("TRANSPROXY  %d for %s pid=%d\n", pFile->h,
           (lockPath ? lockPath : ":auto:"), osGetpid(0)));

  pCtx = sqlite3_malloc64( sizeof(*pCtx) );
  if( pCtx==0 ){
    return SQLITE_NOMEM;
  }
  memset(pCtx, 0, sizeof(*pCtx));

  rc = proxyCreateConchPathname(dbPath, &pCtx->conchFilePath);
  if( rc==SQLITE_OK ){
    rc = proxyCreateUnixFile(pCtx->conchFilePath, &pCtx->conchFile, 0);
    if( rc==SQLITE_CANTOPEN && ((pFile->openFlags&O_RDWR) == 0) ){
      /* if (a) the open flags are not O_RDWR, (b) the conch isn't there, and
      ** (c) the file system is read-only, then enable no-locking access.
      ** Ugh, since O_RDONLY==0x0000 we test for !O_RDWR since unixOpen asserts
      ** that openFlags will have only one of O_RDONLY or O_RDWR.
      */
      struct statfs fsInfo;
      struct stat conchInfo;
      int goLockless = 0;

      if( osStat(pCtx->conchFilePath, &conchInfo) == -1 ) {
        int err = errno;
        if( (err==ENOENT) && (statfs(dbPath, &fsInfo) != -1) ){
          goLockless = (fsInfo.f_flags&MNT_RDONLY) == MNT_RDONLY;
        }
      }
      if( goLockless ){
        pCtx->conchHeld = -1; /* read only FS/ lockless */
        rc = SQLITE_OK;
      }
    }
  }  
  if( rc==SQLITE_OK && lockPath ){
    pCtx->lockProxyPath = sqlite3DbStrDup(0, lockPath);
  }

  if( rc==SQLITE_OK ){
    pCtx->dbPath = sqlite3DbStrDup(0, dbPath);
    if( pCtx->dbPath==NULL ){
      rc = SQLITE_NOMEM;
    }
  }
  if( rc==SQLITE_OK ){
    /* all memory is allocated, proxys are created and assigned, 
    ** switch the locking context and pMethod then return.
    */
    pCtx->oldLockingContext = pFile->lockingContext;
    pFile->lockingContext = pCtx;
    pCtx->pOldMethod = pFile->pMethod;
    pFile->pMethod = &proxyIoMethods;
  }else{
    if( pCtx->conchFile ){ 
      pCtx->conchFile->pMethod->xClose((sqlite3_file *)pCtx->conchFile);
      sqlite3_free(pCtx->conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath); 
    sqlite3_free(pCtx);
  }
  OSTRACE(("TRANSPROXY  %d %s\n", pFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}


/*
** This routine handles sqlite3_file_control() calls that are specific
** to proxy locking.
*/
static int proxyFileControl(sqlite3_file *id, int op, void *pArg){
  switch( op ){
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      if( pFile->pMethod == &proxyIoMethods ){
        proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
        proxyTakeConch(pFile);
        if( pCtx->lockProxyPath ){
          *(const char **)pArg = pCtx->lockProxyPath;
        }else{
          *(const char **)pArg = ":auto: (not held)";
        }
      } else {
        *(const char **)pArg = NULL;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      int rc = SQLITE_OK;
      int isProxyStyle = (pFile->pMethod == &proxyIoMethods);
      if( pArg==NULL || (const char *)pArg==0 ){
        if( isProxyStyle ){
          /* turn off proxy locking - not supported.  If support is added for
          ** switching proxy locking mode off then it will need to fail if
          ** the journal mode is WAL mode. 
          */
          rc = SQLITE_ERROR /*SQLITE_PROTOCOL? SQLITE_MISUSE?*/;
        }else{
          /* turn off proxy locking - already off - NOOP */
          rc = SQLITE_OK;
        }
      }else{
        const char *proxyPath = (const char *)pArg;
        if( isProxyStyle ){
          proxyLockingContext *pCtx = 
            (proxyLockingContext*)pFile->lockingContext;
          if( !strcmp(pArg, ":auto:") 
           || (pCtx->lockProxyPath &&
               !strncmp(pCtx->lockProxyPath, proxyPath, MAXPATHLEN))
          ){
            rc = SQLITE_OK;
          }else{
            rc = switchLockProxyPath(pFile, proxyPath);
          }
        }else{
          /* turn on proxy file locking */
          rc = proxyTransformUnixFile(pFile, proxyPath);
        }
      }
      return rc;
    }
    default: {
      assert( 0 );  /* The call assures that only valid opcodes are sent */
    }
  }
  /*NOTREACHED*/
  return SQLITE_ERROR;
}

/*
** Within this division (the proxying locking implementation) the procedures
** above this point are all utilities.  The lock-related methods of the
** proxy-locking sqlite3_io_method object follow.
*/


/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int proxyCheckReservedLock(sqlite3_file *id, int *pResOut) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      return proxy->pMethod->xCheckReservedLock((sqlite3_file*)proxy, pResOut);
    }else{ /* conchHeld < 0 is lockless */
      pResOut=0;
    }
  }
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int proxyLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xLock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int proxyUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xUnlock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}

/*
** Close a file that uses proxy locks.
*/
static int proxyClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    unixFile *lockProxy = pCtx->lockProxy;
    unixFile *conchFile = pCtx->conchFile;
    int rc = SQLITE_OK;
    
    if( lockProxy ){
      rc = lockProxy->pMethod->xUnlock((sqlite3_file*)lockProxy, NO_LOCK);
      if( rc ) return rc;
      rc = lockProxy->pMethod->xClose((sqlite3_file*)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
      pCtx->lockProxy = 0;
    }
    if( conchFile ){
      if( pCtx->conchHeld ){
        rc = proxyReleaseConch(pFile);
        if( rc ) return rc;
      }
      rc = conchFile->pMethod->xClose((sqlite3_file*)conchFile);
      if( rc ) return rc;
      sqlite3_free(conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath);
    sqlite3DbFree(0, pCtx->dbPath);
    /* restore the original locking context and pMethod then close it */
    pFile->lockingContext = pCtx->oldLockingContext;
    pFile->pMethod = pCtx->pOldMethod;
    sqlite3_free(pCtx);
    return pFile->pMethod->xClose(id);
  }
  return SQLITE_OK;
}



#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The proxy locking style is intended for use with AFP filesystems.
** And since AFP is only supported on MacOSX, the proxy locking is also
** restricted to MacOSX.
** 
**
******************* End of the proxy lock implementation **********************
******************************************************************************/

/*
** Initialize the operating system interface.
**
** This routine registers all VFS implementations for unix-like operating
** systems.  This routine, and the sqlite3_os_end() routine that follows,
** should be the only routines in this file that are visible from other
** files.
**
** This routine is called once during SQLite initialization and by a
** single thread.  The memory allocation and mutex subsystems have not
** necessarily been initialized when this routine is called, and so they
** should not be used.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_init(void){ 
  /* 
  ** The following macro defines an initializer for an sqlite3_vfs object.
  ** The name of the VFS is NAME.  The pAppData is a pointer to a pointer
  ** to the "finder" function.  (pAppData is a pointer to a pointer because
  ** silly C90 rules prohibit a void* from being cast to a function pointer
  ** and so we have to go through the intermediate pointer to avoid problems
  ** when compiling with -pedantic-errors on GCC.)
  **
  ** The FINDER parameter to this macro is the name of the pointer to the
  ** finder-function.  The finder-function returns a pointer to the
  ** sqlite_io_methods object that implements the desired locking
  ** behaviors.  See the division above that contains the IOMETHODS
  ** macro for addition information on finder-functions.
  **
  ** Most finders simply return a pointer to a fixed sqlite3_io_methods
  ** object.  But the "autolockIoFinder" available on MacOSX does a little
  ** more than that; it looks at the filesystem type that hosts the 
  ** database file and tries to choose an locking method appropriate for
  ** that filesystem time.
  */
  #define UNIXVFS(VFSNAME, FINDER) {                        \
    3,                    /* iVersion */                    \
    sizeof(unixFile),     /* szOsFile */                    \
    MAX_PATHNAME,         /* mxPathname */                  \
    0,                    /* pNext */                       \
    VFSNAME,              /* zName */                       \
    (void*)&FINDER,       /* pAppData */                    \
    unixOpen,             /* xOpen */                       \
    unixDelete,           /* xDelete */                     \
    unixAccess,           /* xAccess */                     \
    unixFullPathname,     /* xFullPathname */               \
    unixDlOpen,           /* xDlOpen */                     \
    unixDlError,          /* xDlError */                    \
    unixDlSym,            /* xDlSym */                      \
    unixDlClose,          /* xDlClose */                    \
    unixRandomness,       /* xRandomness */                 \
    unixSleep,            /* xSleep */                      \
    unixCurrentTime,      /* xCurrentTime */                \
    unixGetLastError,     /* xGetLastError */               \
    unixCurrentTimeInt64, /* xCurrentTimeInt64 */           \
    unixSetSystemCall,    /* xSetSystemCall */              \
    unixGetSystemCall,    /* xGetSystemCall */              \
    unixNextSystemCall,   /* xNextSystemCall */             \
  }

  /*
  ** All default VFSes for unix are contained in the following array.
  **
  ** Note that the sqlite3_vfs.pNext field of the VFS object is modified
  ** by the SQLite core when the VFS is registered.  So the following
  ** array cannot be const.
  */
  static sqlite3_vfs aVfs[] = {
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix",          autolockIoFinder ),
#elif OS_VXWORKS
    UNIXVFS("unix",          vxworksIoFinder ),
#else
    UNIXVFS("unix",          posixIoFinder ),
#endif
    UNIXVFS("unix-none",     nolockIoFinder ),
    UNIXVFS("unix-dotfile",  dotlockIoFinder ),
    UNIXVFS("unix-excl",     posixIoFinder ),
#if OS_VXWORKS
    UNIXVFS("unix-namedsem", semIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE || OS_VXWORKS
    UNIXVFS("unix-posix",    posixIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE
    UNIXVFS("unix-flock",    flockIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix-afp",      afpIoFinder ),
    UNIXVFS("unix-nfs",      nfsIoFinder ),
    UNIXVFS("unix-proxy",    proxyIoFinder ),
#endif
  };
  unsigned int i;          /* Loop counter */

  /* Double-check that the aSyscall[] array has been constructed
  ** correctly.  See ticket [bb3a86e890c8e96ab] */
  assert( ArraySize(aSyscall)==25 );

  /* Register all VFSes defined in the aVfs[] array */
  for(i=0; i<(sizeof(aVfs)/sizeof(sqlite3_vfs)); i++){
    sqlite3_vfs_register(&aVfs[i], i==0);
  }
  return SQLITE_OK; 
}

/*
** Shutdown the operating system interface.
**
** Some operating systems might need to do some cleanup in this routine,
** to release dynamically allocated objects.  But not on unix.
** This routine is a no-op for unix.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_end(void){ 
  return SQLITE_OK; 
}
 
#endif /* SQLITE_OS_UNIX */

/************** End of os_unix.c *********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
******************************************************************************/

/******************************************************************************
****************************** No-op Locking **********************************
**
** Of the various locking implementations available, this is by far the
** simplest:  locking is ignored.  No attempt is made to lock the database
** file for reading or writing.
**
** This locking mode is appropriate for use on read-only databases
** (ex: databases that are burned into CD-ROM, for example.)  It can
** also be used if the application employs some external mechanism to
** prevent simultaneous access of the same database by two or more
** database connections.  But there is a serious risk of database
** corruption if this locking mode is used in situations where multiple
** database connections are accessing the same database file at the same
** time and one or more of those connections are writing.
*/

static int nolockCheckReservedLock(sqlite3_file *NotUsed, int *pResOut){
  UNUSED_PARAMETER(NotUsed);
  *pResOut = 0;
  return SQLITE_OK;
}
static int nolockLock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}
static int nolockUnlock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}

/*
** Close the file.
*/
static int nolockClose(sqlite3_file *id) {
  return closeUnixFile(id);
}

/******************* End of the no-op lock implementation *********************
******************************************************************************/

/******************************************************************************
************************* Begin dot-file Locking ******************************
**
** The dotfile locking implementation uses the existence of separate lock
** files (really a directory) to control access to the database.  This works
** on just about every filesystem imaginable.  But there are serious downsides:
**
**    (1)  There is zero concurrency.  A single reader blocks all other
**         connections from reading or writing the database.
**
**    (2)  An application crash or power loss can leave stale lock files
**         sitting around that need to be cleared manually.
**
** Nevertheless, a dotlock is an appropriate locking mode for use if no
** other locking strategy is available.
**
** Dotfile locking works by creating a subdirectory in the same directory as
** the database and with the same name but with a ".lock" extension added.
** The existence of a lock directory implies an EXCLUSIVE lock.  All other
** lock types (SHARED, RESERVED, PENDING) are mapped into EXCLUSIVE.
*/

/*
** The file suffix added to the data base filename in order to create the
** lock directory.
*/
#define DOTLOCK_SUFFIX ".lock"

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
**
** In dotfile locking, either a lock exists or it does not.  So in this
** variation of CheckReservedLock(), *pResOut is set to true if any lock
** is held on the file and false if the file is unlocked.
*/
static int dotlockCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    /* Either this connection or some other connection in the same process
    ** holds a lock on the file.  No need to check further. */
    reserved = 1;
  }else{
    /* The lock is held if and only if the lockfile exists */
    const char *zLockFile = (const char*)pFile->lockingContext;
    reserved = osAccess(zLockFile, 0)==0;
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (dotlock)\n", pFile->h, rc, reserved));
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
**
** With dotfile locking, we really only support state (4): EXCLUSIVE.
** But we track the other locking levels internally.
*/
static int dotlockLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc = SQLITE_OK;


  /* If we have any lock, then the lock file already exists.  All we have
  ** to do is adjust our internal record of the lock level.
  */
  if( pFile->eFileLock > NO_LOCK ){
    pFile->eFileLock = eFileLock;
    /* Always update the timestamp on the old file */
#ifdef HAVE_UTIME
    utime(zLockFile, NULL);
#else
    utimes(zLockFile, NULL);
#endif
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  rc = osMkdir(zLockFile, 0777);
  if( rc<0 ){
    /* failed to open/create the lock directory */
    int tErrno = errno;
    if( EEXIST == tErrno ){
      rc = SQLITE_BUSY;
    } else {
      rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
      if( IS_LOCK_ERROR(rc) ){
        storeLastErrno(pFile, tErrno);
      }
    }
    return rc;
  } 
  
  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
**
** When the locking level reaches NO_LOCK, delete the lock file.
*/
static int dotlockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc;

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (dotlock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }

  /* To downgrade to shared, simply update our internal notion of the
  ** lock state.  No need to mess with the file on disk.
  */
  if( eFileLock==SHARED_LOCK ){
    pFile->eFileLock = SHARED_LOCK;
    return SQLITE_OK;
  }
  
  /* To fully unlock the database, delete the lock file */
  assert( eFileLock==NO_LOCK );
  rc = osRmdir(zLockFile);
  if( rc<0 && errno==ENOTDIR ) rc = osUnlink(zLockFile);
  if( rc<0 ){
    int tErrno = errno;
    rc = 0;
    if( ENOENT != tErrno ){
      rc = SQLITE_IOERR_UNLOCK;
    }
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
** Close a file.  Make sure the lock has been released before closing.
*/
static int dotlockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    dotlockUnlock(id, NO_LOCK);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
  }
  return rc;
}
/****************** End of the dot-file lock implementation *******************
******************************************************************************/

/******************************************************************************
************************** Begin flock Locking ********************************
**
** Use the flock() system call to do file locking.
**
** flock() locking is like dot-file locking in that the various
** fine-grain locking levels supported by SQLite are collapsed into
** a single exclusive lock.  In other words, SHARED, RESERVED, and
** PENDING locks are the same thing as an EXCLUSIVE lock.  SQLite
** still works when you do this, but concurrency is reduced since
** only a single process can be reading the database at a time.
**
** Omit this section if SQLITE_ENABLE_LOCKING_STYLE is turned off
*/
#if SQLITE_ENABLE_LOCKING_STYLE

/*
** Retry flock() calls that fail with EINTR
*/
#ifdef EINTR
static int robust_flock(int fd, int op){
  int rc;
  do{ rc = flock(fd,op); }while( rc<0 && errno==EINTR );
  return rc;
}
#else
# define robust_flock(a,b) flock(a,b)
#endif
     

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int flockCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    /* attempt to get the lock */
    int lrc = robust_flock(pFile->h, LOCK_EX | LOCK_NB);
    if( !lrc ){
      /* got the lock, unlock it */
      lrc = robust_flock(pFile->h, LOCK_UN);
      if ( lrc ) {
        int tErrno = errno;
        /* unlock failed with an error */
        lrc = SQLITE_IOERR_UNLOCK; 
        if( IS_LOCK_ERROR(lrc) ){
          storeLastErrno(pFile, tErrno);
          rc = lrc;
        }
      }
    } else {
      int tErrno = errno;
      reserved = 1;
      /* someone else might have it reserved */
      lrc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK); 
      if( IS_LOCK_ERROR(lrc) ){
        storeLastErrno(pFile, tErrno);
        rc = lrc;
      }
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (flock)\n", pFile->h, rc, reserved));

#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_OK;
    reserved=1;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** flock() only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int flockLock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;

  assert( pFile );

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  
  if (robust_flock(pFile->h, LOCK_EX | LOCK_NB)) {
    int tErrno = errno;
    /* didn't get, must be busy */
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
  } else {
    /* got it, set the type and return ok */
    pFile->eFileLock = eFileLock;
  }
  OSTRACE(("LOCK    %d %s %s (flock)\n", pFile->h, azFileLock(eFileLock), 
           rc==SQLITE_OK ? "ok" : "failed"));
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_BUSY;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int flockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  
  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (flock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really, unlock. */
  if( robust_flock(pFile->h, LOCK_UN) ){
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
    return SQLITE_OK;
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
    return SQLITE_IOERR_UNLOCK;
  }else{
    pFile->eFileLock = NO_LOCK;
    return SQLITE_OK;
  }
}

/*
** Close a file.
*/
static int flockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    flockUnlock(id, NO_LOCK);
    rc = closeUnixFile(id);
  }
  return rc;
}

#endif /* SQLITE_ENABLE_LOCKING_STYLE && !OS_VXWORK */

/******************* End of the flock lock implementation *********************
******************************************************************************/

/******************************************************************************
************************ Begin Named Semaphore Locking ************************
**
** Named semaphore locking is only supported on VxWorks.
**
** Semaphore locking is like dot-lock and flock in that it really only
** supports EXCLUSIVE locking.  Only a single process can read or write
** the database file at a time.  This reduces potential concurrency, but
** makes the lock implementation much easier.
*/
#if OS_VXWORKS

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int semXCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    sem_t *pSem = pFile->pInode->pSem;

    if( sem_trywait(pSem)==-1 ){
      int tErrno = errno;
      if( EAGAIN != tErrno ){
        rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_CHECKRESERVEDLOCK);
        storeLastErrno(pFile, tErrno);
      } else {
        /* someone else has the lock when we are in NO_LOCK */
        reserved = (pFile->eFileLock < SHARED_LOCK);
      }
    }else{
      /* we could have it if we want it */
      sem_post(pSem);
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (sem)\n", pFile->h, rc, reserved));

  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** Semaphore locks only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int semXLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;
  int rc = SQLITE_OK;

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    rc = SQLITE_OK;
    goto sem_end_lock;
  }
  
  /* lock semaphore now but bail out when already locked. */
  if( sem_trywait(pSem)==-1 ){
    rc = SQLITE_BUSY;
    goto sem_end_lock;
  }

  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;

 sem_end_lock:
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int semXUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;

  assert( pFile );
  assert( pSem );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (sem)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really unlock. */
  if ( sem_post(pSem)==-1 ) {
    int rc, tErrno = errno;
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_UNLOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
 ** Close a file.
 */
static int semXClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    semXUnlock(id, NO_LOCK);
    assert( pFile );
    unixEnterMutex();
    releaseInodeInfo(pFile);
    unixLeaveMutex();
    closeUnixFile(id);
  }
  return SQLITE_OK;
}

#endif /* OS_VXWORKS */
/*
** Named semaphore locking is only available on VxWorks.
**
*************** End of the named semaphore lock implementation ****************
******************************************************************************/


/******************************************************************************
*************************** Begin AFP Locking *********************************
**
** AFP is the Apple Filing Protocol.  AFP is a network filesystem found
** on Apple Macintosh computers - both OS9 and OSX.
**
** Third-party implementations of AFP are available.  But this code here
** only works on OSX.
*/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
** The afpLockingContext structure contains all afp lock specific state
*/
typedef struct afpLockingContext afpLockingContext;
struct afpLockingContext {
  int reserved;
  const char *dbPath;             /* Name of the open file */
};

struct ByteRangeLockPB2
{
  unsigned long long offset;        /* offset to first byte to lock */
  unsigned long long length;        /* nbr of bytes to lock */
  unsigned long long retRangeStart; /* nbr of 1st byte locked if successful */
  unsigned char unLockFlag;         /* 1 = unlock, 0 = lock */
  unsigned char startEndFlag;       /* 1=rel to end of fork, 0=rel to start */
  int fd;                           /* file desc to assoc this lock with */
};

#define afpfsByteRangeLock2FSCTL        _IOWR('z', 23, struct ByteRangeLockPB2)

/*
** This is a utility for setting or clearing a bit-range lock on an
** AFP filesystem.
** 
** Return SQLITE_OK on success, SQLITE_BUSY on failure.
*/
static int afpSetLock(
  const char *path,              /* Name of the file to be locked or unlocked */
  unixFile *pFile,               /* Open file descriptor on path */
  unsigned long long offset,     /* First byte to be locked */
  unsigned long long length,     /* Number of bytes to lock */
  int setLockFlag                /* True to set lock.  False to clear lock */
){
  struct ByteRangeLockPB2 pb;
  int err;
  
  pb.unLockFlag = setLockFlag ? 0 : 1;
  pb.startEndFlag = 0;
  pb.offset = offset;
  pb.length = length; 
  pb.fd = pFile->h;
  
  OSTRACE(("AFPSETLOCK [%s] for %d%s in range %llx:%llx\n", 
    (setLockFlag?"ON":"OFF"), pFile->h, (pb.fd==-1?"[testval-1]":""),
    offset, length));
  err = fsctl(path, afpfsByteRangeLock2FSCTL, &pb, 0);
  if ( err==-1 ) {
    int rc;
    int tErrno = errno;
    OSTRACE(("AFPSETLOCK failed to fsctl() '%s' %d %s\n",
             path, tErrno, strerror(tErrno)));
#ifdef SQLITE_IGNORE_AFP_LOCK_ERRORS
    rc = SQLITE_BUSY;
#else
    rc = sqliteErrorFromPosixError(tErrno,
                    setLockFlag ? SQLITE_IOERR_LOCK : SQLITE_IOERR_UNLOCK);
#endif /* SQLITE_IGNORE_AFP_LOCK_ERRORS */
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc;
  } else {
    return SQLITE_OK;
  }
}

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int afpCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  afpLockingContext *context;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  context = (afpLockingContext *) pFile->lockingContext;
  if( context->reserved ){
    *pResOut = 1;
    return SQLITE_OK;
  }
  unixEnterMutex(); /* Because pFile->pInode is shared across threads */
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->pInode->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it.
   */
  if( !reserved ){
    /* lock the RESERVED byte */
    int lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);  
    if( SQLITE_OK==lrc ){
      /* if we succeeded in taking the reserved lock, unlock it to restore
      ** the original state */
      lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
    } else {
      /* if we failed to get the lock then someone else must have it */
      reserved = 1;
    }
    if( IS_LOCK_ERROR(lrc) ){
      rc=lrc;
    }
  }
  
  unixLeaveMutex();
  OSTRACE(("TEST WR-LOCK %d %d %d (afp)\n", pFile->h, rc, reserved));
  
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int afpLock(sqlite3_file *id, int eFileLock){
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode = pFile->pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  
  assert( pFile );
  OSTRACE(("LOCK    %d %s was %s(%s,%d) pid=%d (afp)\n", pFile->h,
           azFileLock(eFileLock), azFileLock(pFile->eFileLock),
           azFileLock(pInode->eFileLock), pInode->nShared , osGetpid(0)));

  /* If there is already a lock of this type or more restrictive on the
  ** unixFile, do nothing. Don't use the afp_end_lock: exit path, as
  ** unixEnterMutex() hasn't been called yet.
  */
  if( pFile->eFileLock>=eFileLock ){
    OSTRACE(("LOCK    %d %s ok (already held) (afp)\n", pFile->h,
           azFileLock(eFileLock)));
    return SQLITE_OK;
  }

  /* Make sure the locking sequence is correct
  **  (1) We never move from unlocked to anything higher than shared lock.
  **  (2) SQLite never explicitly requests a pendig lock.
  **  (3) A shared lock is always held when a reserve lock is requested.
  */
  assert( pFile->eFileLock!=NO_LOCK || eFileLock==SHARED_LOCK );
  assert( eFileLock!=PENDING_LOCK );
  assert( eFileLock!=RESERVED_LOCK || pFile->eFileLock==SHARED_LOCK );
  
  /* This mutex is needed because pFile->pInode is shared across threads
  */
  unixEnterMutex();
  pInode = pFile->pInode;

  /* If some thread using this PID has a lock via a different unixFile*
  ** handle that precludes the requested lock, return BUSY.
  */
  if( (pFile->eFileLock!=pInode->eFileLock && 
       (pInode->eFileLock>=PENDING_LOCK || eFileLock>SHARED_LOCK))
     ){
    rc = SQLITE_BUSY;
    goto afp_end_lock;
  }
  
  /* If a SHARED lock is requested, and some thread using this PID already
  ** has a SHARED or RESERVED lock, then increment reference counts and
  ** return SQLITE_OK.
  */
  if( eFileLock==SHARED_LOCK && 
     (pInode->eFileLock==SHARED_LOCK || pInode->eFileLock==RESERVED_LOCK) ){
    assert( eFileLock==SHARED_LOCK );
    assert( pFile->eFileLock==0 );
    assert( pInode->nShared>0 );
    pFile->eFileLock = SHARED_LOCK;
    pInode->nShared++;
    pInode->nLock++;
    goto afp_end_lock;
  }
    
  /* A PENDING lock is needed before acquiring a SHARED lock and before
  ** acquiring an EXCLUSIVE lock.  For the SHARED lock, the PENDING will
  ** be released.
  */
  if( eFileLock==SHARED_LOCK 
      || (eFileLock==EXCLUSIVE_LOCK && pFile->eFileLock<PENDING_LOCK)
  ){
    int failed;
    failed = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 1);
    if (failed) {
      rc = failed;
      goto afp_end_lock;
    }
  }
  
  /* If control gets to this point, then actually go ahead and make
  ** operating system calls for the specified lock.
  */
  if( eFileLock==SHARED_LOCK ){
    int lrc1, lrc2, lrc1Errno = 0;
    long lk, mask;
    
    assert( pInode->nShared==0 );
    assert( pInode->eFileLock==0 );
        
    mask = (sizeof(long)==8) ? LARGEST_INT64 : 0x7fffffff;
    /* Now get the read-lock SHARED_LOCK */
    /* note that the quality of the randomness doesn't matter that much */
    lk = random(); 
    pInode->sharedByte = (lk & mask)%(SHARED_SIZE - 1);
    lrc1 = afpSetLock(context->dbPath, pFile, 
          SHARED_FIRST+pInode->sharedByte, 1, 1);
    if( IS_LOCK_ERROR(lrc1) ){
      lrc1Errno = pFile->lastErrno;
    }
    /* Drop the temporary PENDING lock */
    lrc2 = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    
    if( IS_LOCK_ERROR(lrc1) ) {
      storeLastErrno(pFile, lrc1Errno);
      rc = lrc1;
      goto afp_end_lock;
    } else if( IS_LOCK_ERROR(lrc2) ){
      rc = lrc2;
      goto afp_end_lock;
    } else if( lrc1 != SQLITE_OK ) {
      rc = lrc1;
    } else {
      pFile->eFileLock = SHARED_LOCK;
      pInode->nLock++;
      pInode->nShared = 1;
    }
  }else if( eFileLock==EXCLUSIVE_LOCK && pInode->nShared>1 ){
    /* We are trying for an exclusive lock but another thread in this
     ** same process is still holding a shared lock. */
    rc = SQLITE_BUSY;
  }else{
    /* The request was for a RESERVED or EXCLUSIVE lock.  It is
    ** assumed that there is a SHARED or greater lock on the file
    ** already.
    */
    int failed = 0;
    assert( 0!=pFile->eFileLock );
    if (eFileLock >= RESERVED_LOCK && pFile->eFileLock < RESERVED_LOCK) {
        /* Acquire a RESERVED lock */
        failed = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);
      if( !failed ){
        context->reserved = 1;
      }
    }
    if (!failed && eFileLock == EXCLUSIVE_LOCK) {
      /* Acquire an EXCLUSIVE lock */
        
      /* Remove the shared lock before trying the range.  we'll need to 
      ** reestablish the shared lock if we can't get the  afpUnlock
      */
      if( !(failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST +
                         pInode->sharedByte, 1, 0)) ){
        int failed2 = SQLITE_OK;
        /* now attemmpt to get the exclusive lock range */
        failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST, 
                               SHARED_SIZE, 1);
        if( failed && (failed2 = afpSetLock(context->dbPath, pFile, 
                       SHARED_FIRST + pInode->sharedByte, 1, 1)) ){
          /* Can't reestablish the shared lock.  Sqlite can't deal, this is
          ** a critical I/O error
          */
          rc = ((failed & SQLITE_IOERR) == SQLITE_IOERR) ? failed2 : 
               SQLITE_IOERR_LOCK;
          goto afp_end_lock;
        } 
      }else{
        rc = failed; 
      }
    }
    if( failed ){
      rc = failed;
    }
  }
  
  if( rc==SQLITE_OK ){
    pFile->eFileLock = eFileLock;
    pInode->eFileLock = eFileLock;
  }else if( eFileLock==EXCLUSIVE_LOCK ){
    pFile->eFileLock = PENDING_LOCK;
    pInode->eFileLock = PENDING_LOCK;
  }
  
afp_end_lock:
  unixLeaveMutex();
  OSTRACE(("LOCK    %d %s %s (afp)\n", pFile->h, azFileLock(eFileLock), 
         rc==SQLITE_OK ? "ok" : "failed"));
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int afpUnlock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  int skipShared = 0;
#ifdef SQLITE_TEST
  int h = pFile->h;
#endif

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d(%d,%d) pid=%d (afp)\n", pFile->h, eFileLock,
           pFile->eFileLock, pFile->pInode->eFileLock, pFile->pInode->nShared,
           osGetpid(0)));

  assert( eFileLock<=SHARED_LOCK );
  if( pFile->eFileLock<=eFileLock ){
    return SQLITE_OK;
  }
  unixEnterMutex();
  pInode = pFile->pInode;
  assert( pInode->nShared!=0 );
  if( pFile->eFileLock>SHARED_LOCK ){
    assert( pInode->eFileLock==pFile->eFileLock );
    SimulateIOErrorBenign(1);
    SimulateIOError( h=(-1) )
    SimulateIOErrorBenign(0);
    
#ifdef SQLITE_DEBUG
    /* When reducing a lock such that other processes can start
    ** reading the database file again, make sure that the
    ** transaction counter was updated if any part of the database
    ** file changed.  If the transaction counter is not updated,
    ** other connections to the same file might not realize that
    ** the file has changed and hence might not know to flush their
    ** cache.  The use of a stale cache can lead to database corruption.
    */
    assert( pFile->inNormalWrite==0
           || pFile->dbUpdate==0
           || pFile->transCntrChng==1 );
    pFile->inNormalWrite = 0;
#endif
    
    if( pFile->eFileLock==EXCLUSIVE_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, SHARED_FIRST, SHARED_SIZE, 0);
      if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1) ){
        /* only re-establish the shared lock if necessary */
        int sharedLockByte = SHARED_FIRST+pInode->sharedByte;
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 1);
      } else {
        skipShared = 1;
      }
    }
    if( rc==SQLITE_OK && pFile->eFileLock>=PENDING_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    } 
    if( rc==SQLITE_OK && pFile->eFileLock>=RESERVED_LOCK && context->reserved ){
      rc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
      if( !rc ){ 
        context->reserved = 0; 
      }
    }
    if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1)){
      pInode->eFileLock = SHARED_LOCK;
    }
  }
  if( rc==SQLITE_OK && eFileLock==NO_LOCK ){

    /* Decrement the shared lock counter.  Release the lock using an
    ** OS call only when all threads in this same process have released
    ** the lock.
    */
    unsigned long long sharedLockByte = SHARED_FIRST+pInode->sharedByte;
    pInode->nShared--;
    if( pInode->nShared==0 ){
      SimulateIOErrorBenign(1);
      SimulateIOError( h=(-1) )
      SimulateIOErrorBenign(0);
      if( !skipShared ){
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 0);
      }
      if( !rc ){
        pInode->eFileLock = NO_LOCK;
        pFile->eFileLock = NO_LOCK;
      }
    }
    if( rc==SQLITE_OK ){
      pInode->nLock--;
      assert( pInode->nLock>=0 );
      if( pInode->nLock==0 ){
        closePendingFds(pFile);
      }
    }
  }
  
  unixLeaveMutex();
  if( rc==SQLITE_OK ) pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Close a file & cleanup AFP specific locking context 
*/
static int afpClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    afpUnlock(id, NO_LOCK);
    unixEnterMutex();
    if( pFile->pInode && pFile->pInode->nLock ){
      /* If there are outstanding locks, do not actually close the file just
      ** yet because that would clear those locks.  Instead, add the file
      ** descriptor to pInode->aPending.  It will be automatically closed when
      ** the last lock is cleared.
      */
      setPendingFd(pFile);
    }
    releaseInodeInfo(pFile);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
    unixLeaveMutex();
  }
  return rc;
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the AFP lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  If you don't compile for a mac, then the "unix-afp"
** VFS is not available.
**
********************* End of the AFP lock implementation **********************
******************************************************************************/

/******************************************************************************
*************************** Begin NFS Locking ********************************/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
 ** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
 ** must be either NO_LOCK or SHARED_LOCK.
 **
 ** If the locking level of the file descriptor is already at or below
 ** the requested locking level, this routine is a no-op.
 */
static int nfsUnlock(sqlite3_file *id, int eFileLock){
  return posixUnlock(id, eFileLock, 1);
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the NFS lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  
**
********************* End of the NFS lock implementation **********************
******************************************************************************/

/******************************************************************************
**************** Non-locking sqlite3_file methods *****************************
**
** The next division contains implementations for all methods of the 
** sqlite3_file object other than the locking methods.  The locking
** methods were defined in divisions above (one locking method per
** division).  Those methods that are common to all locking modes
** are gather together into this division.
*/

/*
** Seek to the offset passed as the second argument, then read cnt 
** bytes into pBuf. Return the number of bytes actually read.
**
** NB:  If you define USE_PREAD or USE_PREAD64, then it might also
** be necessary to define _XOPEN_SOURCE to be 500.  This varies from
** one system to another.  Since SQLite does not define USE_PREAD
** in any form by default, we will not attempt to define _XOPEN_SOURCE.
** See tickets #2741 and #2681.
**
** To avoid stomping the errno value on a failed read the lastErrno value
** is set before returning.
*/
static int seekAndRead(unixFile *id, sqlite3_int64 offset, void *pBuf, int cnt){
  int got;
  int prior = 0;
#if (!defined(USE_PREAD) && !defined(USE_PREAD64))
  i64 newOffset;
#endif
  TIMER_START;
  assert( cnt==(cnt&0x1ffff) );
  assert( id->h>2 );
  do{
#if defined(USE_PREAD)
    got = osPread(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#elif defined(USE_PREAD64)
    got = osPread64(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#else
    newOffset = lseek(id->h, offset, SEEK_SET);
    SimulateIOError( newOffset-- );
    if( newOffset!=offset ){
      if( newOffset == -1 ){
        storeLastErrno((unixFile*)id, errno);
      }else{
        storeLastErrno((unixFile*)id, 0);
      }
      return -1;
    }
    got = osRead(id->h, pBuf, cnt);
#endif
    if( got==cnt ) break;
    if( got<0 ){
      if( errno==EINTR ){ got = 1; continue; }
      prior = 0;
      storeLastErrno((unixFile*)id,  errno);
      break;
    }else if( got>0 ){
      cnt -= got;
      offset += got;
      prior += got;
      pBuf = (void*)(got + (char*)pBuf);
    }
  }while( got>0 );
  TIMER_END;
  OSTRACE(("READ    %-3d %5d %7lld %llu\n",
            id->h, got+prior, offset-prior, TIMER_ELAPSED));
  return got+prior;
}

/*
** Read data from a file into a buffer.  Return SQLITE_OK if all
** bytes were read successfully and SQLITE_IOERR if anything goes
** wrong.
*/
static int unixRead(
  sqlite3_file *id, 
  void *pBuf, 
  int amt,
  sqlite3_int64 offset
){
  unixFile *pFile = (unixFile *)id;
  int got;
  assert( id );
  assert( offset>=0 );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this read request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif

  got = seekAndRead(pFile, offset, pBuf, amt);
  if( got==amt ){
    return SQLITE_OK;
  }else if( got<0 ){
    /* lastErrno set by seekAndRead */
    return SQLITE_IOERR_READ;
  }else{
    storeLastErrno(pFile, 0);   /* not a system error */
    /* Unread parts of the buffer must be zero-filled */
    memset(&((char*)pBuf)[got], 0, amt-got);
    return SQLITE_IOERR_SHORT_READ;
  }
}

/*
** Attempt to seek the file-descriptor passed as the first argument to
** absolute offset iOff, then attempt to write nBuf bytes of data from
** pBuf to it. If an error occurs, return -1 and set *piErrno. Otherwise, 
** return the actual number of bytes written (which may be less than
** nBuf).
*/
static int seekAndWriteFd(
  int fd,                         /* File descriptor to write to */
  i64 iOff,                       /* File offset to begin writing at */
  const void *pBuf,               /* Copy data from this buffer to the file */
  int nBuf,                       /* Size of buffer pBuf in bytes */
  int *piErrno                    /* OUT: Error number if error occurs */
){
  int rc = 0;                     /* Value returned by system call */

  assert( nBuf==(nBuf&0x1ffff) );
  assert( fd>2 );
  nBuf &= 0x1ffff;
  TIMER_START;

#if defined(USE_PREAD)
  do{ rc = (int)osPwrite(fd, pBuf, nBuf, iOff); }while( rc<0 && errno==EINTR );
#elif defined(USE_PREAD64)
  do{ rc = (int)osPwrite64(fd, pBuf, nBuf, iOff);}while( rc<0 && errno==EINTR);
#else
  do{
    i64 iSeek = lseek(fd, iOff, SEEK_SET);
    SimulateIOError( iSeek-- );

    if( iSeek!=iOff ){
      if( piErrno ) *piErrno = (iSeek==-1 ? errno : 0);
      return -1;
    }
    rc = osWrite(fd, pBuf, nBuf);
  }while( rc<0 && errno==EINTR );
#endif

  TIMER_END;
  OSTRACE(("WRITE   %-3d %5d %7lld %llu\n", fd, rc, iOff, TIMER_ELAPSED));

  if( rc<0 && piErrno ) *piErrno = errno;
  return rc;
}


/*
** Seek to the offset in id->offset then read cnt bytes into pBuf.
** Return the number of bytes actually read.  Update the offset.
**
** To avoid stomping the errno value on a failed write the lastErrno value
** is set before returning.
*/
static int seekAndWrite(unixFile *id, i64 offset, const void *pBuf, int cnt){
  return seekAndWriteFd(id->h, offset, pBuf, cnt, &id->lastErrno);
}


/*
** Write data from a buffer into a file.  Return SQLITE_OK on success
** or some other error code on failure.
*/
static int unixWrite(
  sqlite3_file *id, 
  const void *pBuf, 
  int amt,
  sqlite3_int64 offset 
){
  unixFile *pFile = (unixFile*)id;
  int wrote = 0;
  assert( id );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#ifdef SQLITE_DEBUG
  /* If we are doing a normal write to a database file (as opposed to
  ** doing a hot-journal rollback or a write to some file other than a
  ** normal database file) then record the fact that the database
  ** has changed.  If the transaction counter is modified, record that
  ** fact too.
  */
  if( pFile->inNormalWrite ){
    pFile->dbUpdate = 1;  /* The database has been modified */
    if( offset<=24 && offset+amt>=27 ){
      int rc;
      char oldCntr[4];
      SimulateIOErrorBenign(1);
      rc = seekAndRead(pFile, 24, oldCntr, 4);
      SimulateIOErrorBenign(0);
      if( rc!=4 || memcmp(oldCntr, &((char*)pBuf)[24-offset], 4)!=0 ){
        pFile->transCntrChng = 1;  /* The transaction counter has changed */
      }
    }
  }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this write request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif
 
  while( (wrote = seekAndWrite(pFile, offset, pBuf, amt))<amt && wrote>0 ){
    amt -= wrote;
    offset += wrote;
    pBuf = &((char*)pBuf)[wrote];
  }
  SimulateIOError(( wrote=(-1), amt=1 ));
  SimulateDiskfullError(( wrote=0, amt=1 ));

  if( amt>wrote ){
    if( wrote<0 && pFile->lastErrno!=ENOSPC ){
      /* lastErrno set by seekAndWrite */
      return SQLITE_IOERR_WRITE;
    }else{
      storeLastErrno(pFile, 0); /* not a system error */
      return SQLITE_FULL;
    }
  }

  return SQLITE_OK;
}

#ifdef SQLITE_TEST
/*
** Count the number of fullsyncs and normal syncs.  This is used to test
** that syncs and fullsyncs are occurring at the right times.
*/
SQLITE_API int sqlite3_sync_count = 0;
SQLITE_API int sqlite3_fullsync_count = 0;
#endif

/*
** We do not trust systems to provide a working fdatasync().  Some do.
** Others do no.  To be safe, we will stick with the (slightly slower)
** fsync(). If you know that your system does support fdatasync() correctly,
** then simply compile with -Dfdatasync=fdatasync or -DHAVE_FDATASYNC
*/
#if !defined(fdatasync) && !HAVE_FDATASYNC
# define fdatasync fsync
#endif

/*
** Define HAVE_FULLFSYNC to 0 or 1 depending on whether or not
** the F_FULLFSYNC macro is defined.  F_FULLFSYNC is currently
** only available on Mac OS X.  But that could change.
*/
#ifdef F_FULLFSYNC
# define HAVE_FULLFSYNC 1
#else
# define HAVE_FULLFSYNC 0
#endif


/*
** The fsync() system call does not work as advertised on many
** unix systems.  The following procedure is an attempt to make
** it work better.
**
** The SQLITE_NO_SYNC macro disables all fsync()s.  This is useful
** for testing when we want to run through the test suite quickly.
** You are strongly advised *not* to deploy with SQLITE_NO_SYNC
** enabled, however, since with SQLITE_NO_SYNC enabled, an OS crash
** or power failure will likely corrupt the database file.
**
** SQLite sets the dataOnly flag if the size of the file is unchanged.
** The idea behind dataOnly is that it should only write the file content
** to disk, not the inode.  We only set dataOnly if the file size is 
** unchanged since the file size is part of the inode.  However, 
** Ted Ts'o tells us that fdatasync() will also write the inode if the
** file size has changed.  The only real difference between fdatasync()
** and fsync(), Ted tells us, is that fdatasync() will not flush the
** inode if the mtime or owner or other inode attributes have changed.
** We only care about the file size, not the other file attributes, so
** as far as SQLite is concerned, an fdatasync() is always adequate.
** So, we always use fdatasync() if it is available, regardless of
** the value of the dataOnly flag.
*/
static int full_fsync(int fd, int fullSync, int dataOnly){
  int rc;

  /* The following "ifdef/elif/else/" block has the same structure as
  ** the one below. It is replicated here solely to avoid cluttering 
  ** up the real code with the UNUSED_PARAMETER() macros.
  */
#ifdef SQLITE_NO_SYNC
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#elif HAVE_FULLFSYNC
  UNUSED_PARAMETER(dataOnly);
#else
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#endif

  /* Record the number of times that we do a normal fsync() and 
  ** FULLSYNC.  This is used during testing to verify that this procedure
  ** gets called with the correct arguments.
  */
#ifdef SQLITE_TEST
  if( fullSync ) sqlite3_fullsync_count++;
  sqlite3_sync_count++;
#endif

  /* If we compiled with the SQLITE_NO_SYNC flag, then syncing is a
  ** no-op
  */
#ifdef SQLITE_NO_SYNC
  rc = SQLITE_OK;
#elif HAVE_FULLFSYNC
  if( fullSync ){
    rc = osFcntl(fd, F_FULLFSYNC, 0);
  }else{
    rc = 1;
  }
  /* If the FULLFSYNC failed, fall back to attempting an fsync().
  ** It shouldn't be possible for fullfsync to fail on the local 
  ** file system (on OSX), so failure indicates that FULLFSYNC
  ** isn't supported for this file system. So, attempt an fsync 
  ** and (for now) ignore the overhead of a superfluous fcntl call.  
  ** It'd be better to detect fullfsync support once and avoid 
  ** the fcntl call every time sync is called.
  */
  if( rc ) rc = fsync(fd);

#elif defined(__APPLE__)
  /* fdatasync() on HFS+ doesn't yet flush the file size if it changed correctly
  ** so currently we default to the macro that redefines fdatasync to fsync
  */
  rc = fsync(fd);
#else 
  rc = fdatasync(fd);
#if OS_VXWORKS
  if( rc==-1 && errno==ENOTSUP ){
    rc = fsync(fd);
  }
#endif /* OS_VXWORKS */
#endif /* ifdef SQLITE_NO_SYNC elif HAVE_FULLFSYNC */

  if( OS_VXWORKS && rc!= -1 ){
    rc = 0;
  }
  return rc;
}

/*
** Open a file descriptor to the directory containing file zFilename.
** If successful, *pFd is set to the opened file descriptor and
** SQLITE_OK is returned. If an error occurs, either SQLITE_NOMEM
** or SQLITE_CANTOPEN is returned and *pFd is set to an undefined
** value.
**
** The directory file descriptor is used for only one thing - to
** fsync() a directory to make sure file creation and deletion events
** are flushed to disk.  Such fsyncs are not needed on newer
** journaling filesystems, but are required on older filesystems.
**
** This routine can be overridden using the xSetSysCall interface.
** The ability to override this routine was added in support of the
** chromium sandbox.  Opening a directory is a security risk (we are
** told) so making it overrideable allows the chromium sandbox to
** replace this routine with a harmless no-op.  To make this routine
** a no-op, replace it with a stub that returns SQLITE_OK but leaves
** *pFd set to a negative number.
**
** If SQLITE_OK is returned, the caller is responsible for closing
** the file descriptor *pFd using close().
*/
static int openDirectory(const char *zFilename, int *pFd){
  int ii;
  int fd = -1;
  char zDirname[MAX_PATHNAME+1];

  sqlite3_snprintf(MAX_PATHNAME, zDirname, "%s", zFilename);
  for(ii=(int)strlen(zDirname); ii>1 && zDirname[ii]!='/'; ii--);
  if( ii>0 ){
    zDirname[ii] = '\0';
    fd = robust_open(zDirname, O_RDONLY|O_BINARY, 0);
    if( fd>=0 ){
      OSTRACE(("OPENDIR %-3d %s\n", fd, zDirname));
    }
  }
  *pFd = fd;
  return (fd>=0?SQLITE_OK:unixLogError(SQLITE_CANTOPEN_BKPT, "open", zDirname));
}

/*
** Make sure all writes to a particular file are committed to disk.
**
** If dataOnly==0 then both the file itself and its metadata (file
** size, access time, etc) are synced.  If dataOnly!=0 then only the
** file data is synced.
**
** Under Unix, also make sure that the directory entry for the file
** has been created by fsync-ing the directory that contains the file.
** If we do not do this and we encounter a power failure, the directory
** entry for the journal might not exist after we reboot.  The next
** SQLite to access the file will not know that the journal exists (because
** the directory entry for the journal was never created) and the transaction
** will not roll back - possibly leading to database corruption.
*/
static int unixSync(sqlite3_file *id, int flags){
  int rc;
  unixFile *pFile = (unixFile*)id;

  int isDataOnly = (flags&SQLITE_SYNC_DATAONLY);
  int isFullsync = (flags&0x0F)==SQLITE_SYNC_FULL;

  /* Check that one of SQLITE_SYNC_NORMAL or FULL was passed */
  assert((flags&0x0F)==SQLITE_SYNC_NORMAL
      || (flags&0x0F)==SQLITE_SYNC_FULL
  );

  /* Unix cannot, but some systems may return SQLITE_FULL from here. This
  ** line is to test that doing so does not cause any problems.
  */
  SimulateDiskfullError( return SQLITE_FULL );

  assert( pFile );
  OSTRACE(("SYNC    %-3d\n", pFile->h));
  rc = full_fsync(pFile->h, isFullsync, isDataOnly);
  SimulateIOError( rc=1 );
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_FSYNC, "full_fsync", pFile->zPath);
  }

  /* Also fsync the directory containing the file if the DIRSYNC flag
  ** is set.  This is a one-time occurrence.  Many systems (examples: AIX)
  ** are unable to fsync a directory, so ignore errors on the fsync.
  */
  if( pFile->ctrlFlags & UNIXFILE_DIRSYNC ){
    int dirfd;
    OSTRACE(("DIRSYNC %s (have_fullfsync=%d fullsync=%d)\n", pFile->zPath,
            HAVE_FULLFSYNC, isFullsync));
    rc = osOpenDirectory(pFile->zPath, &dirfd);
    if( rc==SQLITE_OK && dirfd>=0 ){
      full_fsync(dirfd, 0, 0);
      robust_close(pFile, dirfd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
    pFile->ctrlFlags &= ~UNIXFILE_DIRSYNC;
  }
  return rc;
}

/*
** Truncate an open file to a specified size
*/
static int unixTruncate(sqlite3_file *id, i64 nByte){
  unixFile *pFile = (unixFile *)id;
  int rc;
  assert( pFile );
  SimulateIOError( return SQLITE_IOERR_TRUNCATE );

  /* If the user has configured a chunk-size for this file, truncate the
  ** file so that it consists of an integer number of chunks (i.e. the
  ** actual file size after the operation may be larger than the requested
  ** size).
  */
  if( pFile->szChunk>0 ){
    nByte = ((nByte + pFile->szChunk - 1)/pFile->szChunk) * pFile->szChunk;
  }

  rc = robust_ftruncate(pFile->h, nByte);
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
  }else{
#ifdef SQLITE_DEBUG
    /* If we are doing a normal write to a database file (as opposed to
    ** doing a hot-journal rollback or a write to some file other than a
    ** normal database file) and we truncate the file to zero length,
    ** that effectively updates the change counter.  This might happen
    ** when restoring a database using the backup API from a zero-length
    ** source.
    */
    if( pFile->inNormalWrite && nByte==0 ){
      pFile->transCntrChng = 1;
    }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
    /* If the file was just truncated to a size smaller than the currently
    ** mapped region, reduce the effective mapping size as well. SQLite will
    ** use read() and write() to access data beyond this point from now on.  
    */
    if( nByte<pFile->mmapSize ){
      pFile->mmapSize = nByte;
    }
#endif

    return SQLITE_OK;
  }
}

/*
** Determine the current size of a file in bytes
*/
static int unixFileSize(sqlite3_file *id, i64 *pSize){
  int rc;
  struct stat buf;
  assert( id );
  rc = osFstat(((unixFile*)id)->h, &buf);
  SimulateIOError( rc=1 );
  if( rc!=0 ){
    storeLastErrno((unixFile*)id, errno);
    return SQLITE_IOERR_FSTAT;
  }
  *pSize = buf.st_size;

  /* When opening a zero-size database, the findInodeInfo() procedure
  ** writes a single byte into that file in order to work around a bug
  ** in the OS-X msdos filesystem.  In order to avoid problems with upper
  ** layers, we need to report this file size as zero even though it is
  ** really 1.   Ticket #3260.
  */
  if( *pSize==1 ) *pSize = 0;


  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Handler for proxy-locking file-control verbs.  Defined below in the
** proxying locking division.
*/
static int proxyFileControl(sqlite3_file*,int,void*);
#endif

/* 
** This function is called to handle the SQLITE_FCNTL_SIZE_HINT 
** file-control operation.  Enlarge the database to nBytes in size
** (rounded up to the next chunk-size).  If the database is already
** nBytes or larger, this routine is a no-op.
*/
static int fcntlSizeHint(unixFile *pFile, i64 nByte){
  if( pFile->szChunk>0 ){
    i64 nSize;                    /* Required file size */
    struct stat buf;              /* Used to hold return values of fstat() */
   
    if( osFstat(pFile->h, &buf) ){
      return SQLITE_IOERR_FSTAT;
    }

    nSize = ((nByte+pFile->szChunk-1) / pFile->szChunk) * pFile->szChunk;
    if( nSize>(i64)buf.st_size ){

#if defined(HAVE_POSIX_FALLOCATE) && HAVE_POSIX_FALLOCATE
      /* The code below is handling the return value of osFallocate() 
      ** correctly. posix_fallocate() is defined to "returns zero on success, 
      ** or an error number on  failure". See the manpage for details. */
      int err;
      do{
        err = osFallocate(pFile->h, buf.st_size, nSize-buf.st_size);
      }while( err==EINTR );
      if( err ) return SQLITE_IOERR_WRITE;
#else
      /* If the OS does not have posix_fallocate(), fake it. Write a 
      ** single byte to the last byte in each block that falls entirely
      ** within the extended region. Then, if required, a single byte
      ** at offset (nSize-1), to set the size of the file correctly.
      ** This is a similar technique to that used by glibc on systems
      ** that do not have a real fallocate() call.
      */
      int nBlk = buf.st_blksize;  /* File-system block size */
      int nWrite = 0;             /* Number of bytes written by seekAndWrite */
      i64 iWrite;                 /* Next offset to write to */

      iWrite = ((buf.st_size + 2*nBlk - 1)/nBlk)*nBlk-1;
      assert( iWrite>=buf.st_size );
      assert( (iWrite/nBlk)==((buf.st_size+nBlk-1)/nBlk) );
      assert( ((iWrite+1)%nBlk)==0 );
      for(/*no-op*/; iWrite<nSize; iWrite+=nBlk ){
        nWrite = seekAndWrite(pFile, iWrite, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
      if( nWrite==0 || (nSize%nBlk) ){
        nWrite = seekAndWrite(pFile, nSize-1, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
#endif
    }
  }

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFile->mmapSizeMax>0 && nByte>pFile->mmapSize ){
    int rc;
    if( pFile->szChunk<=0 ){
      if( robust_ftruncate(pFile->h, nByte) ){
        storeLastErrno(pFile, errno);
        return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
      }
    }

    rc = unixMapfile(pFile, nByte);
    return rc;
  }
#endif

  return SQLITE_OK;
}

/*
** If *pArg is initially negative then this is a query.  Set *pArg to
** 1 or 0 depending on whether or not bit mask of pFile->ctrlFlags is set.
**
** If *pArg is 0 or 1, then clear or set the mask bit of pFile->ctrlFlags.
*/
static void unixModeBit(unixFile *pFile, unsigned char mask, int *pArg){
  if( *pArg<0 ){
    *pArg = (pFile->ctrlFlags & mask)!=0;
  }else if( (*pArg)==0 ){
    pFile->ctrlFlags &= ~mask;
  }else{
    pFile->ctrlFlags |= mask;
  }
}

/* Forward declaration */
static int unixGetTempname(int nBuf, char *zBuf);

/*
** Information and control of an open file handle.
*/
static int unixFileControl(sqlite3_file *id, int op, void *pArg){
  unixFile *pFile = (unixFile*)id;
  switch( op ){
    case SQLITE_FCNTL_WAL_BLOCK: {
      /* pFile->ctrlFlags |= UNIXFILE_BLOCK; // Deferred feature */
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LOCKSTATE: {
      *(int*)pArg = pFile->eFileLock;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LAST_ERRNO: {
      *(int*)pArg = pFile->lastErrno;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_CHUNK_SIZE: {
      pFile->szChunk = *(int *)pArg;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SIZE_HINT: {
      int rc;
      SimulateIOErrorBenign(1);
      rc = fcntlSizeHint(pFile, *(i64 *)pArg);
      SimulateIOErrorBenign(0);
      return rc;
    }
    case SQLITE_FCNTL_PERSIST_WAL: {
      unixModeBit(pFile, UNIXFILE_PERSIST_WAL, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_POWERSAFE_OVERWRITE: {
      unixModeBit(pFile, UNIXFILE_PSOW, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_VFSNAME: {
      *(char**)pArg = sqlite3_mprintf("%s", pFile->pVfs->zName);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_TEMPFILENAME: {
      char *zTFile = sqlite3_malloc64( pFile->pVfs->mxPathname );
      if( zTFile ){
        unixGetTempname(pFile->pVfs->mxPathname, zTFile);
        *(char**)pArg = zTFile;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_HAS_MOVED: {
      *(int*)pArg = fileHasMoved(pFile);
      return SQLITE_OK;
    }
#if SQLITE_MAX_MMAP_SIZE>0
    case SQLITE_FCNTL_MMAP_SIZE: {
      i64 newLimit = *(i64*)pArg;
      int rc = SQLITE_OK;
      if( newLimit>sqlite3GlobalConfig.mxMmap ){
        newLimit = sqlite3GlobalConfig.mxMmap;
      }
      *(i64*)pArg = pFile->mmapSizeMax;
      if( newLimit>=0 && newLimit!=pFile->mmapSizeMax && pFile->nFetchOut==0 ){
        pFile->mmapSizeMax = newLimit;
        if( pFile->mmapSize>0 ){
          unixUnmapfile(pFile);
          rc = unixMapfile(pFile, -1);
        }
      }
      return rc;
    }
#endif
#ifdef SQLITE_DEBUG
    /* The pager calls this method to signal that it has done
    ** a rollback and that the database is therefore unchanged and
    ** it hence it is OK for the transaction change counter to be
    ** unchanged.
    */
    case SQLITE_FCNTL_DB_UNCHANGED: {
      ((unixFile*)id)->dbUpdate = 0;
      return SQLITE_OK;
    }
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    case SQLITE_FCNTL_SET_LOCKPROXYFILE:
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      return proxyFileControl(id,op,pArg);
    }
#endif /* SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__) */
  }
  return SQLITE_NOTFOUND;
}

/*
** Return the sector size in bytes of the underlying block device for
** the specified file. This is almost always 512 bytes, but may be
** larger for some devices.
**
** SQLite code assumes this function cannot fail. It also assumes that
** if two files are created in the same file-system directory (i.e.
** a database and its journal file) that the sector size will be the
** same for both.
*/
#ifndef __QNXNTO__ 
static int unixSectorSize(sqlite3_file *NotUsed){
  UNUSED_PARAMETER(NotUsed);
  return SQLITE_DEFAULT_SECTOR_SIZE;
}
#endif

/*
** The following version of unixSectorSize() is optimized for QNX.
*/
#ifdef __QNXNTO__
#include <sys/dcmd_blk.h>
#include <sys/statvfs.h>
static int unixSectorSize(sqlite3_file *id){
  unixFile *pFile = (unixFile*)id;
  if( pFile->sectorSize == 0 ){
    struct statvfs fsInfo;
       
    /* Set defaults for non-supported filesystems */
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
    pFile->deviceCharacteristics = 0;
    if( fstatvfs(pFile->h, &fsInfo) == -1 ) {
      return pFile->sectorSize;
    }

    if( !strcmp(fsInfo.f_basetype, "tmp") ) {
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC4K |       /* All ram filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "etfs") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* etfs cluster size writes are atomic */
        (pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) |
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx6") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC |         /* All filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx4") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "dos") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else{
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC512 |      /* blocks are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        0;
    }
  }
  /* Last chance verification.  If the sector size isn't a multiple of 512
  ** then it isn't valid.*/
  if( pFile->sectorSize % 512 != 0 ){
    pFile->deviceCharacteristics = 0;
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
  }
  return pFile->sectorSize;
}
#endif /* __QNXNTO__ */

/*
** Return the device characteristics for the file.
**
** This VFS is set up to return SQLITE_IOCAP_POWERSAFE_OVERWRITE by default.
** However, that choice is controversial since technically the underlying
** file system does not always provide powersafe overwrites.  (In other
** words, after a power-loss event, parts of the file that were never
** written might end up being altered.)  However, non-PSOW behavior is very,
** very rare.  And asserting PSOW makes a large reduction in the amount
** of required I/O for journaling, since a lot of padding is eliminated.
**  Hence, while POWERSAFE_OVERWRITE is on by default, there is a file-control
** available to turn it off and URI query parameter available to turn it off.
*/
static int unixDeviceCharacteristics(sqlite3_file *id){
  unixFile *p = (unixFile*)id;
  int rc = 0;
#ifdef __QNXNTO__
  if( p->sectorSize==0 ) unixSectorSize(id);
  rc = p->deviceCharacteristics;
#endif
  if( p->ctrlFlags & UNIXFILE_PSOW ){
    rc |= SQLITE_IOCAP_POWERSAFE_OVERWRITE;
  }
  return rc;
}

#if !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0

/*
** Return the system page size.
**
** This function should not be called directly by other code in this file. 
** Instead, it should be called via macro osGetpagesize().
*/
static int unixGetpagesize(void){
#if OS_VXWORKS
  return 1024;
#elif defined(_BSD_SOURCE)
  return getpagesize();
#else
  return (int)sysconf(_SC_PAGESIZE);
#endif
}

#endif /* !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0 */

#ifndef SQLITE_OMIT_WAL

/*
** Object used to represent an shared memory buffer.  
**
** When multiple threads all reference the same wal-index, each thread
** has its own unixShm object, but they all point to a single instance
** of this unixShmNode object.  In other words, each wal-index is opened
** only once per process.
**
** Each unixShmNode object is connected to a single unixInodeInfo object.
** We could coalesce this object into unixInodeInfo, but that would mean
** every open file that does not use shared memory (in other words, most
** open files) would have to carry around this extra information.  So
** the unixInodeInfo object contains a pointer to this unixShmNode object
** and the unixShmNode object is created only when needed.
**
** unixMutexHeld() must be true when creating or destroying
** this object or while reading or writing the following fields:
**
**      nRef
**
** The following fields are read-only after the object is created:
** 
**      fid
**      zFilename
**
** Either unixShmNode.mutex must be held or unixShmNode.nRef==0 and
** unixMutexHeld() is true when reading or writing any other field
** in this structure.
*/
struct unixShmNode {
  unixInodeInfo *pInode;     /* unixInodeInfo that owns this SHM node */
  sqlite3_mutex *mutex;      /* Mutex to access this object */
  char *zFilename;           /* Name of the mmapped file */
  int h;                     /* Open file descriptor */
  int szRegion;              /* Size of shared-memory regions */
  u16 nRegion;               /* Size of array apRegion */
  u8 isReadonly;             /* True if read-only */
  char **apRegion;           /* Array of mapped shared-memory regions */
  int nRef;                  /* Number of unixShm objects pointing to this */
  unixShm *pFirst;           /* All unixShm objects pointing to this */
#ifdef SQLITE_DEBUG
  u8 exclMask;               /* Mask of exclusive locks held */
  u8 sharedMask;             /* Mask of shared locks held */
  u8 nextShmId;              /* Next available unixShm.id value */
#endif
};

/*
** Structure used internally by this VFS to record the state of an
** open shared memory connection.
**
** The following fields are initialized when this object is created and
** are read-only thereafter:
**
**    unixShm.pFile
**    unixShm.id
**
** All other fields are read/write.  The unixShm.pFile->mutex must be held
** while accessing any read/write fields.
*/
struct unixShm {
  unixShmNode *pShmNode;     /* The underlying unixShmNode object */
  unixShm *pNext;            /* Next unixShm with the same unixShmNode */
  u8 hasMutex;               /* True if holding the unixShmNode mutex */
  u8 id;                     /* Id of this connection within its unixShmNode */
  u16 sharedMask;            /* Mask of shared locks held */
  u16 exclMask;              /* Mask of exclusive locks held */
};

/*
** Constants used for locking
*/
#define UNIX_SHM_BASE   ((22+SQLITE_SHM_NLOCK)*4)         /* first lock byte */
#define UNIX_SHM_DMS    (UNIX_SHM_BASE+SQLITE_SHM_NLOCK)  /* deadman switch */

/*
** Apply posix advisory locks for all bytes from ofst through ofst+n-1.
**
** Locks block if the mask is exactly UNIX_SHM_C and are non-blocking
** otherwise.
*/
static int unixShmSystemLock(
  unixFile *pFile,       /* Open connection to the WAL file */
  int lockType,          /* F_UNLCK, F_RDLCK, or F_WRLCK */
  int ofst,              /* First byte of the locking range */
  int n                  /* Number of bytes to lock */
){
  unixShmNode *pShmNode; /* Apply locks to this open shared-memory segment */
  struct flock f;        /* The posix advisory locking structure */
  int rc = SQLITE_OK;    /* Result code form fcntl() */

  /* Access to the unixShmNode object is serialized by the caller */
  pShmNode = pFile->pInode->pShmNode;
  assert( sqlite3_mutex_held(pShmNode->mutex) || pShmNode->nRef==0 );

  /* Shared locks never span more than one byte */
  assert( n==1 || lockType!=F_RDLCK );

  /* Locks are within range */
  assert( n>=1 && n<SQLITE_SHM_NLOCK );

  if( pShmNode->h>=0 ){
    int lkType;
    /* Initialize the locking parameters */
    memset(&f, 0, sizeof(f));
    f.l_type = lockType;
    f.l_whence = SEEK_SET;
    f.l_start = ofst;
    f.l_len = n;

    lkType = (pFile->ctrlFlags & UNIXFILE_BLOCK)!=0 ? F_SETLKW : F_SETLK;
    rc = osFcntl(pShmNode->h, lkType, &f);
    rc = (rc!=(-1)) ? SQLITE_OK : SQLITE_BUSY;
    pFile->ctrlFlags &= ~UNIXFILE_BLOCK;
  }

  /* Update the global lock state and do debug tracing */
#ifdef SQLITE_DEBUG
  { u16 mask;
  OSTRACE(("SHM-LOCK "));
  mask = ofst>31 ? 0xffff : (1<<(ofst+n)) - (1<<ofst);
  if( rc==SQLITE_OK ){
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask &= ~mask;
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask |= mask;
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d ok", ofst));
      pShmNode->exclMask |= mask;
      pShmNode->sharedMask &= ~mask;
    }
  }else{
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d failed", ofst));
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock failed"));
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d failed", ofst));
    }
  }
  OSTRACE((" - afterwards %03x,%03x\n",
           pShmNode->sharedMask, pShmNode->exclMask));
  }
#endif

  return rc;        
}

/*
** Return the minimum number of 32KB shm regions that should be mapped at
** a time, assuming that each mapping must be an integer multiple of the
** current system page-size.
**
** Usually, this is 1. The exception seems to be systems that are configured
** to use 64KB pages - in this case each mapping must cover at least two
** shm regions.
*/
static int unixShmRegionPerMap(void){
  int shmsz = 32*1024;            /* SHM region size */
  int pgsz = osGetpagesize();   /* System page size */
  assert( ((pgsz-1)&pgsz)==0 );   /* Page size must be a power of 2 */
  if( pgsz<shmsz ) return 1;
  return pgsz/shmsz;
}

/*
** Purge the unixShmNodeList list of all entries with unixShmNode.nRef==0.
**
** This is not a VFS shared-memory method; it is a utility function called
** by VFS shared-memory methods.
*/
static void unixShmPurge(unixFile *pFd){
  unixShmNode *p = pFd->pInode->pShmNode;
  assert( unixMutexHeld() );
  if( p && p->nRef==0 ){
    int nShmPerMap = unixShmRegionPerMap();
    int i;
    assert( p->pInode==pFd->pInode );
    sqlite3_mutex_free(p->mutex);
    for(i=0; i<p->nRegion; i+=nShmPerMap){
      if( p->h>=0 ){
        osMunmap(p->apRegion[i], p->szRegion);
      }else{
        sqlite3_free(p->apRegion[i]);
      }
    }
    sqlite3_free(p->apRegion);
    if( p->h>=0 ){
      robust_close(pFd, p->h, __LINE__);
      p->h = -1;
    }
    p->pInode->pShmNode = 0;
    sqlite3_free(p);
  }
}

/*
** Open a shared-memory area associated with open database file pDbFd.  
** This particular implementation uses mmapped files.
**
** The file used to implement shared-memory is in the same directory
** as the open database file and has the same name as the open database
** file with the "-shm" suffix added.  For example, if the database file
** is "/home/user1/config.db" then the file that is created and mmapped
** for shared memory will be called "/home/user1/config.db-shm".  
**
** Another approach to is to use files in /dev/shm or /dev/tmp or an
** some other tmpfs mount. But if a file in a different directory
** from the database file is used, then differing access permissions
** or a chroot() might cause two different processes on the same
** database to end up using different files for shared memory - 
** meaning that their memory would not really be shared - resulting
** in database corruption.  Nevertheless, this tmpfs file usage
** can be enabled at compile-time using -DSQLITE_SHM_DIRECTORY="/dev/shm"
** or the equivalent.  The use of the SQLITE_SHM_DIRECTORY compile-time
** option results in an incompatible build of SQLite;  builds of SQLite
** that with differing SQLITE_SHM_DIRECTORY settings attempt to use the
** same database file at the same time, database corruption will likely
** result. The SQLITE_SHM_DIRECTORY compile-time option is considered
** "unsupported" and may go away in a future SQLite release.
**
** When opening a new shared-memory file, if no other instances of that
** file are currently open, in this process or in other processes, then
** the file must be truncated to zero length or have its header cleared.
**
** If the original database file (pDbFd) is using the "unix-excl" VFS
** that means that an exclusive lock is held on the database file and
** that no other processes are able to read or write the database.  In
** that case, we do not really need shared memory.  No shared memory
** file is created.  The shared memory will be simulated with heap memory.
*/
static int unixOpenSharedMemory(unixFile *pDbFd){
  struct unixShm *p = 0;          /* The connection to be opened */
  struct unixShmNode *pShmNode;   /* The underlying mmapped file */
  int rc;                         /* Result code */
  unixInodeInfo *pInode;          /* The inode of fd */
  char *zShmFilename;             /* Name of the file used for SHM */
  int nShmFilename;               /* Size of the SHM filename in bytes */

  /* Allocate space for the new unixShm object. */
  p = sqlite3_malloc64( sizeof(*p) );
  if( p==0 ) return SQLITE_NOMEM;
  memset(p, 0, sizeof(*p));
  assert( pDbFd->pShm==0 );

  /* Check to see if a unixShmNode object already exists. Reuse an existing
  ** one if present. Create a new one if necessary.
  */
  unixEnterMutex();
  pInode = pDbFd->pInode;
  pShmNode = pInode->pShmNode;
  if( pShmNode==0 ){
    struct stat sStat;                 /* fstat() info for database file */
#ifndef SQLITE_SHM_DIRECTORY
    const char *zBasePath = pDbFd->zPath;
#endif

    /* Call fstat() to figure out the permissions on the database file. If
    ** a new *-shm file is created, an attempt will be made to create it
    ** with the same permissions.
    */
    if( osFstat(pDbFd->h, &sStat) && pInode->bProcessLock==0 ){
      rc = SQLITE_IOERR_FSTAT;
      goto shm_open_err;
    }

#ifdef SQLITE_SHM_DIRECTORY
    nShmFilename = sizeof(SQLITE_SHM_DIRECTORY) + 31;
#else
    nShmFilename = 6 + (int)strlen(zBasePath);
#endif
    pShmNode = sqlite3_malloc64( sizeof(*pShmNode) + nShmFilename );
    if( pShmNode==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }
    memset(pShmNode, 0, sizeof(*pShmNode)+nShmFilename);
    zShmFilename = pShmNode->zFilename = (char*)&pShmNode[1];
#ifdef SQLITE_SHM_DIRECTORY
    sqlite3_snprintf(nShmFilename, zShmFilename, 
                     SQLITE_SHM_DIRECTORY "/sqlite-shm-%x-%x",
                     (u32)sStat.st_ino, (u32)sStat.st_dev);
#else
    sqlite3_snprintf(nShmFilename, zShmFilename, "%s-shm", zBasePath);
    sqlite3FileSuffix3(pDbFd->zPath, zShmFilename);
#endif
    pShmNode->h = -1;
    pDbFd->pInode->pShmNode = pShmNode;
    pShmNode->pInode = pDbFd->pInode;
    pShmNode->mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
    if( pShmNode->mutex==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }

    if( pInode->bProcessLock==0 ){
      int openFlags = O_RDWR | O_CREAT;
      if( sqlite3_uri_boolean(pDbFd->zPath, "readonly_shm", 0) ){
        openFlags = O_RDONLY;
        pShmNode->isReadonly = 1;
      }
      pShmNode->h = robust_open(zShmFilename, openFlags, (sStat.st_mode&0777));
      if( pShmNode->h<0 ){
        rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zShmFilename);
        goto shm_open_err;
      }

      /* If this process is running as root, make sure that the SHM file
      ** is owned by the same user that owns the original database.  Otherwise,
      ** the original owner will not be able to connect.
      */
      osFchown(pShmNode->h, sStat.st_uid, sStat.st_gid);
  
      /* Check to see if another process is holding the dead-man switch.
      ** If not, truncate the file to zero length. 
      */
      rc = SQLITE_OK;
      if( unixShmSystemLock(pDbFd, F_WRLCK, UNIX_SHM_DMS, 1)==SQLITE_OK ){
        if( robust_ftruncate(pShmNode->h, 0) ){
          rc = unixLogError(SQLITE_IOERR_SHMOPEN, "ftruncate", zShmFilename);
        }
      }
      if( rc==SQLITE_OK ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, UNIX_SHM_DMS, 1);
      }
      if( rc ) goto shm_open_err;
    }
  }

  /* Make the new connection a child of the unixShmNode */
  p->pShmNode = pShmNode;
#ifdef SQLITE_DEBUG
  p->id = pShmNode->nextShmId++;
#endif
  pShmNode->nRef++;
  pDbFd->pShm = p;
  unixLeaveMutex();

  /* The reference count on pShmNode has already been incremented under
  ** the cover of the unixEnterMutex() mutex and the pointer from the
  ** new (struct unixShm) object to the pShmNode has been set. All that is
  ** left to do is to link the new object into the linked list starting
  ** at pShmNode->pFirst. This must be done while holding the pShmNode->mutex 
  ** mutex.
  */
  sqlite3_mutex_enter(pShmNode->mutex);
  p->pNext = pShmNode->pFirst;
  pShmNode->pFirst = p;
  sqlite3_mutex_leave(pShmNode->mutex);
  return SQLITE_OK;

  /* Jump here on any error */
shm_open_err:
  unixShmPurge(pDbFd);       /* This call frees pShmNode if required */
  sqlite3_free(p);
  unixLeaveMutex();
  return rc;
}

/*
** This function is called to obtain a pointer to region iRegion of the 
** shared-memory associated with the database file fd. Shared-memory regions 
** are numbered starting from zero. Each shared-memory region is szRegion 
** bytes in size.
**
** If an error occurs, an error code is returned and *pp is set to NULL.
**
** Otherwise, if the bExtend parameter is 0 and the requested shared-memory
** region has not been allocated (by any client, including one running in a
** separate process), then *pp is set to NULL and SQLITE_OK returned. If 
** bExtend is non-zero and the requested shared-memory region has not yet 
** been allocated, it is allocated by this function.
**
** If the shared-memory region has already been allocated or is allocated by
** this call as described above, then it is mapped into this processes 
** address space (if it is not already), *pp is set to point to the mapped 
** memory and SQLITE_OK returned.
*/
static int unixShmMap(
  sqlite3_file *fd,               /* Handle open on database file */
  int iRegion,                    /* Region to retrieve */
  int szRegion,                   /* Size of regions */
  int bExtend,                    /* True to extend file if necessary */
  void volatile **pp              /* OUT: Mapped memory */
){
  unixFile *pDbFd = (unixFile*)fd;
  unixShm *p;
  unixShmNode *pShmNode;
  int rc = SQLITE_OK;
  int nShmPerMap = unixShmRegionPerMap();
  int nReqRegion;

  /* If the shared-memory file has not yet been opened, open it now. */
  if( pDbFd->pShm==0 ){
    rc = unixOpenSharedMemory(pDbFd);
    if( rc!=SQLITE_OK ) return rc;
  }

  p = pDbFd->pShm;
  pShmNode = p->pShmNode;
  sqlite3_mutex_enter(pShmNode->mutex);
  assert( szRegion==pShmNode->szRegion || pShmNode->nRegion==0 );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  /* Minimum number of regions required to be mapped. */
  nReqRegion = ((iRegion+nShmPerMap) / nShmPerMap) * nShmPerMap;

  if( pShmNode->nRegion<nReqRegion ){
    char **apNew;                      /* New apRegion[] array */
    int nByte = nReqRegion*szRegion;   /* Minimum required file size */
    struct stat sStat;                 /* Used by fstat() */

    pShmNode->szRegion = szRegion;

    if( pShmNode->h>=0 ){
      /* The requested region is not mapped into this processes address space.
      ** Check to see if it has been allocated (i.e. if the wal-index file is
      ** large enough to contain the requested region).
      */
      if( osFstat(pShmNode->h, &sStat) ){
        rc = SQLITE_IOERR_SHMSIZE;
        goto shmpage_out;
      }
  
      if( sStat.st_size<nByte ){
        /* The requested memory region does not exist. If bExtend is set to
        ** false, exit early. *pp will be set to NULL and SQLITE_OK returned.
        */
        if( !bExtend ){
          goto shmpage_out;
        }

        /* Alternatively, if bExtend is true, extend the file. Do this by
        ** writing a single byte to the end of each (OS) page being
        ** allocated or extended. Technically, we need only write to the
        ** last page in order to extend the file. But writing to all new
        ** pages forces the OS to allocate them immediately, which reduces
        ** the chances of SIGBUS while accessing the mapped region later on.
        */
        else{
          static const int pgsz = 4096;
          int iPg;

          /* Write to the last byte of each newly allocated or extended page */
          assert( (nByte % pgsz)==0 );
          for(iPg=(sStat.st_size/pgsz); iPg<(nByte/pgsz); iPg++){
            if( seekAndWriteFd(pShmNode->h, iPg*pgsz + pgsz-1, "", 1, 0)!=1 ){
              const char *zFile = pShmNode->zFilename;
              rc = unixLogError(SQLITE_IOERR_SHMSIZE, "write", zFile);
              goto shmpage_out;
            }
          }
        }
      }
    }

    /* Map the requested memory region into this processes address space. */
    apNew = (char **)sqlite3_realloc(
        pShmNode->apRegion, nReqRegion*sizeof(char *)
    );
    if( !apNew ){
      rc = SQLITE_IOERR_NOMEM;
      goto shmpage_out;
    }
    pShmNode->apRegion = apNew;
    while( pShmNode->nRegion<nReqRegion ){
      int nMap = szRegion*nShmPerMap;
      int i;
      void *pMem;
      if( pShmNode->h>=0 ){
        pMem = osMmap(0, nMap,
            pShmNode->isReadonly ? PROT_READ : PROT_READ|PROT_WRITE, 
            MAP_SHARED, pShmNode->h, szRegion*(i64)pShmNode->nRegion
        );
        if( pMem==MAP_FAILED ){
          rc = unixLogError(SQLITE_IOERR_SHMMAP, "mmap", pShmNode->zFilename);
          goto shmpage_out;
        }
      }else{
        pMem = sqlite3_malloc64(szRegion);
        if( pMem==0 ){
          rc = SQLITE_NOMEM;
          goto shmpage_out;
        }
        memset(pMem, 0, szRegion);
      }

      for(i=0; i<nShmPerMap; i++){
        pShmNode->apRegion[pShmNode->nRegion+i] = &((char*)pMem)[szRegion*i];
      }
      pShmNode->nRegion += nShmPerMap;
    }
  }

shmpage_out:
  if( pShmNode->nRegion>iRegion ){
    *pp = pShmNode->apRegion[iRegion];
  }else{
    *pp = 0;
  }
  if( pShmNode->isReadonly && rc==SQLITE_OK ) rc = SQLITE_READONLY;
  sqlite3_mutex_leave(pShmNode->mutex);
  return rc;
}

/*
** Change the lock state for a shared-memory segment.
**
** Note that the relationship between SHAREd and EXCLUSIVE locks is a little
** different here than in posix.  In xShmLock(), one can go from unlocked
** to shared and back or from unlocked to exclusive and back.  But one may
** not go from shared to exclusive or from exclusive to shared.
*/
static int unixShmLock(
  sqlite3_file *fd,          /* Database file holding the shared memory */
  int ofst,                  /* First lock to acquire or release */
  int n,                     /* Number of locks to acquire or release */
  int flags                  /* What to do with the lock */
){
  unixFile *pDbFd = (unixFile*)fd;      /* Connection holding shared memory */
  unixShm *p = pDbFd->pShm;             /* The shared memory being locked */
  unixShm *pX;                          /* For looping over all siblings */
  unixShmNode *pShmNode = p->pShmNode;  /* The underlying file iNode */
  int rc = SQLITE_OK;                   /* Result code */
  u16 mask;                             /* Mask of locks to take or release */

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( ofst>=0 && ofst+n<=SQLITE_SHM_NLOCK );
  assert( n>=1 );
  assert( flags==(SQLITE_SHM_LOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_LOCK | SQLITE_SHM_EXCLUSIVE)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_EXCLUSIVE) );
  assert( n==1 || (flags & SQLITE_SHM_EXCLUSIVE)!=0 );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  mask = (1<<(ofst+n)) - (1<<ofst);
  assert( n>1 || mask==(1<<ofst) );
  sqlite3_mutex_enter(pShmNode->mutex);
  if( flags & SQLITE_SHM_UNLOCK ){
    u16 allMask = 0; /* Mask of locks held by siblings */

    /* See if any siblings hold this same lock */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( pX==p ) continue;
      assert( (pX->exclMask & (p->exclMask|p->sharedMask))==0 );
      allMask |= pX->sharedMask;
    }

    /* Unlock the system-level locks */
    if( (mask & allMask)==0 ){
      rc = unixShmSystemLock(pDbFd, F_UNLCK, ofst+UNIX_SHM_BASE, n);
    }else{
      rc = SQLITE_OK;
    }

    /* Undo the local locks */
    if( rc==SQLITE_OK ){
      p->exclMask &= ~mask;
      p->sharedMask &= ~mask;
    } 
  }else if( flags & SQLITE_SHM_SHARED ){
    u16 allShared = 0;  /* Union of locks held by connections other than "p" */

    /* Find out which shared locks are already held by sibling connections.
    ** If any sibling already holds an exclusive lock, go ahead and return
    ** SQLITE_BUSY.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
      allShared |= pX->sharedMask;
    }

    /* Get shared locks at the system level, if necessary */
    if( rc==SQLITE_OK ){
      if( (allShared & mask)==0 ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, ofst+UNIX_SHM_BASE, n);
      }else{
        rc = SQLITE_OK;
      }
    }

    /* Get the local shared locks */
    if( rc==SQLITE_OK ){
      p->sharedMask |= mask;
    }
  }else{
    /* Make sure no sibling connections hold locks that will block this
    ** lock.  If any do, return SQLITE_BUSY right away.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 || (pX->sharedMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
    }
  
    /* Get the exclusive locks at the system level.  Then if successful
    ** also mark the local connection as being locked.
    */
    if( rc==SQLITE_OK ){
      rc = unixShmSystemLock(pDbFd, F_WRLCK, ofst+UNIX_SHM_BASE, n);
      if( rc==SQLITE_OK ){
        assert( (p->sharedMask & mask)==0 );
        p->exclMask |= mask;
      }
    }
  }
  sqlite3_mutex_leave(pShmNode->mutex);
  OSTRACE(("SHM-LOCK shmid-%d, pid-%d got %03x,%03x\n",
           p->id, osGetpid(0), p->sharedMask, p->exclMask));
  return rc;
}

/*
** Implement a memory barrier or memory fence on shared memory.  
**
** All loads and stores begun before the barrier must complete before
** any load or store begun after the barrier.
*/
static void unixShmBarrier(
  sqlite3_file *fd                /* Database file holding the shared memory */
){
  UNUSED_PARAMETER(fd);
  sqlite3MemoryBarrier();         /* compiler-defined memory barrier */
  unixEnterMutex();               /* Also mutex, for redundancy */
  unixLeaveMutex();
}

/*
** Close a connection to shared-memory.  Delete the underlying 
** storage if deleteFlag is true.
**
** If there is no shared memory associated with the connection then this
** routine is a harmless no-op.
*/
static int unixShmUnmap(
  sqlite3_file *fd,               /* The underlying database file */
  int deleteFlag                  /* Delete shared-memory if true */
){
  unixShm *p;                     /* The connection to be closed */
  unixShmNode *pShmNode;          /* The underlying shared-memory file */
  unixShm **pp;                   /* For looping over sibling connections */
  unixFile *pDbFd;                /* The underlying database file */

  pDbFd = (unixFile*)fd;
  p = pDbFd->pShm;
  if( p==0 ) return SQLITE_OK;
  pShmNode = p->pShmNode;

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );

  /* Remove connection p from the set of connections associated
  ** with pShmNode */
  sqlite3_mutex_enter(pShmNode->mutex);
  for(pp=&pShmNode->pFirst; (*pp)!=p; pp = &(*pp)->pNext){}
  *pp = p->pNext;

  /* Free the connection p */
  sqlite3_free(p);
  pDbFd->pShm = 0;
  sqlite3_mutex_leave(pShmNode->mutex);

  /* If pShmNode->nRef has reached 0, then close the underlying
  ** shared-memory file, too */
  unixEnterMutex();
  assert( pShmNode->nRef>0 );
  pShmNode->nRef--;
  if( pShmNode->nRef==0 ){
    if( deleteFlag && pShmNode->h>=0 ){
      osUnlink(pShmNode->zFilename);
    }
    unixShmPurge(pDbFd);
  }
  unixLeaveMutex();

  return SQLITE_OK;
}


#else
# define unixShmMap     0
# define unixShmLock    0
# define unixShmBarrier 0
# define unixShmUnmap   0
#endif /* #ifndef SQLITE_OMIT_WAL */

#if SQLITE_MAX_MMAP_SIZE>0
/*
** If it is currently memory mapped, unmap file pFd.
*/
static void unixUnmapfile(unixFile *pFd){
  assert( pFd->nFetchOut==0 );
  if( pFd->pMapRegion ){
    osMunmap(pFd->pMapRegion, pFd->mmapSizeActual);
    pFd->pMapRegion = 0;
    pFd->mmapSize = 0;
    pFd->mmapSizeActual = 0;
  }
}

/*
** Attempt to set the size of the memory mapping maintained by file 
** descriptor pFd to nNew bytes. Any existing mapping is discarded.
**
** If successful, this function sets the following variables:
**
**       unixFile.pMapRegion
**       unixFile.mmapSize
**       unixFile.mmapSizeActual
**
** If unsuccessful, an error message is logged via sqlite3_log() and
** the three variables above are zeroed. In this case SQLite should
** continue accessing the database using the xRead() and xWrite()
** methods.
*/
static void unixRemapfile(
  unixFile *pFd,                  /* File descriptor object */
  i64 nNew                        /* Required mapping size */
){
  const char *zErr = "mmap";
  int h = pFd->h;                      /* File descriptor open on db file */
  u8 *pOrig = (u8 *)pFd->pMapRegion;   /* Pointer to current file mapping */
  i64 nOrig = pFd->mmapSizeActual;     /* Size of pOrig region in bytes */
  u8 *pNew = 0;                        /* Location of new mapping */
  int flags = PROT_READ;               /* Flags to pass to mmap() */

  assert( pFd->nFetchOut==0 );
  assert( nNew>pFd->mmapSize );
  assert( nNew<=pFd->mmapSizeMax );
  assert( nNew>0 );
  assert( pFd->mmapSizeActual>=pFd->mmapSize );
  assert( MAP_FAILED!=0 );

  if( (pFd->ctrlFlags & UNIXFILE_RDONLY)==0 ) flags |= PROT_WRITE;

  if( pOrig ){
#if HAVE_MREMAP
    i64 nReuse = pFd->mmapSize;
#else
    const int szSyspage = osGetpagesize();
    i64 nReuse = (pFd->mmapSize & ~(szSyspage-1));
#endif
    u8 *pReq = &pOrig[nReuse];

    /* Unmap any pages of the existing mapping that cannot be reused. */
    if( nReuse!=nOrig ){
      osMunmap(pReq, nOrig-nReuse);
    }

#if HAVE_MREMAP
    pNew = osMremap(pOrig, nReuse, nNew, MREMAP_MAYMOVE);
    zErr = "mremap";
#else
    pNew = osMmap(pReq, nNew-nReuse, flags, MAP_SHARED, h, nReuse);
    if( pNew!=MAP_FAILED ){
      if( pNew!=pReq ){
        osMunmap(pNew, nNew - nReuse);
        pNew = 0;
      }else{
        pNew = pOrig;
      }
    }
#endif

    /* The attempt to extend the existing mapping failed. Free it. */
    if( pNew==MAP_FAILED || pNew==0 ){
      osMunmap(pOrig, nReuse);
    }
  }

  /* If pNew is still NULL, try to create an entirely new mapping. */
  if( pNew==0 ){
    pNew = osMmap(0, nNew, flags, MAP_SHARED, h, 0);
  }

  if( pNew==MAP_FAILED ){
    pNew = 0;
    nNew = 0;
    unixLogError(SQLITE_OK, zErr, pFd->zPath);

    /* If the mmap() above failed, assume that all subsequent mmap() calls
    ** will probably fail too. Fall back to using xRead/xWrite exclusively
    ** in this case.  */
    pFd->mmapSizeMax = 0;
  }
  pFd->pMapRegion = (void *)pNew;
  pFd->mmapSize = pFd->mmapSizeActual = nNew;
}

/*
** Memory map or remap the file opened by file-descriptor pFd (if the file
** is already mapped, the existing mapping is replaced by the new). Or, if 
** there already exists a mapping for this file, and there are still 
** outstanding xFetch() references to it, this function is a no-op.
**
** If parameter nByte is non-negative, then it is the requested size of 
** the mapping to create. Otherwise, if nByte is less than zero, then the 
** requested size is the size of the file on disk. The actual size of the
** created mapping is either the requested size or the value configured 
** using SQLITE_FCNTL_MMAP_LIMIT, whichever is smaller.
**
** SQLITE_OK is returned if no error occurs (even if the mapping is not
** recreated as a result of outstanding references) or an SQLite error
** code otherwise.
*/
static int unixMapfile(unixFile *pFd, i64 nByte){
  i64 nMap = nByte;
  int rc;

  assert( nMap>=0 || pFd->nFetchOut==0 );
  if( pFd->nFetchOut>0 ) return SQLITE_OK;

  if( nMap<0 ){
    struct stat statbuf;          /* Low-level file information */
    rc = osFstat(pFd->h, &statbuf);
    if( rc!=SQLITE_OK ){
      return SQLITE_IOERR_FSTAT;
    }
    nMap = statbuf.st_size;
  }
  if( nMap>pFd->mmapSizeMax ){
    nMap = pFd->mmapSizeMax;
  }

  if( nMap!=pFd->mmapSize ){
    if( nMap>0 ){
      unixRemapfile(pFd, nMap);
    }else{
      unixUnmapfile(pFd);
    }
  }

  return SQLITE_OK;
}
#endif /* SQLITE_MAX_MMAP_SIZE>0 */

/*
** If possible, return a pointer to a mapping of file fd starting at offset
** iOff. The mapping must be valid for at least nAmt bytes.
**
** If such a pointer can be obtained, store it in *pp and return SQLITE_OK.
** Or, if one cannot but no error occurs, set *pp to 0 and return SQLITE_OK.
** Finally, if an error does occur, return an SQLite error code. The final
** value of *pp is undefined in this case.
**
** If this function does return a pointer, the caller must eventually 
** release the reference by calling unixUnfetch().
*/
static int unixFetch(sqlite3_file *fd, i64 iOff, int nAmt, void **pp){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
#endif
  *pp = 0;

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFd->mmapSizeMax>0 ){
    if( pFd->pMapRegion==0 ){
      int rc = unixMapfile(pFd, -1);
      if( rc!=SQLITE_OK ) return rc;
    }
    if( pFd->mmapSize >= iOff+nAmt ){
      *pp = &((u8 *)pFd->pMapRegion)[iOff];
      pFd->nFetchOut++;
    }
  }
#endif
  return SQLITE_OK;
}

/*
** If the third argument is non-NULL, then this function releases a 
** reference obtained by an earlier call to unixFetch(). The second
** argument passed to this function must be the same as the corresponding
** argument that was passed to the unixFetch() invocation. 
**
** Or, if the third argument is NULL, then this function is being called 
** to inform the VFS layer that, according to POSIX, any existing mapping 
** may now be invalid and should be unmapped.
*/
static int unixUnfetch(sqlite3_file *fd, i64 iOff, void *p){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
  UNUSED_PARAMETER(iOff);

  /* If p==0 (unmap the entire file) then there must be no outstanding 
  ** xFetch references. Or, if p!=0 (meaning it is an xFetch reference),
  ** then there must be at least one outstanding.  */
  assert( (p==0)==(pFd->nFetchOut==0) );

  /* If p!=0, it must match the iOff value. */
  assert( p==0 || p==&((u8 *)pFd->pMapRegion)[iOff] );

  if( p ){
    pFd->nFetchOut--;
  }else{
    unixUnmapfile(pFd);
  }

  assert( pFd->nFetchOut>=0 );
#else
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(p);
  UNUSED_PARAMETER(iOff);
#endif
  return SQLITE_OK;
}

/*
** Here ends the implementation of all sqlite3_file methods.
**
********************** End sqlite3_file Methods *******************************
******************************************************************************/

/*
** This division contains definitions of sqlite3_io_methods objects that
** implement various file locking strategies.  It also contains definitions
** of "finder" functions.  A finder-function is used to locate the appropriate
** sqlite3_io_methods object for a particular database file.  The pAppData
** field of the sqlite3_vfs VFS objects are initialized to be pointers to
** the correct finder-function for that VFS.
**
** Most finder functions return a pointer to a fixed sqlite3_io_methods
** object.  The only interesting finder-function is autolockIoFinder, which
** looks at the filesystem type and tries to guess the best locking
** strategy from that.
**
** For finder-function F, two objects are created:
**
**    (1) The real finder-function named "FImpt()".
**
**    (2) A constant pointer to this function named just "F".
**
**
** A pointer to the F pointer is used as the pAppData value for VFS
** objects.  We have to do this instead of letting pAppData point
** directly at the finder-function since C90 rules prevent a void*
** from be cast into a function pointer.
**
**
** Each instance of this macro generates two objects:
**
**   *  A constant sqlite3_io_methods object call METHOD that has locking
**      methods CLOSE, LOCK, UNLOCK, CKRESLOCK.
**
**   *  An I/O method finder function called FINDER that returns a pointer
**      to the METHOD object in the previous bullet.
*/
#define IOMETHODS(FINDER,METHOD,VERSION,CLOSE,LOCK,UNLOCK,CKLOCK,SHMMAP)     \
static const sqlite3_io_methods METHOD = {                                   \
   VERSION,                    /* iVersion */                                \
   CLOSE,                      /* xClose */                                  \
   unixRead,                   /* xRead */                                   \
   unixWrite,                  /* xWrite */                                  \
   unixTruncate,               /* xTruncate */                               \
   unixSync,                   /* xSync */                                   \
   unixFileSize,               /* xFileSize */                               \
   LOCK,                       /* xLock */                                   \
   UNLOCK,                     /* xUnlock */                                 \
   CKLOCK,                     /* xCheckReservedLock */                      \
   unixFileControl,            /* xFileControl */                            \
   unixSectorSize,             /* xSectorSize */                             \
   unixDeviceCharacteristics,  /* xDeviceCapabilities */                     \
   SHMMAP,                     /* xShmMap */                                 \
   unixShmLock,                /* xShmLock */                                \
   unixShmBarrier,             /* xShmBarrier */                             \
   unixShmUnmap,               /* xShmUnmap */                               \
   unixFetch,                  /* xFetch */                                  \
   unixUnfetch,                /* xUnfetch */                                \
};                                                                           \
static const sqlite3_io_methods *FINDER##Impl(const char *z, unixFile *p){   \
  UNUSED_PARAMETER(z); UNUSED_PARAMETER(p);                                  \
  return &METHOD;                                                            \
}                                                                            \
static const sqlite3_io_methods *(*const FINDER)(const char*,unixFile *p)    \
    = FINDER##Impl;

/*
** Here are all of the sqlite3_io_methods objects for each of the
** locking strategies.  Functions that return pointers to these methods
** are also created.
*/
IOMETHODS(
  posixIoFinder,            /* Finder function name */
  posixIoMethods,           /* sqlite3_io_methods object name */
  3,                        /* shared memory and mmap are enabled */
  unixClose,                /* xClose method */
  unixLock,                 /* xLock method */
  unixUnlock,               /* xUnlock method */
  unixCheckReservedLock,    /* xCheckReservedLock method */
  unixShmMap                /* xShmMap method */
)
IOMETHODS(
  nolockIoFinder,           /* Finder function name */
  nolockIoMethods,          /* sqlite3_io_methods object name */
  3,                        /* shared memory is disabled */
  nolockClose,              /* xClose method */
  nolockLock,               /* xLock method */
  nolockUnlock,             /* xUnlock method */
  nolockCheckReservedLock,  /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
IOMETHODS(
  dotlockIoFinder,          /* Finder function name */
  dotlockIoMethods,         /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  dotlockClose,             /* xClose method */
  dotlockLock,              /* xLock method */
  dotlockUnlock,            /* xUnlock method */
  dotlockCheckReservedLock, /* xCheckReservedLock method */
  0                         /* xShmMap method */
)

#if SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  flockIoFinder,            /* Finder function name */
  flockIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  flockClose,               /* xClose method */
  flockLock,                /* xLock method */
  flockUnlock,              /* xUnlock method */
  flockCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if OS_VXWORKS
IOMETHODS(
  semIoFinder,              /* Finder function name */
  semIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  semXClose,                /* xClose method */
  semXLock,                 /* xLock method */
  semXUnlock,               /* xUnlock method */
  semXCheckReservedLock,    /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  afpIoFinder,              /* Finder function name */
  afpIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  afpClose,                 /* xClose method */
  afpLock,                  /* xLock method */
  afpUnlock,                /* xUnlock method */
  afpCheckReservedLock,     /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/*
** The proxy locking method is a "super-method" in the sense that it
** opens secondary file descriptors for the conch and lock files and
** it uses proxy, dot-file, AFP, and flock() locking methods on those
** secondary files.  For this reason, the division that implements
** proxy locking is located much further down in the file.  But we need
** to go ahead and define the sqlite3_io_methods and finder function
** for proxy locking here.  So we forward declare the I/O methods.
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
static int proxyClose(sqlite3_file*);
static int proxyLock(sqlite3_file*, int);
static int proxyUnlock(sqlite3_file*, int);
static int proxyCheckReservedLock(sqlite3_file*, int*);
IOMETHODS(
  proxyIoFinder,            /* Finder function name */
  proxyIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  proxyClose,               /* xClose method */
  proxyLock,                /* xLock method */
  proxyUnlock,              /* xUnlock method */
  proxyCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/* nfs lockd on OSX 10.3+ doesn't clear write locks when a read lock is set */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  nfsIoFinder,               /* Finder function name */
  nfsIoMethods,              /* sqlite3_io_methods object name */
  1,                         /* shared memory is disabled */
  unixClose,                 /* xClose method */
  unixLock,                  /* xLock method */
  nfsUnlock,                 /* xUnlock method */
  unixCheckReservedLock,     /* xCheckReservedLock method */
  0                          /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/* 
** This "finder" function attempts to determine the best locking strategy 
** for the database file "filePath".  It then returns the sqlite3_io_methods
** object that implements that strategy.
**
** This is for MacOSX only.
*/
static const sqlite3_io_methods *autolockIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* open file object for the database file */
){
  static const struct Mapping {
    const char *zFilesystem;              /* Filesystem type name */
    const sqlite3_io_methods *pMethods;   /* Appropriate locking method */
  } aMap[] = {
    { "hfs",    &posixIoMethods },
    { "ufs",    &posixIoMethods },
    { "afpfs",  &afpIoMethods },
    { "smbfs",  &afpIoMethods },
    { "webdav", &nolockIoMethods },
    { 0, 0 }
  };
  int i;
  struct statfs fsInfo;
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }
  if( statfs(filePath, &fsInfo) != -1 ){
    if( fsInfo.f_flags & MNT_RDONLY ){
      return &nolockIoMethods;
    }
    for(i=0; aMap[i].zFilesystem; i++){
      if( strcmp(fsInfo.f_fstypename, aMap[i].zFilesystem)==0 ){
        return aMap[i].pMethods;
      }
    }
  }

  /* Default case. Handles, amongst others, "nfs".
  ** Test byte-range lock using fcntl(). If the call succeeds, 
  ** assume that the file-system supports POSIX style locks. 
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    if( strcmp(fsInfo.f_fstypename, "nfs")==0 ){
      return &nfsIoMethods;
    } else {
      return &posixIoMethods;
    }
  }else{
    return &dotlockIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const autolockIoFinder)(const char*,unixFile*) = autolockIoFinderImpl;

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */

#if OS_VXWORKS
/*
** This "finder" function for VxWorks checks to see if posix advisory
** locking works.  If it does, then that is what is used.  If it does not
** work, then fallback to named semaphore locking.
*/
static const sqlite3_io_methods *vxworksIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* the open file object */
){
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }

  /* Test if fcntl() is supported and use POSIX style locks.
  ** Otherwise fall back to the named semaphore method.
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    return &posixIoMethods;
  }else{
    return &semIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const vxworksIoFinder)(const char*,unixFile*) = vxworksIoFinderImpl;

#endif /* OS_VXWORKS */

/*
** An abstract type for a pointer to an IO method finder function:
*/
typedef const sqlite3_io_methods *(*finder_type)(const char*,unixFile*);


/****************************************************************************
**************************** sqlite3_vfs methods ****************************
**
** This division contains the implementation of methods on the
** sqlite3_vfs object.
*/

/*
** Initialize the contents of the unixFile structure pointed to by pId.
*/
static int fillInUnixFile(
  sqlite3_vfs *pVfs,      /* Pointer to vfs object */
  int h,                  /* Open file descriptor of file being opened */
  sqlite3_file *pId,      /* Write to the unixFile structure here */
  const char *zFilename,  /* Name of the file being opened */
  int ctrlFlags           /* Zero or more UNIXFILE_* values */
){
  const sqlite3_io_methods *pLockingStyle;
  unixFile *pNew = (unixFile *)pId;
  int rc = SQLITE_OK;

  assert( pNew->pInode==NULL );

  /* Usually the path zFilename should not be a relative pathname. The
  ** exception is when opening the proxy "conch" file in builds that
  ** include the special Apple locking styles.
  */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
  assert( zFilename==0 || zFilename[0]=='/' 
    || pVfs->pAppData==(void*)&autolockIoFinder );
#else
  assert( zFilename==0 || zFilename[0]=='/' );
#endif

  /* No locking occurs in temporary files */
  assert( zFilename!=0 || (ctrlFlags & UNIXFILE_NOLOCK)!=0 );

  OSTRACE(("OPEN    %-3d %s\n", h, zFilename));
  pNew->h = h;
  pNew->pVfs = pVfs;
  pNew->zPath = zFilename;
  pNew->ctrlFlags = (u8)ctrlFlags;
#if SQLITE_MAX_MMAP_SIZE>0
  pNew->mmapSizeMax = sqlite3GlobalConfig.szMmap;
#endif
  if( sqlite3_uri_boolean(((ctrlFlags & UNIXFILE_URI) ? zFilename : 0),
                           "psow", SQLITE_POWERSAFE_OVERWRITE) ){
    pNew->ctrlFlags |= UNIXFILE_PSOW;
  }
  if( strcmp(pVfs->zName,"unix-excl")==0 ){
    pNew->ctrlFlags |= UNIXFILE_EXCL;
  }

#if OS_VXWORKS
  pNew->pId = vxworksFindFileId(zFilename);
  if( pNew->pId==0 ){
    ctrlFlags |= UNIXFILE_NOLOCK;
    rc = SQLITE_NOMEM;
  }
#endif

  if( ctrlFlags & UNIXFILE_NOLOCK ){
    pLockingStyle = &nolockIoMethods;
  }else{
    pLockingStyle = (**(finder_type*)pVfs->pAppData)(zFilename, pNew);
#if SQLITE_ENABLE_LOCKING_STYLE
    /* Cache zFilename in the locking context (AFP and dotlock override) for
    ** proxyLock activation is possible (remote proxy is based on db name)
    ** zFilename remains valid until file is closed, to support */
    pNew->lockingContext = (void*)zFilename;
#endif
  }

  if( pLockingStyle == &posixIoMethods
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
    || pLockingStyle == &nfsIoMethods
#endif
  ){
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( rc!=SQLITE_OK ){
      /* If an error occurred in findInodeInfo(), close the file descriptor
      ** immediately, before releasing the mutex. findInodeInfo() may fail
      ** in two scenarios:
      **
      **   (a) A call to fstat() failed.
      **   (b) A malloc failed.
      **
      ** Scenario (b) may only occur if the process is holding no other
      ** file descriptors open on the same file. If there were other file
      ** descriptors on this file, then no malloc would be required by
      ** findInodeInfo(). If this is the case, it is quite safe to close
      ** handle h - as it is guaranteed that no posix locks will be released
      ** by doing so.
      **
      ** If scenario (a) caused the error then things are not so safe. The
      ** implicit assumption here is that if fstat() fails, things are in
      ** such bad shape that dropping a lock or two doesn't matter much.
      */
      robust_close(pNew, h, __LINE__);
      h = -1;
    }
    unixLeaveMutex();
  }

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
  else if( pLockingStyle == &afpIoMethods ){
    /* AFP locking uses the file path so it needs to be included in
    ** the afpLockingContext.
    */
    afpLockingContext *pCtx;
    pNew->lockingContext = pCtx = sqlite3_malloc64( sizeof(*pCtx) );
    if( pCtx==0 ){
      rc = SQLITE_NOMEM;
    }else{
      /* NB: zFilename exists and remains valid until the file is closed
      ** according to requirement F11141.  So we do not need to make a
      ** copy of the filename. */
      pCtx->dbPath = zFilename;
      pCtx->reserved = 0;
      srandomdev();
      unixEnterMutex();
      rc = findInodeInfo(pNew, &pNew->pInode);
      if( rc!=SQLITE_OK ){
        sqlite3_free(pNew->lockingContext);
        robust_close(pNew, h, __LINE__);
        h = -1;
      }
      unixLeaveMutex();        
    }
  }
#endif

  else if( pLockingStyle == &dotlockIoMethods ){
    /* Dotfile locking uses the file path so it needs to be included in
    ** the dotlockLockingContext 
    */
    char *zLockFile;
    int nFilename;
    assert( zFilename!=0 );
    nFilename = (int)strlen(zFilename) + 6;
    zLockFile = (char *)sqlite3_malloc64(nFilename);
    if( zLockFile==0 ){
      rc = SQLITE_NOMEM;
    }else{
      sqlite3_snprintf(nFilename, zLockFile, "%s" DOTLOCK_SUFFIX, zFilename);
    }
    pNew->lockingContext = zLockFile;
  }

#if OS_VXWORKS
  else if( pLockingStyle == &semIoMethods ){
    /* Named semaphore locking uses the file path so it needs to be
    ** included in the semLockingContext
    */
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( (rc==SQLITE_OK) && (pNew->pInode->pSem==NULL) ){
      char *zSemName = pNew->pInode->aSemName;
      int n;
      sqlite3_snprintf(MAX_PATHNAME, zSemName, "/%s.sem",
                       pNew->pId->zCanonicalName);
      for( n=1; zSemName[n]; n++ )
        if( zSemName[n]=='/' ) zSemName[n] = '_';
      pNew->pInode->pSem = sem_open(zSemName, O_CREAT, 0666, 1);
      if( pNew->pInode->pSem == SEM_FAILED ){
        rc = SQLITE_NOMEM;
        pNew->pInode->aSemName[0] = '\0';
      }
    }
    unixLeaveMutex();
  }
#endif
  
  storeLastErrno(pNew, 0);
#if OS_VXWORKS
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
    h = -1;
    osUnlink(zFilename);
    pNew->ctrlFlags |= UNIXFILE_DELETE;
  }
#endif
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
  }else{
    pNew->pMethod = pLockingStyle;
    OpenCounter(+1);
    verifyDbFile(pNew);
  }
  return rc;
}

/*
** Return the name of a directory in which to put temporary files.
** If no suitable temporary file directory can be found, return NULL.
*/
static const char *unixTempFileDir(void){
  static const char *azDirs[] = {
     0,
     0,
     0,
     "/var/tmp",
     "/usr/tmp",
     "/tmp",
     0        /* List terminator */
  };
  unsigned int i;
  struct stat buf;
  const char *zDir = 0;

  azDirs[0] = sqlite3_temp_directory;
  if( !azDirs[1] ) azDirs[1] = getenv("SQLITE_TMPDIR");
  if( !azDirs[2] ) azDirs[2] = getenv("TMPDIR");
  for(i=0; i<sizeof(azDirs)/sizeof(azDirs[0]); zDir=azDirs[i++]){
    if( zDir==0 ) continue;
    if( osStat(zDir, &buf) ) continue;
    if( !S_ISDIR(buf.st_mode) ) continue;
    if( osAccess(zDir, 07) ) continue;
    break;
  }
  return zDir;
}

/*
** Create a temporary file name in zBuf.  zBuf must be allocated
** by the calling process and must be big enough to hold at least
** pVfs->mxPathname bytes.
*/
static int unixGetTempname(int nBuf, char *zBuf){
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  unsigned int i, j;
  const char *zDir;

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. 
  */
  SimulateIOError( return SQLITE_IOERR );

  zDir = unixTempFileDir();
  if( zDir==0 ) zDir = ".";

  /* Check that the output buffer is large enough for the temporary file 
  ** name. If it is not, return SQLITE_ERROR.
  */
  if( (strlen(zDir) + strlen(SQLITE_TEMP_FILE_PREFIX) + 18) >= (size_t)nBuf ){
    return SQLITE_ERROR;
  }

  do{
    sqlite3_snprintf(nBuf-18, zBuf, "%s/"SQLITE_TEMP_FILE_PREFIX, zDir);
    j = (int)strlen(zBuf);
    sqlite3_randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
    zBuf[j+1] = 0;
  }while( osAccess(zBuf,0)==0 );
  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Routine to transform a unixFile into a proxy-locking unixFile.
** Implementation in the proxy-lock division, but used by unixOpen()
** if SQLITE_PREFER_PROXY_LOCKING is defined.
*/
static int proxyTransformUnixFile(unixFile*, const char*);
#endif

/*
** Search for an unused file descriptor that was opened on the database 
** file (not a journal or master-journal file) identified by pathname
** zPath with SQLITE_OPEN_XXX flags matching those passed as the second
** argument to this function.
**
** Such a file descriptor may exist if a database connection was closed
** but the associated file descriptor could not be closed because some
** other file descriptor open on the same file is holding a file-lock.
** Refer to comments in the unixClose() function and the lengthy comment
** describing "Posix Advisory Locking" at the start of this file for 
** further details. Also, ticket #4018.
**
** If a suitable file descriptor is found, then it is returned. If no
** such file descriptor is located, -1 is returned.
*/
static UnixUnusedFd *findReusableFd(const char *zPath, int flags){
  UnixUnusedFd *pUnused = 0;

  /* Do not search for an unused file descriptor on vxworks. Not because
  ** vxworks would not benefit from the change (it might, we're not sure),
  ** but because no way to test it is currently available. It is better 
  ** not to risk breaking vxworks support for the sake of such an obscure 
  ** feature.  */
#if !OS_VXWORKS
  struct stat sStat;                   /* Results of stat() call */

  /* A stat() call may fail for various reasons. If this happens, it is
  ** almost certain that an open() call on the same path will also fail.
  ** For this reason, if an error occurs in the stat() call here, it is
  ** ignored and -1 is returned. The caller will try to open a new file
  ** descriptor on the same path, fail, and return an error to SQLite.
  **
  ** Even if a subsequent open() call does succeed, the consequences of
  ** not searching for a reusable file descriptor are not dire.  */
  if( 0==osStat(zPath, &sStat) ){
    unixInodeInfo *pInode;

    unixEnterMutex();
    pInode = inodeList;
    while( pInode && (pInode->fileId.dev!=sStat.st_dev
                     || pInode->fileId.ino!=sStat.st_ino) ){
       pInode = pInode->pNext;
    }
    if( pInode ){
      UnixUnusedFd **pp;
      for(pp=&pInode->pUnused; *pp && (*pp)->flags!=flags; pp=&((*pp)->pNext));
      pUnused = *pp;
      if( pUnused ){
        *pp = pUnused->pNext;
      }
    }
    unixLeaveMutex();
  }
#endif    /* if !OS_VXWORKS */
  return pUnused;
}

/*
** This function is called by unixOpen() to determine the unix permissions
** to create new files with. If no error occurs, then SQLITE_OK is returned
** and a value suitable for passing as the third argument to open(2) is
** written to *pMode. If an IO error occurs, an SQLite error code is 
** returned and the value of *pMode is not modified.
**
** In most cases, this routine sets *pMode to 0, which will become
** an indication to robust_open() to create the file using
** SQLITE_DEFAULT_FILE_PERMISSIONS adjusted by the umask.
** But if the file being opened is a WAL or regular journal file, then 
** this function queries the file-system for the permissions on the 
** corresponding database file and sets *pMode to this value. Whenever 
** possible, WAL and journal files are created using the same permissions 
** as the associated database file.
**
** If the SQLITE_ENABLE_8_3_NAMES option is enabled, then the
** original filename is unavailable.  But 8_3_NAMES is only used for
** FAT filesystems and permissions do not matter there, so just use
** the default permissions.
*/
static int findCreateFileMode(
  const char *zPath,              /* Path of file (possibly) being created */
  int flags,                      /* Flags passed as 4th argument to xOpen() */
  mode_t *pMode,                  /* OUT: Permissions to open file with */
  uid_t *pUid,                    /* OUT: uid to set on the file */
  gid_t *pGid                     /* OUT: gid to set on the file */
){
  int rc = SQLITE_OK;             /* Return Code */
  *pMode = 0;
  *pUid = 0;
  *pGid = 0;
  if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
    char zDb[MAX_PATHNAME+1];     /* Database file path */
    int nDb;                      /* Number of valid bytes in zDb */
    struct stat sStat;            /* Output of stat() on database file */

    /* zPath is a path to a WAL or journal file. The following block derives
    ** the path to the associated database file from zPath. This block handles
    ** the following naming conventions:
    **
    **   "<path to db>-journal"
    **   "<path to db>-wal"
    **   "<path to db>-journalNN"
    **   "<path to db>-walNN"
    **
    ** where NN is a decimal number. The NN naming schemes are 
    ** used by the test_multiplex.c module.
    */
    nDb = sqlite3Strlen30(zPath) - 1; 
#ifdef SQLITE_ENABLE_8_3_NAMES
    while( nDb>0 && sqlite3Isalnum(zPath[nDb]) ) nDb--;
    if( nDb==0 || zPath[nDb]!='-' ) return SQLITE_OK;
#else
    while( zPath[nDb]!='-' ){
      assert( nDb>0 );
      assert( zPath[nDb]!='\n' );
      nDb--;
    }
#endif
    memcpy(zDb, zPath, nDb);
    zDb[nDb] = '\0';

    if( 0==osStat(zDb, &sStat) ){
      *pMode = sStat.st_mode & 0777;
      *pUid = sStat.st_uid;
      *pGid = sStat.st_gid;
    }else{
      rc = SQLITE_IOERR_FSTAT;
    }
  }else if( flags & SQLITE_OPEN_DELETEONCLOSE ){
    *pMode = 0600;
  }
  return rc;
}

/*
** Open the file zPath.
** 
** Previously, the SQLite OS layer used three functions in place of this
** one:
**
**     sqlite3OsOpenReadWrite();
**     sqlite3OsOpenReadOnly();
**     sqlite3OsOpenExclusive();
**
** These calls correspond to the following combinations of flags:
**
**     ReadWrite() ->     (READWRITE | CREATE)
**     ReadOnly()  ->     (READONLY) 
**     OpenExclusive() -> (READWRITE | CREATE | EXCLUSIVE)
**
** The old OpenExclusive() accepted a boolean argument - "delFlag". If
** true, the file was configured to be automatically deleted when the
** file handle closed. To achieve the same effect using this new 
** interface, add the DELETEONCLOSE flag to those specified above for 
** OpenExclusive().
*/
static int unixOpen(
  sqlite3_vfs *pVfs,           /* The VFS for which this is the xOpen method */
  const char *zPath,           /* Pathname of file to be opened */
  sqlite3_file *pFile,         /* The file descriptor to be filled in */
  int flags,                   /* Input flags to control the opening */
  int *pOutFlags               /* Output flags returned to SQLite core */
){
  unixFile *p = (unixFile *)pFile;
  int fd = -1;                   /* File descriptor returned by open() */
  int openFlags = 0;             /* Flags to pass to open() */
  int eType = flags&0xFFFFFF00;  /* Type of file to open */
  int noLock;                    /* True to omit locking primitives */
  int rc = SQLITE_OK;            /* Function Return Code */
  int ctrlFlags = 0;             /* UNIXFILE_* flags */

  int isExclusive  = (flags & SQLITE_OPEN_EXCLUSIVE);
  int isDelete     = (flags & SQLITE_OPEN_DELETEONCLOSE);
  int isCreate     = (flags & SQLITE_OPEN_CREATE);
  int isReadonly   = (flags & SQLITE_OPEN_READONLY);
  int isReadWrite  = (flags & SQLITE_OPEN_READWRITE);
#if SQLITE_ENABLE_LOCKING_STYLE
  int isAutoProxy  = (flags & SQLITE_OPEN_AUTOPROXY);
#endif
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  struct statfs fsInfo;
#endif

  /* If creating a master or main-file journal, this function will open
  ** a file-descriptor on the directory too. The first time unixSync()
  ** is called the directory file descriptor will be fsync()ed and close()d.
  */
  int syncDir = (isCreate && (
        eType==SQLITE_OPEN_MASTER_JOURNAL 
     || eType==SQLITE_OPEN_MAIN_JOURNAL 
     || eType==SQLITE_OPEN_WAL
  ));

  /* If argument zPath is a NULL pointer, this function is required to open
  ** a temporary file. Use this buffer to store the file name in.
  */
  char zTmpname[MAX_PATHNAME+2];
  const char *zName = zPath;

  /* Check the following statements are true: 
  **
  **   (a) Exactly one of the READWRITE and READONLY flags must be set, and 
  **   (b) if CREATE is set, then READWRITE must also be set, and
  **   (c) if EXCLUSIVE is set, then CREATE must also be set.
  **   (d) if DELETEONCLOSE is set, then CREATE must also be set.
  */
  assert((isReadonly==0 || isReadWrite==0) && (isReadWrite || isReadonly));
  assert(isCreate==0 || isReadWrite);
  assert(isExclusive==0 || isCreate);
  assert(isDelete==0 || isCreate);

  /* The main DB, main journal, WAL file and master journal are never 
  ** automatically deleted. Nor are they ever temporary files.  */
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_DB );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MASTER_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_WAL );

  /* Assert that the upper layer has set one of the "file-type" flags. */
  assert( eType==SQLITE_OPEN_MAIN_DB      || eType==SQLITE_OPEN_TEMP_DB 
       || eType==SQLITE_OPEN_MAIN_JOURNAL || eType==SQLITE_OPEN_TEMP_JOURNAL 
       || eType==SQLITE_OPEN_SUBJOURNAL   || eType==SQLITE_OPEN_MASTER_JOURNAL 
       || eType==SQLITE_OPEN_TRANSIENT_DB || eType==SQLITE_OPEN_WAL
  );

  /* Detect a pid change and reset the PRNG.  There is a race condition
  ** here such that two or more threads all trying to open databases at
  ** the same instant might all reset the PRNG.  But multiple resets
  ** are harmless.
  */
  if( randomnessPid!=osGetpid(0) ){
    randomnessPid = osGetpid(0);
    sqlite3_randomness(0,0);
  }

  memset(p, 0, sizeof(unixFile));

  if( eType==SQLITE_OPEN_MAIN_DB ){
    UnixUnusedFd *pUnused;
    pUnused = findReusableFd(zName, flags);
    if( pUnused ){
      fd = pUnused->fd;
    }else{
      pUnused = sqlite3_malloc64(sizeof(*pUnused));
      if( !pUnused ){
        return SQLITE_NOMEM;
      }
    }
    p->pUnused = pUnused;

    /* Database filenames are double-zero terminated if they are not
    ** URIs with parameters.  Hence, they can always be passed into
    ** sqlite3_uri_parameter(). */
    assert( (flags & SQLITE_OPEN_URI) || zName[strlen(zName)+1]==0 );

  }else if( !zName ){
    /* If zName is NULL, the upper layer is requesting a temp file. */
    assert(isDelete && !syncDir);
    rc = unixGetTempname(MAX_PATHNAME+2, zTmpname);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    zName = zTmpname;

    /* Generated temporary filenames are always double-zero terminated
    ** for use by sqlite3_uri_parameter(). */
    assert( zName[strlen(zName)+1]==0 );
  }

  /* Determine the value of the flags parameter passed to POSIX function
  ** open(). These must be calculated even if open() is not called, as
  ** they may be stored as part of the file handle and used by the 
  ** 'conch file' locking functions later on.  */
  if( isReadonly )  openFlags |= O_RDONLY;
  if( isReadWrite ) openFlags |= O_RDWR;
  if( isCreate )    openFlags |= O_CREAT;
  if( isExclusive ) openFlags |= (O_EXCL|O_NOFOLLOW);
  openFlags |= (O_LARGEFILE|O_BINARY);

  if( fd<0 ){
    mode_t openMode;              /* Permissions to create file with */
    uid_t uid;                    /* Userid for the file */
    gid_t gid;                    /* Groupid for the file */
    rc = findCreateFileMode(zName, flags, &openMode, &uid, &gid);
    if( rc!=SQLITE_OK ){
      assert( !p->pUnused );
      assert( eType==SQLITE_OPEN_WAL || eType==SQLITE_OPEN_MAIN_JOURNAL );
      return rc;
    }
    fd = robust_open(zName, openFlags, openMode);
    OSTRACE(("OPENX   %-3d %s 0%o\n", fd, zName, openFlags));
    if( fd<0 && errno!=EISDIR && isReadWrite && !isExclusive ){
      /* Failed to open the file for read/write access. Try read-only. */
      flags &= ~(SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
      openFlags &= ~(O_RDWR|O_CREAT);
      flags |= SQLITE_OPEN_READONLY;
      openFlags |= O_RDONLY;
      isReadonly = 1;
      fd = robust_open(zName, openFlags, openMode);
    }
    if( fd<0 ){
      rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zName);
      goto open_finished;
    }

    /* If this process is running as root and if creating a new rollback
    ** journal or WAL file, set the ownership of the journal or WAL to be
    ** the same as the original database.
    */
    if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
      osFchown(fd, uid, gid);
    }
  }
  assert( fd>=0 );
  if( pOutFlags ){
    *pOutFlags = flags;
  }

  if( p->pUnused ){
    p->pUnused->fd = fd;
    p->pUnused->flags = flags;
  }

  if( isDelete ){
#if OS_VXWORKS
    zPath = zName;
#elif defined(SQLITE_UNLINK_AFTER_CLOSE)
    zPath = sqlite3_mprintf("%s", zName);
    if( zPath==0 ){
      robust_close(p, fd, __LINE__);
      return SQLITE_NOMEM;
    }
#else
    osUnlink(zName);
#endif
  }
#if SQLITE_ENABLE_LOCKING_STYLE
  else{
    p->openFlags = openFlags;
  }
#endif

  noLock = eType!=SQLITE_OPEN_MAIN_DB;

  
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  if( fstatfs(fd, &fsInfo) == -1 ){
    storeLastErrno(p, errno);
    robust_close(p, fd, __LINE__);
    return SQLITE_IOERR_ACCESS;
  }
  if (0 == strncmp("msdos", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
  if (0 == strncmp("exfat", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
#endif

  /* Set up appropriate ctrlFlags */
  if( isDelete )                ctrlFlags |= UNIXFILE_DELETE;
  if( isReadonly )              ctrlFlags |= UNIXFILE_RDONLY;
  if( noLock )                  ctrlFlags |= UNIXFILE_NOLOCK;
  if( syncDir )                 ctrlFlags |= UNIXFILE_DIRSYNC;
  if( flags & SQLITE_OPEN_URI ) ctrlFlags |= UNIXFILE_URI;

#if SQLITE_ENABLE_LOCKING_STYLE
#if SQLITE_PREFER_PROXY_LOCKING
  isAutoProxy = 1;
#endif
  if( isAutoProxy && (zPath!=NULL) && (!noLock) && pVfs->xOpen ){
    char *envforce = getenv("SQLITE_FORCE_PROXY_LOCKING");
    int useProxy = 0;

    /* SQLITE_FORCE_PROXY_LOCKING==1 means force always use proxy, 0 means 
    ** never use proxy, NULL means use proxy for non-local files only.  */
    if( envforce!=NULL ){
      useProxy = atoi(envforce)>0;
    }else{
      useProxy = !(fsInfo.f_flags&MNT_LOCAL);
    }
    if( useProxy ){
      rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);
      if( rc==SQLITE_OK ){
        rc = proxyTransformUnixFile((unixFile*)pFile, ":auto:");
        if( rc!=SQLITE_OK ){
          /* Use unixClose to clean up the resources added in fillInUnixFile 
          ** and clear all the structure's references.  Specifically, 
          ** pFile->pMethods will be NULL so sqlite3OsClose will be a no-op 
          */
          unixClose(pFile);
          return rc;
        }
      }
      goto open_finished;
    }
  }
#endif
  
  rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);

open_finished:
  if( rc!=SQLITE_OK ){
    sqlite3_free(p->pUnused);
  }
  return rc;
}


/*
** Delete the file at zPath. If the dirSync argument is true, fsync()
** the directory after deleting the file.
*/
static int unixDelete(
  sqlite3_vfs *NotUsed,     /* VFS containing this as the xDelete method */
  const char *zPath,        /* Name of file to be deleted */
  int dirSync               /* If true, fsync() directory after deleting file */
){
  int rc = SQLITE_OK;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError(return SQLITE_IOERR_DELETE);
  if( osUnlink(zPath)==(-1) ){
    if( errno==ENOENT
#if OS_VXWORKS
        || osAccess(zPath,0)!=0
#endif
    ){
      rc = SQLITE_IOERR_DELETE_NOENT;
    }else{
      rc = unixLogError(SQLITE_IOERR_DELETE, "unlink", zPath);
    }
    return rc;
  }
#ifndef SQLITE_DISABLE_DIRSYNC
  if( (dirSync & 1)!=0 ){
    int fd;
    rc = osOpenDirectory(zPath, &fd);
    if( rc==SQLITE_OK ){
#if OS_VXWORKS
      if( fsync(fd)==-1 )
#else
      if( fsync(fd) )
#endif
      {
        rc = unixLogError(SQLITE_IOERR_DIR_FSYNC, "fsync", zPath);
      }
      robust_close(0, fd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
  }
#endif
  return rc;
}

/*
** Test the existence of or access permissions of file zPath. The
** test performed depends on the value of flags:
**
**     SQLITE_ACCESS_EXISTS: Return 1 if the file exists
**     SQLITE_ACCESS_READWRITE: Return 1 if the file is read and writable.
**     SQLITE_ACCESS_READONLY: Return 1 if the file is readable.
**
** Otherwise return 0.
*/
static int unixAccess(
  sqlite3_vfs *NotUsed,   /* The VFS containing this xAccess method */
  const char *zPath,      /* Path of the file to examine */
  int flags,              /* What do we want to learn about the zPath file? */
  int *pResOut            /* Write result boolean here */
){
  int amode = 0;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError( return SQLITE_IOERR_ACCESS; );
  switch( flags ){
    case SQLITE_ACCESS_EXISTS:
      amode = F_OK;
      break;
    case SQLITE_ACCESS_READWRITE:
      amode = W_OK|R_OK;
      break;
    case SQLITE_ACCESS_READ:
      amode = R_OK;
      break;

    default:
      assert(!"Invalid flags argument");
  }
  *pResOut = (osAccess(zPath, amode)==0);
  if( flags==SQLITE_ACCESS_EXISTS && *pResOut ){
    struct stat buf;
    if( 0==osStat(zPath, &buf) && buf.st_size==0 ){
      *pResOut = 0;
    }
  }
  return SQLITE_OK;
}


/*
** Turn a relative pathname into a full pathname. The relative path
** is stored as a nul-terminated string in the buffer pointed to by
** zPath. 
**
** zOut points to a buffer of at least sqlite3_vfs.mxPathname bytes 
** (in this case, MAX_PATHNAME bytes). The full-path is written to
** this buffer before returning.
*/
static int unixFullPathname(
  sqlite3_vfs *pVfs,            /* Pointer to vfs object */
  const char *zPath,            /* Possibly relative input path */
  int nOut,                     /* Size of output buffer in bytes */
  char *zOut                    /* Output buffer */
){

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. This function could fail if, for example, the
  ** current working directory has been unlinked.
  */
  SimulateIOError( return SQLITE_ERROR );

  assert( pVfs->mxPathname==MAX_PATHNAME );
  UNUSED_PARAMETER(pVfs);

  zOut[nOut-1] = '\0';
  if( zPath[0]=='/' ){
    sqlite3_snprintf(nOut, zOut, "%s", zPath);
  }else{
    int nCwd;
    if( osGetcwd(zOut, nOut-1)==0 ){
      return unixLogError(SQLITE_CANTOPEN_BKPT, "getcwd", zPath);
    }
    nCwd = (int)strlen(zOut);
    sqlite3_snprintf(nOut-nCwd, &zOut[nCwd], "/%s", zPath);
  }
  return SQLITE_OK;
}


#ifndef SQLITE_OMIT_LOAD_EXTENSION
/*
** Interfaces for opening a shared library, finding entry points
** within the shared library, and closing the shared library.
*/
#include <dlfcn.h>
static void *unixDlOpen(sqlite3_vfs *NotUsed, const char *zFilename){
  UNUSED_PARAMETER(NotUsed);
  return dlopen(zFilename, RTLD_NOW | RTLD_GLOBAL);
}

/*
** SQLite calls this function immediately after a call to unixDlSym() or
** unixDlOpen() fails (returns a null pointer). If a more detailed error
** message is available, it is written to zBufOut. If no error message
** is available, zBufOut is left unmodified and SQLite uses a default
** error message.
*/
static void unixDlError(sqlite3_vfs *NotUsed, int nBuf, char *zBufOut){
  const char *zErr;
  UNUSED_PARAMETER(NotUsed);
  unixEnterMutex();
  zErr = dlerror();
  if( zErr ){
    sqlite3_snprintf(nBuf, zBufOut, "%s", zErr);
  }
  unixLeaveMutex();
}
static void (*unixDlSym(sqlite3_vfs *NotUsed, void *p, const char*zSym))(void){
  /* 
  ** GCC with -pedantic-errors says that C90 does not allow a void* to be
  ** cast into a pointer to a function.  And yet the library dlsym() routine
  ** returns a void* which is really a pointer to a function.  So how do we
  ** use dlsym() with -pedantic-errors?
  **
  ** Variable x below is defined to be a pointer to a function taking
  ** parameters void* and const char* and returning a pointer to a function.
  ** We initialize x by assigning it a pointer to the dlsym() function.
  ** (That assignment requires a cast.)  Then we call the function that
  ** x points to.  
  **
  ** This work-around is unlikely to work correctly on any system where
  ** you really cannot cast a function pointer into void*.  But then, on the
  ** other hand, dlsym() will not work on such a system either, so we have
  ** not really lost anything.
  */
  void (*(*x)(void*,const char*))(void);
  UNUSED_PARAMETER(NotUsed);
  x = (void(*(*)(void*,const char*))(void))dlsym;
  return (*x)(p, zSym);
}
static void unixDlClose(sqlite3_vfs *NotUsed, void *pHandle){
  UNUSED_PARAMETER(NotUsed);
  dlclose(pHandle);
}
#else /* if SQLITE_OMIT_LOAD_EXTENSION is defined: */
  #define unixDlOpen  0
  #define unixDlError 0
  #define unixDlSym   0
  #define unixDlClose 0
#endif

/*
** Write nBuf bytes of random data to the supplied buffer zBuf.
*/
static int unixRandomness(sqlite3_vfs *NotUsed, int nBuf, char *zBuf){
  UNUSED_PARAMETER(NotUsed);
  assert((size_t)nBuf>=(sizeof(time_t)+sizeof(int)));

  /* We have to initialize zBuf to prevent valgrind from reporting
  ** errors.  The reports issued by valgrind are incorrect - we would
  ** prefer that the randomness be increased by making use of the
  ** uninitialized space in zBuf - but valgrind errors tend to worry
  ** some users.  Rather than argue, it seems easier just to initialize
  ** the whole array and silence valgrind, even if that means less randomness
  ** in the random seed.
  **
  ** When testing, initializing zBuf[] to zero is all we do.  That means
  ** that we always use the same random number sequence.  This makes the
  ** tests repeatable.
  */
  memset(zBuf, 0, nBuf);
  randomnessPid = osGetpid(0);  
#if !defined(SQLITE_TEST) && !defined(SQLITE_OMIT_RANDOMNESS)
  {
    int fd, got;
    fd = robust_open("/dev/urandom", O_RDONLY, 0);
    if( fd<0 ){
      time_t t;
      time(&t);
      memcpy(zBuf, &t, sizeof(t));
      memcpy(&zBuf[sizeof(t)], &randomnessPid, sizeof(randomnessPid));
      assert( sizeof(t)+sizeof(randomnessPid)<=(size_t)nBuf );
      nBuf = sizeof(t) + sizeof(randomnessPid);
    }else{
      do{ got = osRead(fd, zBuf, nBuf); }while( got<0 && errno==EINTR );
      robust_close(0, fd, __LINE__);
    }
  }
#endif
  return nBuf;
}


/*
** Sleep for a little while.  Return the amount of time slept.
** The argument is the number of microseconds we want to sleep.
** The return value is the number of microseconds of sleep actually
** requested from the underlying operating system, a number which
** might be greater than or equal to the argument, but not less
** than the argument.
*/
static int unixSleep(sqlite3_vfs *NotUsed, int microseconds){
#if OS_VXWORKS
  struct timespec sp;

  sp.tv_sec = microseconds / 1000000;
  sp.tv_nsec = (microseconds % 1000000) * 1000;
  nanosleep(&sp, NULL);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#elif defined(HAVE_USLEEP) && HAVE_USLEEP
  usleep(microseconds);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#else
  int seconds = (microseconds+999999)/1000000;
  sleep(seconds);
  UNUSED_PARAMETER(NotUsed);
  return seconds*1000000;
#endif
}

/*
** The following variable, if set to a non-zero value, is interpreted as
** the number of seconds since 1970 and is used to set the result of
** sqlite3OsCurrentTime() during testing.
*/
#ifdef SQLITE_TEST
SQLITE_API int sqlite3_current_time = 0;  /* Fake system time in seconds since 1970. */
#endif

/*
** Find the current time (in Universal Coordinated Time).  Write into *piNow
** the current time and date as a Julian Day number times 86_400_000.  In
** other words, write into *piNow the number of milliseconds since the Julian
** epoch of noon in Greenwich on November 24, 4714 B.C according to the
** proleptic Gregorian calendar.
**
** On success, return SQLITE_OK.  Return SQLITE_ERROR if the time and date 
** cannot be found.
*/
static int unixCurrentTimeInt64(sqlite3_vfs *NotUsed, sqlite3_int64 *piNow){
  static const sqlite3_int64 unixEpoch = 24405875*(sqlite3_int64)8640000;
  int rc = SQLITE_OK;
#if defined(NO_GETTOD)
  time_t t;
  time(&t);
  *piNow = ((sqlite3_int64)t)*1000 + unixEpoch;
#elif OS_VXWORKS
  struct timespec sNow;
  clock_gettime(CLOCK_REALTIME, &sNow);
  *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_nsec/1000000;
#else
  struct timeval sNow;
  if( gettimeofday(&sNow, 0)==0 ){
    *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_usec/1000;
  }else{
    rc = SQLITE_ERROR;
  }
#endif

#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *piNow = 1000*(sqlite3_int64)sqlite3_current_time + unixEpoch;
  }
#endif
  UNUSED_PARAMETER(NotUsed);
  return rc;
}

/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
static int unixCurrentTime(sqlite3_vfs *NotUsed, double *prNow){
  sqlite3_int64 i = 0;
  int rc;
  UNUSED_PARAMETER(NotUsed);
  rc = unixCurrentTimeInt64(0, &i);
  *prNow = i/86400000.0;
  return rc;
}

/*
** We added the xGetLastError() method with the intention of providing
** better low-level error messages when operating-system problems come up
** during SQLite operation.  But so far, none of that has been implemented
** in the core.  So this routine is never called.  For now, it is merely
** a place-holder.
*/
static int unixGetLastError(sqlite3_vfs *NotUsed, int NotUsed2, char *NotUsed3){
  UNUSED_PARAMETER(NotUsed);
  UNUSED_PARAMETER(NotUsed2);
  UNUSED_PARAMETER(NotUsed3);
  return 0;
}


/*
************************ End of sqlite3_vfs methods ***************************
******************************************************************************/

/******************************************************************************
************************** Begin Proxy Locking ********************************
**
** Proxy locking is a "uber-locking-method" in this sense:  It uses the
** other locking methods on secondary lock files.  Proxy locking is a
** meta-layer over top of the primitive locking implemented above.  For
** this reason, the division that implements of proxy locking is deferred
** until late in the file (here) after all of the other I/O methods have
** been defined - so that the primitive locking methods are available
** as services to help with the implementation of proxy locking.
**
****
**
** The default locking schemes in SQLite use byte-range locks on the
** database file to coordinate safe, concurrent access by multiple readers
** and writers [http://sqlite.org/lockingv3.html].  The five file locking
** states (UNLOCKED, PENDING, SHARED, RESERVED, EXCLUSIVE) are implemented
** as POSIX read & write locks over fixed set of locations (via fsctl),
** on AFP and SMB only exclusive byte-range locks are available via fsctl
** with _IOWR('z', 23, struct ByteRangeLockPB2) to track the same 5 states.
** To simulate a F_RDLCK on the shared range, on AFP a randomly selected
** address in the shared range is taken for a SHARED lock, the entire
** shared range is taken for an EXCLUSIVE lock):
**
**      PENDING_BYTE        0x40000000
**      RESERVED_BYTE       0x40000001
**      SHARED_RANGE        0x40000002 -> 0x40000200
**
** This works well on the local file system, but shows a nearly 100x
** slowdown in read performance on AFP because the AFP client disables
** the read cache when byte-range locks are present.  Enabling the read
** cache exposes a cache coherency problem that is present on all OS X
** supported network file systems.  NFS and AFP both observe the
** close-to-open semantics for ensuring cache coherency
** [http://nfs.sourceforge.net/#faq_a8], which does not effectively
** address the requirements for concurrent database access by multiple
** readers and writers
** [http://www.nabble.com/SQLite-on-NFS-cache-coherency-td15655701.html].
**
** To address the performance and cache coherency issues, proxy file locking
** changes the way database access is controlled by limiting access to a
** single host at a time and moving file locks off of the database file
** and onto a proxy file on the local file system.  
**
**
** Using proxy locks
** -----------------
**
** C APIs
**
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_SET_LOCKPROXYFILE,
**                       <proxy_path> | ":auto:");
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_GET_LOCKPROXYFILE,
**                       &<proxy_path>);
**
**
** SQL pragmas
**
**  PRAGMA [database.]lock_proxy_file=<proxy_path> | :auto:
**  PRAGMA [database.]lock_proxy_file
**
** Specifying ":auto:" means that if there is a conch file with a matching
** host ID in it, the proxy path in the conch file will be used, otherwise
** a proxy path based on the user's temp dir
** (via confstr(_CS_DARWIN_USER_TEMP_DIR,...)) will be used and the
** actual proxy file name is generated from the name and path of the
** database file.  For example:
**
**       For database path "/Users/me/foo.db" 
**       The lock path will be "<tmpdir>/sqliteplocks/_Users_me_foo.db:auto:")
**
** Once a lock proxy is configured for a database connection, it can not
** be removed, however it may be switched to a different proxy path via
** the above APIs (assuming the conch file is not being held by another
** connection or process). 
**
**
** How proxy locking works
** -----------------------
**
** Proxy file locking relies primarily on two new supporting files: 
**
**   *  conch file to limit access to the database file to a single host
**      at a time
**
**   *  proxy file to act as a proxy for the advisory locks normally
**      taken on the database
**
** The conch file - to use a proxy file, sqlite must first "hold the conch"
** by taking an sqlite-style shared lock on the conch file, reading the
** contents and comparing the host's unique host ID (see below) and lock
** proxy path against the values stored in the conch.  The conch file is
** stored in the same directory as the database file and the file name
** is patterned after the database file name as ".<databasename>-conch".
** If the conch file does not exist, or its contents do not match the
** host ID and/or proxy path, then the lock is escalated to an exclusive
** lock and the conch file contents is updated with the host ID and proxy
** path and the lock is downgraded to a shared lock again.  If the conch
** is held by another process (with a shared lock), the exclusive lock
** will fail and SQLITE_BUSY is returned.
**
** The proxy file - a single-byte file used for all advisory file locks
** normally taken on the database file.   This allows for safe sharing
** of the database file for multiple readers and writers on the same
** host (the conch ensures that they all use the same local lock file).
**
** Requesting the lock proxy does not immediately take the conch, it is
** only taken when the first request to lock database file is made.  
** This matches the semantics of the traditional locking behavior, where
** opening a connection to a database file does not take a lock on it.
** The shared lock and an open file descriptor are maintained until 
** the connection to the database is closed. 
**
** The proxy file and the lock file are never deleted so they only need
** to be created the first time they are used.
**
** Configuration options
** ---------------------
**
**  SQLITE_PREFER_PROXY_LOCKING
**
**       Database files accessed on non-local file systems are
**       automatically configured for proxy locking, lock files are
**       named automatically using the same logic as
**       PRAGMA lock_proxy_file=":auto:"
**    
**  SQLITE_PROXY_DEBUG
**
**       Enables the logging of error messages during host id file
**       retrieval and creation
**
**  LOCKPROXYDIR
**
**       Overrides the default directory used for lock proxy files that
**       are named automatically via the ":auto:" setting
**
**  SQLITE_DEFAULT_PROXYDIR_PERMISSIONS
**
**       Permissions to use when creating a directory for storing the
**       lock proxy files, only used when LOCKPROXYDIR is not set.
**    
**    
** As mentioned above, when compiled with SQLITE_PREFER_PROXY_LOCKING,
** setting the environment variable SQLITE_FORCE_PROXY_LOCKING to 1 will
** force proxy locking to be used for every database file opened, and 0
** will force automatic proxy locking to be disabled for all database
** files (explicitly calling the SQLITE_FCNTL_SET_LOCKPROXYFILE pragma or
** sqlite_file_control API is not affected by SQLITE_FORCE_PROXY_LOCKING).
*/

/*
** Proxy locking is only available on MacOSX 
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE

/*
** The proxyLockingContext has the path and file structures for the remote 
** and local proxy files in it
*/
typedef struct proxyLockingContext proxyLockingContext;
struct proxyLockingContext {
  unixFile *conchFile;         /* Open conch file */
  char *conchFilePath;         /* Name of the conch file */
  unixFile *lockProxy;         /* Open proxy lock file */
  char *lockProxyPath;         /* Name of the proxy lock file */
  char *dbPath;                /* Name of the open file */
  int conchHeld;               /* 1 if the conch is held, -1 if lockless */
  int nFails;                  /* Number of conch taking failures */
  void *oldLockingContext;     /* Original lockingcontext to restore on close */
  sqlite3_io_methods const *pOldMethod;     /* Original I/O methods for close */
};

/* 
** The proxy lock file path for the database at dbPath is written into lPath, 
** which must point to valid, writable memory large enough for a maxLen length
** file path. 
*/
static int proxyGetLockPath(const char *dbPath, char *lPath, size_t maxLen){
  int len;
  int dbLen;
  int i;

#ifdef LOCKPROXYDIR
  len = strlcpy(lPath, LOCKPROXYDIR, maxLen);
#else
# ifdef _CS_DARWIN_USER_TEMP_DIR
  {
    if( !confstr(_CS_DARWIN_USER_TEMP_DIR, lPath, maxLen) ){
      OSTRACE(("GETLOCKPATH  failed %s errno=%d pid=%d\n",
               lPath, errno, osGetpid(0)));
      return SQLITE_IOERR_LOCK;
    }
    len = strlcat(lPath, "sqliteplocks", maxLen);    
  }
# else
  len = strlcpy(lPath, "/tmp/", maxLen);
# endif
#endif

  if( lPath[len-1]!='/' ){
    len = strlcat(lPath, "/", maxLen);
  }
  
  /* transform the db path to a unique cache name */
  dbLen = (int)strlen(dbPath);
  for( i=0; i<dbLen && (i+len+7)<(int)maxLen; i++){
    char c = dbPath[i];
    lPath[i+len] = (c=='/')?'_':c;
  }
  lPath[i+len]='\0';
  strlcat(lPath, ":auto:", maxLen);
  OSTRACE(("GETLOCKPATH  proxy lock path=%s pid=%d\n", lPath, osGetpid(0)));
  return SQLITE_OK;
}

/* 
 ** Creates the lock file and any missing directories in lockPath
 */
static int proxyCreateLockPath(const char *lockPath){
  int i, len;
  char buf[MAXPATHLEN];
  int start = 0;
  
  assert(lockPath!=NULL);
  /* try to create all the intermediate directories */
  len = (int)strlen(lockPath);
  buf[0] = lockPath[0];
  for( i=1; i<len; i++ ){
    if( lockPath[i] == '/' && (i - start > 0) ){
      /* only mkdir if leaf dir != "." or "/" or ".." */
      if( i-start>2 || (i-start==1 && buf[start] != '.' && buf[start] != '/') 
         || (i-start==2 && buf[start] != '.' && buf[start+1] != '.') ){
        buf[i]='\0';
        if( osMkdir(buf, SQLITE_DEFAULT_PROXYDIR_PERMISSIONS) ){
          int err=errno;
          if( err!=EEXIST ) {
            OSTRACE(("CREATELOCKPATH  FAILED creating %s, "
                     "'%s' proxy lock path=%s pid=%d\n",
                     buf, strerror(err), lockPath, osGetpid(0)));
            return err;
          }
        }
      }
      start=i+1;
    }
    buf[i] = lockPath[i];
  }
  OSTRACE(("CREATELOCKPATH  proxy lock path=%s pid=%d\n", lockPath, osGetpid(0)));
  return 0;
}

/*
** Create a new VFS file descriptor (stored in memory obtained from
** sqlite3_malloc) and open the file named "path" in the file descriptor.
**
** The caller is responsible not only for closing the file descriptor
** but also for freeing the memory associated with the file descriptor.
*/
static int proxyCreateUnixFile(
    const char *path,        /* path for the new unixFile */
    unixFile **ppFile,       /* unixFile created and returned by ref */
    int islockfile           /* if non zero missing dirs will be created */
) {
  int fd = -1;
  unixFile *pNew;
  int rc = SQLITE_OK;
  int openFlags = O_RDWR | O_CREAT;
  sqlite3_vfs dummyVfs;
  int terrno = 0;
  UnixUnusedFd *pUnused = NULL;

  /* 1. first try to open/create the file
  ** 2. if that fails, and this is a lock file (not-conch), try creating
  ** the parent directories and then try again.
  ** 3. if that fails, try to open the file read-only
  ** otherwise return BUSY (if lock file) or CANTOPEN for the conch file
  */
  pUnused = findReusableFd(path, openFlags);
  if( pUnused ){
    fd = pUnused->fd;
  }else{
    pUnused = sqlite3_malloc64(sizeof(*pUnused));
    if( !pUnused ){
      return SQLITE_NOMEM;
    }
  }
  if( fd<0 ){
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
    if( fd<0 && errno==ENOENT && islockfile ){
      if( proxyCreateLockPath(path) == SQLITE_OK ){
        fd = robust_open(path, openFlags, 0);
      }
    }
  }
  if( fd<0 ){
    openFlags = O_RDONLY;
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
  }
  if( fd<0 ){
    if( islockfile ){
      return SQLITE_BUSY;
    }
    switch (terrno) {
      case EACCES:
        return SQLITE_PERM;
      case EIO: 
        return SQLITE_IOERR_LOCK; /* even though it is the conch */
      default:
        return SQLITE_CANTOPEN_BKPT;
    }
  }
  
  pNew = (unixFile *)sqlite3_malloc64(sizeof(*pNew));
  if( pNew==NULL ){
    rc = SQLITE_NOMEM;
    goto end_create_proxy;
  }
  memset(pNew, 0, sizeof(unixFile));
  pNew->openFlags = openFlags;
  memset(&dummyVfs, 0, sizeof(dummyVfs));
  dummyVfs.pAppData = (void*)&autolockIoFinder;
  dummyVfs.zName = "dummy";
  pUnused->fd = fd;
  pUnused->flags = openFlags;
  pNew->pUnused = pUnused;
  
  rc = fillInUnixFile(&dummyVfs, fd, (sqlite3_file*)pNew, path, 0);
  if( rc==SQLITE_OK ){
    *ppFile = pNew;
    return SQLITE_OK;
  }
end_create_proxy:    
  robust_close(pNew, fd, __LINE__);
  sqlite3_free(pNew);
  sqlite3_free(pUnused);
  return rc;
}

#ifdef SQLITE_TEST
/* simulate multiple hosts by creating unique hostid file paths */
SQLITE_API int sqlite3_hostid_num = 0;
#endif

#define PROXY_HOSTIDLEN    16  /* conch file host id length */

#ifdef HAVE_GETHOSTUUID
/* Not always defined in the headers as it ought to be */
extern int gethostuuid(uuid_t id, const struct timespec *wait);
#endif

/* get the host ID via gethostuuid(), pHostID must point to PROXY_HOSTIDLEN 
** bytes of writable memory.
*/
static int proxyGetHostID(unsigned char *pHostID, int *pError){
  assert(PROXY_HOSTIDLEN == sizeof(uuid_t));
  memset(pHostID, 0, PROXY_HOSTIDLEN);
#ifdef HAVE_GETHOSTUUID
  {
    struct timespec timeout = {1, 0}; /* 1 sec timeout */
    if( gethostuuid(pHostID, &timeout) ){
      int err = errno;
      if( pError ){
        *pError = err;
      }
      return SQLITE_IOERR;
    }
  }
#else
  UNUSED_PARAMETER(pError);
#endif
#ifdef SQLITE_TEST
  /* simulate multiple hosts by creating unique hostid file paths */
  if( sqlite3_hostid_num != 0){
    pHostID[0] = (char)(pHostID[0] + (char)(sqlite3_hostid_num & 0xFF));
  }
#endif
  
  return SQLITE_OK;
}

/* The conch file contains the header, host id and lock file path
 */
#define PROXY_CONCHVERSION 2   /* 1-byte header, 16-byte host id, path */
#define PROXY_HEADERLEN    1   /* conch file header length */
#define PROXY_PATHINDEX    (PROXY_HEADERLEN+PROXY_HOSTIDLEN)
#define PROXY_MAXCONCHLEN  (PROXY_HEADERLEN+PROXY_HOSTIDLEN+MAXPATHLEN)

/* 
** Takes an open conch file, copies the contents to a new path and then moves 
** it back.  The newly created file's file descriptor is assigned to the
** conch file structure and finally the original conch file descriptor is 
** closed.  Returns zero if successful.
*/
static int proxyBreakConchLock(unixFile *pFile, uuid_t myHostID){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  char tPath[MAXPATHLEN];
  char buf[PROXY_MAXCONCHLEN];
  char *cPath = pCtx->conchFilePath;
  size_t readLen = 0;
  size_t pathLen = 0;
  char errmsg[64] = "";
  int fd = -1;
  int rc = -1;
  UNUSED_PARAMETER(myHostID);

  /* create a new path by replace the trailing '-conch' with '-break' */
  pathLen = strlcpy(tPath, cPath, MAXPATHLEN);
  if( pathLen>MAXPATHLEN || pathLen<6 || 
     (strlcpy(&tPath[pathLen-5], "break", 6) != 5) ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"path error (len %d)",(int)pathLen);
    goto end_breaklock;
  }
  /* read the conch content */
  readLen = osPread(conchFile->h, buf, PROXY_MAXCONCHLEN, 0);
  if( readLen<PROXY_PATHINDEX ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"read error (len %d)",(int)readLen);
    goto end_breaklock;
  }
  /* write it out to the temporary break file */
  fd = robust_open(tPath, (O_RDWR|O_CREAT|O_EXCL), 0);
  if( fd<0 ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "create failed (%d)", errno);
    goto end_breaklock;
  }
  if( osPwrite(fd, buf, readLen, 0) != (ssize_t)readLen ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "write failed (%d)", errno);
    goto end_breaklock;
  }
  if( rename(tPath, cPath) ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "rename failed (%d)", errno);
    goto end_breaklock;
  }
  rc = 0;
  fprintf(stderr, "broke stale lock on %s\n", cPath);
  robust_close(pFile, conchFile->h, __LINE__);
  conchFile->h = fd;
  conchFile->openFlags = O_RDWR | O_CREAT;

end_breaklock:
  if( rc ){
    if( fd>=0 ){
      osUnlink(tPath);
      robust_close(pFile, fd, __LINE__);
    }
    fprintf(stderr, "failed to break stale lock on %s, %s\n", cPath, errmsg);
  }
  return rc;
}

/* Take the requested lock on the conch file and break a stale lock if the 
** host id matches.
*/
static int proxyConchLock(unixFile *pFile, uuid_t myHostID, int lockType){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  int rc = SQLITE_OK;
  int nTries = 0;
  struct timespec conchModTime;
  
  memset(&conchModTime, 0, sizeof(conchModTime));
  do {
    rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
    nTries ++;
    if( rc==SQLITE_BUSY ){
      /* If the lock failed (busy):
       * 1st try: get the mod time of the conch, wait 0.5s and try again. 
       * 2nd try: fail if the mod time changed or host id is different, wait 
       *           10 sec and try again
       * 3rd try: break the lock unless the mod time has changed.
       */
      struct stat buf;
      if( osFstat(conchFile->h, &buf) ){
        storeLastErrno(pFile, errno);
        return SQLITE_IOERR_LOCK;
      }
      
      if( nTries==1 ){
        conchModTime = buf.st_mtimespec;
        usleep(500000); /* wait 0.5 sec and try the lock again*/
        continue;  
      }

      assert( nTries>1 );
      if( conchModTime.tv_sec != buf.st_mtimespec.tv_sec || 
         conchModTime.tv_nsec != buf.st_mtimespec.tv_nsec ){
        return SQLITE_BUSY;
      }
      
      if( nTries==2 ){  
        char tBuf[PROXY_MAXCONCHLEN];
        int len = osPread(conchFile->h, tBuf, PROXY_MAXCONCHLEN, 0);
        if( len<0 ){
          storeLastErrno(pFile, errno);
          return SQLITE_IOERR_LOCK;
        }
        if( len>PROXY_PATHINDEX && tBuf[0]==(char)PROXY_CONCHVERSION){
          /* don't break the lock if the host id doesn't match */
          if( 0!=memcmp(&tBuf[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN) ){
            return SQLITE_BUSY;
          }
        }else{
          /* don't break the lock on short read or a version mismatch */
          return SQLITE_BUSY;
        }
        usleep(10000000); /* wait 10 sec and try the lock again */
        continue; 
      }
      
      assert( nTries==3 );
      if( 0==proxyBreakConchLock(pFile, myHostID) ){
        rc = SQLITE_OK;
        if( lockType==EXCLUSIVE_LOCK ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, SHARED_LOCK);
        }
        if( !rc ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
        }
      }
    }
  } while( rc==SQLITE_BUSY && nTries<3 );
  
  return rc;
}

/* Takes the conch by taking a shared lock and read the contents conch, if 
** lockPath is non-NULL, the host ID and lock file path must match.  A NULL 
** lockPath means that the lockPath in the conch file will be used if the 
** host IDs match, or a new lock path will be generated automatically 
** and written to the conch file.
*/
static int proxyTakeConch(unixFile *pFile){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  
  if( pCtx->conchHeld!=0 ){
    return SQLITE_OK;
  }else{
    unixFile *conchFile = pCtx->conchFile;
    uuid_t myHostID;
    int pError = 0;
    char readBuf[PROXY_MAXCONCHLEN];
    char lockPath[MAXPATHLEN];
    char *tempLockPath = NULL;
    int rc = SQLITE_OK;
    int createConch = 0;
    int hostIdMatch = 0;
    int readLen = 0;
    int tryOldLockPath = 0;
    int forceNewLockPath = 0;
    
    OSTRACE(("TAKECONCH  %d for %s pid=%d\n", conchFile->h,
             (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"),
             osGetpid(0)));

    rc = proxyGetHostID(myHostID, &pError);
    if( (rc&0xff)==SQLITE_IOERR ){
      storeLastErrno(pFile, pError);
      goto end_takeconch;
    }
    rc = proxyConchLock(pFile, myHostID, SHARED_LOCK);
    if( rc!=SQLITE_OK ){
      goto end_takeconch;
    }
    /* read the existing conch file */
    readLen = seekAndRead((unixFile*)conchFile, 0, readBuf, PROXY_MAXCONCHLEN);
    if( readLen<0 ){
      /* I/O error: lastErrno set by seekAndRead */
      storeLastErrno(pFile, conchFile->lastErrno);
      rc = SQLITE_IOERR_READ;
      goto end_takeconch;
    }else if( readLen<=(PROXY_HEADERLEN+PROXY_HOSTIDLEN) || 
             readBuf[0]!=(char)PROXY_CONCHVERSION ){
      /* a short read or version format mismatch means we need to create a new 
      ** conch file. 
      */
      createConch = 1;
    }
    /* if the host id matches and the lock path already exists in the conch
    ** we'll try to use the path there, if we can't open that path, we'll 
    ** retry with a new auto-generated path 
    */
    do { /* in case we need to try again for an :auto: named lock file */

      if( !createConch && !forceNewLockPath ){
        hostIdMatch = !memcmp(&readBuf[PROXY_HEADERLEN], myHostID, 
                                  PROXY_HOSTIDLEN);
        /* if the conch has data compare the contents */
        if( !pCtx->lockProxyPath ){
          /* for auto-named local lock file, just check the host ID and we'll
           ** use the local lock file path that's already in there
           */
          if( hostIdMatch ){
            size_t pathLen = (readLen - PROXY_PATHINDEX);
            
            if( pathLen>=MAXPATHLEN ){
              pathLen=MAXPATHLEN-1;
            }
            memcpy(lockPath, &readBuf[PROXY_PATHINDEX], pathLen);
            lockPath[pathLen] = 0;
            tempLockPath = lockPath;
            tryOldLockPath = 1;
            /* create a copy of the lock path if the conch is taken */
            goto end_takeconch;
          }
        }else if( hostIdMatch
               && !strncmp(pCtx->lockProxyPath, &readBuf[PROXY_PATHINDEX],
                           readLen-PROXY_PATHINDEX)
        ){
          /* conch host and lock path match */
          goto end_takeconch; 
        }
      }
      
      /* if the conch isn't writable and doesn't match, we can't take it */
      if( (conchFile->openFlags&O_RDWR) == 0 ){
        rc = SQLITE_BUSY;
        goto end_takeconch;
      }
      
      /* either the conch didn't match or we need to create a new one */
      if( !pCtx->lockProxyPath ){
        proxyGetLockPath(pCtx->dbPath, lockPath, MAXPATHLEN);
        tempLockPath = lockPath;
        /* create a copy of the lock path _only_ if the conch is taken */
      }
      
      /* update conch with host and path (this will fail if other process
      ** has a shared lock already), if the host id matches, use the big
      ** stick.
      */
      futimes(conchFile->h, NULL);
      if( hostIdMatch && !createConch ){
        if( conchFile->pInode && conchFile->pInode->nShared>1 ){
          /* We are trying for an exclusive lock but another thread in this
           ** same process is still holding a shared lock. */
          rc = SQLITE_BUSY;
        } else {          
          rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
        }
      }else{
        rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
      }
      if( rc==SQLITE_OK ){
        char writeBuffer[PROXY_MAXCONCHLEN];
        int writeSize = 0;
        
        writeBuffer[0] = (char)PROXY_CONCHVERSION;
        memcpy(&writeBuffer[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN);
        if( pCtx->lockProxyPath!=NULL ){
          strlcpy(&writeBuffer[PROXY_PATHINDEX], pCtx->lockProxyPath,
                  MAXPATHLEN);
        }else{
          strlcpy(&writeBuffer[PROXY_PATHINDEX], tempLockPath, MAXPATHLEN);
        }
        writeSize = PROXY_PATHINDEX + strlen(&writeBuffer[PROXY_PATHINDEX]);
        robust_ftruncate(conchFile->h, writeSize);
        rc = unixWrite((sqlite3_file *)conchFile, writeBuffer, writeSize, 0);
        fsync(conchFile->h);
        /* If we created a new conch file (not just updated the contents of a 
         ** valid conch file), try to match the permissions of the database 
         */
        if( rc==SQLITE_OK && createConch ){
          struct stat buf;
          int err = osFstat(pFile->h, &buf);
          if( err==0 ){
            mode_t cmode = buf.st_mode&(S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP |
                                        S_IROTH|S_IWOTH);
            /* try to match the database file R/W permissions, ignore failure */
#ifndef SQLITE_PROXY_DEBUG
            osFchmod(conchFile->h, cmode);
#else
            do{
              rc = osFchmod(conchFile->h, cmode);
            }while( rc==(-1) && errno==EINTR );
            if( rc!=0 ){
              int code = errno;
              fprintf(stderr, "fchmod %o FAILED with %d %s\n",
                      cmode, code, strerror(code));
            } else {
              fprintf(stderr, "fchmod %o SUCCEDED\n",cmode);
            }
          }else{
            int code = errno;
            fprintf(stderr, "STAT FAILED[%d] with %d %s\n", 
                    err, code, strerror(code));
#endif
          }
        }
      }
      conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, SHARED_LOCK);
      
    end_takeconch:
      OSTRACE(("TRANSPROXY: CLOSE  %d\n", pFile->h));
      if( rc==SQLITE_OK && pFile->openFlags ){
        int fd;
        if( pFile->h>=0 ){
          robust_close(pFile, pFile->h, __LINE__);
        }
        pFile->h = -1;
        fd = robust_open(pCtx->dbPath, pFile->openFlags, 0);
        OSTRACE(("TRANSPROXY: OPEN  %d\n", fd));
        if( fd>=0 ){
          pFile->h = fd;
        }else{
          rc=SQLITE_CANTOPEN_BKPT; /* SQLITE_BUSY? proxyTakeConch called
           during locking */
        }
      }
      if( rc==SQLITE_OK && !pCtx->lockProxy ){
        char *path = tempLockPath ? tempLockPath : pCtx->lockProxyPath;
        rc = proxyCreateUnixFile(path, &pCtx->lockProxy, 1);
        if( rc!=SQLITE_OK && rc!=SQLITE_NOMEM && tryOldLockPath ){
          /* we couldn't create the proxy lock file with the old lock file path
           ** so try again via auto-naming 
           */
          forceNewLockPath = 1;
          tryOldLockPath = 0;
          continue; /* go back to the do {} while start point, try again */
        }
      }
      if( rc==SQLITE_OK ){
        /* Need to make a copy of path if we extracted the value
         ** from the conch file or the path was allocated on the stack
         */
        if( tempLockPath ){
          pCtx->lockProxyPath = sqlite3DbStrDup(0, tempLockPath);
          if( !pCtx->lockProxyPath ){
            rc = SQLITE_NOMEM;
          }
        }
      }
      if( rc==SQLITE_OK ){
        pCtx->conchHeld = 1;
        
        if( pCtx->lockProxy->pMethod == &afpIoMethods ){
          afpLockingContext *afpCtx;
          afpCtx = (afpLockingContext *)pCtx->lockProxy->lockingContext;
          afpCtx->dbPath = pCtx->lockProxyPath;
        }
      } else {
        conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
      }
      OSTRACE(("TAKECONCH  %d %s\n", conchFile->h,
               rc==SQLITE_OK?"ok":"failed"));
      return rc;
    } while (1); /* in case we need to retry the :auto: lock file - 
                 ** we should never get here except via the 'continue' call. */
  }
}

/*
** If pFile holds a lock on a conch file, then release that lock.
*/
static int proxyReleaseConch(unixFile *pFile){
  int rc = SQLITE_OK;         /* Subroutine return code */
  proxyLockingContext *pCtx;  /* The locking context for the proxy lock */
  unixFile *conchFile;        /* Name of the conch file */

  pCtx = (proxyLockingContext *)pFile->lockingContext;
  conchFile = pCtx->conchFile;
  OSTRACE(("RELEASECONCH  %d for %s pid=%d\n", conchFile->h,
           (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"), 
           osGetpid(0)));
  if( pCtx->conchHeld>0 ){
    rc = conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
  }
  pCtx->conchHeld = 0;
  OSTRACE(("RELEASECONCH  %d %s\n", conchFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}

/*
** Given the name of a database file, compute the name of its conch file.
** Store the conch filename in memory obtained from sqlite3_malloc64().
** Make *pConchPath point to the new name.  Return SQLITE_OK on success
** or SQLITE_NOMEM if unable to obtain memory.
**
** The caller is responsible for ensuring that the allocated memory
** space is eventually freed.
**
** *pConchPath is set to NULL if a memory allocation error occurs.
*/
static int proxyCreateConchPathname(char *dbPath, char **pConchPath){
  int i;                        /* Loop counter */
  int len = (int)strlen(dbPath); /* Length of database filename - dbPath */
  char *conchPath;              /* buffer in which to construct conch name */

  /* Allocate space for the conch filename and initialize the name to
  ** the name of the original database file. */  
  *pConchPath = conchPath = (char *)sqlite3_malloc64(len + 8);
  if( conchPath==0 ){
    return SQLITE_NOMEM;
  }
  memcpy(conchPath, dbPath, len+1);
  
  /* now insert a "." before the last / character */
  for( i=(len-1); i>=0; i-- ){
    if( conchPath[i]=='/' ){
      i++;
      break;
    }
  }
  conchPath[i]='.';
  while ( i<len ){
    conchPath[i+1]=dbPath[i];
    i++;
  }

  /* append the "-conch" suffix to the file */
  memcpy(&conchPath[i+1], "-conch", 7);
  assert( (int)strlen(conchPath) == len+7 );

  return SQLITE_OK;
}


/* Takes a fully configured proxy locking-style unix file and switches
** the local lock file path 
*/
static int switchLockProxyPath(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
  char *oldPath = pCtx->lockProxyPath;
  int rc = SQLITE_OK;

  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }  

  /* nothing to do if the path is NULL, :auto: or matches the existing path */
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ||
    (oldPath && !strncmp(oldPath, path, MAXPATHLEN)) ){
    return SQLITE_OK;
  }else{
    unixFile *lockProxy = pCtx->lockProxy;
    pCtx->lockProxy=NULL;
    pCtx->conchHeld = 0;
    if( lockProxy!=NULL ){
      rc=lockProxy->pMethod->xClose((sqlite3_file *)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
    }
    sqlite3_free(oldPath);
    pCtx->lockProxyPath = sqlite3DbStrDup(0, path);
  }
  
  return rc;
}

/*
** pFile is a file that has been opened by a prior xOpen call.  dbPath
** is a string buffer at least MAXPATHLEN+1 characters in size.
**
** This routine find the filename associated with pFile and writes it
** int dbPath.
*/
static int proxyGetDbPathForUnixFile(unixFile *pFile, char *dbPath){
#if defined(__APPLE__)
  if( pFile->pMethod == &afpIoMethods ){
    /* afp style keeps a reference to the db path in the filePath field 
    ** of the struct */
    assert( (int)strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, ((afpLockingContext *)pFile->lockingContext)->dbPath,
            MAXPATHLEN);
  } else
#endif
  if( pFile->pMethod == &dotlockIoMethods ){
    /* dot lock style uses the locking context to store the dot lock
    ** file path */
    int len = strlen((char *)pFile->lockingContext) - strlen(DOTLOCK_SUFFIX);
    memcpy(dbPath, (char *)pFile->lockingContext, len + 1);
  }else{
    /* all other styles use the locking context to store the db file path */
    assert( strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, (char *)pFile->lockingContext, MAXPATHLEN);
  }
  return SQLITE_OK;
}

/*
** Takes an already filled in unix file and alters it so all file locking 
** will be performed on the local proxy lock file.  The following fields
** are preserved in the locking context so that they can be restored and 
** the unix structure properly cleaned up at close time:
**  ->lockingContext
**  ->pMethod
*/
static int proxyTransformUnixFile(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx;
  char dbPath[MAXPATHLEN+1];       /* Name of the database file */
  char *lockPath=NULL;
  int rc = SQLITE_OK;
  
  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }
  proxyGetDbPathForUnixFile(pFile, dbPath);
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ){
    lockPath=NULL;
  }else{
    lockPath=(char *)path;
  }
  
  OSTRACE(("TRANSPROXY  %d for %s pid=%d\n", pFile->h,
           (lockPath ? lockPath : ":auto:"), osGetpid(0)));

  pCtx = sqlite3_malloc64( sizeof(*pCtx) );
  if( pCtx==0 ){
    return SQLITE_NOMEM;
  }
  memset(pCtx, 0, sizeof(*pCtx));

  rc = proxyCreateConchPathname(dbPath, &pCtx->conchFilePath);
  if( rc==SQLITE_OK ){
    rc = proxyCreateUnixFile(pCtx->conchFilePath, &pCtx->conchFile, 0);
    if( rc==SQLITE_CANTOPEN && ((pFile->openFlags&O_RDWR) == 0) ){
      /* if (a) the open flags are not O_RDWR, (b) the conch isn't there, and
      ** (c) the file system is read-only, then enable no-locking access.
      ** Ugh, since O_RDONLY==0x0000 we test for !O_RDWR since unixOpen asserts
      ** that openFlags will have only one of O_RDONLY or O_RDWR.
      */
      struct statfs fsInfo;
      struct stat conchInfo;
      int goLockless = 0;

      if( osStat(pCtx->conchFilePath, &conchInfo) == -1 ) {
        int err = errno;
        if( (err==ENOENT) && (statfs(dbPath, &fsInfo) != -1) ){
          goLockless = (fsInfo.f_flags&MNT_RDONLY) == MNT_RDONLY;
        }
      }
      if( goLockless ){
        pCtx->conchHeld = -1; /* read only FS/ lockless */
        rc = SQLITE_OK;
      }
    }
  }  
  if( rc==SQLITE_OK && lockPath ){
    pCtx->lockProxyPath = sqlite3DbStrDup(0, lockPath);
  }

  if( rc==SQLITE_OK ){
    pCtx->dbPath = sqlite3DbStrDup(0, dbPath);
    if( pCtx->dbPath==NULL ){
      rc = SQLITE_NOMEM;
    }
  }
  if( rc==SQLITE_OK ){
    /* all memory is allocated, proxys are created and assigned, 
    ** switch the locking context and pMethod then return.
    */
    pCtx->oldLockingContext = pFile->lockingContext;
    pFile->lockingContext = pCtx;
    pCtx->pOldMethod = pFile->pMethod;
    pFile->pMethod = &proxyIoMethods;
  }else{
    if( pCtx->conchFile ){ 
      pCtx->conchFile->pMethod->xClose((sqlite3_file *)pCtx->conchFile);
      sqlite3_free(pCtx->conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath); 
    sqlite3_free(pCtx);
  }
  OSTRACE(("TRANSPROXY  %d %s\n", pFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}


/*
** This routine handles sqlite3_file_control() calls that are specific
** to proxy locking.
*/
static int proxyFileControl(sqlite3_file *id, int op, void *pArg){
  switch( op ){
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      if( pFile->pMethod == &proxyIoMethods ){
        proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
        proxyTakeConch(pFile);
        if( pCtx->lockProxyPath ){
          *(const char **)pArg = pCtx->lockProxyPath;
        }else{
          *(const char **)pArg = ":auto: (not held)";
        }
      } else {
        *(const char **)pArg = NULL;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      int rc = SQLITE_OK;
      int isProxyStyle = (pFile->pMethod == &proxyIoMethods);
      if( pArg==NULL || (const char *)pArg==0 ){
        if( isProxyStyle ){
          /* turn off proxy locking - not supported.  If support is added for
          ** switching proxy locking mode off then it will need to fail if
          ** the journal mode is WAL mode. 
          */
          rc = SQLITE_ERROR /*SQLITE_PROTOCOL? SQLITE_MISUSE?*/;
        }else{
          /* turn off proxy locking - already off - NOOP */
          rc = SQLITE_OK;
        }
      }else{
        const char *proxyPath = (const char *)pArg;
        if( isProxyStyle ){
          proxyLockingContext *pCtx = 
            (proxyLockingContext*)pFile->lockingContext;
          if( !strcmp(pArg, ":auto:") 
           || (pCtx->lockProxyPath &&
               !strncmp(pCtx->lockProxyPath, proxyPath, MAXPATHLEN))
          ){
            rc = SQLITE_OK;
          }else{
            rc = switchLockProxyPath(pFile, proxyPath);
          }
        }else{
          /* turn on proxy file locking */
          rc = proxyTransformUnixFile(pFile, proxyPath);
        }
      }
      return rc;
    }
    default: {
      assert( 0 );  /* The call assures that only valid opcodes are sent */
    }
  }
  /*NOTREACHED*/
  return SQLITE_ERROR;
}

/*
** Within this division (the proxying locking implementation) the procedures
** above this point are all utilities.  The lock-related methods of the
** proxy-locking sqlite3_io_method object follow.
*/


/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int proxyCheckReservedLock(sqlite3_file *id, int *pResOut) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      return proxy->pMethod->xCheckReservedLock((sqlite3_file*)proxy, pResOut);
    }else{ /* conchHeld < 0 is lockless */
      pResOut=0;
    }
  }
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int proxyLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xLock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int proxyUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xUnlock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}

/*
** Close a file that uses proxy locks.
*/
static int proxyClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    unixFile *lockProxy = pCtx->lockProxy;
    unixFile *conchFile = pCtx->conchFile;
    int rc = SQLITE_OK;
    
    if( lockProxy ){
      rc = lockProxy->pMethod->xUnlock((sqlite3_file*)lockProxy, NO_LOCK);
      if( rc ) return rc;
      rc = lockProxy->pMethod->xClose((sqlite3_file*)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
      pCtx->lockProxy = 0;
    }
    if( conchFile ){
      if( pCtx->conchHeld ){
        rc = proxyReleaseConch(pFile);
        if( rc ) return rc;
      }
      rc = conchFile->pMethod->xClose((sqlite3_file*)conchFile);
      if( rc ) return rc;
      sqlite3_free(conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath);
    sqlite3DbFree(0, pCtx->dbPath);
    /* restore the original locking context and pMethod then close it */
    pFile->lockingContext = pCtx->oldLockingContext;
    pFile->pMethod = pCtx->pOldMethod;
    sqlite3_free(pCtx);
    return pFile->pMethod->xClose(id);
  }
  return SQLITE_OK;
}



#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The proxy locking style is intended for use with AFP filesystems.
** And since AFP is only supported on MacOSX, the proxy locking is also
** restricted to MacOSX.
** 
**
******************* End of the proxy lock implementation **********************
******************************************************************************/

/*
** Initialize the operating system interface.
**
** This routine registers all VFS implementations for unix-like operating
** systems.  This routine, and the sqlite3_os_end() routine that follows,
** should be the only routines in this file that are visible from other
** files.
**
** This routine is called once during SQLite initialization and by a
** single thread.  The memory allocation and mutex subsystems have not
** necessarily been initialized when this routine is called, and so they
** should not be used.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_init(void){ 
  /* 
  ** The following macro defines an initializer for an sqlite3_vfs object.
  ** The name of the VFS is NAME.  The pAppData is a pointer to a pointer
  ** to the "finder" function.  (pAppData is a pointer to a pointer because
  ** silly C90 rules prohibit a void* from being cast to a function pointer
  ** and so we have to go through the intermediate pointer to avoid problems
  ** when compiling with -pedantic-errors on GCC.)
  **
  ** The FINDER parameter to this macro is the name of the pointer to the
  ** finder-function.  The finder-function returns a pointer to the
  ** sqlite_io_methods object that implements the desired locking
  ** behaviors.  See the division above that contains the IOMETHODS
  ** macro for addition information on finder-functions.
  **
  ** Most finders simply return a pointer to a fixed sqlite3_io_methods
  ** object.  But the "autolockIoFinder" available on MacOSX does a little
  ** more than that; it looks at the filesystem type that hosts the 
  ** database file and tries to choose an locking method appropriate for
  ** that filesystem time.
  */
  #define UNIXVFS(VFSNAME, FINDER) {                        \
    3,                    /* iVersion */                    \
    sizeof(unixFile),     /* szOsFile */                    \
    MAX_PATHNAME,         /* mxPathname */                  \
    0,                    /* pNext */                       \
    VFSNAME,              /* zName */                       \
    (void*)&FINDER,       /* pAppData */                    \
    unixOpen,             /* xOpen */                       \
    unixDelete,           /* xDelete */                     \
    unixAccess,           /* xAccess */                     \
    unixFullPathname,     /* xFullPathname */               \
    unixDlOpen,           /* xDlOpen */                     \
    unixDlError,          /* xDlError */                    \
    unixDlSym,            /* xDlSym */                      \
    unixDlClose,          /* xDlClose */                    \
    unixRandomness,       /* xRandomness */                 \
    unixSleep,            /* xSleep */                      \
    unixCurrentTime,      /* xCurrentTime */                \
    unixGetLastError,     /* xGetLastError */               \
    unixCurrentTimeInt64, /* xCurrentTimeInt64 */           \
    unixSetSystemCall,    /* xSetSystemCall */              \
    unixGetSystemCall,    /* xGetSystemCall */              \
    unixNextSystemCall,   /* xNextSystemCall */             \
  }

  /*
  ** All default VFSes for unix are contained in the following array.
  **
  ** Note that the sqlite3_vfs.pNext field of the VFS object is modified
  ** by the SQLite core when the VFS is registered.  So the following
  ** array cannot be const.
  */
  static sqlite3_vfs aVfs[] = {
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix",          autolockIoFinder ),
#elif OS_VXWORKS
    UNIXVFS("unix",          vxworksIoFinder ),
#else
    UNIXVFS("unix",          posixIoFinder ),
#endif
    UNIXVFS("unix-none",     nolockIoFinder ),
    UNIXVFS("unix-dotfile",  dotlockIoFinder ),
    UNIXVFS("unix-excl",     posixIoFinder ),
#if OS_VXWORKS
    UNIXVFS("unix-namedsem", semIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE || OS_VXWORKS
    UNIXVFS("unix-posix",    posixIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE
    UNIXVFS("unix-flock",    flockIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix-afp",      afpIoFinder ),
    UNIXVFS("unix-nfs",      nfsIoFinder ),
    UNIXVFS("unix-proxy",    proxyIoFinder ),
#endif
  };
  unsigned int i;          /* Loop counter */

  /* Double-check that the aSyscall[] array has been constructed
  ** correctly.  See ticket [bb3a86e890c8e96ab] */
  assert( ArraySize(aSyscall)==25 );

  /* Register all VFSes defined in the aVfs[] array */
  for(i=0; i<(sizeof(aVfs)/sizeof(sqlite3_vfs)); i++){
    sqlite3_vfs_register(&aVfs[i], i==0);
  }
  return SQLITE_OK; 
}

/*
** Shutdown the operating system interface.
**
** Some operating systems might need to do some cleanup in this routine,
** to release dynamically allocated objects.  But not on unix.
** This routine is a no-op for unix.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_end(void){ 
  return SQLITE_OK; 
}
 
#endif /* SQLITE_OS_UNIX */

/************** End of os_unix.c *********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
******************************************************************************/

/******************************************************************************
****************************** No-op Locking **********************************
**
** Of the various locking implementations available, this is by far the
** simplest:  locking is ignored.  No attempt is made to lock the database
** file for reading or writing.
**
** This locking mode is appropriate for use on read-only databases
** (ex: databases that are burned into CD-ROM, for example.)  It can
** also be used if the application employs some external mechanism to
** prevent simultaneous access of the same database by two or more
** database connections.  But there is a serious risk of database
** corruption if this locking mode is used in situations where multiple
** database connections are accessing the same database file at the same
** time and one or more of those connections are writing.
*/

static int nolockCheckReservedLock(sqlite3_file *NotUsed, int *pResOut){
  UNUSED_PARAMETER(NotUsed);
  *pResOut = 0;
  return SQLITE_OK;
}
static int nolockLock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}
static int nolockUnlock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}

/*
** Close the file.
*/
static int nolockClose(sqlite3_file *id) {
  return closeUnixFile(id);
}

/******************* End of the no-op lock implementation *********************
******************************************************************************/

/******************************************************************************
************************* Begin dot-file Locking ******************************
**
** The dotfile locking implementation uses the existence of separate lock
** files (really a directory) to control access to the database.  This works
** on just about every filesystem imaginable.  But there are serious downsides:
**
**    (1)  There is zero concurrency.  A single reader blocks all other
**         connections from reading or writing the database.
**
**    (2)  An application crash or power loss can leave stale lock files
**         sitting around that need to be cleared manually.
**
** Nevertheless, a dotlock is an appropriate locking mode for use if no
** other locking strategy is available.
**
** Dotfile locking works by creating a subdirectory in the same directory as
** the database and with the same name but with a ".lock" extension added.
** The existence of a lock directory implies an EXCLUSIVE lock.  All other
** lock types (SHARED, RESERVED, PENDING) are mapped into EXCLUSIVE.
*/

/*
** The file suffix added to the data base filename in order to create the
** lock directory.
*/
#define DOTLOCK_SUFFIX ".lock"

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
**
** In dotfile locking, either a lock exists or it does not.  So in this
** variation of CheckReservedLock(), *pResOut is set to true if any lock
** is held on the file and false if the file is unlocked.
*/
static int dotlockCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    /* Either this connection or some other connection in the same process
    ** holds a lock on the file.  No need to check further. */
    reserved = 1;
  }else{
    /* The lock is held if and only if the lockfile exists */
    const char *zLockFile = (const char*)pFile->lockingContext;
    reserved = osAccess(zLockFile, 0)==0;
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (dotlock)\n", pFile->h, rc, reserved));
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
**
** With dotfile locking, we really only support state (4): EXCLUSIVE.
** But we track the other locking levels internally.
*/
static int dotlockLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc = SQLITE_OK;


  /* If we have any lock, then the lock file already exists.  All we have
  ** to do is adjust our internal record of the lock level.
  */
  if( pFile->eFileLock > NO_LOCK ){
    pFile->eFileLock = eFileLock;
    /* Always update the timestamp on the old file */
#ifdef HAVE_UTIME
    utime(zLockFile, NULL);
#else
    utimes(zLockFile, NULL);
#endif
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  rc = osMkdir(zLockFile, 0777);
  if( rc<0 ){
    /* failed to open/create the lock directory */
    int tErrno = errno;
    if( EEXIST == tErrno ){
      rc = SQLITE_BUSY;
    } else {
      rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
      if( IS_LOCK_ERROR(rc) ){
        storeLastErrno(pFile, tErrno);
      }
    }
    return rc;
  } 
  
  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
**
** When the locking level reaches NO_LOCK, delete the lock file.
*/
static int dotlockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc;

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (dotlock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }

  /* To downgrade to shared, simply update our internal notion of the
  ** lock state.  No need to mess with the file on disk.
  */
  if( eFileLock==SHARED_LOCK ){
    pFile->eFileLock = SHARED_LOCK;
    return SQLITE_OK;
  }
  
  /* To fully unlock the database, delete the lock file */
  assert( eFileLock==NO_LOCK );
  rc = osRmdir(zLockFile);
  if( rc<0 && errno==ENOTDIR ) rc = osUnlink(zLockFile);
  if( rc<0 ){
    int tErrno = errno;
    rc = 0;
    if( ENOENT != tErrno ){
      rc = SQLITE_IOERR_UNLOCK;
    }
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
** Close a file.  Make sure the lock has been released before closing.
*/
static int dotlockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    dotlockUnlock(id, NO_LOCK);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
  }
  return rc;
}
/****************** End of the dot-file lock implementation *******************
******************************************************************************/

/******************************************************************************
************************** Begin flock Locking ********************************
**
** Use the flock() system call to do file locking.
**
** flock() locking is like dot-file locking in that the various
** fine-grain locking levels supported by SQLite are collapsed into
** a single exclusive lock.  In other words, SHARED, RESERVED, and
** PENDING locks are the same thing as an EXCLUSIVE lock.  SQLite
** still works when you do this, but concurrency is reduced since
** only a single process can be reading the database at a time.
**
** Omit this section if SQLITE_ENABLE_LOCKING_STYLE is turned off
*/
#if SQLITE_ENABLE_LOCKING_STYLE

/*
** Retry flock() calls that fail with EINTR
*/
#ifdef EINTR
static int robust_flock(int fd, int op){
  int rc;
  do{ rc = flock(fd,op); }while( rc<0 && errno==EINTR );
  return rc;
}
#else
# define robust_flock(a,b) flock(a,b)
#endif
     

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int flockCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    /* attempt to get the lock */
    int lrc = robust_flock(pFile->h, LOCK_EX | LOCK_NB);
    if( !lrc ){
      /* got the lock, unlock it */
      lrc = robust_flock(pFile->h, LOCK_UN);
      if ( lrc ) {
        int tErrno = errno;
        /* unlock failed with an error */
        lrc = SQLITE_IOERR_UNLOCK; 
        if( IS_LOCK_ERROR(lrc) ){
          storeLastErrno(pFile, tErrno);
          rc = lrc;
        }
      }
    } else {
      int tErrno = errno;
      reserved = 1;
      /* someone else might have it reserved */
      lrc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK); 
      if( IS_LOCK_ERROR(lrc) ){
        storeLastErrno(pFile, tErrno);
        rc = lrc;
      }
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (flock)\n", pFile->h, rc, reserved));

#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_OK;
    reserved=1;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** flock() only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int flockLock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;

  assert( pFile );

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  
  if (robust_flock(pFile->h, LOCK_EX | LOCK_NB)) {
    int tErrno = errno;
    /* didn't get, must be busy */
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
  } else {
    /* got it, set the type and return ok */
    pFile->eFileLock = eFileLock;
  }
  OSTRACE(("LOCK    %d %s %s (flock)\n", pFile->h, azFileLock(eFileLock), 
           rc==SQLITE_OK ? "ok" : "failed"));
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_BUSY;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int flockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  
  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (flock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really, unlock. */
  if( robust_flock(pFile->h, LOCK_UN) ){
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
    return SQLITE_OK;
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
    return SQLITE_IOERR_UNLOCK;
  }else{
    pFile->eFileLock = NO_LOCK;
    return SQLITE_OK;
  }
}

/*
** Close a file.
*/
static int flockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    flockUnlock(id, NO_LOCK);
    rc = closeUnixFile(id);
  }
  return rc;
}

#endif /* SQLITE_ENABLE_LOCKING_STYLE && !OS_VXWORK */

/******************* End of the flock lock implementation *********************
******************************************************************************/

/******************************************************************************
************************ Begin Named Semaphore Locking ************************
**
** Named semaphore locking is only supported on VxWorks.
**
** Semaphore locking is like dot-lock and flock in that it really only
** supports EXCLUSIVE locking.  Only a single process can read or write
** the database file at a time.  This reduces potential concurrency, but
** makes the lock implementation much easier.
*/
#if OS_VXWORKS

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int semXCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    sem_t *pSem = pFile->pInode->pSem;

    if( sem_trywait(pSem)==-1 ){
      int tErrno = errno;
      if( EAGAIN != tErrno ){
        rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_CHECKRESERVEDLOCK);
        storeLastErrno(pFile, tErrno);
      } else {
        /* someone else has the lock when we are in NO_LOCK */
        reserved = (pFile->eFileLock < SHARED_LOCK);
      }
    }else{
      /* we could have it if we want it */
      sem_post(pSem);
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (sem)\n", pFile->h, rc, reserved));

  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** Semaphore locks only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int semXLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;
  int rc = SQLITE_OK;

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    rc = SQLITE_OK;
    goto sem_end_lock;
  }
  
  /* lock semaphore now but bail out when already locked. */
  if( sem_trywait(pSem)==-1 ){
    rc = SQLITE_BUSY;
    goto sem_end_lock;
  }

  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;

 sem_end_lock:
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int semXUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;

  assert( pFile );
  assert( pSem );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (sem)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really unlock. */
  if ( sem_post(pSem)==-1 ) {
    int rc, tErrno = errno;
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_UNLOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
 ** Close a file.
 */
static int semXClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    semXUnlock(id, NO_LOCK);
    assert( pFile );
    unixEnterMutex();
    releaseInodeInfo(pFile);
    unixLeaveMutex();
    closeUnixFile(id);
  }
  return SQLITE_OK;
}

#endif /* OS_VXWORKS */
/*
** Named semaphore locking is only available on VxWorks.
**
*************** End of the named semaphore lock implementation ****************
******************************************************************************/


/******************************************************************************
*************************** Begin AFP Locking *********************************
**
** AFP is the Apple Filing Protocol.  AFP is a network filesystem found
** on Apple Macintosh computers - both OS9 and OSX.
**
** Third-party implementations of AFP are available.  But this code here
** only works on OSX.
*/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
** The afpLockingContext structure contains all afp lock specific state
*/
typedef struct afpLockingContext afpLockingContext;
struct afpLockingContext {
  int reserved;
  const char *dbPath;             /* Name of the open file */
};

struct ByteRangeLockPB2
{
  unsigned long long offset;        /* offset to first byte to lock */
  unsigned long long length;        /* nbr of bytes to lock */
  unsigned long long retRangeStart; /* nbr of 1st byte locked if successful */
  unsigned char unLockFlag;         /* 1 = unlock, 0 = lock */
  unsigned char startEndFlag;       /* 1=rel to end of fork, 0=rel to start */
  int fd;                           /* file desc to assoc this lock with */
};

#define afpfsByteRangeLock2FSCTL        _IOWR('z', 23, struct ByteRangeLockPB2)

/*
** This is a utility for setting or clearing a bit-range lock on an
** AFP filesystem.
** 
** Return SQLITE_OK on success, SQLITE_BUSY on failure.
*/
static int afpSetLock(
  const char *path,              /* Name of the file to be locked or unlocked */
  unixFile *pFile,               /* Open file descriptor on path */
  unsigned long long offset,     /* First byte to be locked */
  unsigned long long length,     /* Number of bytes to lock */
  int setLockFlag                /* True to set lock.  False to clear lock */
){
  struct ByteRangeLockPB2 pb;
  int err;
  
  pb.unLockFlag = setLockFlag ? 0 : 1;
  pb.startEndFlag = 0;
  pb.offset = offset;
  pb.length = length; 
  pb.fd = pFile->h;
  
  OSTRACE(("AFPSETLOCK [%s] for %d%s in range %llx:%llx\n", 
    (setLockFlag?"ON":"OFF"), pFile->h, (pb.fd==-1?"[testval-1]":""),
    offset, length));
  err = fsctl(path, afpfsByteRangeLock2FSCTL, &pb, 0);
  if ( err==-1 ) {
    int rc;
    int tErrno = errno;
    OSTRACE(("AFPSETLOCK failed to fsctl() '%s' %d %s\n",
             path, tErrno, strerror(tErrno)));
#ifdef SQLITE_IGNORE_AFP_LOCK_ERRORS
    rc = SQLITE_BUSY;
#else
    rc = sqliteErrorFromPosixError(tErrno,
                    setLockFlag ? SQLITE_IOERR_LOCK : SQLITE_IOERR_UNLOCK);
#endif /* SQLITE_IGNORE_AFP_LOCK_ERRORS */
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc;
  } else {
    return SQLITE_OK;
  }
}

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int afpCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  afpLockingContext *context;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  context = (afpLockingContext *) pFile->lockingContext;
  if( context->reserved ){
    *pResOut = 1;
    return SQLITE_OK;
  }
  unixEnterMutex(); /* Because pFile->pInode is shared across threads */
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->pInode->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it.
   */
  if( !reserved ){
    /* lock the RESERVED byte */
    int lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);  
    if( SQLITE_OK==lrc ){
      /* if we succeeded in taking the reserved lock, unlock it to restore
      ** the original state */
      lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
    } else {
      /* if we failed to get the lock then someone else must have it */
      reserved = 1;
    }
    if( IS_LOCK_ERROR(lrc) ){
      rc=lrc;
    }
  }
  
  unixLeaveMutex();
  OSTRACE(("TEST WR-LOCK %d %d %d (afp)\n", pFile->h, rc, reserved));
  
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int afpLock(sqlite3_file *id, int eFileLock){
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode = pFile->pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  
  assert( pFile );
  OSTRACE(("LOCK    %d %s was %s(%s,%d) pid=%d (afp)\n", pFile->h,
           azFileLock(eFileLock), azFileLock(pFile->eFileLock),
           azFileLock(pInode->eFileLock), pInode->nShared , osGetpid(0)));

  /* If there is already a lock of this type or more restrictive on the
  ** unixFile, do nothing. Don't use the afp_end_lock: exit path, as
  ** unixEnterMutex() hasn't been called yet.
  */
  if( pFile->eFileLock>=eFileLock ){
    OSTRACE(("LOCK    %d %s ok (already held) (afp)\n", pFile->h,
           azFileLock(eFileLock)));
    return SQLITE_OK;
  }

  /* Make sure the locking sequence is correct
  **  (1) We never move from unlocked to anything higher than shared lock.
  **  (2) SQLite never explicitly requests a pendig lock.
  **  (3) A shared lock is always held when a reserve lock is requested.
  */
  assert( pFile->eFileLock!=NO_LOCK || eFileLock==SHARED_LOCK );
  assert( eFileLock!=PENDING_LOCK );
  assert( eFileLock!=RESERVED_LOCK || pFile->eFileLock==SHARED_LOCK );
  
  /* This mutex is needed because pFile->pInode is shared across threads
  */
  unixEnterMutex();
  pInode = pFile->pInode;

  /* If some thread using this PID has a lock via a different unixFile*
  ** handle that precludes the requested lock, return BUSY.
  */
  if( (pFile->eFileLock!=pInode->eFileLock && 
       (pInode->eFileLock>=PENDING_LOCK || eFileLock>SHARED_LOCK))
     ){
    rc = SQLITE_BUSY;
    goto afp_end_lock;
  }
  
  /* If a SHARED lock is requested, and some thread using this PID already
  ** has a SHARED or RESERVED lock, then increment reference counts and
  ** return SQLITE_OK.
  */
  if( eFileLock==SHARED_LOCK && 
     (pInode->eFileLock==SHARED_LOCK || pInode->eFileLock==RESERVED_LOCK) ){
    assert( eFileLock==SHARED_LOCK );
    assert( pFile->eFileLock==0 );
    assert( pInode->nShared>0 );
    pFile->eFileLock = SHARED_LOCK;
    pInode->nShared++;
    pInode->nLock++;
    goto afp_end_lock;
  }
    
  /* A PENDING lock is needed before acquiring a SHARED lock and before
  ** acquiring an EXCLUSIVE lock.  For the SHARED lock, the PENDING will
  ** be released.
  */
  if( eFileLock==SHARED_LOCK 
      || (eFileLock==EXCLUSIVE_LOCK && pFile->eFileLock<PENDING_LOCK)
  ){
    int failed;
    failed = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 1);
    if (failed) {
      rc = failed;
      goto afp_end_lock;
    }
  }
  
  /* If control gets to this point, then actually go ahead and make
  ** operating system calls for the specified lock.
  */
  if( eFileLock==SHARED_LOCK ){
    int lrc1, lrc2, lrc1Errno = 0;
    long lk, mask;
    
    assert( pInode->nShared==0 );
    assert( pInode->eFileLock==0 );
        
    mask = (sizeof(long)==8) ? LARGEST_INT64 : 0x7fffffff;
    /* Now get the read-lock SHARED_LOCK */
    /* note that the quality of the randomness doesn't matter that much */
    lk = random(); 
    pInode->sharedByte = (lk & mask)%(SHARED_SIZE - 1);
    lrc1 = afpSetLock(context->dbPath, pFile, 
          SHARED_FIRST+pInode->sharedByte, 1, 1);
    if( IS_LOCK_ERROR(lrc1) ){
      lrc1Errno = pFile->lastErrno;
    }
    /* Drop the temporary PENDING lock */
    lrc2 = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    
    if( IS_LOCK_ERROR(lrc1) ) {
      storeLastErrno(pFile, lrc1Errno);
      rc = lrc1;
      goto afp_end_lock;
    } else if( IS_LOCK_ERROR(lrc2) ){
      rc = lrc2;
      goto afp_end_lock;
    } else if( lrc1 != SQLITE_OK ) {
      rc = lrc1;
    } else {
      pFile->eFileLock = SHARED_LOCK;
      pInode->nLock++;
      pInode->nShared = 1;
    }
  }else if( eFileLock==EXCLUSIVE_LOCK && pInode->nShared>1 ){
    /* We are trying for an exclusive lock but another thread in this
     ** same process is still holding a shared lock. */
    rc = SQLITE_BUSY;
  }else{
    /* The request was for a RESERVED or EXCLUSIVE lock.  It is
    ** assumed that there is a SHARED or greater lock on the file
    ** already.
    */
    int failed = 0;
    assert( 0!=pFile->eFileLock );
    if (eFileLock >= RESERVED_LOCK && pFile->eFileLock < RESERVED_LOCK) {
        /* Acquire a RESERVED lock */
        failed = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);
      if( !failed ){
        context->reserved = 1;
      }
    }
    if (!failed && eFileLock == EXCLUSIVE_LOCK) {
      /* Acquire an EXCLUSIVE lock */
        
      /* Remove the shared lock before trying the range.  we'll need to 
      ** reestablish the shared lock if we can't get the  afpUnlock
      */
      if( !(failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST +
                         pInode->sharedByte, 1, 0)) ){
        int failed2 = SQLITE_OK;
        /* now attemmpt to get the exclusive lock range */
        failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST, 
                               SHARED_SIZE, 1);
        if( failed && (failed2 = afpSetLock(context->dbPath, pFile, 
                       SHARED_FIRST + pInode->sharedByte, 1, 1)) ){
          /* Can't reestablish the shared lock.  Sqlite can't deal, this is
          ** a critical I/O error
          */
          rc = ((failed & SQLITE_IOERR) == SQLITE_IOERR) ? failed2 : 
               SQLITE_IOERR_LOCK;
          goto afp_end_lock;
        } 
      }else{
        rc = failed; 
      }
    }
    if( failed ){
      rc = failed;
    }
  }
  
  if( rc==SQLITE_OK ){
    pFile->eFileLock = eFileLock;
    pInode->eFileLock = eFileLock;
  }else if( eFileLock==EXCLUSIVE_LOCK ){
    pFile->eFileLock = PENDING_LOCK;
    pInode->eFileLock = PENDING_LOCK;
  }
  
afp_end_lock:
  unixLeaveMutex();
  OSTRACE(("LOCK    %d %s %s (afp)\n", pFile->h, azFileLock(eFileLock), 
         rc==SQLITE_OK ? "ok" : "failed"));
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int afpUnlock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  int skipShared = 0;
#ifdef SQLITE_TEST
  int h = pFile->h;
#endif

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d(%d,%d) pid=%d (afp)\n", pFile->h, eFileLock,
           pFile->eFileLock, pFile->pInode->eFileLock, pFile->pInode->nShared,
           osGetpid(0)));

  assert( eFileLock<=SHARED_LOCK );
  if( pFile->eFileLock<=eFileLock ){
    return SQLITE_OK;
  }
  unixEnterMutex();
  pInode = pFile->pInode;
  assert( pInode->nShared!=0 );
  if( pFile->eFileLock>SHARED_LOCK ){
    assert( pInode->eFileLock==pFile->eFileLock );
    SimulateIOErrorBenign(1);
    SimulateIOError( h=(-1) )
    SimulateIOErrorBenign(0);
    
#ifdef SQLITE_DEBUG
    /* When reducing a lock such that other processes can start
    ** reading the database file again, make sure that the
    ** transaction counter was updated if any part of the database
    ** file changed.  If the transaction counter is not updated,
    ** other connections to the same file might not realize that
    ** the file has changed and hence might not know to flush their
    ** cache.  The use of a stale cache can lead to database corruption.
    */
    assert( pFile->inNormalWrite==0
           || pFile->dbUpdate==0
           || pFile->transCntrChng==1 );
    pFile->inNormalWrite = 0;
#endif
    
    if( pFile->eFileLock==EXCLUSIVE_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, SHARED_FIRST, SHARED_SIZE, 0);
      if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1) ){
        /* only re-establish the shared lock if necessary */
        int sharedLockByte = SHARED_FIRST+pInode->sharedByte;
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 1);
      } else {
        skipShared = 1;
      }
    }
    if( rc==SQLITE_OK && pFile->eFileLock>=PENDING_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    } 
    if( rc==SQLITE_OK && pFile->eFileLock>=RESERVED_LOCK && context->reserved ){
      rc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
      if( !rc ){ 
        context->reserved = 0; 
      }
    }
    if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1)){
      pInode->eFileLock = SHARED_LOCK;
    }
  }
  if( rc==SQLITE_OK && eFileLock==NO_LOCK ){

    /* Decrement the shared lock counter.  Release the lock using an
    ** OS call only when all threads in this same process have released
    ** the lock.
    */
    unsigned long long sharedLockByte = SHARED_FIRST+pInode->sharedByte;
    pInode->nShared--;
    if( pInode->nShared==0 ){
      SimulateIOErrorBenign(1);
      SimulateIOError( h=(-1) )
      SimulateIOErrorBenign(0);
      if( !skipShared ){
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 0);
      }
      if( !rc ){
        pInode->eFileLock = NO_LOCK;
        pFile->eFileLock = NO_LOCK;
      }
    }
    if( rc==SQLITE_OK ){
      pInode->nLock--;
      assert( pInode->nLock>=0 );
      if( pInode->nLock==0 ){
        closePendingFds(pFile);
      }
    }
  }
  
  unixLeaveMutex();
  if( rc==SQLITE_OK ) pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Close a file & cleanup AFP specific locking context 
*/
static int afpClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    afpUnlock(id, NO_LOCK);
    unixEnterMutex();
    if( pFile->pInode && pFile->pInode->nLock ){
      /* If there are outstanding locks, do not actually close the file just
      ** yet because that would clear those locks.  Instead, add the file
      ** descriptor to pInode->aPending.  It will be automatically closed when
      ** the last lock is cleared.
      */
      setPendingFd(pFile);
    }
    releaseInodeInfo(pFile);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
    unixLeaveMutex();
  }
  return rc;
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the AFP lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  If you don't compile for a mac, then the "unix-afp"
** VFS is not available.
**
********************* End of the AFP lock implementation **********************
******************************************************************************/

/******************************************************************************
*************************** Begin NFS Locking ********************************/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
 ** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
 ** must be either NO_LOCK or SHARED_LOCK.
 **
 ** If the locking level of the file descriptor is already at or below
 ** the requested locking level, this routine is a no-op.
 */
static int nfsUnlock(sqlite3_file *id, int eFileLock){
  return posixUnlock(id, eFileLock, 1);
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the NFS lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  
**
********************* End of the NFS lock implementation **********************
******************************************************************************/

/******************************************************************************
**************** Non-locking sqlite3_file methods *****************************
**
** The next division contains implementations for all methods of the 
** sqlite3_file object other than the locking methods.  The locking
** methods were defined in divisions above (one locking method per
** division).  Those methods that are common to all locking modes
** are gather together into this division.
*/

/*
** Seek to the offset passed as the second argument, then read cnt 
** bytes into pBuf. Return the number of bytes actually read.
**
** NB:  If you define USE_PREAD or USE_PREAD64, then it might also
** be necessary to define _XOPEN_SOURCE to be 500.  This varies from
** one system to another.  Since SQLite does not define USE_PREAD
** in any form by default, we will not attempt to define _XOPEN_SOURCE.
** See tickets #2741 and #2681.
**
** To avoid stomping the errno value on a failed read the lastErrno value
** is set before returning.
*/
static int seekAndRead(unixFile *id, sqlite3_int64 offset, void *pBuf, int cnt){
  int got;
  int prior = 0;
#if (!defined(USE_PREAD) && !defined(USE_PREAD64))
  i64 newOffset;
#endif
  TIMER_START;
  assert( cnt==(cnt&0x1ffff) );
  assert( id->h>2 );
  do{
#if defined(USE_PREAD)
    got = osPread(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#elif defined(USE_PREAD64)
    got = osPread64(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#else
    newOffset = lseek(id->h, offset, SEEK_SET);
    SimulateIOError( newOffset-- );
    if( newOffset!=offset ){
      if( newOffset == -1 ){
        storeLastErrno((unixFile*)id, errno);
      }else{
        storeLastErrno((unixFile*)id, 0);
      }
      return -1;
    }
    got = osRead(id->h, pBuf, cnt);
#endif
    if( got==cnt ) break;
    if( got<0 ){
      if( errno==EINTR ){ got = 1; continue; }
      prior = 0;
      storeLastErrno((unixFile*)id,  errno);
      break;
    }else if( got>0 ){
      cnt -= got;
      offset += got;
      prior += got;
      pBuf = (void*)(got + (char*)pBuf);
    }
  }while( got>0 );
  TIMER_END;
  OSTRACE(("READ    %-3d %5d %7lld %llu\n",
            id->h, got+prior, offset-prior, TIMER_ELAPSED));
  return got+prior;
}

/*
** Read data from a file into a buffer.  Return SQLITE_OK if all
** bytes were read successfully and SQLITE_IOERR if anything goes
** wrong.
*/
static int unixRead(
  sqlite3_file *id, 
  void *pBuf, 
  int amt,
  sqlite3_int64 offset
){
  unixFile *pFile = (unixFile *)id;
  int got;
  assert( id );
  assert( offset>=0 );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this read request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif

  got = seekAndRead(pFile, offset, pBuf, amt);
  if( got==amt ){
    return SQLITE_OK;
  }else if( got<0 ){
    /* lastErrno set by seekAndRead */
    return SQLITE_IOERR_READ;
  }else{
    storeLastErrno(pFile, 0);   /* not a system error */
    /* Unread parts of the buffer must be zero-filled */
    memset(&((char*)pBuf)[got], 0, amt-got);
    return SQLITE_IOERR_SHORT_READ;
  }
}

/*
** Attempt to seek the file-descriptor passed as the first argument to
** absolute offset iOff, then attempt to write nBuf bytes of data from
** pBuf to it. If an error occurs, return -1 and set *piErrno. Otherwise, 
** return the actual number of bytes written (which may be less than
** nBuf).
*/
static int seekAndWriteFd(
  int fd,                         /* File descriptor to write to */
  i64 iOff,                       /* File offset to begin writing at */
  const void *pBuf,               /* Copy data from this buffer to the file */
  int nBuf,                       /* Size of buffer pBuf in bytes */
  int *piErrno                    /* OUT: Error number if error occurs */
){
  int rc = 0;                     /* Value returned by system call */

  assert( nBuf==(nBuf&0x1ffff) );
  assert( fd>2 );
  nBuf &= 0x1ffff;
  TIMER_START;

#if defined(USE_PREAD)
  do{ rc = (int)osPwrite(fd, pBuf, nBuf, iOff); }while( rc<0 && errno==EINTR );
#elif defined(USE_PREAD64)
  do{ rc = (int)osPwrite64(fd, pBuf, nBuf, iOff);}while( rc<0 && errno==EINTR);
#else
  do{
    i64 iSeek = lseek(fd, iOff, SEEK_SET);
    SimulateIOError( iSeek-- );

    if( iSeek!=iOff ){
      if( piErrno ) *piErrno = (iSeek==-1 ? errno : 0);
      return -1;
    }
    rc = osWrite(fd, pBuf, nBuf);
  }while( rc<0 && errno==EINTR );
#endif

  TIMER_END;
  OSTRACE(("WRITE   %-3d %5d %7lld %llu\n", fd, rc, iOff, TIMER_ELAPSED));

  if( rc<0 && piErrno ) *piErrno = errno;
  return rc;
}


/*
** Seek to the offset in id->offset then read cnt bytes into pBuf.
** Return the number of bytes actually read.  Update the offset.
**
** To avoid stomping the errno value on a failed write the lastErrno value
** is set before returning.
*/
static int seekAndWrite(unixFile *id, i64 offset, const void *pBuf, int cnt){
  return seekAndWriteFd(id->h, offset, pBuf, cnt, &id->lastErrno);
}


/*
** Write data from a buffer into a file.  Return SQLITE_OK on success
** or some other error code on failure.
*/
static int unixWrite(
  sqlite3_file *id, 
  const void *pBuf, 
  int amt,
  sqlite3_int64 offset 
){
  unixFile *pFile = (unixFile*)id;
  int wrote = 0;
  assert( id );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#ifdef SQLITE_DEBUG
  /* If we are doing a normal write to a database file (as opposed to
  ** doing a hot-journal rollback or a write to some file other than a
  ** normal database file) then record the fact that the database
  ** has changed.  If the transaction counter is modified, record that
  ** fact too.
  */
  if( pFile->inNormalWrite ){
    pFile->dbUpdate = 1;  /* The database has been modified */
    if( offset<=24 && offset+amt>=27 ){
      int rc;
      char oldCntr[4];
      SimulateIOErrorBenign(1);
      rc = seekAndRead(pFile, 24, oldCntr, 4);
      SimulateIOErrorBenign(0);
      if( rc!=4 || memcmp(oldCntr, &((char*)pBuf)[24-offset], 4)!=0 ){
        pFile->transCntrChng = 1;  /* The transaction counter has changed */
      }
    }
  }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this write request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif
 
  while( (wrote = seekAndWrite(pFile, offset, pBuf, amt))<amt && wrote>0 ){
    amt -= wrote;
    offset += wrote;
    pBuf = &((char*)pBuf)[wrote];
  }
  SimulateIOError(( wrote=(-1), amt=1 ));
  SimulateDiskfullError(( wrote=0, amt=1 ));

  if( amt>wrote ){
    if( wrote<0 && pFile->lastErrno!=ENOSPC ){
      /* lastErrno set by seekAndWrite */
      return SQLITE_IOERR_WRITE;
    }else{
      storeLastErrno(pFile, 0); /* not a system error */
      return SQLITE_FULL;
    }
  }

  return SQLITE_OK;
}

#ifdef SQLITE_TEST
/*
** Count the number of fullsyncs and normal syncs.  This is used to test
** that syncs and fullsyncs are occurring at the right times.
*/
SQLITE_API int sqlite3_sync_count = 0;
SQLITE_API int sqlite3_fullsync_count = 0;
#endif

/*
** We do not trust systems to provide a working fdatasync().  Some do.
** Others do no.  To be safe, we will stick with the (slightly slower)
** fsync(). If you know that your system does support fdatasync() correctly,
** then simply compile with -Dfdatasync=fdatasync or -DHAVE_FDATASYNC
*/
#if !defined(fdatasync) && !HAVE_FDATASYNC
# define fdatasync fsync
#endif

/*
** Define HAVE_FULLFSYNC to 0 or 1 depending on whether or not
** the F_FULLFSYNC macro is defined.  F_FULLFSYNC is currently
** only available on Mac OS X.  But that could change.
*/
#ifdef F_FULLFSYNC
# define HAVE_FULLFSYNC 1
#else
# define HAVE_FULLFSYNC 0
#endif


/*
** The fsync() system call does not work as advertised on many
** unix systems.  The following procedure is an attempt to make
** it work better.
**
** The SQLITE_NO_SYNC macro disables all fsync()s.  This is useful
** for testing when we want to run through the test suite quickly.
** You are strongly advised *not* to deploy with SQLITE_NO_SYNC
** enabled, however, since with SQLITE_NO_SYNC enabled, an OS crash
** or power failure will likely corrupt the database file.
**
** SQLite sets the dataOnly flag if the size of the file is unchanged.
** The idea behind dataOnly is that it should only write the file content
** to disk, not the inode.  We only set dataOnly if the file size is 
** unchanged since the file size is part of the inode.  However, 
** Ted Ts'o tells us that fdatasync() will also write the inode if the
** file size has changed.  The only real difference between fdatasync()
** and fsync(), Ted tells us, is that fdatasync() will not flush the
** inode if the mtime or owner or other inode attributes have changed.
** We only care about the file size, not the other file attributes, so
** as far as SQLite is concerned, an fdatasync() is always adequate.
** So, we always use fdatasync() if it is available, regardless of
** the value of the dataOnly flag.
*/
static int full_fsync(int fd, int fullSync, int dataOnly){
  int rc;

  /* The following "ifdef/elif/else/" block has the same structure as
  ** the one below. It is replicated here solely to avoid cluttering 
  ** up the real code with the UNUSED_PARAMETER() macros.
  */
#ifdef SQLITE_NO_SYNC
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#elif HAVE_FULLFSYNC
  UNUSED_PARAMETER(dataOnly);
#else
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#endif

  /* Record the number of times that we do a normal fsync() and 
  ** FULLSYNC.  This is used during testing to verify that this procedure
  ** gets called with the correct arguments.
  */
#ifdef SQLITE_TEST
  if( fullSync ) sqlite3_fullsync_count++;
  sqlite3_sync_count++;
#endif

  /* If we compiled with the SQLITE_NO_SYNC flag, then syncing is a
  ** no-op
  */
#ifdef SQLITE_NO_SYNC
  rc = SQLITE_OK;
#elif HAVE_FULLFSYNC
  if( fullSync ){
    rc = osFcntl(fd, F_FULLFSYNC, 0);
  }else{
    rc = 1;
  }
  /* If the FULLFSYNC failed, fall back to attempting an fsync().
  ** It shouldn't be possible for fullfsync to fail on the local 
  ** file system (on OSX), so failure indicates that FULLFSYNC
  ** isn't supported for this file system. So, attempt an fsync 
  ** and (for now) ignore the overhead of a superfluous fcntl call.  
  ** It'd be better to detect fullfsync support once and avoid 
  ** the fcntl call every time sync is called.
  */
  if( rc ) rc = fsync(fd);

#elif defined(__APPLE__)
  /* fdatasync() on HFS+ doesn't yet flush the file size if it changed correctly
  ** so currently we default to the macro that redefines fdatasync to fsync
  */
  rc = fsync(fd);
#else 
  rc = fdatasync(fd);
#if OS_VXWORKS
  if( rc==-1 && errno==ENOTSUP ){
    rc = fsync(fd);
  }
#endif /* OS_VXWORKS */
#endif /* ifdef SQLITE_NO_SYNC elif HAVE_FULLFSYNC */

  if( OS_VXWORKS && rc!= -1 ){
    rc = 0;
  }
  return rc;
}

/*
** Open a file descriptor to the directory containing file zFilename.
** If successful, *pFd is set to the opened file descriptor and
** SQLITE_OK is returned. If an error occurs, either SQLITE_NOMEM
** or SQLITE_CANTOPEN is returned and *pFd is set to an undefined
** value.
**
** The directory file descriptor is used for only one thing - to
** fsync() a directory to make sure file creation and deletion events
** are flushed to disk.  Such fsyncs are not needed on newer
** journaling filesystems, but are required on older filesystems.
**
** This routine can be overridden using the xSetSysCall interface.
** The ability to override this routine was added in support of the
** chromium sandbox.  Opening a directory is a security risk (we are
** told) so making it overrideable allows the chromium sandbox to
** replace this routine with a harmless no-op.  To make this routine
** a no-op, replace it with a stub that returns SQLITE_OK but leaves
** *pFd set to a negative number.
**
** If SQLITE_OK is returned, the caller is responsible for closing
** the file descriptor *pFd using close().
*/
static int openDirectory(const char *zFilename, int *pFd){
  int ii;
  int fd = -1;
  char zDirname[MAX_PATHNAME+1];

  sqlite3_snprintf(MAX_PATHNAME, zDirname, "%s", zFilename);
  for(ii=(int)strlen(zDirname); ii>1 && zDirname[ii]!='/'; ii--);
  if( ii>0 ){
    zDirname[ii] = '\0';
    fd = robust_open(zDirname, O_RDONLY|O_BINARY, 0);
    if( fd>=0 ){
      OSTRACE(("OPENDIR %-3d %s\n", fd, zDirname));
    }
  }
  *pFd = fd;
  return (fd>=0?SQLITE_OK:unixLogError(SQLITE_CANTOPEN_BKPT, "open", zDirname));
}

/*
** Make sure all writes to a particular file are committed to disk.
**
** If dataOnly==0 then both the file itself and its metadata (file
** size, access time, etc) are synced.  If dataOnly!=0 then only the
** file data is synced.
**
** Under Unix, also make sure that the directory entry for the file
** has been created by fsync-ing the directory that contains the file.
** If we do not do this and we encounter a power failure, the directory
** entry for the journal might not exist after we reboot.  The next
** SQLite to access the file will not know that the journal exists (because
** the directory entry for the journal was never created) and the transaction
** will not roll back - possibly leading to database corruption.
*/
static int unixSync(sqlite3_file *id, int flags){
  int rc;
  unixFile *pFile = (unixFile*)id;

  int isDataOnly = (flags&SQLITE_SYNC_DATAONLY);
  int isFullsync = (flags&0x0F)==SQLITE_SYNC_FULL;

  /* Check that one of SQLITE_SYNC_NORMAL or FULL was passed */
  assert((flags&0x0F)==SQLITE_SYNC_NORMAL
      || (flags&0x0F)==SQLITE_SYNC_FULL
  );

  /* Unix cannot, but some systems may return SQLITE_FULL from here. This
  ** line is to test that doing so does not cause any problems.
  */
  SimulateDiskfullError( return SQLITE_FULL );

  assert( pFile );
  OSTRACE(("SYNC    %-3d\n", pFile->h));
  rc = full_fsync(pFile->h, isFullsync, isDataOnly);
  SimulateIOError( rc=1 );
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_FSYNC, "full_fsync", pFile->zPath);
  }

  /* Also fsync the directory containing the file if the DIRSYNC flag
  ** is set.  This is a one-time occurrence.  Many systems (examples: AIX)
  ** are unable to fsync a directory, so ignore errors on the fsync.
  */
  if( pFile->ctrlFlags & UNIXFILE_DIRSYNC ){
    int dirfd;
    OSTRACE(("DIRSYNC %s (have_fullfsync=%d fullsync=%d)\n", pFile->zPath,
            HAVE_FULLFSYNC, isFullsync));
    rc = osOpenDirectory(pFile->zPath, &dirfd);
    if( rc==SQLITE_OK && dirfd>=0 ){
      full_fsync(dirfd, 0, 0);
      robust_close(pFile, dirfd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
    pFile->ctrlFlags &= ~UNIXFILE_DIRSYNC;
  }
  return rc;
}

/*
** Truncate an open file to a specified size
*/
static int unixTruncate(sqlite3_file *id, i64 nByte){
  unixFile *pFile = (unixFile *)id;
  int rc;
  assert( pFile );
  SimulateIOError( return SQLITE_IOERR_TRUNCATE );

  /* If the user has configured a chunk-size for this file, truncate the
  ** file so that it consists of an integer number of chunks (i.e. the
  ** actual file size after the operation may be larger than the requested
  ** size).
  */
  if( pFile->szChunk>0 ){
    nByte = ((nByte + pFile->szChunk - 1)/pFile->szChunk) * pFile->szChunk;
  }

  rc = robust_ftruncate(pFile->h, nByte);
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
  }else{
#ifdef SQLITE_DEBUG
    /* If we are doing a normal write to a database file (as opposed to
    ** doing a hot-journal rollback or a write to some file other than a
    ** normal database file) and we truncate the file to zero length,
    ** that effectively updates the change counter.  This might happen
    ** when restoring a database using the backup API from a zero-length
    ** source.
    */
    if( pFile->inNormalWrite && nByte==0 ){
      pFile->transCntrChng = 1;
    }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
    /* If the file was just truncated to a size smaller than the currently
    ** mapped region, reduce the effective mapping size as well. SQLite will
    ** use read() and write() to access data beyond this point from now on.  
    */
    if( nByte<pFile->mmapSize ){
      pFile->mmapSize = nByte;
    }
#endif

    return SQLITE_OK;
  }
}

/*
** Determine the current size of a file in bytes
*/
static int unixFileSize(sqlite3_file *id, i64 *pSize){
  int rc;
  struct stat buf;
  assert( id );
  rc = osFstat(((unixFile*)id)->h, &buf);
  SimulateIOError( rc=1 );
  if( rc!=0 ){
    storeLastErrno((unixFile*)id, errno);
    return SQLITE_IOERR_FSTAT;
  }
  *pSize = buf.st_size;

  /* When opening a zero-size database, the findInodeInfo() procedure
  ** writes a single byte into that file in order to work around a bug
  ** in the OS-X msdos filesystem.  In order to avoid problems with upper
  ** layers, we need to report this file size as zero even though it is
  ** really 1.   Ticket #3260.
  */
  if( *pSize==1 ) *pSize = 0;


  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Handler for proxy-locking file-control verbs.  Defined below in the
** proxying locking division.
*/
static int proxyFileControl(sqlite3_file*,int,void*);
#endif

/* 
** This function is called to handle the SQLITE_FCNTL_SIZE_HINT 
** file-control operation.  Enlarge the database to nBytes in size
** (rounded up to the next chunk-size).  If the database is already
** nBytes or larger, this routine is a no-op.
*/
static int fcntlSizeHint(unixFile *pFile, i64 nByte){
  if( pFile->szChunk>0 ){
    i64 nSize;                    /* Required file size */
    struct stat buf;              /* Used to hold return values of fstat() */
   
    if( osFstat(pFile->h, &buf) ){
      return SQLITE_IOERR_FSTAT;
    }

    nSize = ((nByte+pFile->szChunk-1) / pFile->szChunk) * pFile->szChunk;
    if( nSize>(i64)buf.st_size ){

#if defined(HAVE_POSIX_FALLOCATE) && HAVE_POSIX_FALLOCATE
      /* The code below is handling the return value of osFallocate() 
      ** correctly. posix_fallocate() is defined to "returns zero on success, 
      ** or an error number on  failure". See the manpage for details. */
      int err;
      do{
        err = osFallocate(pFile->h, buf.st_size, nSize-buf.st_size);
      }while( err==EINTR );
      if( err ) return SQLITE_IOERR_WRITE;
#else
      /* If the OS does not have posix_fallocate(), fake it. Write a 
      ** single byte to the last byte in each block that falls entirely
      ** within the extended region. Then, if required, a single byte
      ** at offset (nSize-1), to set the size of the file correctly.
      ** This is a similar technique to that used by glibc on systems
      ** that do not have a real fallocate() call.
      */
      int nBlk = buf.st_blksize;  /* File-system block size */
      int nWrite = 0;             /* Number of bytes written by seekAndWrite */
      i64 iWrite;                 /* Next offset to write to */

      iWrite = ((buf.st_size + 2*nBlk - 1)/nBlk)*nBlk-1;
      assert( iWrite>=buf.st_size );
      assert( (iWrite/nBlk)==((buf.st_size+nBlk-1)/nBlk) );
      assert( ((iWrite+1)%nBlk)==0 );
      for(/*no-op*/; iWrite<nSize; iWrite+=nBlk ){
        nWrite = seekAndWrite(pFile, iWrite, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
      if( nWrite==0 || (nSize%nBlk) ){
        nWrite = seekAndWrite(pFile, nSize-1, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
#endif
    }
  }

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFile->mmapSizeMax>0 && nByte>pFile->mmapSize ){
    int rc;
    if( pFile->szChunk<=0 ){
      if( robust_ftruncate(pFile->h, nByte) ){
        storeLastErrno(pFile, errno);
        return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
      }
    }

    rc = unixMapfile(pFile, nByte);
    return rc;
  }
#endif

  return SQLITE_OK;
}

/*
** If *pArg is initially negative then this is a query.  Set *pArg to
** 1 or 0 depending on whether or not bit mask of pFile->ctrlFlags is set.
**
** If *pArg is 0 or 1, then clear or set the mask bit of pFile->ctrlFlags.
*/
static void unixModeBit(unixFile *pFile, unsigned char mask, int *pArg){
  if( *pArg<0 ){
    *pArg = (pFile->ctrlFlags & mask)!=0;
  }else if( (*pArg)==0 ){
    pFile->ctrlFlags &= ~mask;
  }else{
    pFile->ctrlFlags |= mask;
  }
}

/* Forward declaration */
static int unixGetTempname(int nBuf, char *zBuf);

/*
** Information and control of an open file handle.
*/
static int unixFileControl(sqlite3_file *id, int op, void *pArg){
  unixFile *pFile = (unixFile*)id;
  switch( op ){
    case SQLITE_FCNTL_WAL_BLOCK: {
      /* pFile->ctrlFlags |= UNIXFILE_BLOCK; // Deferred feature */
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LOCKSTATE: {
      *(int*)pArg = pFile->eFileLock;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LAST_ERRNO: {
      *(int*)pArg = pFile->lastErrno;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_CHUNK_SIZE: {
      pFile->szChunk = *(int *)pArg;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SIZE_HINT: {
      int rc;
      SimulateIOErrorBenign(1);
      rc = fcntlSizeHint(pFile, *(i64 *)pArg);
      SimulateIOErrorBenign(0);
      return rc;
    }
    case SQLITE_FCNTL_PERSIST_WAL: {
      unixModeBit(pFile, UNIXFILE_PERSIST_WAL, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_POWERSAFE_OVERWRITE: {
      unixModeBit(pFile, UNIXFILE_PSOW, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_VFSNAME: {
      *(char**)pArg = sqlite3_mprintf("%s", pFile->pVfs->zName);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_TEMPFILENAME: {
      char *zTFile = sqlite3_malloc64( pFile->pVfs->mxPathname );
      if( zTFile ){
        unixGetTempname(pFile->pVfs->mxPathname, zTFile);
        *(char**)pArg = zTFile;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_HAS_MOVED: {
      *(int*)pArg = fileHasMoved(pFile);
      return SQLITE_OK;
    }
#if SQLITE_MAX_MMAP_SIZE>0
    case SQLITE_FCNTL_MMAP_SIZE: {
      i64 newLimit = *(i64*)pArg;
      int rc = SQLITE_OK;
      if( newLimit>sqlite3GlobalConfig.mxMmap ){
        newLimit = sqlite3GlobalConfig.mxMmap;
      }
      *(i64*)pArg = pFile->mmapSizeMax;
      if( newLimit>=0 && newLimit!=pFile->mmapSizeMax && pFile->nFetchOut==0 ){
        pFile->mmapSizeMax = newLimit;
        if( pFile->mmapSize>0 ){
          unixUnmapfile(pFile);
          rc = unixMapfile(pFile, -1);
        }
      }
      return rc;
    }
#endif
#ifdef SQLITE_DEBUG
    /* The pager calls this method to signal that it has done
    ** a rollback and that the database is therefore unchanged and
    ** it hence it is OK for the transaction change counter to be
    ** unchanged.
    */
    case SQLITE_FCNTL_DB_UNCHANGED: {
      ((unixFile*)id)->dbUpdate = 0;
      return SQLITE_OK;
    }
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    case SQLITE_FCNTL_SET_LOCKPROXYFILE:
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      return proxyFileControl(id,op,pArg);
    }
#endif /* SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__) */
  }
  return SQLITE_NOTFOUND;
}

/*
** Return the sector size in bytes of the underlying block device for
** the specified file. This is almost always 512 bytes, but may be
** larger for some devices.
**
** SQLite code assumes this function cannot fail. It also assumes that
** if two files are created in the same file-system directory (i.e.
** a database and its journal file) that the sector size will be the
** same for both.
*/
#ifndef __QNXNTO__ 
static int unixSectorSize(sqlite3_file *NotUsed){
  UNUSED_PARAMETER(NotUsed);
  return SQLITE_DEFAULT_SECTOR_SIZE;
}
#endif

/*
** The following version of unixSectorSize() is optimized for QNX.
*/
#ifdef __QNXNTO__
#include <sys/dcmd_blk.h>
#include <sys/statvfs.h>
static int unixSectorSize(sqlite3_file *id){
  unixFile *pFile = (unixFile*)id;
  if( pFile->sectorSize == 0 ){
    struct statvfs fsInfo;
       
    /* Set defaults for non-supported filesystems */
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
    pFile->deviceCharacteristics = 0;
    if( fstatvfs(pFile->h, &fsInfo) == -1 ) {
      return pFile->sectorSize;
    }

    if( !strcmp(fsInfo.f_basetype, "tmp") ) {
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC4K |       /* All ram filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "etfs") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* etfs cluster size writes are atomic */
        (pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) |
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx6") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC |         /* All filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx4") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "dos") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else{
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC512 |      /* blocks are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        0;
    }
  }
  /* Last chance verification.  If the sector size isn't a multiple of 512
  ** then it isn't valid.*/
  if( pFile->sectorSize % 512 != 0 ){
    pFile->deviceCharacteristics = 0;
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
  }
  return pFile->sectorSize;
}
#endif /* __QNXNTO__ */

/*
** Return the device characteristics for the file.
**
** This VFS is set up to return SQLITE_IOCAP_POWERSAFE_OVERWRITE by default.
** However, that choice is controversial since technically the underlying
** file system does not always provide powersafe overwrites.  (In other
** words, after a power-loss event, parts of the file that were never
** written might end up being altered.)  However, non-PSOW behavior is very,
** very rare.  And asserting PSOW makes a large reduction in the amount
** of required I/O for journaling, since a lot of padding is eliminated.
**  Hence, while POWERSAFE_OVERWRITE is on by default, there is a file-control
** available to turn it off and URI query parameter available to turn it off.
*/
static int unixDeviceCharacteristics(sqlite3_file *id){
  unixFile *p = (unixFile*)id;
  int rc = 0;
#ifdef __QNXNTO__
  if( p->sectorSize==0 ) unixSectorSize(id);
  rc = p->deviceCharacteristics;
#endif
  if( p->ctrlFlags & UNIXFILE_PSOW ){
    rc |= SQLITE_IOCAP_POWERSAFE_OVERWRITE;
  }
  return rc;
}

#if !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0

/*
** Return the system page size.
**
** This function should not be called directly by other code in this file. 
** Instead, it should be called via macro osGetpagesize().
*/
static int unixGetpagesize(void){
#if OS_VXWORKS
  return 1024;
#elif defined(_BSD_SOURCE)
  return getpagesize();
#else
  return (int)sysconf(_SC_PAGESIZE);
#endif
}

#endif /* !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0 */

#ifndef SQLITE_OMIT_WAL

/*
** Object used to represent an shared memory buffer.  
**
** When multiple threads all reference the same wal-index, each thread
** has its own unixShm object, but they all point to a single instance
** of this unixShmNode object.  In other words, each wal-index is opened
** only once per process.
**
** Each unixShmNode object is connected to a single unixInodeInfo object.
** We could coalesce this object into unixInodeInfo, but that would mean
** every open file that does not use shared memory (in other words, most
** open files) would have to carry around this extra information.  So
** the unixInodeInfo object contains a pointer to this unixShmNode object
** and the unixShmNode object is created only when needed.
**
** unixMutexHeld() must be true when creating or destroying
** this object or while reading or writing the following fields:
**
**      nRef
**
** The following fields are read-only after the object is created:
** 
**      fid
**      zFilename
**
** Either unixShmNode.mutex must be held or unixShmNode.nRef==0 and
** unixMutexHeld() is true when reading or writing any other field
** in this structure.
*/
struct unixShmNode {
  unixInodeInfo *pInode;     /* unixInodeInfo that owns this SHM node */
  sqlite3_mutex *mutex;      /* Mutex to access this object */
  char *zFilename;           /* Name of the mmapped file */
  int h;                     /* Open file descriptor */
  int szRegion;              /* Size of shared-memory regions */
  u16 nRegion;               /* Size of array apRegion */
  u8 isReadonly;             /* True if read-only */
  char **apRegion;           /* Array of mapped shared-memory regions */
  int nRef;                  /* Number of unixShm objects pointing to this */
  unixShm *pFirst;           /* All unixShm objects pointing to this */
#ifdef SQLITE_DEBUG
  u8 exclMask;               /* Mask of exclusive locks held */
  u8 sharedMask;             /* Mask of shared locks held */
  u8 nextShmId;              /* Next available unixShm.id value */
#endif
};

/*
** Structure used internally by this VFS to record the state of an
** open shared memory connection.
**
** The following fields are initialized when this object is created and
** are read-only thereafter:
**
**    unixShm.pFile
**    unixShm.id
**
** All other fields are read/write.  The unixShm.pFile->mutex must be held
** while accessing any read/write fields.
*/
struct unixShm {
  unixShmNode *pShmNode;     /* The underlying unixShmNode object */
  unixShm *pNext;            /* Next unixShm with the same unixShmNode */
  u8 hasMutex;               /* True if holding the unixShmNode mutex */
  u8 id;                     /* Id of this connection within its unixShmNode */
  u16 sharedMask;            /* Mask of shared locks held */
  u16 exclMask;              /* Mask of exclusive locks held */
};

/*
** Constants used for locking
*/
#define UNIX_SHM_BASE   ((22+SQLITE_SHM_NLOCK)*4)         /* first lock byte */
#define UNIX_SHM_DMS    (UNIX_SHM_BASE+SQLITE_SHM_NLOCK)  /* deadman switch */

/*
** Apply posix advisory locks for all bytes from ofst through ofst+n-1.
**
** Locks block if the mask is exactly UNIX_SHM_C and are non-blocking
** otherwise.
*/
static int unixShmSystemLock(
  unixFile *pFile,       /* Open connection to the WAL file */
  int lockType,          /* F_UNLCK, F_RDLCK, or F_WRLCK */
  int ofst,              /* First byte of the locking range */
  int n                  /* Number of bytes to lock */
){
  unixShmNode *pShmNode; /* Apply locks to this open shared-memory segment */
  struct flock f;        /* The posix advisory locking structure */
  int rc = SQLITE_OK;    /* Result code form fcntl() */

  /* Access to the unixShmNode object is serialized by the caller */
  pShmNode = pFile->pInode->pShmNode;
  assert( sqlite3_mutex_held(pShmNode->mutex) || pShmNode->nRef==0 );

  /* Shared locks never span more than one byte */
  assert( n==1 || lockType!=F_RDLCK );

  /* Locks are within range */
  assert( n>=1 && n<SQLITE_SHM_NLOCK );

  if( pShmNode->h>=0 ){
    int lkType;
    /* Initialize the locking parameters */
    memset(&f, 0, sizeof(f));
    f.l_type = lockType;
    f.l_whence = SEEK_SET;
    f.l_start = ofst;
    f.l_len = n;

    lkType = (pFile->ctrlFlags & UNIXFILE_BLOCK)!=0 ? F_SETLKW : F_SETLK;
    rc = osFcntl(pShmNode->h, lkType, &f);
    rc = (rc!=(-1)) ? SQLITE_OK : SQLITE_BUSY;
    pFile->ctrlFlags &= ~UNIXFILE_BLOCK;
  }

  /* Update the global lock state and do debug tracing */
#ifdef SQLITE_DEBUG
  { u16 mask;
  OSTRACE(("SHM-LOCK "));
  mask = ofst>31 ? 0xffff : (1<<(ofst+n)) - (1<<ofst);
  if( rc==SQLITE_OK ){
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask &= ~mask;
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask |= mask;
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d ok", ofst));
      pShmNode->exclMask |= mask;
      pShmNode->sharedMask &= ~mask;
    }
  }else{
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d failed", ofst));
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock failed"));
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d failed", ofst));
    }
  }
  OSTRACE((" - afterwards %03x,%03x\n",
           pShmNode->sharedMask, pShmNode->exclMask));
  }
#endif

  return rc;        
}

/*
** Return the minimum number of 32KB shm regions that should be mapped at
** a time, assuming that each mapping must be an integer multiple of the
** current system page-size.
**
** Usually, this is 1. The exception seems to be systems that are configured
** to use 64KB pages - in this case each mapping must cover at least two
** shm regions.
*/
static int unixShmRegionPerMap(void){
  int shmsz = 32*1024;            /* SHM region size */
  int pgsz = osGetpagesize();   /* System page size */
  assert( ((pgsz-1)&pgsz)==0 );   /* Page size must be a power of 2 */
  if( pgsz<shmsz ) return 1;
  return pgsz/shmsz;
}

/*
** Purge the unixShmNodeList list of all entries with unixShmNode.nRef==0.
**
** This is not a VFS shared-memory method; it is a utility function called
** by VFS shared-memory methods.
*/
static void unixShmPurge(unixFile *pFd){
  unixShmNode *p = pFd->pInode->pShmNode;
  assert( unixMutexHeld() );
  if( p && p->nRef==0 ){
    int nShmPerMap = unixShmRegionPerMap();
    int i;
    assert( p->pInode==pFd->pInode );
    sqlite3_mutex_free(p->mutex);
    for(i=0; i<p->nRegion; i+=nShmPerMap){
      if( p->h>=0 ){
        osMunmap(p->apRegion[i], p->szRegion);
      }else{
        sqlite3_free(p->apRegion[i]);
      }
    }
    sqlite3_free(p->apRegion);
    if( p->h>=0 ){
      robust_close(pFd, p->h, __LINE__);
      p->h = -1;
    }
    p->pInode->pShmNode = 0;
    sqlite3_free(p);
  }
}

/*
** Open a shared-memory area associated with open database file pDbFd.  
** This particular implementation uses mmapped files.
**
** The file used to implement shared-memory is in the same directory
** as the open database file and has the same name as the open database
** file with the "-shm" suffix added.  For example, if the database file
** is "/home/user1/config.db" then the file that is created and mmapped
** for shared memory will be called "/home/user1/config.db-shm".  
**
** Another approach to is to use files in /dev/shm or /dev/tmp or an
** some other tmpfs mount. But if a file in a different directory
** from the database file is used, then differing access permissions
** or a chroot() might cause two different processes on the same
** database to end up using different files for shared memory - 
** meaning that their memory would not really be shared - resulting
** in database corruption.  Nevertheless, this tmpfs file usage
** can be enabled at compile-time using -DSQLITE_SHM_DIRECTORY="/dev/shm"
** or the equivalent.  The use of the SQLITE_SHM_DIRECTORY compile-time
** option results in an incompatible build of SQLite;  builds of SQLite
** that with differing SQLITE_SHM_DIRECTORY settings attempt to use the
** same database file at the same time, database corruption will likely
** result. The SQLITE_SHM_DIRECTORY compile-time option is considered
** "unsupported" and may go away in a future SQLite release.
**
** When opening a new shared-memory file, if no other instances of that
** file are currently open, in this process or in other processes, then
** the file must be truncated to zero length or have its header cleared.
**
** If the original database file (pDbFd) is using the "unix-excl" VFS
** that means that an exclusive lock is held on the database file and
** that no other processes are able to read or write the database.  In
** that case, we do not really need shared memory.  No shared memory
** file is created.  The shared memory will be simulated with heap memory.
*/
static int unixOpenSharedMemory(unixFile *pDbFd){
  struct unixShm *p = 0;          /* The connection to be opened */
  struct unixShmNode *pShmNode;   /* The underlying mmapped file */
  int rc;                         /* Result code */
  unixInodeInfo *pInode;          /* The inode of fd */
  char *zShmFilename;             /* Name of the file used for SHM */
  int nShmFilename;               /* Size of the SHM filename in bytes */

  /* Allocate space for the new unixShm object. */
  p = sqlite3_malloc64( sizeof(*p) );
  if( p==0 ) return SQLITE_NOMEM;
  memset(p, 0, sizeof(*p));
  assert( pDbFd->pShm==0 );

  /* Check to see if a unixShmNode object already exists. Reuse an existing
  ** one if present. Create a new one if necessary.
  */
  unixEnterMutex();
  pInode = pDbFd->pInode;
  pShmNode = pInode->pShmNode;
  if( pShmNode==0 ){
    struct stat sStat;                 /* fstat() info for database file */
#ifndef SQLITE_SHM_DIRECTORY
    const char *zBasePath = pDbFd->zPath;
#endif

    /* Call fstat() to figure out the permissions on the database file. If
    ** a new *-shm file is created, an attempt will be made to create it
    ** with the same permissions.
    */
    if( osFstat(pDbFd->h, &sStat) && pInode->bProcessLock==0 ){
      rc = SQLITE_IOERR_FSTAT;
      goto shm_open_err;
    }

#ifdef SQLITE_SHM_DIRECTORY
    nShmFilename = sizeof(SQLITE_SHM_DIRECTORY) + 31;
#else
    nShmFilename = 6 + (int)strlen(zBasePath);
#endif
    pShmNode = sqlite3_malloc64( sizeof(*pShmNode) + nShmFilename );
    if( pShmNode==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }
    memset(pShmNode, 0, sizeof(*pShmNode)+nShmFilename);
    zShmFilename = pShmNode->zFilename = (char*)&pShmNode[1];
#ifdef SQLITE_SHM_DIRECTORY
    sqlite3_snprintf(nShmFilename, zShmFilename, 
                     SQLITE_SHM_DIRECTORY "/sqlite-shm-%x-%x",
                     (u32)sStat.st_ino, (u32)sStat.st_dev);
#else
    sqlite3_snprintf(nShmFilename, zShmFilename, "%s-shm", zBasePath);
    sqlite3FileSuffix3(pDbFd->zPath, zShmFilename);
#endif
    pShmNode->h = -1;
    pDbFd->pInode->pShmNode = pShmNode;
    pShmNode->pInode = pDbFd->pInode;
    pShmNode->mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
    if( pShmNode->mutex==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }

    if( pInode->bProcessLock==0 ){
      int openFlags = O_RDWR | O_CREAT;
      if( sqlite3_uri_boolean(pDbFd->zPath, "readonly_shm", 0) ){
        openFlags = O_RDONLY;
        pShmNode->isReadonly = 1;
      }
      pShmNode->h = robust_open(zShmFilename, openFlags, (sStat.st_mode&0777));
      if( pShmNode->h<0 ){
        rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zShmFilename);
        goto shm_open_err;
      }

      /* If this process is running as root, make sure that the SHM file
      ** is owned by the same user that owns the original database.  Otherwise,
      ** the original owner will not be able to connect.
      */
      osFchown(pShmNode->h, sStat.st_uid, sStat.st_gid);
  
      /* Check to see if another process is holding the dead-man switch.
      ** If not, truncate the file to zero length. 
      */
      rc = SQLITE_OK;
      if( unixShmSystemLock(pDbFd, F_WRLCK, UNIX_SHM_DMS, 1)==SQLITE_OK ){
        if( robust_ftruncate(pShmNode->h, 0) ){
          rc = unixLogError(SQLITE_IOERR_SHMOPEN, "ftruncate", zShmFilename);
        }
      }
      if( rc==SQLITE_OK ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, UNIX_SHM_DMS, 1);
      }
      if( rc ) goto shm_open_err;
    }
  }

  /* Make the new connection a child of the unixShmNode */
  p->pShmNode = pShmNode;
#ifdef SQLITE_DEBUG
  p->id = pShmNode->nextShmId++;
#endif
  pShmNode->nRef++;
  pDbFd->pShm = p;
  unixLeaveMutex();

  /* The reference count on pShmNode has already been incremented under
  ** the cover of the unixEnterMutex() mutex and the pointer from the
  ** new (struct unixShm) object to the pShmNode has been set. All that is
  ** left to do is to link the new object into the linked list starting
  ** at pShmNode->pFirst. This must be done while holding the pShmNode->mutex 
  ** mutex.
  */
  sqlite3_mutex_enter(pShmNode->mutex);
  p->pNext = pShmNode->pFirst;
  pShmNode->pFirst = p;
  sqlite3_mutex_leave(pShmNode->mutex);
  return SQLITE_OK;

  /* Jump here on any error */
shm_open_err:
  unixShmPurge(pDbFd);       /* This call frees pShmNode if required */
  sqlite3_free(p);
  unixLeaveMutex();
  return rc;
}

/*
** This function is called to obtain a pointer to region iRegion of the 
** shared-memory associated with the database file fd. Shared-memory regions 
** are numbered starting from zero. Each shared-memory region is szRegion 
** bytes in size.
**
** If an error occurs, an error code is returned and *pp is set to NULL.
**
** Otherwise, if the bExtend parameter is 0 and the requested shared-memory
** region has not been allocated (by any client, including one running in a
** separate process), then *pp is set to NULL and SQLITE_OK returned. If 
** bExtend is non-zero and the requested shared-memory region has not yet 
** been allocated, it is allocated by this function.
**
** If the shared-memory region has already been allocated or is allocated by
** this call as described above, then it is mapped into this processes 
** address space (if it is not already), *pp is set to point to the mapped 
** memory and SQLITE_OK returned.
*/
static int unixShmMap(
  sqlite3_file *fd,               /* Handle open on database file */
  int iRegion,                    /* Region to retrieve */
  int szRegion,                   /* Size of regions */
  int bExtend,                    /* True to extend file if necessary */
  void volatile **pp              /* OUT: Mapped memory */
){
  unixFile *pDbFd = (unixFile*)fd;
  unixShm *p;
  unixShmNode *pShmNode;
  int rc = SQLITE_OK;
  int nShmPerMap = unixShmRegionPerMap();
  int nReqRegion;

  /* If the shared-memory file has not yet been opened, open it now. */
  if( pDbFd->pShm==0 ){
    rc = unixOpenSharedMemory(pDbFd);
    if( rc!=SQLITE_OK ) return rc;
  }

  p = pDbFd->pShm;
  pShmNode = p->pShmNode;
  sqlite3_mutex_enter(pShmNode->mutex);
  assert( szRegion==pShmNode->szRegion || pShmNode->nRegion==0 );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  /* Minimum number of regions required to be mapped. */
  nReqRegion = ((iRegion+nShmPerMap) / nShmPerMap) * nShmPerMap;

  if( pShmNode->nRegion<nReqRegion ){
    char **apNew;                      /* New apRegion[] array */
    int nByte = nReqRegion*szRegion;   /* Minimum required file size */
    struct stat sStat;                 /* Used by fstat() */

    pShmNode->szRegion = szRegion;

    if( pShmNode->h>=0 ){
      /* The requested region is not mapped into this processes address space.
      ** Check to see if it has been allocated (i.e. if the wal-index file is
      ** large enough to contain the requested region).
      */
      if( osFstat(pShmNode->h, &sStat) ){
        rc = SQLITE_IOERR_SHMSIZE;
        goto shmpage_out;
      }
  
      if( sStat.st_size<nByte ){
        /* The requested memory region does not exist. If bExtend is set to
        ** false, exit early. *pp will be set to NULL and SQLITE_OK returned.
        */
        if( !bExtend ){
          goto shmpage_out;
        }

        /* Alternatively, if bExtend is true, extend the file. Do this by
        ** writing a single byte to the end of each (OS) page being
        ** allocated or extended. Technically, we need only write to the
        ** last page in order to extend the file. But writing to all new
        ** pages forces the OS to allocate them immediately, which reduces
        ** the chances of SIGBUS while accessing the mapped region later on.
        */
        else{
          static const int pgsz = 4096;
          int iPg;

          /* Write to the last byte of each newly allocated or extended page */
          assert( (nByte % pgsz)==0 );
          for(iPg=(sStat.st_size/pgsz); iPg<(nByte/pgsz); iPg++){
            if( seekAndWriteFd(pShmNode->h, iPg*pgsz + pgsz-1, "", 1, 0)!=1 ){
              const char *zFile = pShmNode->zFilename;
              rc = unixLogError(SQLITE_IOERR_SHMSIZE, "write", zFile);
              goto shmpage_out;
            }
          }
        }
      }
    }

    /* Map the requested memory region into this processes address space. */
    apNew = (char **)sqlite3_realloc(
        pShmNode->apRegion, nReqRegion*sizeof(char *)
    );
    if( !apNew ){
      rc = SQLITE_IOERR_NOMEM;
      goto shmpage_out;
    }
    pShmNode->apRegion = apNew;
    while( pShmNode->nRegion<nReqRegion ){
      int nMap = szRegion*nShmPerMap;
      int i;
      void *pMem;
      if( pShmNode->h>=0 ){
        pMem = osMmap(0, nMap,
            pShmNode->isReadonly ? PROT_READ : PROT_READ|PROT_WRITE, 
            MAP_SHARED, pShmNode->h, szRegion*(i64)pShmNode->nRegion
        );
        if( pMem==MAP_FAILED ){
          rc = unixLogError(SQLITE_IOERR_SHMMAP, "mmap", pShmNode->zFilename);
          goto shmpage_out;
        }
      }else{
        pMem = sqlite3_malloc64(szRegion);
        if( pMem==0 ){
          rc = SQLITE_NOMEM;
          goto shmpage_out;
        }
        memset(pMem, 0, szRegion);
      }

      for(i=0; i<nShmPerMap; i++){
        pShmNode->apRegion[pShmNode->nRegion+i] = &((char*)pMem)[szRegion*i];
      }
      pShmNode->nRegion += nShmPerMap;
    }
  }

shmpage_out:
  if( pShmNode->nRegion>iRegion ){
    *pp = pShmNode->apRegion[iRegion];
  }else{
    *pp = 0;
  }
  if( pShmNode->isReadonly && rc==SQLITE_OK ) rc = SQLITE_READONLY;
  sqlite3_mutex_leave(pShmNode->mutex);
  return rc;
}

/*
** Change the lock state for a shared-memory segment.
**
** Note that the relationship between SHAREd and EXCLUSIVE locks is a little
** different here than in posix.  In xShmLock(), one can go from unlocked
** to shared and back or from unlocked to exclusive and back.  But one may
** not go from shared to exclusive or from exclusive to shared.
*/
static int unixShmLock(
  sqlite3_file *fd,          /* Database file holding the shared memory */
  int ofst,                  /* First lock to acquire or release */
  int n,                     /* Number of locks to acquire or release */
  int flags                  /* What to do with the lock */
){
  unixFile *pDbFd = (unixFile*)fd;      /* Connection holding shared memory */
  unixShm *p = pDbFd->pShm;             /* The shared memory being locked */
  unixShm *pX;                          /* For looping over all siblings */
  unixShmNode *pShmNode = p->pShmNode;  /* The underlying file iNode */
  int rc = SQLITE_OK;                   /* Result code */
  u16 mask;                             /* Mask of locks to take or release */

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( ofst>=0 && ofst+n<=SQLITE_SHM_NLOCK );
  assert( n>=1 );
  assert( flags==(SQLITE_SHM_LOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_LOCK | SQLITE_SHM_EXCLUSIVE)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_EXCLUSIVE) );
  assert( n==1 || (flags & SQLITE_SHM_EXCLUSIVE)!=0 );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  mask = (1<<(ofst+n)) - (1<<ofst);
  assert( n>1 || mask==(1<<ofst) );
  sqlite3_mutex_enter(pShmNode->mutex);
  if( flags & SQLITE_SHM_UNLOCK ){
    u16 allMask = 0; /* Mask of locks held by siblings */

    /* See if any siblings hold this same lock */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( pX==p ) continue;
      assert( (pX->exclMask & (p->exclMask|p->sharedMask))==0 );
      allMask |= pX->sharedMask;
    }

    /* Unlock the system-level locks */
    if( (mask & allMask)==0 ){
      rc = unixShmSystemLock(pDbFd, F_UNLCK, ofst+UNIX_SHM_BASE, n);
    }else{
      rc = SQLITE_OK;
    }

    /* Undo the local locks */
    if( rc==SQLITE_OK ){
      p->exclMask &= ~mask;
      p->sharedMask &= ~mask;
    } 
  }else if( flags & SQLITE_SHM_SHARED ){
    u16 allShared = 0;  /* Union of locks held by connections other than "p" */

    /* Find out which shared locks are already held by sibling connections.
    ** If any sibling already holds an exclusive lock, go ahead and return
    ** SQLITE_BUSY.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
      allShared |= pX->sharedMask;
    }

    /* Get shared locks at the system level, if necessary */
    if( rc==SQLITE_OK ){
      if( (allShared & mask)==0 ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, ofst+UNIX_SHM_BASE, n);
      }else{
        rc = SQLITE_OK;
      }
    }

    /* Get the local shared locks */
    if( rc==SQLITE_OK ){
      p->sharedMask |= mask;
    }
  }else{
    /* Make sure no sibling connections hold locks that will block this
    ** lock.  If any do, return SQLITE_BUSY right away.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 || (pX->sharedMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
    }
  
    /* Get the exclusive locks at the system level.  Then if successful
    ** also mark the local connection as being locked.
    */
    if( rc==SQLITE_OK ){
      rc = unixShmSystemLock(pDbFd, F_WRLCK, ofst+UNIX_SHM_BASE, n);
      if( rc==SQLITE_OK ){
        assert( (p->sharedMask & mask)==0 );
        p->exclMask |= mask;
      }
    }
  }
  sqlite3_mutex_leave(pShmNode->mutex);
  OSTRACE(("SHM-LOCK shmid-%d, pid-%d got %03x,%03x\n",
           p->id, osGetpid(0), p->sharedMask, p->exclMask));
  return rc;
}

/*
** Implement a memory barrier or memory fence on shared memory.  
**
** All loads and stores begun before the barrier must complete before
** any load or store begun after the barrier.
*/
static void unixShmBarrier(
  sqlite3_file *fd                /* Database file holding the shared memory */
){
  UNUSED_PARAMETER(fd);
  sqlite3MemoryBarrier();         /* compiler-defined memory barrier */
  unixEnterMutex();               /* Also mutex, for redundancy */
  unixLeaveMutex();
}

/*
** Close a connection to shared-memory.  Delete the underlying 
** storage if deleteFlag is true.
**
** If there is no shared memory associated with the connection then this
** routine is a harmless no-op.
*/
static int unixShmUnmap(
  sqlite3_file *fd,               /* The underlying database file */
  int deleteFlag                  /* Delete shared-memory if true */
){
  unixShm *p;                     /* The connection to be closed */
  unixShmNode *pShmNode;          /* The underlying shared-memory file */
  unixShm **pp;                   /* For looping over sibling connections */
  unixFile *pDbFd;                /* The underlying database file */

  pDbFd = (unixFile*)fd;
  p = pDbFd->pShm;
  if( p==0 ) return SQLITE_OK;
  pShmNode = p->pShmNode;

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );

  /* Remove connection p from the set of connections associated
  ** with pShmNode */
  sqlite3_mutex_enter(pShmNode->mutex);
  for(pp=&pShmNode->pFirst; (*pp)!=p; pp = &(*pp)->pNext){}
  *pp = p->pNext;

  /* Free the connection p */
  sqlite3_free(p);
  pDbFd->pShm = 0;
  sqlite3_mutex_leave(pShmNode->mutex);

  /* If pShmNode->nRef has reached 0, then close the underlying
  ** shared-memory file, too */
  unixEnterMutex();
  assert( pShmNode->nRef>0 );
  pShmNode->nRef--;
  if( pShmNode->nRef==0 ){
    if( deleteFlag && pShmNode->h>=0 ){
      osUnlink(pShmNode->zFilename);
    }
    unixShmPurge(pDbFd);
  }
  unixLeaveMutex();

  return SQLITE_OK;
}


#else
# define unixShmMap     0
# define unixShmLock    0
# define unixShmBarrier 0
# define unixShmUnmap   0
#endif /* #ifndef SQLITE_OMIT_WAL */

#if SQLITE_MAX_MMAP_SIZE>0
/*
** If it is currently memory mapped, unmap file pFd.
*/
static void unixUnmapfile(unixFile *pFd){
  assert( pFd->nFetchOut==0 );
  if( pFd->pMapRegion ){
    osMunmap(pFd->pMapRegion, pFd->mmapSizeActual);
    pFd->pMapRegion = 0;
    pFd->mmapSize = 0;
    pFd->mmapSizeActual = 0;
  }
}

/*
** Attempt to set the size of the memory mapping maintained by file 
** descriptor pFd to nNew bytes. Any existing mapping is discarded.
**
** If successful, this function sets the following variables:
**
**       unixFile.pMapRegion
**       unixFile.mmapSize
**       unixFile.mmapSizeActual
**
** If unsuccessful, an error message is logged via sqlite3_log() and
** the three variables above are zeroed. In this case SQLite should
** continue accessing the database using the xRead() and xWrite()
** methods.
*/
static void unixRemapfile(
  unixFile *pFd,                  /* File descriptor object */
  i64 nNew                        /* Required mapping size */
){
  const char *zErr = "mmap";
  int h = pFd->h;                      /* File descriptor open on db file */
  u8 *pOrig = (u8 *)pFd->pMapRegion;   /* Pointer to current file mapping */
  i64 nOrig = pFd->mmapSizeActual;     /* Size of pOrig region in bytes */
  u8 *pNew = 0;                        /* Location of new mapping */
  int flags = PROT_READ;               /* Flags to pass to mmap() */

  assert( pFd->nFetchOut==0 );
  assert( nNew>pFd->mmapSize );
  assert( nNew<=pFd->mmapSizeMax );
  assert( nNew>0 );
  assert( pFd->mmapSizeActual>=pFd->mmapSize );
  assert( MAP_FAILED!=0 );

  if( (pFd->ctrlFlags & UNIXFILE_RDONLY)==0 ) flags |= PROT_WRITE;

  if( pOrig ){
#if HAVE_MREMAP
    i64 nReuse = pFd->mmapSize;
#else
    const int szSyspage = osGetpagesize();
    i64 nReuse = (pFd->mmapSize & ~(szSyspage-1));
#endif
    u8 *pReq = &pOrig[nReuse];

    /* Unmap any pages of the existing mapping that cannot be reused. */
    if( nReuse!=nOrig ){
      osMunmap(pReq, nOrig-nReuse);
    }

#if HAVE_MREMAP
    pNew = osMremap(pOrig, nReuse, nNew, MREMAP_MAYMOVE);
    zErr = "mremap";
#else
    pNew = osMmap(pReq, nNew-nReuse, flags, MAP_SHARED, h, nReuse);
    if( pNew!=MAP_FAILED ){
      if( pNew!=pReq ){
        osMunmap(pNew, nNew - nReuse);
        pNew = 0;
      }else{
        pNew = pOrig;
      }
    }
#endif

    /* The attempt to extend the existing mapping failed. Free it. */
    if( pNew==MAP_FAILED || pNew==0 ){
      osMunmap(pOrig, nReuse);
    }
  }

  /* If pNew is still NULL, try to create an entirely new mapping. */
  if( pNew==0 ){
    pNew = osMmap(0, nNew, flags, MAP_SHARED, h, 0);
  }

  if( pNew==MAP_FAILED ){
    pNew = 0;
    nNew = 0;
    unixLogError(SQLITE_OK, zErr, pFd->zPath);

    /* If the mmap() above failed, assume that all subsequent mmap() calls
    ** will probably fail too. Fall back to using xRead/xWrite exclusively
    ** in this case.  */
    pFd->mmapSizeMax = 0;
  }
  pFd->pMapRegion = (void *)pNew;
  pFd->mmapSize = pFd->mmapSizeActual = nNew;
}

/*
** Memory map or remap the file opened by file-descriptor pFd (if the file
** is already mapped, the existing mapping is replaced by the new). Or, if 
** there already exists a mapping for this file, and there are still 
** outstanding xFetch() references to it, this function is a no-op.
**
** If parameter nByte is non-negative, then it is the requested size of 
** the mapping to create. Otherwise, if nByte is less than zero, then the 
** requested size is the size of the file on disk. The actual size of the
** created mapping is either the requested size or the value configured 
** using SQLITE_FCNTL_MMAP_LIMIT, whichever is smaller.
**
** SQLITE_OK is returned if no error occurs (even if the mapping is not
** recreated as a result of outstanding references) or an SQLite error
** code otherwise.
*/
static int unixMapfile(unixFile *pFd, i64 nByte){
  i64 nMap = nByte;
  int rc;

  assert( nMap>=0 || pFd->nFetchOut==0 );
  if( pFd->nFetchOut>0 ) return SQLITE_OK;

  if( nMap<0 ){
    struct stat statbuf;          /* Low-level file information */
    rc = osFstat(pFd->h, &statbuf);
    if( rc!=SQLITE_OK ){
      return SQLITE_IOERR_FSTAT;
    }
    nMap = statbuf.st_size;
  }
  if( nMap>pFd->mmapSizeMax ){
    nMap = pFd->mmapSizeMax;
  }

  if( nMap!=pFd->mmapSize ){
    if( nMap>0 ){
      unixRemapfile(pFd, nMap);
    }else{
      unixUnmapfile(pFd);
    }
  }

  return SQLITE_OK;
}
#endif /* SQLITE_MAX_MMAP_SIZE>0 */

/*
** If possible, return a pointer to a mapping of file fd starting at offset
** iOff. The mapping must be valid for at least nAmt bytes.
**
** If such a pointer can be obtained, store it in *pp and return SQLITE_OK.
** Or, if one cannot but no error occurs, set *pp to 0 and return SQLITE_OK.
** Finally, if an error does occur, return an SQLite error code. The final
** value of *pp is undefined in this case.
**
** If this function does return a pointer, the caller must eventually 
** release the reference by calling unixUnfetch().
*/
static int unixFetch(sqlite3_file *fd, i64 iOff, int nAmt, void **pp){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
#endif
  *pp = 0;

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFd->mmapSizeMax>0 ){
    if( pFd->pMapRegion==0 ){
      int rc = unixMapfile(pFd, -1);
      if( rc!=SQLITE_OK ) return rc;
    }
    if( pFd->mmapSize >= iOff+nAmt ){
      *pp = &((u8 *)pFd->pMapRegion)[iOff];
      pFd->nFetchOut++;
    }
  }
#endif
  return SQLITE_OK;
}

/*
** If the third argument is non-NULL, then this function releases a 
** reference obtained by an earlier call to unixFetch(). The second
** argument passed to this function must be the same as the corresponding
** argument that was passed to the unixFetch() invocation. 
**
** Or, if the third argument is NULL, then this function is being called 
** to inform the VFS layer that, according to POSIX, any existing mapping 
** may now be invalid and should be unmapped.
*/
static int unixUnfetch(sqlite3_file *fd, i64 iOff, void *p){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
  UNUSED_PARAMETER(iOff);

  /* If p==0 (unmap the entire file) then there must be no outstanding 
  ** xFetch references. Or, if p!=0 (meaning it is an xFetch reference),
  ** then there must be at least one outstanding.  */
  assert( (p==0)==(pFd->nFetchOut==0) );

  /* If p!=0, it must match the iOff value. */
  assert( p==0 || p==&((u8 *)pFd->pMapRegion)[iOff] );

  if( p ){
    pFd->nFetchOut--;
  }else{
    unixUnmapfile(pFd);
  }

  assert( pFd->nFetchOut>=0 );
#else
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(p);
  UNUSED_PARAMETER(iOff);
#endif
  return SQLITE_OK;
}

/*
** Here ends the implementation of all sqlite3_file methods.
**
********************** End sqlite3_file Methods *******************************
******************************************************************************/

/*
** This division contains definitions of sqlite3_io_methods objects that
** implement various file locking strategies.  It also contains definitions
** of "finder" functions.  A finder-function is used to locate the appropriate
** sqlite3_io_methods object for a particular database file.  The pAppData
** field of the sqlite3_vfs VFS objects are initialized to be pointers to
** the correct finder-function for that VFS.
**
** Most finder functions return a pointer to a fixed sqlite3_io_methods
** object.  The only interesting finder-function is autolockIoFinder, which
** looks at the filesystem type and tries to guess the best locking
** strategy from that.
**
** For finder-function F, two objects are created:
**
**    (1) The real finder-function named "FImpt()".
**
**    (2) A constant pointer to this function named just "F".
**
**
** A pointer to the F pointer is used as the pAppData value for VFS
** objects.  We have to do this instead of letting pAppData point
** directly at the finder-function since C90 rules prevent a void*
** from be cast into a function pointer.
**
**
** Each instance of this macro generates two objects:
**
**   *  A constant sqlite3_io_methods object call METHOD that has locking
**      methods CLOSE, LOCK, UNLOCK, CKRESLOCK.
**
**   *  An I/O method finder function called FINDER that returns a pointer
**      to the METHOD object in the previous bullet.
*/
#define IOMETHODS(FINDER,METHOD,VERSION,CLOSE,LOCK,UNLOCK,CKLOCK,SHMMAP)     \
static const sqlite3_io_methods METHOD = {                                   \
   VERSION,                    /* iVersion */                                \
   CLOSE,                      /* xClose */                                  \
   unixRead,                   /* xRead */                                   \
   unixWrite,                  /* xWrite */                                  \
   unixTruncate,               /* xTruncate */                               \
   unixSync,                   /* xSync */                                   \
   unixFileSize,               /* xFileSize */                               \
   LOCK,                       /* xLock */                                   \
   UNLOCK,                     /* xUnlock */                                 \
   CKLOCK,                     /* xCheckReservedLock */                      \
   unixFileControl,            /* xFileControl */                            \
   unixSectorSize,             /* xSectorSize */                             \
   unixDeviceCharacteristics,  /* xDeviceCapabilities */                     \
   SHMMAP,                     /* xShmMap */                                 \
   unixShmLock,                /* xShmLock */                                \
   unixShmBarrier,             /* xShmBarrier */                             \
   unixShmUnmap,               /* xShmUnmap */                               \
   unixFetch,                  /* xFetch */                                  \
   unixUnfetch,                /* xUnfetch */                                \
};                                                                           \
static const sqlite3_io_methods *FINDER##Impl(const char *z, unixFile *p){   \
  UNUSED_PARAMETER(z); UNUSED_PARAMETER(p);                                  \
  return &METHOD;                                                            \
}                                                                            \
static const sqlite3_io_methods *(*const FINDER)(const char*,unixFile *p)    \
    = FINDER##Impl;

/*
** Here are all of the sqlite3_io_methods objects for each of the
** locking strategies.  Functions that return pointers to these methods
** are also created.
*/
IOMETHODS(
  posixIoFinder,            /* Finder function name */
  posixIoMethods,           /* sqlite3_io_methods object name */
  3,                        /* shared memory and mmap are enabled */
  unixClose,                /* xClose method */
  unixLock,                 /* xLock method */
  unixUnlock,               /* xUnlock method */
  unixCheckReservedLock,    /* xCheckReservedLock method */
  unixShmMap                /* xShmMap method */
)
IOMETHODS(
  nolockIoFinder,           /* Finder function name */
  nolockIoMethods,          /* sqlite3_io_methods object name */
  3,                        /* shared memory is disabled */
  nolockClose,              /* xClose method */
  nolockLock,               /* xLock method */
  nolockUnlock,             /* xUnlock method */
  nolockCheckReservedLock,  /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
IOMETHODS(
  dotlockIoFinder,          /* Finder function name */
  dotlockIoMethods,         /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  dotlockClose,             /* xClose method */
  dotlockLock,              /* xLock method */
  dotlockUnlock,            /* xUnlock method */
  dotlockCheckReservedLock, /* xCheckReservedLock method */
  0                         /* xShmMap method */
)

#if SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  flockIoFinder,            /* Finder function name */
  flockIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  flockClose,               /* xClose method */
  flockLock,                /* xLock method */
  flockUnlock,              /* xUnlock method */
  flockCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if OS_VXWORKS
IOMETHODS(
  semIoFinder,              /* Finder function name */
  semIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  semXClose,                /* xClose method */
  semXLock,                 /* xLock method */
  semXUnlock,               /* xUnlock method */
  semXCheckReservedLock,    /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  afpIoFinder,              /* Finder function name */
  afpIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  afpClose,                 /* xClose method */
  afpLock,                  /* xLock method */
  afpUnlock,                /* xUnlock method */
  afpCheckReservedLock,     /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/*
** The proxy locking method is a "super-method" in the sense that it
** opens secondary file descriptors for the conch and lock files and
** it uses proxy, dot-file, AFP, and flock() locking methods on those
** secondary files.  For this reason, the division that implements
** proxy locking is located much further down in the file.  But we need
** to go ahead and define the sqlite3_io_methods and finder function
** for proxy locking here.  So we forward declare the I/O methods.
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
static int proxyClose(sqlite3_file*);
static int proxyLock(sqlite3_file*, int);
static int proxyUnlock(sqlite3_file*, int);
static int proxyCheckReservedLock(sqlite3_file*, int*);
IOMETHODS(
  proxyIoFinder,            /* Finder function name */
  proxyIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  proxyClose,               /* xClose method */
  proxyLock,                /* xLock method */
  proxyUnlock,              /* xUnlock method */
  proxyCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/* nfs lockd on OSX 10.3+ doesn't clear write locks when a read lock is set */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  nfsIoFinder,               /* Finder function name */
  nfsIoMethods,              /* sqlite3_io_methods object name */
  1,                         /* shared memory is disabled */
  unixClose,                 /* xClose method */
  unixLock,                  /* xLock method */
  nfsUnlock,                 /* xUnlock method */
  unixCheckReservedLock,     /* xCheckReservedLock method */
  0                          /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/* 
** This "finder" function attempts to determine the best locking strategy 
** for the database file "filePath".  It then returns the sqlite3_io_methods
** object that implements that strategy.
**
** This is for MacOSX only.
*/
static const sqlite3_io_methods *autolockIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* open file object for the database file */
){
  static const struct Mapping {
    const char *zFilesystem;              /* Filesystem type name */
    const sqlite3_io_methods *pMethods;   /* Appropriate locking method */
  } aMap[] = {
    { "hfs",    &posixIoMethods },
    { "ufs",    &posixIoMethods },
    { "afpfs",  &afpIoMethods },
    { "smbfs",  &afpIoMethods },
    { "webdav", &nolockIoMethods },
    { 0, 0 }
  };
  int i;
  struct statfs fsInfo;
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }
  if( statfs(filePath, &fsInfo) != -1 ){
    if( fsInfo.f_flags & MNT_RDONLY ){
      return &nolockIoMethods;
    }
    for(i=0; aMap[i].zFilesystem; i++){
      if( strcmp(fsInfo.f_fstypename, aMap[i].zFilesystem)==0 ){
        return aMap[i].pMethods;
      }
    }
  }

  /* Default case. Handles, amongst others, "nfs".
  ** Test byte-range lock using fcntl(). If the call succeeds, 
  ** assume that the file-system supports POSIX style locks. 
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    if( strcmp(fsInfo.f_fstypename, "nfs")==0 ){
      return &nfsIoMethods;
    } else {
      return &posixIoMethods;
    }
  }else{
    return &dotlockIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const autolockIoFinder)(const char*,unixFile*) = autolockIoFinderImpl;

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */

#if OS_VXWORKS
/*
** This "finder" function for VxWorks checks to see if posix advisory
** locking works.  If it does, then that is what is used.  If it does not
** work, then fallback to named semaphore locking.
*/
static const sqlite3_io_methods *vxworksIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* the open file object */
){
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }

  /* Test if fcntl() is supported and use POSIX style locks.
  ** Otherwise fall back to the named semaphore method.
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    return &posixIoMethods;
  }else{
    return &semIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const vxworksIoFinder)(const char*,unixFile*) = vxworksIoFinderImpl;

#endif /* OS_VXWORKS */

/*
** An abstract type for a pointer to an IO method finder function:
*/
typedef const sqlite3_io_methods *(*finder_type)(const char*,unixFile*);


/****************************************************************************
**************************** sqlite3_vfs methods ****************************
**
** This division contains the implementation of methods on the
** sqlite3_vfs object.
*/

/*
** Initialize the contents of the unixFile structure pointed to by pId.
*/
static int fillInUnixFile(
  sqlite3_vfs *pVfs,      /* Pointer to vfs object */
  int h,                  /* Open file descriptor of file being opened */
  sqlite3_file *pId,      /* Write to the unixFile structure here */
  const char *zFilename,  /* Name of the file being opened */
  int ctrlFlags           /* Zero or more UNIXFILE_* values */
){
  const sqlite3_io_methods *pLockingStyle;
  unixFile *pNew = (unixFile *)pId;
  int rc = SQLITE_OK;

  assert( pNew->pInode==NULL );

  /* Usually the path zFilename should not be a relative pathname. The
  ** exception is when opening the proxy "conch" file in builds that
  ** include the special Apple locking styles.
  */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
  assert( zFilename==0 || zFilename[0]=='/' 
    || pVfs->pAppData==(void*)&autolockIoFinder );
#else
  assert( zFilename==0 || zFilename[0]=='/' );
#endif

  /* No locking occurs in temporary files */
  assert( zFilename!=0 || (ctrlFlags & UNIXFILE_NOLOCK)!=0 );

  OSTRACE(("OPEN    %-3d %s\n", h, zFilename));
  pNew->h = h;
  pNew->pVfs = pVfs;
  pNew->zPath = zFilename;
  pNew->ctrlFlags = (u8)ctrlFlags;
#if SQLITE_MAX_MMAP_SIZE>0
  pNew->mmapSizeMax = sqlite3GlobalConfig.szMmap;
#endif
  if( sqlite3_uri_boolean(((ctrlFlags & UNIXFILE_URI) ? zFilename : 0),
                           "psow", SQLITE_POWERSAFE_OVERWRITE) ){
    pNew->ctrlFlags |= UNIXFILE_PSOW;
  }
  if( strcmp(pVfs->zName,"unix-excl")==0 ){
    pNew->ctrlFlags |= UNIXFILE_EXCL;
  }

#if OS_VXWORKS
  pNew->pId = vxworksFindFileId(zFilename);
  if( pNew->pId==0 ){
    ctrlFlags |= UNIXFILE_NOLOCK;
    rc = SQLITE_NOMEM;
  }
#endif

  if( ctrlFlags & UNIXFILE_NOLOCK ){
    pLockingStyle = &nolockIoMethods;
  }else{
    pLockingStyle = (**(finder_type*)pVfs->pAppData)(zFilename, pNew);
#if SQLITE_ENABLE_LOCKING_STYLE
    /* Cache zFilename in the locking context (AFP and dotlock override) for
    ** proxyLock activation is possible (remote proxy is based on db name)
    ** zFilename remains valid until file is closed, to support */
    pNew->lockingContext = (void*)zFilename;
#endif
  }

  if( pLockingStyle == &posixIoMethods
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
    || pLockingStyle == &nfsIoMethods
#endif
  ){
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( rc!=SQLITE_OK ){
      /* If an error occurred in findInodeInfo(), close the file descriptor
      ** immediately, before releasing the mutex. findInodeInfo() may fail
      ** in two scenarios:
      **
      **   (a) A call to fstat() failed.
      **   (b) A malloc failed.
      **
      ** Scenario (b) may only occur if the process is holding no other
      ** file descriptors open on the same file. If there were other file
      ** descriptors on this file, then no malloc would be required by
      ** findInodeInfo(). If this is the case, it is quite safe to close
      ** handle h - as it is guaranteed that no posix locks will be released
      ** by doing so.
      **
      ** If scenario (a) caused the error then things are not so safe. The
      ** implicit assumption here is that if fstat() fails, things are in
      ** such bad shape that dropping a lock or two doesn't matter much.
      */
      robust_close(pNew, h, __LINE__);
      h = -1;
    }
    unixLeaveMutex();
  }

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
  else if( pLockingStyle == &afpIoMethods ){
    /* AFP locking uses the file path so it needs to be included in
    ** the afpLockingContext.
    */
    afpLockingContext *pCtx;
    pNew->lockingContext = pCtx = sqlite3_malloc64( sizeof(*pCtx) );
    if( pCtx==0 ){
      rc = SQLITE_NOMEM;
    }else{
      /* NB: zFilename exists and remains valid until the file is closed
      ** according to requirement F11141.  So we do not need to make a
      ** copy of the filename. */
      pCtx->dbPath = zFilename;
      pCtx->reserved = 0;
      srandomdev();
      unixEnterMutex();
      rc = findInodeInfo(pNew, &pNew->pInode);
      if( rc!=SQLITE_OK ){
        sqlite3_free(pNew->lockingContext);
        robust_close(pNew, h, __LINE__);
        h = -1;
      }
      unixLeaveMutex();        
    }
  }
#endif

  else if( pLockingStyle == &dotlockIoMethods ){
    /* Dotfile locking uses the file path so it needs to be included in
    ** the dotlockLockingContext 
    */
    char *zLockFile;
    int nFilename;
    assert( zFilename!=0 );
    nFilename = (int)strlen(zFilename) + 6;
    zLockFile = (char *)sqlite3_malloc64(nFilename);
    if( zLockFile==0 ){
      rc = SQLITE_NOMEM;
    }else{
      sqlite3_snprintf(nFilename, zLockFile, "%s" DOTLOCK_SUFFIX, zFilename);
    }
    pNew->lockingContext = zLockFile;
  }

#if OS_VXWORKS
  else if( pLockingStyle == &semIoMethods ){
    /* Named semaphore locking uses the file path so it needs to be
    ** included in the semLockingContext
    */
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( (rc==SQLITE_OK) && (pNew->pInode->pSem==NULL) ){
      char *zSemName = pNew->pInode->aSemName;
      int n;
      sqlite3_snprintf(MAX_PATHNAME, zSemName, "/%s.sem",
                       pNew->pId->zCanonicalName);
      for( n=1; zSemName[n]; n++ )
        if( zSemName[n]=='/' ) zSemName[n] = '_';
      pNew->pInode->pSem = sem_open(zSemName, O_CREAT, 0666, 1);
      if( pNew->pInode->pSem == SEM_FAILED ){
        rc = SQLITE_NOMEM;
        pNew->pInode->aSemName[0] = '\0';
      }
    }
    unixLeaveMutex();
  }
#endif
  
  storeLastErrno(pNew, 0);
#if OS_VXWORKS
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
    h = -1;
    osUnlink(zFilename);
    pNew->ctrlFlags |= UNIXFILE_DELETE;
  }
#endif
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
  }else{
    pNew->pMethod = pLockingStyle;
    OpenCounter(+1);
    verifyDbFile(pNew);
  }
  return rc;
}

/*
** Return the name of a directory in which to put temporary files.
** If no suitable temporary file directory can be found, return NULL.
*/
static const char *unixTempFileDir(void){
  static const char *azDirs[] = {
     0,
     0,
     0,
     "/var/tmp",
     "/usr/tmp",
     "/tmp",
     0        /* List terminator */
  };
  unsigned int i;
  struct stat buf;
  const char *zDir = 0;

  azDirs[0] = sqlite3_temp_directory;
  if( !azDirs[1] ) azDirs[1] = getenv("SQLITE_TMPDIR");
  if( !azDirs[2] ) azDirs[2] = getenv("TMPDIR");
  for(i=0; i<sizeof(azDirs)/sizeof(azDirs[0]); zDir=azDirs[i++]){
    if( zDir==0 ) continue;
    if( osStat(zDir, &buf) ) continue;
    if( !S_ISDIR(buf.st_mode) ) continue;
    if( osAccess(zDir, 07) ) continue;
    break;
  }
  return zDir;
}

/*
** Create a temporary file name in zBuf.  zBuf must be allocated
** by the calling process and must be big enough to hold at least
** pVfs->mxPathname bytes.
*/
static int unixGetTempname(int nBuf, char *zBuf){
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  unsigned int i, j;
  const char *zDir;

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. 
  */
  SimulateIOError( return SQLITE_IOERR );

  zDir = unixTempFileDir();
  if( zDir==0 ) zDir = ".";

  /* Check that the output buffer is large enough for the temporary file 
  ** name. If it is not, return SQLITE_ERROR.
  */
  if( (strlen(zDir) + strlen(SQLITE_TEMP_FILE_PREFIX) + 18) >= (size_t)nBuf ){
    return SQLITE_ERROR;
  }

  do{
    sqlite3_snprintf(nBuf-18, zBuf, "%s/"SQLITE_TEMP_FILE_PREFIX, zDir);
    j = (int)strlen(zBuf);
    sqlite3_randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
    zBuf[j+1] = 0;
  }while( osAccess(zBuf,0)==0 );
  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Routine to transform a unixFile into a proxy-locking unixFile.
** Implementation in the proxy-lock division, but used by unixOpen()
** if SQLITE_PREFER_PROXY_LOCKING is defined.
*/
static int proxyTransformUnixFile(unixFile*, const char*);
#endif

/*
** Search for an unused file descriptor that was opened on the database 
** file (not a journal or master-journal file) identified by pathname
** zPath with SQLITE_OPEN_XXX flags matching those passed as the second
** argument to this function.
**
** Such a file descriptor may exist if a database connection was closed
** but the associated file descriptor could not be closed because some
** other file descriptor open on the same file is holding a file-lock.
** Refer to comments in the unixClose() function and the lengthy comment
** describing "Posix Advisory Locking" at the start of this file for 
** further details. Also, ticket #4018.
**
** If a suitable file descriptor is found, then it is returned. If no
** such file descriptor is located, -1 is returned.
*/
static UnixUnusedFd *findReusableFd(const char *zPath, int flags){
  UnixUnusedFd *pUnused = 0;

  /* Do not search for an unused file descriptor on vxworks. Not because
  ** vxworks would not benefit from the change (it might, we're not sure),
  ** but because no way to test it is currently available. It is better 
  ** not to risk breaking vxworks support for the sake of such an obscure 
  ** feature.  */
#if !OS_VXWORKS
  struct stat sStat;                   /* Results of stat() call */

  /* A stat() call may fail for various reasons. If this happens, it is
  ** almost certain that an open() call on the same path will also fail.
  ** For this reason, if an error occurs in the stat() call here, it is
  ** ignored and -1 is returned. The caller will try to open a new file
  ** descriptor on the same path, fail, and return an error to SQLite.
  **
  ** Even if a subsequent open() call does succeed, the consequences of
  ** not searching for a reusable file descriptor are not dire.  */
  if( 0==osStat(zPath, &sStat) ){
    unixInodeInfo *pInode;

    unixEnterMutex();
    pInode = inodeList;
    while( pInode && (pInode->fileId.dev!=sStat.st_dev
                     || pInode->fileId.ino!=sStat.st_ino) ){
       pInode = pInode->pNext;
    }
    if( pInode ){
      UnixUnusedFd **pp;
      for(pp=&pInode->pUnused; *pp && (*pp)->flags!=flags; pp=&((*pp)->pNext));
      pUnused = *pp;
      if( pUnused ){
        *pp = pUnused->pNext;
      }
    }
    unixLeaveMutex();
  }
#endif    /* if !OS_VXWORKS */
  return pUnused;
}

/*
** This function is called by unixOpen() to determine the unix permissions
** to create new files with. If no error occurs, then SQLITE_OK is returned
** and a value suitable for passing as the third argument to open(2) is
** written to *pMode. If an IO error occurs, an SQLite error code is 
** returned and the value of *pMode is not modified.
**
** In most cases, this routine sets *pMode to 0, which will become
** an indication to robust_open() to create the file using
** SQLITE_DEFAULT_FILE_PERMISSIONS adjusted by the umask.
** But if the file being opened is a WAL or regular journal file, then 
** this function queries the file-system for the permissions on the 
** corresponding database file and sets *pMode to this value. Whenever 
** possible, WAL and journal files are created using the same permissions 
** as the associated database file.
**
** If the SQLITE_ENABLE_8_3_NAMES option is enabled, then the
** original filename is unavailable.  But 8_3_NAMES is only used for
** FAT filesystems and permissions do not matter there, so just use
** the default permissions.
*/
static int findCreateFileMode(
  const char *zPath,              /* Path of file (possibly) being created */
  int flags,                      /* Flags passed as 4th argument to xOpen() */
  mode_t *pMode,                  /* OUT: Permissions to open file with */
  uid_t *pUid,                    /* OUT: uid to set on the file */
  gid_t *pGid                     /* OUT: gid to set on the file */
){
  int rc = SQLITE_OK;             /* Return Code */
  *pMode = 0;
  *pUid = 0;
  *pGid = 0;
  if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
    char zDb[MAX_PATHNAME+1];     /* Database file path */
    int nDb;                      /* Number of valid bytes in zDb */
    struct stat sStat;            /* Output of stat() on database file */

    /* zPath is a path to a WAL or journal file. The following block derives
    ** the path to the associated database file from zPath. This block handles
    ** the following naming conventions:
    **
    **   "<path to db>-journal"
    **   "<path to db>-wal"
    **   "<path to db>-journalNN"
    **   "<path to db>-walNN"
    **
    ** where NN is a decimal number. The NN naming schemes are 
    ** used by the test_multiplex.c module.
    */
    nDb = sqlite3Strlen30(zPath) - 1; 
#ifdef SQLITE_ENABLE_8_3_NAMES
    while( nDb>0 && sqlite3Isalnum(zPath[nDb]) ) nDb--;
    if( nDb==0 || zPath[nDb]!='-' ) return SQLITE_OK;
#else
    while( zPath[nDb]!='-' ){
      assert( nDb>0 );
      assert( zPath[nDb]!='\n' );
      nDb--;
    }
#endif
    memcpy(zDb, zPath, nDb);
    zDb[nDb] = '\0';

    if( 0==osStat(zDb, &sStat) ){
      *pMode = sStat.st_mode & 0777;
      *pUid = sStat.st_uid;
      *pGid = sStat.st_gid;
    }else{
      rc = SQLITE_IOERR_FSTAT;
    }
  }else if( flags & SQLITE_OPEN_DELETEONCLOSE ){
    *pMode = 0600;
  }
  return rc;
}

/*
** Open the file zPath.
** 
** Previously, the SQLite OS layer used three functions in place of this
** one:
**
**     sqlite3OsOpenReadWrite();
**     sqlite3OsOpenReadOnly();
**     sqlite3OsOpenExclusive();
**
** These calls correspond to the following combinations of flags:
**
**     ReadWrite() ->     (READWRITE | CREATE)
**     ReadOnly()  ->     (READONLY) 
**     OpenExclusive() -> (READWRITE | CREATE | EXCLUSIVE)
**
** The old OpenExclusive() accepted a boolean argument - "delFlag". If
** true, the file was configured to be automatically deleted when the
** file handle closed. To achieve the same effect using this new 
** interface, add the DELETEONCLOSE flag to those specified above for 
** OpenExclusive().
*/
static int unixOpen(
  sqlite3_vfs *pVfs,           /* The VFS for which this is the xOpen method */
  const char *zPath,           /* Pathname of file to be opened */
  sqlite3_file *pFile,         /* The file descriptor to be filled in */
  int flags,                   /* Input flags to control the opening */
  int *pOutFlags               /* Output flags returned to SQLite core */
){
  unixFile *p = (unixFile *)pFile;
  int fd = -1;                   /* File descriptor returned by open() */
  int openFlags = 0;             /* Flags to pass to open() */
  int eType = flags&0xFFFFFF00;  /* Type of file to open */
  int noLock;                    /* True to omit locking primitives */
  int rc = SQLITE_OK;            /* Function Return Code */
  int ctrlFlags = 0;             /* UNIXFILE_* flags */

  int isExclusive  = (flags & SQLITE_OPEN_EXCLUSIVE);
  int isDelete     = (flags & SQLITE_OPEN_DELETEONCLOSE);
  int isCreate     = (flags & SQLITE_OPEN_CREATE);
  int isReadonly   = (flags & SQLITE_OPEN_READONLY);
  int isReadWrite  = (flags & SQLITE_OPEN_READWRITE);
#if SQLITE_ENABLE_LOCKING_STYLE
  int isAutoProxy  = (flags & SQLITE_OPEN_AUTOPROXY);
#endif
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  struct statfs fsInfo;
#endif

  /* If creating a master or main-file journal, this function will open
  ** a file-descriptor on the directory too. The first time unixSync()
  ** is called the directory file descriptor will be fsync()ed and close()d.
  */
  int syncDir = (isCreate && (
        eType==SQLITE_OPEN_MASTER_JOURNAL 
     || eType==SQLITE_OPEN_MAIN_JOURNAL 
     || eType==SQLITE_OPEN_WAL
  ));

  /* If argument zPath is a NULL pointer, this function is required to open
  ** a temporary file. Use this buffer to store the file name in.
  */
  char zTmpname[MAX_PATHNAME+2];
  const char *zName = zPath;

  /* Check the following statements are true: 
  **
  **   (a) Exactly one of the READWRITE and READONLY flags must be set, and 
  **   (b) if CREATE is set, then READWRITE must also be set, and
  **   (c) if EXCLUSIVE is set, then CREATE must also be set.
  **   (d) if DELETEONCLOSE is set, then CREATE must also be set.
  */
  assert((isReadonly==0 || isReadWrite==0) && (isReadWrite || isReadonly));
  assert(isCreate==0 || isReadWrite);
  assert(isExclusive==0 || isCreate);
  assert(isDelete==0 || isCreate);

  /* The main DB, main journal, WAL file and master journal are never 
  ** automatically deleted. Nor are they ever temporary files.  */
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_DB );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MASTER_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_WAL );

  /* Assert that the upper layer has set one of the "file-type" flags. */
  assert( eType==SQLITE_OPEN_MAIN_DB      || eType==SQLITE_OPEN_TEMP_DB 
       || eType==SQLITE_OPEN_MAIN_JOURNAL || eType==SQLITE_OPEN_TEMP_JOURNAL 
       || eType==SQLITE_OPEN_SUBJOURNAL   || eType==SQLITE_OPEN_MASTER_JOURNAL 
       || eType==SQLITE_OPEN_TRANSIENT_DB || eType==SQLITE_OPEN_WAL
  );

  /* Detect a pid change and reset the PRNG.  There is a race condition
  ** here such that two or more threads all trying to open databases at
  ** the same instant might all reset the PRNG.  But multiple resets
  ** are harmless.
  */
  if( randomnessPid!=osGetpid(0) ){
    randomnessPid = osGetpid(0);
    sqlite3_randomness(0,0);
  }

  memset(p, 0, sizeof(unixFile));

  if( eType==SQLITE_OPEN_MAIN_DB ){
    UnixUnusedFd *pUnused;
    pUnused = findReusableFd(zName, flags);
    if( pUnused ){
      fd = pUnused->fd;
    }else{
      pUnused = sqlite3_malloc64(sizeof(*pUnused));
      if( !pUnused ){
        return SQLITE_NOMEM;
      }
    }
    p->pUnused = pUnused;

    /* Database filenames are double-zero terminated if they are not
    ** URIs with parameters.  Hence, they can always be passed into
    ** sqlite3_uri_parameter(). */
    assert( (flags & SQLITE_OPEN_URI) || zName[strlen(zName)+1]==0 );

  }else if( !zName ){
    /* If zName is NULL, the upper layer is requesting a temp file. */
    assert(isDelete && !syncDir);
    rc = unixGetTempname(MAX_PATHNAME+2, zTmpname);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    zName = zTmpname;

    /* Generated temporary filenames are always double-zero terminated
    ** for use by sqlite3_uri_parameter(). */
    assert( zName[strlen(zName)+1]==0 );
  }

  /* Determine the value of the flags parameter passed to POSIX function
  ** open(). These must be calculated even if open() is not called, as
  ** they may be stored as part of the file handle and used by the 
  ** 'conch file' locking functions later on.  */
  if( isReadonly )  openFlags |= O_RDONLY;
  if( isReadWrite ) openFlags |= O_RDWR;
  if( isCreate )    openFlags |= O_CREAT;
  if( isExclusive ) openFlags |= (O_EXCL|O_NOFOLLOW);
  openFlags |= (O_LARGEFILE|O_BINARY);

  if( fd<0 ){
    mode_t openMode;              /* Permissions to create file with */
    uid_t uid;                    /* Userid for the file */
    gid_t gid;                    /* Groupid for the file */
    rc = findCreateFileMode(zName, flags, &openMode, &uid, &gid);
    if( rc!=SQLITE_OK ){
      assert( !p->pUnused );
      assert( eType==SQLITE_OPEN_WAL || eType==SQLITE_OPEN_MAIN_JOURNAL );
      return rc;
    }
    fd = robust_open(zName, openFlags, openMode);
    OSTRACE(("OPENX   %-3d %s 0%o\n", fd, zName, openFlags));
    if( fd<0 && errno!=EISDIR && isReadWrite && !isExclusive ){
      /* Failed to open the file for read/write access. Try read-only. */
      flags &= ~(SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
      openFlags &= ~(O_RDWR|O_CREAT);
      flags |= SQLITE_OPEN_READONLY;
      openFlags |= O_RDONLY;
      isReadonly = 1;
      fd = robust_open(zName, openFlags, openMode);
    }
    if( fd<0 ){
      rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zName);
      goto open_finished;
    }

    /* If this process is running as root and if creating a new rollback
    ** journal or WAL file, set the ownership of the journal or WAL to be
    ** the same as the original database.
    */
    if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
      osFchown(fd, uid, gid);
    }
  }
  assert( fd>=0 );
  if( pOutFlags ){
    *pOutFlags = flags;
  }

  if( p->pUnused ){
    p->pUnused->fd = fd;
    p->pUnused->flags = flags;
  }

  if( isDelete ){
#if OS_VXWORKS
    zPath = zName;
#elif defined(SQLITE_UNLINK_AFTER_CLOSE)
    zPath = sqlite3_mprintf("%s", zName);
    if( zPath==0 ){
      robust_close(p, fd, __LINE__);
      return SQLITE_NOMEM;
    }
#else
    osUnlink(zName);
#endif
  }
#if SQLITE_ENABLE_LOCKING_STYLE
  else{
    p->openFlags = openFlags;
  }
#endif

  noLock = eType!=SQLITE_OPEN_MAIN_DB;

  
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  if( fstatfs(fd, &fsInfo) == -1 ){
    storeLastErrno(p, errno);
    robust_close(p, fd, __LINE__);
    return SQLITE_IOERR_ACCESS;
  }
  if (0 == strncmp("msdos", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
  if (0 == strncmp("exfat", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
#endif

  /* Set up appropriate ctrlFlags */
  if( isDelete )                ctrlFlags |= UNIXFILE_DELETE;
  if( isReadonly )              ctrlFlags |= UNIXFILE_RDONLY;
  if( noLock )                  ctrlFlags |= UNIXFILE_NOLOCK;
  if( syncDir )                 ctrlFlags |= UNIXFILE_DIRSYNC;
  if( flags & SQLITE_OPEN_URI ) ctrlFlags |= UNIXFILE_URI;

#if SQLITE_ENABLE_LOCKING_STYLE
#if SQLITE_PREFER_PROXY_LOCKING
  isAutoProxy = 1;
#endif
  if( isAutoProxy && (zPath!=NULL) && (!noLock) && pVfs->xOpen ){
    char *envforce = getenv("SQLITE_FORCE_PROXY_LOCKING");
    int useProxy = 0;

    /* SQLITE_FORCE_PROXY_LOCKING==1 means force always use proxy, 0 means 
    ** never use proxy, NULL means use proxy for non-local files only.  */
    if( envforce!=NULL ){
      useProxy = atoi(envforce)>0;
    }else{
      useProxy = !(fsInfo.f_flags&MNT_LOCAL);
    }
    if( useProxy ){
      rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);
      if( rc==SQLITE_OK ){
        rc = proxyTransformUnixFile((unixFile*)pFile, ":auto:");
        if( rc!=SQLITE_OK ){
          /* Use unixClose to clean up the resources added in fillInUnixFile 
          ** and clear all the structure's references.  Specifically, 
          ** pFile->pMethods will be NULL so sqlite3OsClose will be a no-op 
          */
          unixClose(pFile);
          return rc;
        }
      }
      goto open_finished;
    }
  }
#endif
  
  rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);

open_finished:
  if( rc!=SQLITE_OK ){
    sqlite3_free(p->pUnused);
  }
  return rc;
}


/*
** Delete the file at zPath. If the dirSync argument is true, fsync()
** the directory after deleting the file.
*/
static int unixDelete(
  sqlite3_vfs *NotUsed,     /* VFS containing this as the xDelete method */
  const char *zPath,        /* Name of file to be deleted */
  int dirSync               /* If true, fsync() directory after deleting file */
){
  int rc = SQLITE_OK;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError(return SQLITE_IOERR_DELETE);
  if( osUnlink(zPath)==(-1) ){
    if( errno==ENOENT
#if OS_VXWORKS
        || osAccess(zPath,0)!=0
#endif
    ){
      rc = SQLITE_IOERR_DELETE_NOENT;
    }else{
      rc = unixLogError(SQLITE_IOERR_DELETE, "unlink", zPath);
    }
    return rc;
  }
#ifndef SQLITE_DISABLE_DIRSYNC
  if( (dirSync & 1)!=0 ){
    int fd;
    rc = osOpenDirectory(zPath, &fd);
    if( rc==SQLITE_OK ){
#if OS_VXWORKS
      if( fsync(fd)==-1 )
#else
      if( fsync(fd) )
#endif
      {
        rc = unixLogError(SQLITE_IOERR_DIR_FSYNC, "fsync", zPath);
      }
      robust_close(0, fd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
  }
#endif
  return rc;
}

/*
** Test the existence of or access permissions of file zPath. The
** test performed depends on the value of flags:
**
**     SQLITE_ACCESS_EXISTS: Return 1 if the file exists
**     SQLITE_ACCESS_READWRITE: Return 1 if the file is read and writable.
**     SQLITE_ACCESS_READONLY: Return 1 if the file is readable.
**
** Otherwise return 0.
*/
static int unixAccess(
  sqlite3_vfs *NotUsed,   /* The VFS containing this xAccess method */
  const char *zPath,      /* Path of the file to examine */
  int flags,              /* What do we want to learn about the zPath file? */
  int *pResOut            /* Write result boolean here */
){
  int amode = 0;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError( return SQLITE_IOERR_ACCESS; );
  switch( flags ){
    case SQLITE_ACCESS_EXISTS:
      amode = F_OK;
      break;
    case SQLITE_ACCESS_READWRITE:
      amode = W_OK|R_OK;
      break;
    case SQLITE_ACCESS_READ:
      amode = R_OK;
      break;

    default:
      assert(!"Invalid flags argument");
  }
  *pResOut = (osAccess(zPath, amode)==0);
  if( flags==SQLITE_ACCESS_EXISTS && *pResOut ){
    struct stat buf;
    if( 0==osStat(zPath, &buf) && buf.st_size==0 ){
      *pResOut = 0;
    }
  }
  return SQLITE_OK;
}


/*
** Turn a relative pathname into a full pathname. The relative path
** is stored as a nul-terminated string in the buffer pointed to by
** zPath. 
**
** zOut points to a buffer of at least sqlite3_vfs.mxPathname bytes 
** (in this case, MAX_PATHNAME bytes). The full-path is written to
** this buffer before returning.
*/
static int unixFullPathname(
  sqlite3_vfs *pVfs,            /* Pointer to vfs object */
  const char *zPath,            /* Possibly relative input path */
  int nOut,                     /* Size of output buffer in bytes */
  char *zOut                    /* Output buffer */
){

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. This function could fail if, for example, the
  ** current working directory has been unlinked.
  */
  SimulateIOError( return SQLITE_ERROR );

  assert( pVfs->mxPathname==MAX_PATHNAME );
  UNUSED_PARAMETER(pVfs);

  zOut[nOut-1] = '\0';
  if( zPath[0]=='/' ){
    sqlite3_snprintf(nOut, zOut, "%s", zPath);
  }else{
    int nCwd;
    if( osGetcwd(zOut, nOut-1)==0 ){
      return unixLogError(SQLITE_CANTOPEN_BKPT, "getcwd", zPath);
    }
    nCwd = (int)strlen(zOut);
    sqlite3_snprintf(nOut-nCwd, &zOut[nCwd], "/%s", zPath);
  }
  return SQLITE_OK;
}


#ifndef SQLITE_OMIT_LOAD_EXTENSION
/*
** Interfaces for opening a shared library, finding entry points
** within the shared library, and closing the shared library.
*/
#include <dlfcn.h>
static void *unixDlOpen(sqlite3_vfs *NotUsed, const char *zFilename){
  UNUSED_PARAMETER(NotUsed);
  return dlopen(zFilename, RTLD_NOW | RTLD_GLOBAL);
}

/*
** SQLite calls this function immediately after a call to unixDlSym() or
** unixDlOpen() fails (returns a null pointer). If a more detailed error
** message is available, it is written to zBufOut. If no error message
** is available, zBufOut is left unmodified and SQLite uses a default
** error message.
*/
static void unixDlError(sqlite3_vfs *NotUsed, int nBuf, char *zBufOut){
  const char *zErr;
  UNUSED_PARAMETER(NotUsed);
  unixEnterMutex();
  zErr = dlerror();
  if( zErr ){
    sqlite3_snprintf(nBuf, zBufOut, "%s", zErr);
  }
  unixLeaveMutex();
}
static void (*unixDlSym(sqlite3_vfs *NotUsed, void *p, const char*zSym))(void){
  /* 
  ** GCC with -pedantic-errors says that C90 does not allow a void* to be
  ** cast into a pointer to a function.  And yet the library dlsym() routine
  ** returns a void* which is really a pointer to a function.  So how do we
  ** use dlsym() with -pedantic-errors?
  **
  ** Variable x below is defined to be a pointer to a function taking
  ** parameters void* and const char* and returning a pointer to a function.
  ** We initialize x by assigning it a pointer to the dlsym() function.
  ** (That assignment requires a cast.)  Then we call the function that
  ** x points to.  
  **
  ** This work-around is unlikely to work correctly on any system where
  ** you really cannot cast a function pointer into void*.  But then, on the
  ** other hand, dlsym() will not work on such a system either, so we have
  ** not really lost anything.
  */
  void (*(*x)(void*,const char*))(void);
  UNUSED_PARAMETER(NotUsed);
  x = (void(*(*)(void*,const char*))(void))dlsym;
  return (*x)(p, zSym);
}
static void unixDlClose(sqlite3_vfs *NotUsed, void *pHandle){
  UNUSED_PARAMETER(NotUsed);
  dlclose(pHandle);
}
#else /* if SQLITE_OMIT_LOAD_EXTENSION is defined: */
  #define unixDlOpen  0
  #define unixDlError 0
  #define unixDlSym   0
  #define unixDlClose 0
#endif

/*
** Write nBuf bytes of random data to the supplied buffer zBuf.
*/
static int unixRandomness(sqlite3_vfs *NotUsed, int nBuf, char *zBuf){
  UNUSED_PARAMETER(NotUsed);
  assert((size_t)nBuf>=(sizeof(time_t)+sizeof(int)));

  /* We have to initialize zBuf to prevent valgrind from reporting
  ** errors.  The reports issued by valgrind are incorrect - we would
  ** prefer that the randomness be increased by making use of the
  ** uninitialized space in zBuf - but valgrind errors tend to worry
  ** some users.  Rather than argue, it seems easier just to initialize
  ** the whole array and silence valgrind, even if that means less randomness
  ** in the random seed.
  **
  ** When testing, initializing zBuf[] to zero is all we do.  That means
  ** that we always use the same random number sequence.  This makes the
  ** tests repeatable.
  */
  memset(zBuf, 0, nBuf);
  randomnessPid = osGetpid(0);  
#if !defined(SQLITE_TEST) && !defined(SQLITE_OMIT_RANDOMNESS)
  {
    int fd, got;
    fd = robust_open("/dev/urandom", O_RDONLY, 0);
    if( fd<0 ){
      time_t t;
      time(&t);
      memcpy(zBuf, &t, sizeof(t));
      memcpy(&zBuf[sizeof(t)], &randomnessPid, sizeof(randomnessPid));
      assert( sizeof(t)+sizeof(randomnessPid)<=(size_t)nBuf );
      nBuf = sizeof(t) + sizeof(randomnessPid);
    }else{
      do{ got = osRead(fd, zBuf, nBuf); }while( got<0 && errno==EINTR );
      robust_close(0, fd, __LINE__);
    }
  }
#endif
  return nBuf;
}


/*
** Sleep for a little while.  Return the amount of time slept.
** The argument is the number of microseconds we want to sleep.
** The return value is the number of microseconds of sleep actually
** requested from the underlying operating system, a number which
** might be greater than or equal to the argument, but not less
** than the argument.
*/
static int unixSleep(sqlite3_vfs *NotUsed, int microseconds){
#if OS_VXWORKS
  struct timespec sp;

  sp.tv_sec = microseconds / 1000000;
  sp.tv_nsec = (microseconds % 1000000) * 1000;
  nanosleep(&sp, NULL);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#elif defined(HAVE_USLEEP) && HAVE_USLEEP
  usleep(microseconds);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#else
  int seconds = (microseconds+999999)/1000000;
  sleep(seconds);
  UNUSED_PARAMETER(NotUsed);
  return seconds*1000000;
#endif
}

/*
** The following variable, if set to a non-zero value, is interpreted as
** the number of seconds since 1970 and is used to set the result of
** sqlite3OsCurrentTime() during testing.
*/
#ifdef SQLITE_TEST
SQLITE_API int sqlite3_current_time = 0;  /* Fake system time in seconds since 1970. */
#endif

/*
** Find the current time (in Universal Coordinated Time).  Write into *piNow
** the current time and date as a Julian Day number times 86_400_000.  In
** other words, write into *piNow the number of milliseconds since the Julian
** epoch of noon in Greenwich on November 24, 4714 B.C according to the
** proleptic Gregorian calendar.
**
** On success, return SQLITE_OK.  Return SQLITE_ERROR if the time and date 
** cannot be found.
*/
static int unixCurrentTimeInt64(sqlite3_vfs *NotUsed, sqlite3_int64 *piNow){
  static const sqlite3_int64 unixEpoch = 24405875*(sqlite3_int64)8640000;
  int rc = SQLITE_OK;
#if defined(NO_GETTOD)
  time_t t;
  time(&t);
  *piNow = ((sqlite3_int64)t)*1000 + unixEpoch;
#elif OS_VXWORKS
  struct timespec sNow;
  clock_gettime(CLOCK_REALTIME, &sNow);
  *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_nsec/1000000;
#else
  struct timeval sNow;
  if( gettimeofday(&sNow, 0)==0 ){
    *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_usec/1000;
  }else{
    rc = SQLITE_ERROR;
  }
#endif

#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *piNow = 1000*(sqlite3_int64)sqlite3_current_time + unixEpoch;
  }
#endif
  UNUSED_PARAMETER(NotUsed);
  return rc;
}

/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
static int unixCurrentTime(sqlite3_vfs *NotUsed, double *prNow){
  sqlite3_int64 i = 0;
  int rc;
  UNUSED_PARAMETER(NotUsed);
  rc = unixCurrentTimeInt64(0, &i);
  *prNow = i/86400000.0;
  return rc;
}

/*
** We added the xGetLastError() method with the intention of providing
** better low-level error messages when operating-system problems come up
** during SQLite operation.  But so far, none of that has been implemented
** in the core.  So this routine is never called.  For now, it is merely
** a place-holder.
*/
static int unixGetLastError(sqlite3_vfs *NotUsed, int NotUsed2, char *NotUsed3){
  UNUSED_PARAMETER(NotUsed);
  UNUSED_PARAMETER(NotUsed2);
  UNUSED_PARAMETER(NotUsed3);
  return 0;
}


/*
************************ End of sqlite3_vfs methods ***************************
******************************************************************************/

/******************************************************************************
************************** Begin Proxy Locking ********************************
**
** Proxy locking is a "uber-locking-method" in this sense:  It uses the
** other locking methods on secondary lock files.  Proxy locking is a
** meta-layer over top of the primitive locking implemented above.  For
** this reason, the division that implements of proxy locking is deferred
** until late in the file (here) after all of the other I/O methods have
** been defined - so that the primitive locking methods are available
** as services to help with the implementation of proxy locking.
**
****
**
** The default locking schemes in SQLite use byte-range locks on the
** database file to coordinate safe, concurrent access by multiple readers
** and writers [http://sqlite.org/lockingv3.html].  The five file locking
** states (UNLOCKED, PENDING, SHARED, RESERVED, EXCLUSIVE) are implemented
** as POSIX read & write locks over fixed set of locations (via fsctl),
** on AFP and SMB only exclusive byte-range locks are available via fsctl
** with _IOWR('z', 23, struct ByteRangeLockPB2) to track the same 5 states.
** To simulate a F_RDLCK on the shared range, on AFP a randomly selected
** address in the shared range is taken for a SHARED lock, the entire
** shared range is taken for an EXCLUSIVE lock):
**
**      PENDING_BYTE        0x40000000
**      RESERVED_BYTE       0x40000001
**      SHARED_RANGE        0x40000002 -> 0x40000200
**
** This works well on the local file system, but shows a nearly 100x
** slowdown in read performance on AFP because the AFP client disables
** the read cache when byte-range locks are present.  Enabling the read
** cache exposes a cache coherency problem that is present on all OS X
** supported network file systems.  NFS and AFP both observe the
** close-to-open semantics for ensuring cache coherency
** [http://nfs.sourceforge.net/#faq_a8], which does not effectively
** address the requirements for concurrent database access by multiple
** readers and writers
** [http://www.nabble.com/SQLite-on-NFS-cache-coherency-td15655701.html].
**
** To address the performance and cache coherency issues, proxy file locking
** changes the way database access is controlled by limiting access to a
** single host at a time and moving file locks off of the database file
** and onto a proxy file on the local file system.  
**
**
** Using proxy locks
** -----------------
**
** C APIs
**
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_SET_LOCKPROXYFILE,
**                       <proxy_path> | ":auto:");
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_GET_LOCKPROXYFILE,
**                       &<proxy_path>);
**
**
** SQL pragmas
**
**  PRAGMA [database.]lock_proxy_file=<proxy_path> | :auto:
**  PRAGMA [database.]lock_proxy_file
**
** Specifying ":auto:" means that if there is a conch file with a matching
** host ID in it, the proxy path in the conch file will be used, otherwise
** a proxy path based on the user's temp dir
** (via confstr(_CS_DARWIN_USER_TEMP_DIR,...)) will be used and the
** actual proxy file name is generated from the name and path of the
** database file.  For example:
**
**       For database path "/Users/me/foo.db" 
**       The lock path will be "<tmpdir>/sqliteplocks/_Users_me_foo.db:auto:")
**
** Once a lock proxy is configured for a database connection, it can not
** be removed, however it may be switched to a different proxy path via
** the above APIs (assuming the conch file is not being held by another
** connection or process). 
**
**
** How proxy locking works
** -----------------------
**
** Proxy file locking relies primarily on two new supporting files: 
**
**   *  conch file to limit access to the database file to a single host
**      at a time
**
**   *  proxy file to act as a proxy for the advisory locks normally
**      taken on the database
**
** The conch file - to use a proxy file, sqlite must first "hold the conch"
** by taking an sqlite-style shared lock on the conch file, reading the
** contents and comparing the host's unique host ID (see below) and lock
** proxy path against the values stored in the conch.  The conch file is
** stored in the same directory as the database file and the file name
** is patterned after the database file name as ".<databasename>-conch".
** If the conch file does not exist, or its contents do not match the
** host ID and/or proxy path, then the lock is escalated to an exclusive
** lock and the conch file contents is updated with the host ID and proxy
** path and the lock is downgraded to a shared lock again.  If the conch
** is held by another process (with a shared lock), the exclusive lock
** will fail and SQLITE_BUSY is returned.
**
** The proxy file - a single-byte file used for all advisory file locks
** normally taken on the database file.   This allows for safe sharing
** of the database file for multiple readers and writers on the same
** host (the conch ensures that they all use the same local lock file).
**
** Requesting the lock proxy does not immediately take the conch, it is
** only taken when the first request to lock database file is made.  
** This matches the semantics of the traditional locking behavior, where
** opening a connection to a database file does not take a lock on it.
** The shared lock and an open file descriptor are maintained until 
** the connection to the database is closed. 
**
** The proxy file and the lock file are never deleted so they only need
** to be created the first time they are used.
**
** Configuration options
** ---------------------
**
**  SQLITE_PREFER_PROXY_LOCKING
**
**       Database files accessed on non-local file systems are
**       automatically configured for proxy locking, lock files are
**       named automatically using the same logic as
**       PRAGMA lock_proxy_file=":auto:"
**    
**  SQLITE_PROXY_DEBUG
**
**       Enables the logging of error messages during host id file
**       retrieval and creation
**
**  LOCKPROXYDIR
**
**       Overrides the default directory used for lock proxy files that
**       are named automatically via the ":auto:" setting
**
**  SQLITE_DEFAULT_PROXYDIR_PERMISSIONS
**
**       Permissions to use when creating a directory for storing the
**       lock proxy files, only used when LOCKPROXYDIR is not set.
**    
**    
** As mentioned above, when compiled with SQLITE_PREFER_PROXY_LOCKING,
** setting the environment variable SQLITE_FORCE_PROXY_LOCKING to 1 will
** force proxy locking to be used for every database file opened, and 0
** will force automatic proxy locking to be disabled for all database
** files (explicitly calling the SQLITE_FCNTL_SET_LOCKPROXYFILE pragma or
** sqlite_file_control API is not affected by SQLITE_FORCE_PROXY_LOCKING).
*/

/*
** Proxy locking is only available on MacOSX 
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE

/*
** The proxyLockingContext has the path and file structures for the remote 
** and local proxy files in it
*/
typedef struct proxyLockingContext proxyLockingContext;
struct proxyLockingContext {
  unixFile *conchFile;         /* Open conch file */
  char *conchFilePath;         /* Name of the conch file */
  unixFile *lockProxy;         /* Open proxy lock file */
  char *lockProxyPath;         /* Name of the proxy lock file */
  char *dbPath;                /* Name of the open file */
  int conchHeld;               /* 1 if the conch is held, -1 if lockless */
  int nFails;                  /* Number of conch taking failures */
  void *oldLockingContext;     /* Original lockingcontext to restore on close */
  sqlite3_io_methods const *pOldMethod;     /* Original I/O methods for close */
};

/* 
** The proxy lock file path for the database at dbPath is written into lPath, 
** which must point to valid, writable memory large enough for a maxLen length
** file path. 
*/
static int proxyGetLockPath(const char *dbPath, char *lPath, size_t maxLen){
  int len;
  int dbLen;
  int i;

#ifdef LOCKPROXYDIR
  len = strlcpy(lPath, LOCKPROXYDIR, maxLen);
#else
# ifdef _CS_DARWIN_USER_TEMP_DIR
  {
    if( !confstr(_CS_DARWIN_USER_TEMP_DIR, lPath, maxLen) ){
      OSTRACE(("GETLOCKPATH  failed %s errno=%d pid=%d\n",
               lPath, errno, osGetpid(0)));
      return SQLITE_IOERR_LOCK;
    }
    len = strlcat(lPath, "sqliteplocks", maxLen);    
  }
# else
  len = strlcpy(lPath, "/tmp/", maxLen);
# endif
#endif

  if( lPath[len-1]!='/' ){
    len = strlcat(lPath, "/", maxLen);
  }
  
  /* transform the db path to a unique cache name */
  dbLen = (int)strlen(dbPath);
  for( i=0; i<dbLen && (i+len+7)<(int)maxLen; i++){
    char c = dbPath[i];
    lPath[i+len] = (c=='/')?'_':c;
  }
  lPath[i+len]='\0';
  strlcat(lPath, ":auto:", maxLen);
  OSTRACE(("GETLOCKPATH  proxy lock path=%s pid=%d\n", lPath, osGetpid(0)));
  return SQLITE_OK;
}

/* 
 ** Creates the lock file and any missing directories in lockPath
 */
static int proxyCreateLockPath(const char *lockPath){
  int i, len;
  char buf[MAXPATHLEN];
  int start = 0;
  
  assert(lockPath!=NULL);
  /* try to create all the intermediate directories */
  len = (int)strlen(lockPath);
  buf[0] = lockPath[0];
  for( i=1; i<len; i++ ){
    if( lockPath[i] == '/' && (i - start > 0) ){
      /* only mkdir if leaf dir != "." or "/" or ".." */
      if( i-start>2 || (i-start==1 && buf[start] != '.' && buf[start] != '/') 
         || (i-start==2 && buf[start] != '.' && buf[start+1] != '.') ){
        buf[i]='\0';
        if( osMkdir(buf, SQLITE_DEFAULT_PROXYDIR_PERMISSIONS) ){
          int err=errno;
          if( err!=EEXIST ) {
            OSTRACE(("CREATELOCKPATH  FAILED creating %s, "
                     "'%s' proxy lock path=%s pid=%d\n",
                     buf, strerror(err), lockPath, osGetpid(0)));
            return err;
          }
        }
      }
      start=i+1;
    }
    buf[i] = lockPath[i];
  }
  OSTRACE(("CREATELOCKPATH  proxy lock path=%s pid=%d\n", lockPath, osGetpid(0)));
  return 0;
}

/*
** Create a new VFS file descriptor (stored in memory obtained from
** sqlite3_malloc) and open the file named "path" in the file descriptor.
**
** The caller is responsible not only for closing the file descriptor
** but also for freeing the memory associated with the file descriptor.
*/
static int proxyCreateUnixFile(
    const char *path,        /* path for the new unixFile */
    unixFile **ppFile,       /* unixFile created and returned by ref */
    int islockfile           /* if non zero missing dirs will be created */
) {
  int fd = -1;
  unixFile *pNew;
  int rc = SQLITE_OK;
  int openFlags = O_RDWR | O_CREAT;
  sqlite3_vfs dummyVfs;
  int terrno = 0;
  UnixUnusedFd *pUnused = NULL;

  /* 1. first try to open/create the file
  ** 2. if that fails, and this is a lock file (not-conch), try creating
  ** the parent directories and then try again.
  ** 3. if that fails, try to open the file read-only
  ** otherwise return BUSY (if lock file) or CANTOPEN for the conch file
  */
  pUnused = findReusableFd(path, openFlags);
  if( pUnused ){
    fd = pUnused->fd;
  }else{
    pUnused = sqlite3_malloc64(sizeof(*pUnused));
    if( !pUnused ){
      return SQLITE_NOMEM;
    }
  }
  if( fd<0 ){
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
    if( fd<0 && errno==ENOENT && islockfile ){
      if( proxyCreateLockPath(path) == SQLITE_OK ){
        fd = robust_open(path, openFlags, 0);
      }
    }
  }
  if( fd<0 ){
    openFlags = O_RDONLY;
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
  }
  if( fd<0 ){
    if( islockfile ){
      return SQLITE_BUSY;
    }
    switch (terrno) {
      case EACCES:
        return SQLITE_PERM;
      case EIO: 
        return SQLITE_IOERR_LOCK; /* even though it is the conch */
      default:
        return SQLITE_CANTOPEN_BKPT;
    }
  }
  
  pNew = (unixFile *)sqlite3_malloc64(sizeof(*pNew));
  if( pNew==NULL ){
    rc = SQLITE_NOMEM;
    goto end_create_proxy;
  }
  memset(pNew, 0, sizeof(unixFile));
  pNew->openFlags = openFlags;
  memset(&dummyVfs, 0, sizeof(dummyVfs));
  dummyVfs.pAppData = (void*)&autolockIoFinder;
  dummyVfs.zName = "dummy";
  pUnused->fd = fd;
  pUnused->flags = openFlags;
  pNew->pUnused = pUnused;
  
  rc = fillInUnixFile(&dummyVfs, fd, (sqlite3_file*)pNew, path, 0);
  if( rc==SQLITE_OK ){
    *ppFile = pNew;
    return SQLITE_OK;
  }
end_create_proxy:    
  robust_close(pNew, fd, __LINE__);
  sqlite3_free(pNew);
  sqlite3_free(pUnused);
  return rc;
}

#ifdef SQLITE_TEST
/* simulate multiple hosts by creating unique hostid file paths */
SQLITE_API int sqlite3_hostid_num = 0;
#endif

#define PROXY_HOSTIDLEN    16  /* conch file host id length */

#ifdef HAVE_GETHOSTUUID
/* Not always defined in the headers as it ought to be */
extern int gethostuuid(uuid_t id, const struct timespec *wait);
#endif

/* get the host ID via gethostuuid(), pHostID must point to PROXY_HOSTIDLEN 
** bytes of writable memory.
*/
static int proxyGetHostID(unsigned char *pHostID, int *pError){
  assert(PROXY_HOSTIDLEN == sizeof(uuid_t));
  memset(pHostID, 0, PROXY_HOSTIDLEN);
#ifdef HAVE_GETHOSTUUID
  {
    struct timespec timeout = {1, 0}; /* 1 sec timeout */
    if( gethostuuid(pHostID, &timeout) ){
      int err = errno;
      if( pError ){
        *pError = err;
      }
      return SQLITE_IOERR;
    }
  }
#else
  UNUSED_PARAMETER(pError);
#endif
#ifdef SQLITE_TEST
  /* simulate multiple hosts by creating unique hostid file paths */
  if( sqlite3_hostid_num != 0){
    pHostID[0] = (char)(pHostID[0] + (char)(sqlite3_hostid_num & 0xFF));
  }
#endif
  
  return SQLITE_OK;
}

/* The conch file contains the header, host id and lock file path
 */
#define PROXY_CONCHVERSION 2   /* 1-byte header, 16-byte host id, path */
#define PROXY_HEADERLEN    1   /* conch file header length */
#define PROXY_PATHINDEX    (PROXY_HEADERLEN+PROXY_HOSTIDLEN)
#define PROXY_MAXCONCHLEN  (PROXY_HEADERLEN+PROXY_HOSTIDLEN+MAXPATHLEN)

/* 
** Takes an open conch file, copies the contents to a new path and then moves 
** it back.  The newly created file's file descriptor is assigned to the
** conch file structure and finally the original conch file descriptor is 
** closed.  Returns zero if successful.
*/
static int proxyBreakConchLock(unixFile *pFile, uuid_t myHostID){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  char tPath[MAXPATHLEN];
  char buf[PROXY_MAXCONCHLEN];
  char *cPath = pCtx->conchFilePath;
  size_t readLen = 0;
  size_t pathLen = 0;
  char errmsg[64] = "";
  int fd = -1;
  int rc = -1;
  UNUSED_PARAMETER(myHostID);

  /* create a new path by replace the trailing '-conch' with '-break' */
  pathLen = strlcpy(tPath, cPath, MAXPATHLEN);
  if( pathLen>MAXPATHLEN || pathLen<6 || 
     (strlcpy(&tPath[pathLen-5], "break", 6) != 5) ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"path error (len %d)",(int)pathLen);
    goto end_breaklock;
  }
  /* read the conch content */
  readLen = osPread(conchFile->h, buf, PROXY_MAXCONCHLEN, 0);
  if( readLen<PROXY_PATHINDEX ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"read error (len %d)",(int)readLen);
    goto end_breaklock;
  }
  /* write it out to the temporary break file */
  fd = robust_open(tPath, (O_RDWR|O_CREAT|O_EXCL), 0);
  if( fd<0 ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "create failed (%d)", errno);
    goto end_breaklock;
  }
  if( osPwrite(fd, buf, readLen, 0) != (ssize_t)readLen ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "write failed (%d)", errno);
    goto end_breaklock;
  }
  if( rename(tPath, cPath) ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "rename failed (%d)", errno);
    goto end_breaklock;
  }
  rc = 0;
  fprintf(stderr, "broke stale lock on %s\n", cPath);
  robust_close(pFile, conchFile->h, __LINE__);
  conchFile->h = fd;
  conchFile->openFlags = O_RDWR | O_CREAT;

end_breaklock:
  if( rc ){
    if( fd>=0 ){
      osUnlink(tPath);
      robust_close(pFile, fd, __LINE__);
    }
    fprintf(stderr, "failed to break stale lock on %s, %s\n", cPath, errmsg);
  }
  return rc;
}

/* Take the requested lock on the conch file and break a stale lock if the 
** host id matches.
*/
static int proxyConchLock(unixFile *pFile, uuid_t myHostID, int lockType){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  int rc = SQLITE_OK;
  int nTries = 0;
  struct timespec conchModTime;
  
  memset(&conchModTime, 0, sizeof(conchModTime));
  do {
    rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
    nTries ++;
    if( rc==SQLITE_BUSY ){
      /* If the lock failed (busy):
       * 1st try: get the mod time of the conch, wait 0.5s and try again. 
       * 2nd try: fail if the mod time changed or host id is different, wait 
       *           10 sec and try again
       * 3rd try: break the lock unless the mod time has changed.
       */
      struct stat buf;
      if( osFstat(conchFile->h, &buf) ){
        storeLastErrno(pFile, errno);
        return SQLITE_IOERR_LOCK;
      }
      
      if( nTries==1 ){
        conchModTime = buf.st_mtimespec;
        usleep(500000); /* wait 0.5 sec and try the lock again*/
        continue;  
      }

      assert( nTries>1 );
      if( conchModTime.tv_sec != buf.st_mtimespec.tv_sec || 
         conchModTime.tv_nsec != buf.st_mtimespec.tv_nsec ){
        return SQLITE_BUSY;
      }
      
      if( nTries==2 ){  
        char tBuf[PROXY_MAXCONCHLEN];
        int len = osPread(conchFile->h, tBuf, PROXY_MAXCONCHLEN, 0);
        if( len<0 ){
          storeLastErrno(pFile, errno);
          return SQLITE_IOERR_LOCK;
        }
        if( len>PROXY_PATHINDEX && tBuf[0]==(char)PROXY_CONCHVERSION){
          /* don't break the lock if the host id doesn't match */
          if( 0!=memcmp(&tBuf[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN) ){
            return SQLITE_BUSY;
          }
        }else{
          /* don't break the lock on short read or a version mismatch */
          return SQLITE_BUSY;
        }
        usleep(10000000); /* wait 10 sec and try the lock again */
        continue; 
      }
      
      assert( nTries==3 );
      if( 0==proxyBreakConchLock(pFile, myHostID) ){
        rc = SQLITE_OK;
        if( lockType==EXCLUSIVE_LOCK ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, SHARED_LOCK);
        }
        if( !rc ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
        }
      }
    }
  } while( rc==SQLITE_BUSY && nTries<3 );
  
  return rc;
}

/* Takes the conch by taking a shared lock and read the contents conch, if 
** lockPath is non-NULL, the host ID and lock file path must match.  A NULL 
** lockPath means that the lockPath in the conch file will be used if the 
** host IDs match, or a new lock path will be generated automatically 
** and written to the conch file.
*/
static int proxyTakeConch(unixFile *pFile){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  
  if( pCtx->conchHeld!=0 ){
    return SQLITE_OK;
  }else{
    unixFile *conchFile = pCtx->conchFile;
    uuid_t myHostID;
    int pError = 0;
    char readBuf[PROXY_MAXCONCHLEN];
    char lockPath[MAXPATHLEN];
    char *tempLockPath = NULL;
    int rc = SQLITE_OK;
    int createConch = 0;
    int hostIdMatch = 0;
    int readLen = 0;
    int tryOldLockPath = 0;
    int forceNewLockPath = 0;
    
    OSTRACE(("TAKECONCH  %d for %s pid=%d\n", conchFile->h,
             (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"),
             osGetpid(0)));

    rc = proxyGetHostID(myHostID, &pError);
    if( (rc&0xff)==SQLITE_IOERR ){
      storeLastErrno(pFile, pError);
      goto end_takeconch;
    }
    rc = proxyConchLock(pFile, myHostID, SHARED_LOCK);
    if( rc!=SQLITE_OK ){
      goto end_takeconch;
    }
    /* read the existing conch file */
    readLen = seekAndRead((unixFile*)conchFile, 0, readBuf, PROXY_MAXCONCHLEN);
    if( readLen<0 ){
      /* I/O error: lastErrno set by seekAndRead */
      storeLastErrno(pFile, conchFile->lastErrno);
      rc = SQLITE_IOERR_READ;
      goto end_takeconch;
    }else if( readLen<=(PROXY_HEADERLEN+PROXY_HOSTIDLEN) || 
             readBuf[0]!=(char)PROXY_CONCHVERSION ){
      /* a short read or version format mismatch means we need to create a new 
      ** conch file. 
      */
      createConch = 1;
    }
    /* if the host id matches and the lock path already exists in the conch
    ** we'll try to use the path there, if we can't open that path, we'll 
    ** retry with a new auto-generated path 
    */
    do { /* in case we need to try again for an :auto: named lock file */

      if( !createConch && !forceNewLockPath ){
        hostIdMatch = !memcmp(&readBuf[PROXY_HEADERLEN], myHostID, 
                                  PROXY_HOSTIDLEN);
        /* if the conch has data compare the contents */
        if( !pCtx->lockProxyPath ){
          /* for auto-named local lock file, just check the host ID and we'll
           ** use the local lock file path that's already in there
           */
          if( hostIdMatch ){
            size_t pathLen = (readLen - PROXY_PATHINDEX);
            
            if( pathLen>=MAXPATHLEN ){
              pathLen=MAXPATHLEN-1;
            }
            memcpy(lockPath, &readBuf[PROXY_PATHINDEX], pathLen);
            lockPath[pathLen] = 0;
            tempLockPath = lockPath;
            tryOldLockPath = 1;
            /* create a copy of the lock path if the conch is taken */
            goto end_takeconch;
          }
        }else if( hostIdMatch
               && !strncmp(pCtx->lockProxyPath, &readBuf[PROXY_PATHINDEX],
                           readLen-PROXY_PATHINDEX)
        ){
          /* conch host and lock path match */
          goto end_takeconch; 
        }
      }
      
      /* if the conch isn't writable and doesn't match, we can't take it */
      if( (conchFile->openFlags&O_RDWR) == 0 ){
        rc = SQLITE_BUSY;
        goto end_takeconch;
      }
      
      /* either the conch didn't match or we need to create a new one */
      if( !pCtx->lockProxyPath ){
        proxyGetLockPath(pCtx->dbPath, lockPath, MAXPATHLEN);
        tempLockPath = lockPath;
        /* create a copy of the lock path _only_ if the conch is taken */
      }
      
      /* update conch with host and path (this will fail if other process
      ** has a shared lock already), if the host id matches, use the big
      ** stick.
      */
      futimes(conchFile->h, NULL);
      if( hostIdMatch && !createConch ){
        if( conchFile->pInode && conchFile->pInode->nShared>1 ){
          /* We are trying for an exclusive lock but another thread in this
           ** same process is still holding a shared lock. */
          rc = SQLITE_BUSY;
        } else {          
          rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
        }
      }else{
        rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
      }
      if( rc==SQLITE_OK ){
        char writeBuffer[PROXY_MAXCONCHLEN];
        int writeSize = 0;
        
        writeBuffer[0] = (char)PROXY_CONCHVERSION;
        memcpy(&writeBuffer[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN);
        if( pCtx->lockProxyPath!=NULL ){
          strlcpy(&writeBuffer[PROXY_PATHINDEX], pCtx->lockProxyPath,
                  MAXPATHLEN);
        }else{
          strlcpy(&writeBuffer[PROXY_PATHINDEX], tempLockPath, MAXPATHLEN);
        }
        writeSize = PROXY_PATHINDEX + strlen(&writeBuffer[PROXY_PATHINDEX]);
        robust_ftruncate(conchFile->h, writeSize);
        rc = unixWrite((sqlite3_file *)conchFile, writeBuffer, writeSize, 0);
        fsync(conchFile->h);
        /* If we created a new conch file (not just updated the contents of a 
         ** valid conch file), try to match the permissions of the database 
         */
        if( rc==SQLITE_OK && createConch ){
          struct stat buf;
          int err = osFstat(pFile->h, &buf);
          if( err==0 ){
            mode_t cmode = buf.st_mode&(S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP |
                                        S_IROTH|S_IWOTH);
            /* try to match the database file R/W permissions, ignore failure */
#ifndef SQLITE_PROXY_DEBUG
            osFchmod(conchFile->h, cmode);
#else
            do{
              rc = osFchmod(conchFile->h, cmode);
            }while( rc==(-1) && errno==EINTR );
            if( rc!=0 ){
              int code = errno;
              fprintf(stderr, "fchmod %o FAILED with %d %s\n",
                      cmode, code, strerror(code));
            } else {
              fprintf(stderr, "fchmod %o SUCCEDED\n",cmode);
            }
          }else{
            int code = errno;
            fprintf(stderr, "STAT FAILED[%d] with %d %s\n", 
                    err, code, strerror(code));
#endif
          }
        }
      }
      conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, SHARED_LOCK);
      
    end_takeconch:
      OSTRACE(("TRANSPROXY: CLOSE  %d\n", pFile->h));
      if( rc==SQLITE_OK && pFile->openFlags ){
        int fd;
        if( pFile->h>=0 ){
          robust_close(pFile, pFile->h, __LINE__);
        }
        pFile->h = -1;
        fd = robust_open(pCtx->dbPath, pFile->openFlags, 0);
        OSTRACE(("TRANSPROXY: OPEN  %d\n", fd));
        if( fd>=0 ){
          pFile->h = fd;
        }else{
          rc=SQLITE_CANTOPEN_BKPT; /* SQLITE_BUSY? proxyTakeConch called
           during locking */
        }
      }
      if( rc==SQLITE_OK && !pCtx->lockProxy ){
        char *path = tempLockPath ? tempLockPath : pCtx->lockProxyPath;
        rc = proxyCreateUnixFile(path, &pCtx->lockProxy, 1);
        if( rc!=SQLITE_OK && rc!=SQLITE_NOMEM && tryOldLockPath ){
          /* we couldn't create the proxy lock file with the old lock file path
           ** so try again via auto-naming 
           */
          forceNewLockPath = 1;
          tryOldLockPath = 0;
          continue; /* go back to the do {} while start point, try again */
        }
      }
      if( rc==SQLITE_OK ){
        /* Need to make a copy of path if we extracted the value
         ** from the conch file or the path was allocated on the stack
         */
        if( tempLockPath ){
          pCtx->lockProxyPath = sqlite3DbStrDup(0, tempLockPath);
          if( !pCtx->lockProxyPath ){
            rc = SQLITE_NOMEM;
          }
        }
      }
      if( rc==SQLITE_OK ){
        pCtx->conchHeld = 1;
        
        if( pCtx->lockProxy->pMethod == &afpIoMethods ){
          afpLockingContext *afpCtx;
          afpCtx = (afpLockingContext *)pCtx->lockProxy->lockingContext;
          afpCtx->dbPath = pCtx->lockProxyPath;
        }
      } else {
        conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
      }
      OSTRACE(("TAKECONCH  %d %s\n", conchFile->h,
               rc==SQLITE_OK?"ok":"failed"));
      return rc;
    } while (1); /* in case we need to retry the :auto: lock file - 
                 ** we should never get here except via the 'continue' call. */
  }
}

/*
** If pFile holds a lock on a conch file, then release that lock.
*/
static int proxyReleaseConch(unixFile *pFile){
  int rc = SQLITE_OK;         /* Subroutine return code */
  proxyLockingContext *pCtx;  /* The locking context for the proxy lock */
  unixFile *conchFile;        /* Name of the conch file */

  pCtx = (proxyLockingContext *)pFile->lockingContext;
  conchFile = pCtx->conchFile;
  OSTRACE(("RELEASECONCH  %d for %s pid=%d\n", conchFile->h,
           (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"), 
           osGetpid(0)));
  if( pCtx->conchHeld>0 ){
    rc = conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
  }
  pCtx->conchHeld = 0;
  OSTRACE(("RELEASECONCH  %d %s\n", conchFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}

/*
** Given the name of a database file, compute the name of its conch file.
** Store the conch filename in memory obtained from sqlite3_malloc64().
** Make *pConchPath point to the new name.  Return SQLITE_OK on success
** or SQLITE_NOMEM if unable to obtain memory.
**
** The caller is responsible for ensuring that the allocated memory
** space is eventually freed.
**
** *pConchPath is set to NULL if a memory allocation error occurs.
*/
static int proxyCreateConchPathname(char *dbPath, char **pConchPath){
  int i;                        /* Loop counter */
  int len = (int)strlen(dbPath); /* Length of database filename - dbPath */
  char *conchPath;              /* buffer in which to construct conch name */

  /* Allocate space for the conch filename and initialize the name to
  ** the name of the original database file. */  
  *pConchPath = conchPath = (char *)sqlite3_malloc64(len + 8);
  if( conchPath==0 ){
    return SQLITE_NOMEM;
  }
  memcpy(conchPath, dbPath, len+1);
  
  /* now insert a "." before the last / character */
  for( i=(len-1); i>=0; i-- ){
    if( conchPath[i]=='/' ){
      i++;
      break;
    }
  }
  conchPath[i]='.';
  while ( i<len ){
    conchPath[i+1]=dbPath[i];
    i++;
  }

  /* append the "-conch" suffix to the file */
  memcpy(&conchPath[i+1], "-conch", 7);
  assert( (int)strlen(conchPath) == len+7 );

  return SQLITE_OK;
}


/* Takes a fully configured proxy locking-style unix file and switches
** the local lock file path 
*/
static int switchLockProxyPath(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
  char *oldPath = pCtx->lockProxyPath;
  int rc = SQLITE_OK;

  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }  

  /* nothing to do if the path is NULL, :auto: or matches the existing path */
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ||
    (oldPath && !strncmp(oldPath, path, MAXPATHLEN)) ){
    return SQLITE_OK;
  }else{
    unixFile *lockProxy = pCtx->lockProxy;
    pCtx->lockProxy=NULL;
    pCtx->conchHeld = 0;
    if( lockProxy!=NULL ){
      rc=lockProxy->pMethod->xClose((sqlite3_file *)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
    }
    sqlite3_free(oldPath);
    pCtx->lockProxyPath = sqlite3DbStrDup(0, path);
  }
  
  return rc;
}

/*
** pFile is a file that has been opened by a prior xOpen call.  dbPath
** is a string buffer at least MAXPATHLEN+1 characters in size.
**
** This routine find the filename associated with pFile and writes it
** int dbPath.
*/
static int proxyGetDbPathForUnixFile(unixFile *pFile, char *dbPath){
#if defined(__APPLE__)
  if( pFile->pMethod == &afpIoMethods ){
    /* afp style keeps a reference to the db path in the filePath field 
    ** of the struct */
    assert( (int)strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, ((afpLockingContext *)pFile->lockingContext)->dbPath,
            MAXPATHLEN);
  } else
#endif
  if( pFile->pMethod == &dotlockIoMethods ){
    /* dot lock style uses the locking context to store the dot lock
    ** file path */
    int len = strlen((char *)pFile->lockingContext) - strlen(DOTLOCK_SUFFIX);
    memcpy(dbPath, (char *)pFile->lockingContext, len + 1);
  }else{
    /* all other styles use the locking context to store the db file path */
    assert( strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, (char *)pFile->lockingContext, MAXPATHLEN);
  }
  return SQLITE_OK;
}

/*
** Takes an already filled in unix file and alters it so all file locking 
** will be performed on the local proxy lock file.  The following fields
** are preserved in the locking context so that they can be restored and 
** the unix structure properly cleaned up at close time:
**  ->lockingContext
**  ->pMethod
*/
static int proxyTransformUnixFile(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx;
  char dbPath[MAXPATHLEN+1];       /* Name of the database file */
  char *lockPath=NULL;
  int rc = SQLITE_OK;
  
  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }
  proxyGetDbPathForUnixFile(pFile, dbPath);
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ){
    lockPath=NULL;
  }else{
    lockPath=(char *)path;
  }
  
  OSTRACE(("TRANSPROXY  %d for %s pid=%d\n", pFile->h,
           (lockPath ? lockPath : ":auto:"), osGetpid(0)));

  pCtx = sqlite3_malloc64( sizeof(*pCtx) );
  if( pCtx==0 ){
    return SQLITE_NOMEM;
  }
  memset(pCtx, 0, sizeof(*pCtx));

  rc = proxyCreateConchPathname(dbPath, &pCtx->conchFilePath);
  if( rc==SQLITE_OK ){
    rc = proxyCreateUnixFile(pCtx->conchFilePath, &pCtx->conchFile, 0);
    if( rc==SQLITE_CANTOPEN && ((pFile->openFlags&O_RDWR) == 0) ){
      /* if (a) the open flags are not O_RDWR, (b) the conch isn't there, and
      ** (c) the file system is read-only, then enable no-locking access.
      ** Ugh, since O_RDONLY==0x0000 we test for !O_RDWR since unixOpen asserts
      ** that openFlags will have only one of O_RDONLY or O_RDWR.
      */
      struct statfs fsInfo;
      struct stat conchInfo;
      int goLockless = 0;

      if( osStat(pCtx->conchFilePath, &conchInfo) == -1 ) {
        int err = errno;
        if( (err==ENOENT) && (statfs(dbPath, &fsInfo) != -1) ){
          goLockless = (fsInfo.f_flags&MNT_RDONLY) == MNT_RDONLY;
        }
      }
      if( goLockless ){
        pCtx->conchHeld = -1; /* read only FS/ lockless */
        rc = SQLITE_OK;
      }
    }
  }  
  if( rc==SQLITE_OK && lockPath ){
    pCtx->lockProxyPath = sqlite3DbStrDup(0, lockPath);
  }

  if( rc==SQLITE_OK ){
    pCtx->dbPath = sqlite3DbStrDup(0, dbPath);
    if( pCtx->dbPath==NULL ){
      rc = SQLITE_NOMEM;
    }
  }
  if( rc==SQLITE_OK ){
    /* all memory is allocated, proxys are created and assigned, 
    ** switch the locking context and pMethod then return.
    */
    pCtx->oldLockingContext = pFile->lockingContext;
    pFile->lockingContext = pCtx;
    pCtx->pOldMethod = pFile->pMethod;
    pFile->pMethod = &proxyIoMethods;
  }else{
    if( pCtx->conchFile ){ 
      pCtx->conchFile->pMethod->xClose((sqlite3_file *)pCtx->conchFile);
      sqlite3_free(pCtx->conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath); 
    sqlite3_free(pCtx);
  }
  OSTRACE(("TRANSPROXY  %d %s\n", pFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}


/*
** This routine handles sqlite3_file_control() calls that are specific
** to proxy locking.
*/
static int proxyFileControl(sqlite3_file *id, int op, void *pArg){
  switch( op ){
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      if( pFile->pMethod == &proxyIoMethods ){
        proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
        proxyTakeConch(pFile);
        if( pCtx->lockProxyPath ){
          *(const char **)pArg = pCtx->lockProxyPath;
        }else{
          *(const char **)pArg = ":auto: (not held)";
        }
      } else {
        *(const char **)pArg = NULL;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      int rc = SQLITE_OK;
      int isProxyStyle = (pFile->pMethod == &proxyIoMethods);
      if( pArg==NULL || (const char *)pArg==0 ){
        if( isProxyStyle ){
          /* turn off proxy locking - not supported.  If support is added for
          ** switching proxy locking mode off then it will need to fail if
          ** the journal mode is WAL mode. 
          */
          rc = SQLITE_ERROR /*SQLITE_PROTOCOL? SQLITE_MISUSE?*/;
        }else{
          /* turn off proxy locking - already off - NOOP */
          rc = SQLITE_OK;
        }
      }else{
        const char *proxyPath = (const char *)pArg;
        if( isProxyStyle ){
          proxyLockingContext *pCtx = 
            (proxyLockingContext*)pFile->lockingContext;
          if( !strcmp(pArg, ":auto:") 
           || (pCtx->lockProxyPath &&
               !strncmp(pCtx->lockProxyPath, proxyPath, MAXPATHLEN))
          ){
            rc = SQLITE_OK;
          }else{
            rc = switchLockProxyPath(pFile, proxyPath);
          }
        }else{
          /* turn on proxy file locking */
          rc = proxyTransformUnixFile(pFile, proxyPath);
        }
      }
      return rc;
    }
    default: {
      assert( 0 );  /* The call assures that only valid opcodes are sent */
    }
  }
  /*NOTREACHED*/
  return SQLITE_ERROR;
}

/*
** Within this division (the proxying locking implementation) the procedures
** above this point are all utilities.  The lock-related methods of the
** proxy-locking sqlite3_io_method object follow.
*/


/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int proxyCheckReservedLock(sqlite3_file *id, int *pResOut) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      return proxy->pMethod->xCheckReservedLock((sqlite3_file*)proxy, pResOut);
    }else{ /* conchHeld < 0 is lockless */
      pResOut=0;
    }
  }
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int proxyLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xLock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int proxyUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xUnlock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}

/*
** Close a file that uses proxy locks.
*/
static int proxyClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    unixFile *lockProxy = pCtx->lockProxy;
    unixFile *conchFile = pCtx->conchFile;
    int rc = SQLITE_OK;
    
    if( lockProxy ){
      rc = lockProxy->pMethod->xUnlock((sqlite3_file*)lockProxy, NO_LOCK);
      if( rc ) return rc;
      rc = lockProxy->pMethod->xClose((sqlite3_file*)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
      pCtx->lockProxy = 0;
    }
    if( conchFile ){
      if( pCtx->conchHeld ){
        rc = proxyReleaseConch(pFile);
        if( rc ) return rc;
      }
      rc = conchFile->pMethod->xClose((sqlite3_file*)conchFile);
      if( rc ) return rc;
      sqlite3_free(conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath);
    sqlite3DbFree(0, pCtx->dbPath);
    /* restore the original locking context and pMethod then close it */
    pFile->lockingContext = pCtx->oldLockingContext;
    pFile->pMethod = pCtx->pOldMethod;
    sqlite3_free(pCtx);
    return pFile->pMethod->xClose(id);
  }
  return SQLITE_OK;
}



#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The proxy locking style is intended for use with AFP filesystems.
** And since AFP is only supported on MacOSX, the proxy locking is also
** restricted to MacOSX.
** 
**
******************* End of the proxy lock implementation **********************
******************************************************************************/

/*
** Initialize the operating system interface.
**
** This routine registers all VFS implementations for unix-like operating
** systems.  This routine, and the sqlite3_os_end() routine that follows,
** should be the only routines in this file that are visible from other
** files.
**
** This routine is called once during SQLite initialization and by a
** single thread.  The memory allocation and mutex subsystems have not
** necessarily been initialized when this routine is called, and so they
** should not be used.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_init(void){ 
  /* 
  ** The following macro defines an initializer for an sqlite3_vfs object.
  ** The name of the VFS is NAME.  The pAppData is a pointer to a pointer
  ** to the "finder" function.  (pAppData is a pointer to a pointer because
  ** silly C90 rules prohibit a void* from being cast to a function pointer
  ** and so we have to go through the intermediate pointer to avoid problems
  ** when compiling with -pedantic-errors on GCC.)
  **
  ** The FINDER parameter to this macro is the name of the pointer to the
  ** finder-function.  The finder-function returns a pointer to the
  ** sqlite_io_methods object that implements the desired locking
  ** behaviors.  See the division above that contains the IOMETHODS
  ** macro for addition information on finder-functions.
  **
  ** Most finders simply return a pointer to a fixed sqlite3_io_methods
  ** object.  But the "autolockIoFinder" available on MacOSX does a little
  ** more than that; it looks at the filesystem type that hosts the 
  ** database file and tries to choose an locking method appropriate for
  ** that filesystem time.
  */
  #define UNIXVFS(VFSNAME, FINDER) {                        \
    3,                    /* iVersion */                    \
    sizeof(unixFile),     /* szOsFile */                    \
    MAX_PATHNAME,         /* mxPathname */                  \
    0,                    /* pNext */                       \
    VFSNAME,              /* zName */                       \
    (void*)&FINDER,       /* pAppData */                    \
    unixOpen,             /* xOpen */                       \
    unixDelete,           /* xDelete */                     \
    unixAccess,           /* xAccess */                     \
    unixFullPathname,     /* xFullPathname */               \
    unixDlOpen,           /* xDlOpen */                     \
    unixDlError,          /* xDlError */                    \
    unixDlSym,            /* xDlSym */                      \
    unixDlClose,          /* xDlClose */                    \
    unixRandomness,       /* xRandomness */                 \
    unixSleep,            /* xSleep */                      \
    unixCurrentTime,      /* xCurrentTime */                \
    unixGetLastError,     /* xGetLastError */               \
    unixCurrentTimeInt64, /* xCurrentTimeInt64 */           \
    unixSetSystemCall,    /* xSetSystemCall */              \
    unixGetSystemCall,    /* xGetSystemCall */              \
    unixNextSystemCall,   /* xNextSystemCall */             \
  }

  /*
  ** All default VFSes for unix are contained in the following array.
  **
  ** Note that the sqlite3_vfs.pNext field of the VFS object is modified
  ** by the SQLite core when the VFS is registered.  So the following
  ** array cannot be const.
  */
  static sqlite3_vfs aVfs[] = {
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix",          autolockIoFinder ),
#elif OS_VXWORKS
    UNIXVFS("unix",          vxworksIoFinder ),
#else
    UNIXVFS("unix",          posixIoFinder ),
#endif
    UNIXVFS("unix-none",     nolockIoFinder ),
    UNIXVFS("unix-dotfile",  dotlockIoFinder ),
    UNIXVFS("unix-excl",     posixIoFinder ),
#if OS_VXWORKS
    UNIXVFS("unix-namedsem", semIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE || OS_VXWORKS
    UNIXVFS("unix-posix",    posixIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE
    UNIXVFS("unix-flock",    flockIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix-afp",      afpIoFinder ),
    UNIXVFS("unix-nfs",      nfsIoFinder ),
    UNIXVFS("unix-proxy",    proxyIoFinder ),
#endif
  };
  unsigned int i;          /* Loop counter */

  /* Double-check that the aSyscall[] array has been constructed
  ** correctly.  See ticket [bb3a86e890c8e96ab] */
  assert( ArraySize(aSyscall)==25 );

  /* Register all VFSes defined in the aVfs[] array */
  for(i=0; i<(sizeof(aVfs)/sizeof(sqlite3_vfs)); i++){
    sqlite3_vfs_register(&aVfs[i], i==0);
  }
  return SQLITE_OK; 
}

/*
** Shutdown the operating system interface.
**
** Some operating systems might need to do some cleanup in this routine,
** to release dynamically allocated objects.  But not on unix.
** This routine is a no-op for unix.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_end(void){ 
  return SQLITE_OK; 
}
 
#endif /* SQLITE_OS_UNIX */

/************** End of os_unix.c *********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
******************************************************************************/

/******************************************************************************
****************************** No-op Locking **********************************
**
** Of the various locking implementations available, this is by far the
** simplest:  locking is ignored.  No attempt is made to lock the database
** file for reading or writing.
**
** This locking mode is appropriate for use on read-only databases
** (ex: databases that are burned into CD-ROM, for example.)  It can
** also be used if the application employs some external mechanism to
** prevent simultaneous access of the same database by two or more
** database connections.  But there is a serious risk of database
** corruption if this locking mode is used in situations where multiple
** database connections are accessing the same database file at the same
** time and one or more of those connections are writing.
*/

static int nolockCheckReservedLock(sqlite3_file *NotUsed, int *pResOut){
  UNUSED_PARAMETER(NotUsed);
  *pResOut = 0;
  return SQLITE_OK;
}
static int nolockLock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}
static int nolockUnlock(sqlite3_file *NotUsed, int NotUsed2){
  UNUSED_PARAMETER2(NotUsed, NotUsed2);
  return SQLITE_OK;
}

/*
** Close the file.
*/
static int nolockClose(sqlite3_file *id) {
  return closeUnixFile(id);
}

/******************* End of the no-op lock implementation *********************
******************************************************************************/

/******************************************************************************
************************* Begin dot-file Locking ******************************
**
** The dotfile locking implementation uses the existence of separate lock
** files (really a directory) to control access to the database.  This works
** on just about every filesystem imaginable.  But there are serious downsides:
**
**    (1)  There is zero concurrency.  A single reader blocks all other
**         connections from reading or writing the database.
**
**    (2)  An application crash or power loss can leave stale lock files
**         sitting around that need to be cleared manually.
**
** Nevertheless, a dotlock is an appropriate locking mode for use if no
** other locking strategy is available.
**
** Dotfile locking works by creating a subdirectory in the same directory as
** the database and with the same name but with a ".lock" extension added.
** The existence of a lock directory implies an EXCLUSIVE lock.  All other
** lock types (SHARED, RESERVED, PENDING) are mapped into EXCLUSIVE.
*/

/*
** The file suffix added to the data base filename in order to create the
** lock directory.
*/
#define DOTLOCK_SUFFIX ".lock"

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
**
** In dotfile locking, either a lock exists or it does not.  So in this
** variation of CheckReservedLock(), *pResOut is set to true if any lock
** is held on the file and false if the file is unlocked.
*/
static int dotlockCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    /* Either this connection or some other connection in the same process
    ** holds a lock on the file.  No need to check further. */
    reserved = 1;
  }else{
    /* The lock is held if and only if the lockfile exists */
    const char *zLockFile = (const char*)pFile->lockingContext;
    reserved = osAccess(zLockFile, 0)==0;
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (dotlock)\n", pFile->h, rc, reserved));
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
**
** With dotfile locking, we really only support state (4): EXCLUSIVE.
** But we track the other locking levels internally.
*/
static int dotlockLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc = SQLITE_OK;


  /* If we have any lock, then the lock file already exists.  All we have
  ** to do is adjust our internal record of the lock level.
  */
  if( pFile->eFileLock > NO_LOCK ){
    pFile->eFileLock = eFileLock;
    /* Always update the timestamp on the old file */
#ifdef HAVE_UTIME
    utime(zLockFile, NULL);
#else
    utimes(zLockFile, NULL);
#endif
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  rc = osMkdir(zLockFile, 0777);
  if( rc<0 ){
    /* failed to open/create the lock directory */
    int tErrno = errno;
    if( EEXIST == tErrno ){
      rc = SQLITE_BUSY;
    } else {
      rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
      if( IS_LOCK_ERROR(rc) ){
        storeLastErrno(pFile, tErrno);
      }
    }
    return rc;
  } 
  
  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
**
** When the locking level reaches NO_LOCK, delete the lock file.
*/
static int dotlockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  char *zLockFile = (char *)pFile->lockingContext;
  int rc;

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (dotlock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }

  /* To downgrade to shared, simply update our internal notion of the
  ** lock state.  No need to mess with the file on disk.
  */
  if( eFileLock==SHARED_LOCK ){
    pFile->eFileLock = SHARED_LOCK;
    return SQLITE_OK;
  }
  
  /* To fully unlock the database, delete the lock file */
  assert( eFileLock==NO_LOCK );
  rc = osRmdir(zLockFile);
  if( rc<0 && errno==ENOTDIR ) rc = osUnlink(zLockFile);
  if( rc<0 ){
    int tErrno = errno;
    rc = 0;
    if( ENOENT != tErrno ){
      rc = SQLITE_IOERR_UNLOCK;
    }
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
** Close a file.  Make sure the lock has been released before closing.
*/
static int dotlockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    dotlockUnlock(id, NO_LOCK);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
  }
  return rc;
}
/****************** End of the dot-file lock implementation *******************
******************************************************************************/

/******************************************************************************
************************** Begin flock Locking ********************************
**
** Use the flock() system call to do file locking.
**
** flock() locking is like dot-file locking in that the various
** fine-grain locking levels supported by SQLite are collapsed into
** a single exclusive lock.  In other words, SHARED, RESERVED, and
** PENDING locks are the same thing as an EXCLUSIVE lock.  SQLite
** still works when you do this, but concurrency is reduced since
** only a single process can be reading the database at a time.
**
** Omit this section if SQLITE_ENABLE_LOCKING_STYLE is turned off
*/
#if SQLITE_ENABLE_LOCKING_STYLE

/*
** Retry flock() calls that fail with EINTR
*/
#ifdef EINTR
static int robust_flock(int fd, int op){
  int rc;
  do{ rc = flock(fd,op); }while( rc<0 && errno==EINTR );
  return rc;
}
#else
# define robust_flock(a,b) flock(a,b)
#endif
     

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int flockCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    /* attempt to get the lock */
    int lrc = robust_flock(pFile->h, LOCK_EX | LOCK_NB);
    if( !lrc ){
      /* got the lock, unlock it */
      lrc = robust_flock(pFile->h, LOCK_UN);
      if ( lrc ) {
        int tErrno = errno;
        /* unlock failed with an error */
        lrc = SQLITE_IOERR_UNLOCK; 
        if( IS_LOCK_ERROR(lrc) ){
          storeLastErrno(pFile, tErrno);
          rc = lrc;
        }
      }
    } else {
      int tErrno = errno;
      reserved = 1;
      /* someone else might have it reserved */
      lrc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK); 
      if( IS_LOCK_ERROR(lrc) ){
        storeLastErrno(pFile, tErrno);
        rc = lrc;
      }
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (flock)\n", pFile->h, rc, reserved));

#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_OK;
    reserved=1;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** flock() only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int flockLock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;

  assert( pFile );

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* grab an exclusive lock */
  
  if (robust_flock(pFile->h, LOCK_EX | LOCK_NB)) {
    int tErrno = errno;
    /* didn't get, must be busy */
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_LOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
  } else {
    /* got it, set the type and return ok */
    pFile->eFileLock = eFileLock;
  }
  OSTRACE(("LOCK    %d %s %s (flock)\n", pFile->h, azFileLock(eFileLock), 
           rc==SQLITE_OK ? "ok" : "failed"));
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
  if( (rc & SQLITE_IOERR) == SQLITE_IOERR ){
    rc = SQLITE_BUSY;
  }
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int flockUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  
  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (flock)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really, unlock. */
  if( robust_flock(pFile->h, LOCK_UN) ){
#ifdef SQLITE_IGNORE_FLOCK_LOCK_ERRORS
    return SQLITE_OK;
#endif /* SQLITE_IGNORE_FLOCK_LOCK_ERRORS */
    return SQLITE_IOERR_UNLOCK;
  }else{
    pFile->eFileLock = NO_LOCK;
    return SQLITE_OK;
  }
}

/*
** Close a file.
*/
static int flockClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    flockUnlock(id, NO_LOCK);
    rc = closeUnixFile(id);
  }
  return rc;
}

#endif /* SQLITE_ENABLE_LOCKING_STYLE && !OS_VXWORK */

/******************* End of the flock lock implementation *********************
******************************************************************************/

/******************************************************************************
************************ Begin Named Semaphore Locking ************************
**
** Named semaphore locking is only supported on VxWorks.
**
** Semaphore locking is like dot-lock and flock in that it really only
** supports EXCLUSIVE locking.  Only a single process can read or write
** the database file at a time.  This reduces potential concurrency, but
** makes the lock implementation much easier.
*/
#if OS_VXWORKS

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int semXCheckReservedLock(sqlite3_file *id, int *pResOut) {
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;

  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );

  /* Check if a thread in this process holds such a lock */
  if( pFile->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it. */
  if( !reserved ){
    sem_t *pSem = pFile->pInode->pSem;

    if( sem_trywait(pSem)==-1 ){
      int tErrno = errno;
      if( EAGAIN != tErrno ){
        rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_CHECKRESERVEDLOCK);
        storeLastErrno(pFile, tErrno);
      } else {
        /* someone else has the lock when we are in NO_LOCK */
        reserved = (pFile->eFileLock < SHARED_LOCK);
      }
    }else{
      /* we could have it if we want it */
      sem_post(pSem);
    }
  }
  OSTRACE(("TEST WR-LOCK %d %d %d (sem)\n", pFile->h, rc, reserved));

  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** Semaphore locks only really support EXCLUSIVE locks.  We track intermediate
** lock states in the sqlite3_file structure, but all locks SHARED or
** above are really EXCLUSIVE locks and exclude all other processes from
** access the file.
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int semXLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;
  int rc = SQLITE_OK;

  /* if we already have a lock, it is exclusive.  
  ** Just adjust level and punt on outta here. */
  if (pFile->eFileLock > NO_LOCK) {
    pFile->eFileLock = eFileLock;
    rc = SQLITE_OK;
    goto sem_end_lock;
  }
  
  /* lock semaphore now but bail out when already locked. */
  if( sem_trywait(pSem)==-1 ){
    rc = SQLITE_BUSY;
    goto sem_end_lock;
  }

  /* got it, set the type and return ok */
  pFile->eFileLock = eFileLock;

 sem_end_lock:
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int semXUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  sem_t *pSem = pFile->pInode->pSem;

  assert( pFile );
  assert( pSem );
  OSTRACE(("UNLOCK  %d %d was %d pid=%d (sem)\n", pFile->h, eFileLock,
           pFile->eFileLock, osGetpid(0)));
  assert( eFileLock<=SHARED_LOCK );
  
  /* no-op if possible */
  if( pFile->eFileLock==eFileLock ){
    return SQLITE_OK;
  }
  
  /* shared can just be set because we always have an exclusive */
  if (eFileLock==SHARED_LOCK) {
    pFile->eFileLock = eFileLock;
    return SQLITE_OK;
  }
  
  /* no, really unlock. */
  if ( sem_post(pSem)==-1 ) {
    int rc, tErrno = errno;
    rc = sqliteErrorFromPosixError(tErrno, SQLITE_IOERR_UNLOCK);
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc; 
  }
  pFile->eFileLock = NO_LOCK;
  return SQLITE_OK;
}

/*
 ** Close a file.
 */
static int semXClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    semXUnlock(id, NO_LOCK);
    assert( pFile );
    unixEnterMutex();
    releaseInodeInfo(pFile);
    unixLeaveMutex();
    closeUnixFile(id);
  }
  return SQLITE_OK;
}

#endif /* OS_VXWORKS */
/*
** Named semaphore locking is only available on VxWorks.
**
*************** End of the named semaphore lock implementation ****************
******************************************************************************/


/******************************************************************************
*************************** Begin AFP Locking *********************************
**
** AFP is the Apple Filing Protocol.  AFP is a network filesystem found
** on Apple Macintosh computers - both OS9 and OSX.
**
** Third-party implementations of AFP are available.  But this code here
** only works on OSX.
*/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
** The afpLockingContext structure contains all afp lock specific state
*/
typedef struct afpLockingContext afpLockingContext;
struct afpLockingContext {
  int reserved;
  const char *dbPath;             /* Name of the open file */
};

struct ByteRangeLockPB2
{
  unsigned long long offset;        /* offset to first byte to lock */
  unsigned long long length;        /* nbr of bytes to lock */
  unsigned long long retRangeStart; /* nbr of 1st byte locked if successful */
  unsigned char unLockFlag;         /* 1 = unlock, 0 = lock */
  unsigned char startEndFlag;       /* 1=rel to end of fork, 0=rel to start */
  int fd;                           /* file desc to assoc this lock with */
};

#define afpfsByteRangeLock2FSCTL        _IOWR('z', 23, struct ByteRangeLockPB2)

/*
** This is a utility for setting or clearing a bit-range lock on an
** AFP filesystem.
** 
** Return SQLITE_OK on success, SQLITE_BUSY on failure.
*/
static int afpSetLock(
  const char *path,              /* Name of the file to be locked or unlocked */
  unixFile *pFile,               /* Open file descriptor on path */
  unsigned long long offset,     /* First byte to be locked */
  unsigned long long length,     /* Number of bytes to lock */
  int setLockFlag                /* True to set lock.  False to clear lock */
){
  struct ByteRangeLockPB2 pb;
  int err;
  
  pb.unLockFlag = setLockFlag ? 0 : 1;
  pb.startEndFlag = 0;
  pb.offset = offset;
  pb.length = length; 
  pb.fd = pFile->h;
  
  OSTRACE(("AFPSETLOCK [%s] for %d%s in range %llx:%llx\n", 
    (setLockFlag?"ON":"OFF"), pFile->h, (pb.fd==-1?"[testval-1]":""),
    offset, length));
  err = fsctl(path, afpfsByteRangeLock2FSCTL, &pb, 0);
  if ( err==-1 ) {
    int rc;
    int tErrno = errno;
    OSTRACE(("AFPSETLOCK failed to fsctl() '%s' %d %s\n",
             path, tErrno, strerror(tErrno)));
#ifdef SQLITE_IGNORE_AFP_LOCK_ERRORS
    rc = SQLITE_BUSY;
#else
    rc = sqliteErrorFromPosixError(tErrno,
                    setLockFlag ? SQLITE_IOERR_LOCK : SQLITE_IOERR_UNLOCK);
#endif /* SQLITE_IGNORE_AFP_LOCK_ERRORS */
    if( IS_LOCK_ERROR(rc) ){
      storeLastErrno(pFile, tErrno);
    }
    return rc;
  } else {
    return SQLITE_OK;
  }
}

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int afpCheckReservedLock(sqlite3_file *id, int *pResOut){
  int rc = SQLITE_OK;
  int reserved = 0;
  unixFile *pFile = (unixFile*)id;
  afpLockingContext *context;
  
  SimulateIOError( return SQLITE_IOERR_CHECKRESERVEDLOCK; );
  
  assert( pFile );
  context = (afpLockingContext *) pFile->lockingContext;
  if( context->reserved ){
    *pResOut = 1;
    return SQLITE_OK;
  }
  unixEnterMutex(); /* Because pFile->pInode is shared across threads */
  
  /* Check if a thread in this process holds such a lock */
  if( pFile->pInode->eFileLock>SHARED_LOCK ){
    reserved = 1;
  }
  
  /* Otherwise see if some other process holds it.
   */
  if( !reserved ){
    /* lock the RESERVED byte */
    int lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);  
    if( SQLITE_OK==lrc ){
      /* if we succeeded in taking the reserved lock, unlock it to restore
      ** the original state */
      lrc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
    } else {
      /* if we failed to get the lock then someone else must have it */
      reserved = 1;
    }
    if( IS_LOCK_ERROR(lrc) ){
      rc=lrc;
    }
  }
  
  unixLeaveMutex();
  OSTRACE(("TEST WR-LOCK %d %d %d (afp)\n", pFile->h, rc, reserved));
  
  *pResOut = reserved;
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int afpLock(sqlite3_file *id, int eFileLock){
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode = pFile->pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  
  assert( pFile );
  OSTRACE(("LOCK    %d %s was %s(%s,%d) pid=%d (afp)\n", pFile->h,
           azFileLock(eFileLock), azFileLock(pFile->eFileLock),
           azFileLock(pInode->eFileLock), pInode->nShared , osGetpid(0)));

  /* If there is already a lock of this type or more restrictive on the
  ** unixFile, do nothing. Don't use the afp_end_lock: exit path, as
  ** unixEnterMutex() hasn't been called yet.
  */
  if( pFile->eFileLock>=eFileLock ){
    OSTRACE(("LOCK    %d %s ok (already held) (afp)\n", pFile->h,
           azFileLock(eFileLock)));
    return SQLITE_OK;
  }

  /* Make sure the locking sequence is correct
  **  (1) We never move from unlocked to anything higher than shared lock.
  **  (2) SQLite never explicitly requests a pendig lock.
  **  (3) A shared lock is always held when a reserve lock is requested.
  */
  assert( pFile->eFileLock!=NO_LOCK || eFileLock==SHARED_LOCK );
  assert( eFileLock!=PENDING_LOCK );
  assert( eFileLock!=RESERVED_LOCK || pFile->eFileLock==SHARED_LOCK );
  
  /* This mutex is needed because pFile->pInode is shared across threads
  */
  unixEnterMutex();
  pInode = pFile->pInode;

  /* If some thread using this PID has a lock via a different unixFile*
  ** handle that precludes the requested lock, return BUSY.
  */
  if( (pFile->eFileLock!=pInode->eFileLock && 
       (pInode->eFileLock>=PENDING_LOCK || eFileLock>SHARED_LOCK))
     ){
    rc = SQLITE_BUSY;
    goto afp_end_lock;
  }
  
  /* If a SHARED lock is requested, and some thread using this PID already
  ** has a SHARED or RESERVED lock, then increment reference counts and
  ** return SQLITE_OK.
  */
  if( eFileLock==SHARED_LOCK && 
     (pInode->eFileLock==SHARED_LOCK || pInode->eFileLock==RESERVED_LOCK) ){
    assert( eFileLock==SHARED_LOCK );
    assert( pFile->eFileLock==0 );
    assert( pInode->nShared>0 );
    pFile->eFileLock = SHARED_LOCK;
    pInode->nShared++;
    pInode->nLock++;
    goto afp_end_lock;
  }
    
  /* A PENDING lock is needed before acquiring a SHARED lock and before
  ** acquiring an EXCLUSIVE lock.  For the SHARED lock, the PENDING will
  ** be released.
  */
  if( eFileLock==SHARED_LOCK 
      || (eFileLock==EXCLUSIVE_LOCK && pFile->eFileLock<PENDING_LOCK)
  ){
    int failed;
    failed = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 1);
    if (failed) {
      rc = failed;
      goto afp_end_lock;
    }
  }
  
  /* If control gets to this point, then actually go ahead and make
  ** operating system calls for the specified lock.
  */
  if( eFileLock==SHARED_LOCK ){
    int lrc1, lrc2, lrc1Errno = 0;
    long lk, mask;
    
    assert( pInode->nShared==0 );
    assert( pInode->eFileLock==0 );
        
    mask = (sizeof(long)==8) ? LARGEST_INT64 : 0x7fffffff;
    /* Now get the read-lock SHARED_LOCK */
    /* note that the quality of the randomness doesn't matter that much */
    lk = random(); 
    pInode->sharedByte = (lk & mask)%(SHARED_SIZE - 1);
    lrc1 = afpSetLock(context->dbPath, pFile, 
          SHARED_FIRST+pInode->sharedByte, 1, 1);
    if( IS_LOCK_ERROR(lrc1) ){
      lrc1Errno = pFile->lastErrno;
    }
    /* Drop the temporary PENDING lock */
    lrc2 = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    
    if( IS_LOCK_ERROR(lrc1) ) {
      storeLastErrno(pFile, lrc1Errno);
      rc = lrc1;
      goto afp_end_lock;
    } else if( IS_LOCK_ERROR(lrc2) ){
      rc = lrc2;
      goto afp_end_lock;
    } else if( lrc1 != SQLITE_OK ) {
      rc = lrc1;
    } else {
      pFile->eFileLock = SHARED_LOCK;
      pInode->nLock++;
      pInode->nShared = 1;
    }
  }else if( eFileLock==EXCLUSIVE_LOCK && pInode->nShared>1 ){
    /* We are trying for an exclusive lock but another thread in this
     ** same process is still holding a shared lock. */
    rc = SQLITE_BUSY;
  }else{
    /* The request was for a RESERVED or EXCLUSIVE lock.  It is
    ** assumed that there is a SHARED or greater lock on the file
    ** already.
    */
    int failed = 0;
    assert( 0!=pFile->eFileLock );
    if (eFileLock >= RESERVED_LOCK && pFile->eFileLock < RESERVED_LOCK) {
        /* Acquire a RESERVED lock */
        failed = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1,1);
      if( !failed ){
        context->reserved = 1;
      }
    }
    if (!failed && eFileLock == EXCLUSIVE_LOCK) {
      /* Acquire an EXCLUSIVE lock */
        
      /* Remove the shared lock before trying the range.  we'll need to 
      ** reestablish the shared lock if we can't get the  afpUnlock
      */
      if( !(failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST +
                         pInode->sharedByte, 1, 0)) ){
        int failed2 = SQLITE_OK;
        /* now attemmpt to get the exclusive lock range */
        failed = afpSetLock(context->dbPath, pFile, SHARED_FIRST, 
                               SHARED_SIZE, 1);
        if( failed && (failed2 = afpSetLock(context->dbPath, pFile, 
                       SHARED_FIRST + pInode->sharedByte, 1, 1)) ){
          /* Can't reestablish the shared lock.  Sqlite can't deal, this is
          ** a critical I/O error
          */
          rc = ((failed & SQLITE_IOERR) == SQLITE_IOERR) ? failed2 : 
               SQLITE_IOERR_LOCK;
          goto afp_end_lock;
        } 
      }else{
        rc = failed; 
      }
    }
    if( failed ){
      rc = failed;
    }
  }
  
  if( rc==SQLITE_OK ){
    pFile->eFileLock = eFileLock;
    pInode->eFileLock = eFileLock;
  }else if( eFileLock==EXCLUSIVE_LOCK ){
    pFile->eFileLock = PENDING_LOCK;
    pInode->eFileLock = PENDING_LOCK;
  }
  
afp_end_lock:
  unixLeaveMutex();
  OSTRACE(("LOCK    %d %s %s (afp)\n", pFile->h, azFileLock(eFileLock), 
         rc==SQLITE_OK ? "ok" : "failed"));
  return rc;
}

/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int afpUnlock(sqlite3_file *id, int eFileLock) {
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  unixInodeInfo *pInode;
  afpLockingContext *context = (afpLockingContext *) pFile->lockingContext;
  int skipShared = 0;
#ifdef SQLITE_TEST
  int h = pFile->h;
#endif

  assert( pFile );
  OSTRACE(("UNLOCK  %d %d was %d(%d,%d) pid=%d (afp)\n", pFile->h, eFileLock,
           pFile->eFileLock, pFile->pInode->eFileLock, pFile->pInode->nShared,
           osGetpid(0)));

  assert( eFileLock<=SHARED_LOCK );
  if( pFile->eFileLock<=eFileLock ){
    return SQLITE_OK;
  }
  unixEnterMutex();
  pInode = pFile->pInode;
  assert( pInode->nShared!=0 );
  if( pFile->eFileLock>SHARED_LOCK ){
    assert( pInode->eFileLock==pFile->eFileLock );
    SimulateIOErrorBenign(1);
    SimulateIOError( h=(-1) )
    SimulateIOErrorBenign(0);
    
#ifdef SQLITE_DEBUG
    /* When reducing a lock such that other processes can start
    ** reading the database file again, make sure that the
    ** transaction counter was updated if any part of the database
    ** file changed.  If the transaction counter is not updated,
    ** other connections to the same file might not realize that
    ** the file has changed and hence might not know to flush their
    ** cache.  The use of a stale cache can lead to database corruption.
    */
    assert( pFile->inNormalWrite==0
           || pFile->dbUpdate==0
           || pFile->transCntrChng==1 );
    pFile->inNormalWrite = 0;
#endif
    
    if( pFile->eFileLock==EXCLUSIVE_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, SHARED_FIRST, SHARED_SIZE, 0);
      if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1) ){
        /* only re-establish the shared lock if necessary */
        int sharedLockByte = SHARED_FIRST+pInode->sharedByte;
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 1);
      } else {
        skipShared = 1;
      }
    }
    if( rc==SQLITE_OK && pFile->eFileLock>=PENDING_LOCK ){
      rc = afpSetLock(context->dbPath, pFile, PENDING_BYTE, 1, 0);
    } 
    if( rc==SQLITE_OK && pFile->eFileLock>=RESERVED_LOCK && context->reserved ){
      rc = afpSetLock(context->dbPath, pFile, RESERVED_BYTE, 1, 0);
      if( !rc ){ 
        context->reserved = 0; 
      }
    }
    if( rc==SQLITE_OK && (eFileLock==SHARED_LOCK || pInode->nShared>1)){
      pInode->eFileLock = SHARED_LOCK;
    }
  }
  if( rc==SQLITE_OK && eFileLock==NO_LOCK ){

    /* Decrement the shared lock counter.  Release the lock using an
    ** OS call only when all threads in this same process have released
    ** the lock.
    */
    unsigned long long sharedLockByte = SHARED_FIRST+pInode->sharedByte;
    pInode->nShared--;
    if( pInode->nShared==0 ){
      SimulateIOErrorBenign(1);
      SimulateIOError( h=(-1) )
      SimulateIOErrorBenign(0);
      if( !skipShared ){
        rc = afpSetLock(context->dbPath, pFile, sharedLockByte, 1, 0);
      }
      if( !rc ){
        pInode->eFileLock = NO_LOCK;
        pFile->eFileLock = NO_LOCK;
      }
    }
    if( rc==SQLITE_OK ){
      pInode->nLock--;
      assert( pInode->nLock>=0 );
      if( pInode->nLock==0 ){
        closePendingFds(pFile);
      }
    }
  }
  
  unixLeaveMutex();
  if( rc==SQLITE_OK ) pFile->eFileLock = eFileLock;
  return rc;
}

/*
** Close a file & cleanup AFP specific locking context 
*/
static int afpClose(sqlite3_file *id) {
  int rc = SQLITE_OK;
  if( id ){
    unixFile *pFile = (unixFile*)id;
    afpUnlock(id, NO_LOCK);
    unixEnterMutex();
    if( pFile->pInode && pFile->pInode->nLock ){
      /* If there are outstanding locks, do not actually close the file just
      ** yet because that would clear those locks.  Instead, add the file
      ** descriptor to pInode->aPending.  It will be automatically closed when
      ** the last lock is cleared.
      */
      setPendingFd(pFile);
    }
    releaseInodeInfo(pFile);
    sqlite3_free(pFile->lockingContext);
    rc = closeUnixFile(id);
    unixLeaveMutex();
  }
  return rc;
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the AFP lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  If you don't compile for a mac, then the "unix-afp"
** VFS is not available.
**
********************* End of the AFP lock implementation **********************
******************************************************************************/

/******************************************************************************
*************************** Begin NFS Locking ********************************/

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/*
 ** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
 ** must be either NO_LOCK or SHARED_LOCK.
 **
 ** If the locking level of the file descriptor is already at or below
 ** the requested locking level, this routine is a no-op.
 */
static int nfsUnlock(sqlite3_file *id, int eFileLock){
  return posixUnlock(id, eFileLock, 1);
}

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The code above is the NFS lock implementation.  The code is specific
** to MacOSX and does not work on other unix platforms.  No alternative
** is available.  
**
********************* End of the NFS lock implementation **********************
******************************************************************************/

/******************************************************************************
**************** Non-locking sqlite3_file methods *****************************
**
** The next division contains implementations for all methods of the 
** sqlite3_file object other than the locking methods.  The locking
** methods were defined in divisions above (one locking method per
** division).  Those methods that are common to all locking modes
** are gather together into this division.
*/

/*
** Seek to the offset passed as the second argument, then read cnt 
** bytes into pBuf. Return the number of bytes actually read.
**
** NB:  If you define USE_PREAD or USE_PREAD64, then it might also
** be necessary to define _XOPEN_SOURCE to be 500.  This varies from
** one system to another.  Since SQLite does not define USE_PREAD
** in any form by default, we will not attempt to define _XOPEN_SOURCE.
** See tickets #2741 and #2681.
**
** To avoid stomping the errno value on a failed read the lastErrno value
** is set before returning.
*/
static int seekAndRead(unixFile *id, sqlite3_int64 offset, void *pBuf, int cnt){
  int got;
  int prior = 0;
#if (!defined(USE_PREAD) && !defined(USE_PREAD64))
  i64 newOffset;
#endif
  TIMER_START;
  assert( cnt==(cnt&0x1ffff) );
  assert( id->h>2 );
  do{
#if defined(USE_PREAD)
    got = osPread(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#elif defined(USE_PREAD64)
    got = osPread64(id->h, pBuf, cnt, offset);
    SimulateIOError( got = -1 );
#else
    newOffset = lseek(id->h, offset, SEEK_SET);
    SimulateIOError( newOffset-- );
    if( newOffset!=offset ){
      if( newOffset == -1 ){
        storeLastErrno((unixFile*)id, errno);
      }else{
        storeLastErrno((unixFile*)id, 0);
      }
      return -1;
    }
    got = osRead(id->h, pBuf, cnt);
#endif
    if( got==cnt ) break;
    if( got<0 ){
      if( errno==EINTR ){ got = 1; continue; }
      prior = 0;
      storeLastErrno((unixFile*)id,  errno);
      break;
    }else if( got>0 ){
      cnt -= got;
      offset += got;
      prior += got;
      pBuf = (void*)(got + (char*)pBuf);
    }
  }while( got>0 );
  TIMER_END;
  OSTRACE(("READ    %-3d %5d %7lld %llu\n",
            id->h, got+prior, offset-prior, TIMER_ELAPSED));
  return got+prior;
}

/*
** Read data from a file into a buffer.  Return SQLITE_OK if all
** bytes were read successfully and SQLITE_IOERR if anything goes
** wrong.
*/
static int unixRead(
  sqlite3_file *id, 
  void *pBuf, 
  int amt,
  sqlite3_int64 offset
){
  unixFile *pFile = (unixFile *)id;
  int got;
  assert( id );
  assert( offset>=0 );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this read request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(pBuf, &((u8 *)(pFile->pMapRegion))[offset], nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif

  got = seekAndRead(pFile, offset, pBuf, amt);
  if( got==amt ){
    return SQLITE_OK;
  }else if( got<0 ){
    /* lastErrno set by seekAndRead */
    return SQLITE_IOERR_READ;
  }else{
    storeLastErrno(pFile, 0);   /* not a system error */
    /* Unread parts of the buffer must be zero-filled */
    memset(&((char*)pBuf)[got], 0, amt-got);
    return SQLITE_IOERR_SHORT_READ;
  }
}

/*
** Attempt to seek the file-descriptor passed as the first argument to
** absolute offset iOff, then attempt to write nBuf bytes of data from
** pBuf to it. If an error occurs, return -1 and set *piErrno. Otherwise, 
** return the actual number of bytes written (which may be less than
** nBuf).
*/
static int seekAndWriteFd(
  int fd,                         /* File descriptor to write to */
  i64 iOff,                       /* File offset to begin writing at */
  const void *pBuf,               /* Copy data from this buffer to the file */
  int nBuf,                       /* Size of buffer pBuf in bytes */
  int *piErrno                    /* OUT: Error number if error occurs */
){
  int rc = 0;                     /* Value returned by system call */

  assert( nBuf==(nBuf&0x1ffff) );
  assert( fd>2 );
  nBuf &= 0x1ffff;
  TIMER_START;

#if defined(USE_PREAD)
  do{ rc = (int)osPwrite(fd, pBuf, nBuf, iOff); }while( rc<0 && errno==EINTR );
#elif defined(USE_PREAD64)
  do{ rc = (int)osPwrite64(fd, pBuf, nBuf, iOff);}while( rc<0 && errno==EINTR);
#else
  do{
    i64 iSeek = lseek(fd, iOff, SEEK_SET);
    SimulateIOError( iSeek-- );

    if( iSeek!=iOff ){
      if( piErrno ) *piErrno = (iSeek==-1 ? errno : 0);
      return -1;
    }
    rc = osWrite(fd, pBuf, nBuf);
  }while( rc<0 && errno==EINTR );
#endif

  TIMER_END;
  OSTRACE(("WRITE   %-3d %5d %7lld %llu\n", fd, rc, iOff, TIMER_ELAPSED));

  if( rc<0 && piErrno ) *piErrno = errno;
  return rc;
}


/*
** Seek to the offset in id->offset then read cnt bytes into pBuf.
** Return the number of bytes actually read.  Update the offset.
**
** To avoid stomping the errno value on a failed write the lastErrno value
** is set before returning.
*/
static int seekAndWrite(unixFile *id, i64 offset, const void *pBuf, int cnt){
  return seekAndWriteFd(id->h, offset, pBuf, cnt, &id->lastErrno);
}


/*
** Write data from a buffer into a file.  Return SQLITE_OK on success
** or some other error code on failure.
*/
static int unixWrite(
  sqlite3_file *id, 
  const void *pBuf, 
  int amt,
  sqlite3_int64 offset 
){
  unixFile *pFile = (unixFile*)id;
  int wrote = 0;
  assert( id );
  assert( amt>0 );

  /* If this is a database file (not a journal, master-journal or temp
  ** file), the bytes in the locking range should never be read or written. */
#if 0
  assert( pFile->pUnused==0
       || offset>=PENDING_BYTE+512
       || offset+amt<=PENDING_BYTE 
  );
#endif

#ifdef SQLITE_DEBUG
  /* If we are doing a normal write to a database file (as opposed to
  ** doing a hot-journal rollback or a write to some file other than a
  ** normal database file) then record the fact that the database
  ** has changed.  If the transaction counter is modified, record that
  ** fact too.
  */
  if( pFile->inNormalWrite ){
    pFile->dbUpdate = 1;  /* The database has been modified */
    if( offset<=24 && offset+amt>=27 ){
      int rc;
      char oldCntr[4];
      SimulateIOErrorBenign(1);
      rc = seekAndRead(pFile, 24, oldCntr, 4);
      SimulateIOErrorBenign(0);
      if( rc!=4 || memcmp(oldCntr, &((char*)pBuf)[24-offset], 4)!=0 ){
        pFile->transCntrChng = 1;  /* The transaction counter has changed */
      }
    }
  }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
  /* Deal with as much of this write request as possible by transfering
  ** data from the memory mapping using memcpy().  */
  if( offset<pFile->mmapSize ){
    if( offset+amt <= pFile->mmapSize ){
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, amt);
      return SQLITE_OK;
    }else{
      int nCopy = pFile->mmapSize - offset;
      memcpy(&((u8 *)(pFile->pMapRegion))[offset], pBuf, nCopy);
      pBuf = &((u8 *)pBuf)[nCopy];
      amt -= nCopy;
      offset += nCopy;
    }
  }
#endif
 
  while( (wrote = seekAndWrite(pFile, offset, pBuf, amt))<amt && wrote>0 ){
    amt -= wrote;
    offset += wrote;
    pBuf = &((char*)pBuf)[wrote];
  }
  SimulateIOError(( wrote=(-1), amt=1 ));
  SimulateDiskfullError(( wrote=0, amt=1 ));

  if( amt>wrote ){
    if( wrote<0 && pFile->lastErrno!=ENOSPC ){
      /* lastErrno set by seekAndWrite */
      return SQLITE_IOERR_WRITE;
    }else{
      storeLastErrno(pFile, 0); /* not a system error */
      return SQLITE_FULL;
    }
  }

  return SQLITE_OK;
}

#ifdef SQLITE_TEST
/*
** Count the number of fullsyncs and normal syncs.  This is used to test
** that syncs and fullsyncs are occurring at the right times.
*/
SQLITE_API int sqlite3_sync_count = 0;
SQLITE_API int sqlite3_fullsync_count = 0;
#endif

/*
** We do not trust systems to provide a working fdatasync().  Some do.
** Others do no.  To be safe, we will stick with the (slightly slower)
** fsync(). If you know that your system does support fdatasync() correctly,
** then simply compile with -Dfdatasync=fdatasync or -DHAVE_FDATASYNC
*/
#if !defined(fdatasync) && !HAVE_FDATASYNC
# define fdatasync fsync
#endif

/*
** Define HAVE_FULLFSYNC to 0 or 1 depending on whether or not
** the F_FULLFSYNC macro is defined.  F_FULLFSYNC is currently
** only available on Mac OS X.  But that could change.
*/
#ifdef F_FULLFSYNC
# define HAVE_FULLFSYNC 1
#else
# define HAVE_FULLFSYNC 0
#endif


/*
** The fsync() system call does not work as advertised on many
** unix systems.  The following procedure is an attempt to make
** it work better.
**
** The SQLITE_NO_SYNC macro disables all fsync()s.  This is useful
** for testing when we want to run through the test suite quickly.
** You are strongly advised *not* to deploy with SQLITE_NO_SYNC
** enabled, however, since with SQLITE_NO_SYNC enabled, an OS crash
** or power failure will likely corrupt the database file.
**
** SQLite sets the dataOnly flag if the size of the file is unchanged.
** The idea behind dataOnly is that it should only write the file content
** to disk, not the inode.  We only set dataOnly if the file size is 
** unchanged since the file size is part of the inode.  However, 
** Ted Ts'o tells us that fdatasync() will also write the inode if the
** file size has changed.  The only real difference between fdatasync()
** and fsync(), Ted tells us, is that fdatasync() will not flush the
** inode if the mtime or owner or other inode attributes have changed.
** We only care about the file size, not the other file attributes, so
** as far as SQLite is concerned, an fdatasync() is always adequate.
** So, we always use fdatasync() if it is available, regardless of
** the value of the dataOnly flag.
*/
static int full_fsync(int fd, int fullSync, int dataOnly){
  int rc;

  /* The following "ifdef/elif/else/" block has the same structure as
  ** the one below. It is replicated here solely to avoid cluttering 
  ** up the real code with the UNUSED_PARAMETER() macros.
  */
#ifdef SQLITE_NO_SYNC
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#elif HAVE_FULLFSYNC
  UNUSED_PARAMETER(dataOnly);
#else
  UNUSED_PARAMETER(fullSync);
  UNUSED_PARAMETER(dataOnly);
#endif

  /* Record the number of times that we do a normal fsync() and 
  ** FULLSYNC.  This is used during testing to verify that this procedure
  ** gets called with the correct arguments.
  */
#ifdef SQLITE_TEST
  if( fullSync ) sqlite3_fullsync_count++;
  sqlite3_sync_count++;
#endif

  /* If we compiled with the SQLITE_NO_SYNC flag, then syncing is a
  ** no-op
  */
#ifdef SQLITE_NO_SYNC
  rc = SQLITE_OK;
#elif HAVE_FULLFSYNC
  if( fullSync ){
    rc = osFcntl(fd, F_FULLFSYNC, 0);
  }else{
    rc = 1;
  }
  /* If the FULLFSYNC failed, fall back to attempting an fsync().
  ** It shouldn't be possible for fullfsync to fail on the local 
  ** file system (on OSX), so failure indicates that FULLFSYNC
  ** isn't supported for this file system. So, attempt an fsync 
  ** and (for now) ignore the overhead of a superfluous fcntl call.  
  ** It'd be better to detect fullfsync support once and avoid 
  ** the fcntl call every time sync is called.
  */
  if( rc ) rc = fsync(fd);

#elif defined(__APPLE__)
  /* fdatasync() on HFS+ doesn't yet flush the file size if it changed correctly
  ** so currently we default to the macro that redefines fdatasync to fsync
  */
  rc = fsync(fd);
#else 
  rc = fdatasync(fd);
#if OS_VXWORKS
  if( rc==-1 && errno==ENOTSUP ){
    rc = fsync(fd);
  }
#endif /* OS_VXWORKS */
#endif /* ifdef SQLITE_NO_SYNC elif HAVE_FULLFSYNC */

  if( OS_VXWORKS && rc!= -1 ){
    rc = 0;
  }
  return rc;
}

/*
** Open a file descriptor to the directory containing file zFilename.
** If successful, *pFd is set to the opened file descriptor and
** SQLITE_OK is returned. If an error occurs, either SQLITE_NOMEM
** or SQLITE_CANTOPEN is returned and *pFd is set to an undefined
** value.
**
** The directory file descriptor is used for only one thing - to
** fsync() a directory to make sure file creation and deletion events
** are flushed to disk.  Such fsyncs are not needed on newer
** journaling filesystems, but are required on older filesystems.
**
** This routine can be overridden using the xSetSysCall interface.
** The ability to override this routine was added in support of the
** chromium sandbox.  Opening a directory is a security risk (we are
** told) so making it overrideable allows the chromium sandbox to
** replace this routine with a harmless no-op.  To make this routine
** a no-op, replace it with a stub that returns SQLITE_OK but leaves
** *pFd set to a negative number.
**
** If SQLITE_OK is returned, the caller is responsible for closing
** the file descriptor *pFd using close().
*/
static int openDirectory(const char *zFilename, int *pFd){
  int ii;
  int fd = -1;
  char zDirname[MAX_PATHNAME+1];

  sqlite3_snprintf(MAX_PATHNAME, zDirname, "%s", zFilename);
  for(ii=(int)strlen(zDirname); ii>1 && zDirname[ii]!='/'; ii--);
  if( ii>0 ){
    zDirname[ii] = '\0';
    fd = robust_open(zDirname, O_RDONLY|O_BINARY, 0);
    if( fd>=0 ){
      OSTRACE(("OPENDIR %-3d %s\n", fd, zDirname));
    }
  }
  *pFd = fd;
  return (fd>=0?SQLITE_OK:unixLogError(SQLITE_CANTOPEN_BKPT, "open", zDirname));
}

/*
** Make sure all writes to a particular file are committed to disk.
**
** If dataOnly==0 then both the file itself and its metadata (file
** size, access time, etc) are synced.  If dataOnly!=0 then only the
** file data is synced.
**
** Under Unix, also make sure that the directory entry for the file
** has been created by fsync-ing the directory that contains the file.
** If we do not do this and we encounter a power failure, the directory
** entry for the journal might not exist after we reboot.  The next
** SQLite to access the file will not know that the journal exists (because
** the directory entry for the journal was never created) and the transaction
** will not roll back - possibly leading to database corruption.
*/
static int unixSync(sqlite3_file *id, int flags){
  int rc;
  unixFile *pFile = (unixFile*)id;

  int isDataOnly = (flags&SQLITE_SYNC_DATAONLY);
  int isFullsync = (flags&0x0F)==SQLITE_SYNC_FULL;

  /* Check that one of SQLITE_SYNC_NORMAL or FULL was passed */
  assert((flags&0x0F)==SQLITE_SYNC_NORMAL
      || (flags&0x0F)==SQLITE_SYNC_FULL
  );

  /* Unix cannot, but some systems may return SQLITE_FULL from here. This
  ** line is to test that doing so does not cause any problems.
  */
  SimulateDiskfullError( return SQLITE_FULL );

  assert( pFile );
  OSTRACE(("SYNC    %-3d\n", pFile->h));
  rc = full_fsync(pFile->h, isFullsync, isDataOnly);
  SimulateIOError( rc=1 );
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_FSYNC, "full_fsync", pFile->zPath);
  }

  /* Also fsync the directory containing the file if the DIRSYNC flag
  ** is set.  This is a one-time occurrence.  Many systems (examples: AIX)
  ** are unable to fsync a directory, so ignore errors on the fsync.
  */
  if( pFile->ctrlFlags & UNIXFILE_DIRSYNC ){
    int dirfd;
    OSTRACE(("DIRSYNC %s (have_fullfsync=%d fullsync=%d)\n", pFile->zPath,
            HAVE_FULLFSYNC, isFullsync));
    rc = osOpenDirectory(pFile->zPath, &dirfd);
    if( rc==SQLITE_OK && dirfd>=0 ){
      full_fsync(dirfd, 0, 0);
      robust_close(pFile, dirfd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
    pFile->ctrlFlags &= ~UNIXFILE_DIRSYNC;
  }
  return rc;
}

/*
** Truncate an open file to a specified size
*/
static int unixTruncate(sqlite3_file *id, i64 nByte){
  unixFile *pFile = (unixFile *)id;
  int rc;
  assert( pFile );
  SimulateIOError( return SQLITE_IOERR_TRUNCATE );

  /* If the user has configured a chunk-size for this file, truncate the
  ** file so that it consists of an integer number of chunks (i.e. the
  ** actual file size after the operation may be larger than the requested
  ** size).
  */
  if( pFile->szChunk>0 ){
    nByte = ((nByte + pFile->szChunk - 1)/pFile->szChunk) * pFile->szChunk;
  }

  rc = robust_ftruncate(pFile->h, nByte);
  if( rc ){
    storeLastErrno(pFile, errno);
    return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
  }else{
#ifdef SQLITE_DEBUG
    /* If we are doing a normal write to a database file (as opposed to
    ** doing a hot-journal rollback or a write to some file other than a
    ** normal database file) and we truncate the file to zero length,
    ** that effectively updates the change counter.  This might happen
    ** when restoring a database using the backup API from a zero-length
    ** source.
    */
    if( pFile->inNormalWrite && nByte==0 ){
      pFile->transCntrChng = 1;
    }
#endif

#if SQLITE_MAX_MMAP_SIZE>0
    /* If the file was just truncated to a size smaller than the currently
    ** mapped region, reduce the effective mapping size as well. SQLite will
    ** use read() and write() to access data beyond this point from now on.  
    */
    if( nByte<pFile->mmapSize ){
      pFile->mmapSize = nByte;
    }
#endif

    return SQLITE_OK;
  }
}

/*
** Determine the current size of a file in bytes
*/
static int unixFileSize(sqlite3_file *id, i64 *pSize){
  int rc;
  struct stat buf;
  assert( id );
  rc = osFstat(((unixFile*)id)->h, &buf);
  SimulateIOError( rc=1 );
  if( rc!=0 ){
    storeLastErrno((unixFile*)id, errno);
    return SQLITE_IOERR_FSTAT;
  }
  *pSize = buf.st_size;

  /* When opening a zero-size database, the findInodeInfo() procedure
  ** writes a single byte into that file in order to work around a bug
  ** in the OS-X msdos filesystem.  In order to avoid problems with upper
  ** layers, we need to report this file size as zero even though it is
  ** really 1.   Ticket #3260.
  */
  if( *pSize==1 ) *pSize = 0;


  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Handler for proxy-locking file-control verbs.  Defined below in the
** proxying locking division.
*/
static int proxyFileControl(sqlite3_file*,int,void*);
#endif

/* 
** This function is called to handle the SQLITE_FCNTL_SIZE_HINT 
** file-control operation.  Enlarge the database to nBytes in size
** (rounded up to the next chunk-size).  If the database is already
** nBytes or larger, this routine is a no-op.
*/
static int fcntlSizeHint(unixFile *pFile, i64 nByte){
  if( pFile->szChunk>0 ){
    i64 nSize;                    /* Required file size */
    struct stat buf;              /* Used to hold return values of fstat() */
   
    if( osFstat(pFile->h, &buf) ){
      return SQLITE_IOERR_FSTAT;
    }

    nSize = ((nByte+pFile->szChunk-1) / pFile->szChunk) * pFile->szChunk;
    if( nSize>(i64)buf.st_size ){

#if defined(HAVE_POSIX_FALLOCATE) && HAVE_POSIX_FALLOCATE
      /* The code below is handling the return value of osFallocate() 
      ** correctly. posix_fallocate() is defined to "returns zero on success, 
      ** or an error number on  failure". See the manpage for details. */
      int err;
      do{
        err = osFallocate(pFile->h, buf.st_size, nSize-buf.st_size);
      }while( err==EINTR );
      if( err ) return SQLITE_IOERR_WRITE;
#else
      /* If the OS does not have posix_fallocate(), fake it. Write a 
      ** single byte to the last byte in each block that falls entirely
      ** within the extended region. Then, if required, a single byte
      ** at offset (nSize-1), to set the size of the file correctly.
      ** This is a similar technique to that used by glibc on systems
      ** that do not have a real fallocate() call.
      */
      int nBlk = buf.st_blksize;  /* File-system block size */
      int nWrite = 0;             /* Number of bytes written by seekAndWrite */
      i64 iWrite;                 /* Next offset to write to */

      iWrite = ((buf.st_size + 2*nBlk - 1)/nBlk)*nBlk-1;
      assert( iWrite>=buf.st_size );
      assert( (iWrite/nBlk)==((buf.st_size+nBlk-1)/nBlk) );
      assert( ((iWrite+1)%nBlk)==0 );
      for(/*no-op*/; iWrite<nSize; iWrite+=nBlk ){
        nWrite = seekAndWrite(pFile, iWrite, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
      if( nWrite==0 || (nSize%nBlk) ){
        nWrite = seekAndWrite(pFile, nSize-1, "", 1);
        if( nWrite!=1 ) return SQLITE_IOERR_WRITE;
      }
#endif
    }
  }

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFile->mmapSizeMax>0 && nByte>pFile->mmapSize ){
    int rc;
    if( pFile->szChunk<=0 ){
      if( robust_ftruncate(pFile->h, nByte) ){
        storeLastErrno(pFile, errno);
        return unixLogError(SQLITE_IOERR_TRUNCATE, "ftruncate", pFile->zPath);
      }
    }

    rc = unixMapfile(pFile, nByte);
    return rc;
  }
#endif

  return SQLITE_OK;
}

/*
** If *pArg is initially negative then this is a query.  Set *pArg to
** 1 or 0 depending on whether or not bit mask of pFile->ctrlFlags is set.
**
** If *pArg is 0 or 1, then clear or set the mask bit of pFile->ctrlFlags.
*/
static void unixModeBit(unixFile *pFile, unsigned char mask, int *pArg){
  if( *pArg<0 ){
    *pArg = (pFile->ctrlFlags & mask)!=0;
  }else if( (*pArg)==0 ){
    pFile->ctrlFlags &= ~mask;
  }else{
    pFile->ctrlFlags |= mask;
  }
}

/* Forward declaration */
static int unixGetTempname(int nBuf, char *zBuf);

/*
** Information and control of an open file handle.
*/
static int unixFileControl(sqlite3_file *id, int op, void *pArg){
  unixFile *pFile = (unixFile*)id;
  switch( op ){
    case SQLITE_FCNTL_WAL_BLOCK: {
      /* pFile->ctrlFlags |= UNIXFILE_BLOCK; // Deferred feature */
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LOCKSTATE: {
      *(int*)pArg = pFile->eFileLock;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_LAST_ERRNO: {
      *(int*)pArg = pFile->lastErrno;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_CHUNK_SIZE: {
      pFile->szChunk = *(int *)pArg;
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SIZE_HINT: {
      int rc;
      SimulateIOErrorBenign(1);
      rc = fcntlSizeHint(pFile, *(i64 *)pArg);
      SimulateIOErrorBenign(0);
      return rc;
    }
    case SQLITE_FCNTL_PERSIST_WAL: {
      unixModeBit(pFile, UNIXFILE_PERSIST_WAL, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_POWERSAFE_OVERWRITE: {
      unixModeBit(pFile, UNIXFILE_PSOW, (int*)pArg);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_VFSNAME: {
      *(char**)pArg = sqlite3_mprintf("%s", pFile->pVfs->zName);
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_TEMPFILENAME: {
      char *zTFile = sqlite3_malloc64( pFile->pVfs->mxPathname );
      if( zTFile ){
        unixGetTempname(pFile->pVfs->mxPathname, zTFile);
        *(char**)pArg = zTFile;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_HAS_MOVED: {
      *(int*)pArg = fileHasMoved(pFile);
      return SQLITE_OK;
    }
#if SQLITE_MAX_MMAP_SIZE>0
    case SQLITE_FCNTL_MMAP_SIZE: {
      i64 newLimit = *(i64*)pArg;
      int rc = SQLITE_OK;
      if( newLimit>sqlite3GlobalConfig.mxMmap ){
        newLimit = sqlite3GlobalConfig.mxMmap;
      }
      *(i64*)pArg = pFile->mmapSizeMax;
      if( newLimit>=0 && newLimit!=pFile->mmapSizeMax && pFile->nFetchOut==0 ){
        pFile->mmapSizeMax = newLimit;
        if( pFile->mmapSize>0 ){
          unixUnmapfile(pFile);
          rc = unixMapfile(pFile, -1);
        }
      }
      return rc;
    }
#endif
#ifdef SQLITE_DEBUG
    /* The pager calls this method to signal that it has done
    ** a rollback and that the database is therefore unchanged and
    ** it hence it is OK for the transaction change counter to be
    ** unchanged.
    */
    case SQLITE_FCNTL_DB_UNCHANGED: {
      ((unixFile*)id)->dbUpdate = 0;
      return SQLITE_OK;
    }
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    case SQLITE_FCNTL_SET_LOCKPROXYFILE:
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      return proxyFileControl(id,op,pArg);
    }
#endif /* SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__) */
  }
  return SQLITE_NOTFOUND;
}

/*
** Return the sector size in bytes of the underlying block device for
** the specified file. This is almost always 512 bytes, but may be
** larger for some devices.
**
** SQLite code assumes this function cannot fail. It also assumes that
** if two files are created in the same file-system directory (i.e.
** a database and its journal file) that the sector size will be the
** same for both.
*/
#ifndef __QNXNTO__ 
static int unixSectorSize(sqlite3_file *NotUsed){
  UNUSED_PARAMETER(NotUsed);
  return SQLITE_DEFAULT_SECTOR_SIZE;
}
#endif

/*
** The following version of unixSectorSize() is optimized for QNX.
*/
#ifdef __QNXNTO__
#include <sys/dcmd_blk.h>
#include <sys/statvfs.h>
static int unixSectorSize(sqlite3_file *id){
  unixFile *pFile = (unixFile*)id;
  if( pFile->sectorSize == 0 ){
    struct statvfs fsInfo;
       
    /* Set defaults for non-supported filesystems */
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
    pFile->deviceCharacteristics = 0;
    if( fstatvfs(pFile->h, &fsInfo) == -1 ) {
      return pFile->sectorSize;
    }

    if( !strcmp(fsInfo.f_basetype, "tmp") ) {
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC4K |       /* All ram filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "etfs") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* etfs cluster size writes are atomic */
        (pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) |
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx6") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC |         /* All filesystem writes are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( !strcmp(fsInfo.f_basetype, "qnx4") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else if( strstr(fsInfo.f_basetype, "dos") ){
      pFile->sectorSize = fsInfo.f_bsize;
      pFile->deviceCharacteristics =
        /* full bitset of atomics from max sector size and smaller */
        ((pFile->sectorSize / 512 * SQLITE_IOCAP_ATOMIC512) << 1) - 2 |
        SQLITE_IOCAP_SEQUENTIAL |     /* The ram filesystem has no write behind
                                      ** so it is ordered */
        0;
    }else{
      pFile->deviceCharacteristics =
        SQLITE_IOCAP_ATOMIC512 |      /* blocks are atomic */
        SQLITE_IOCAP_SAFE_APPEND |    /* growing the file does not occur until
                                      ** the write succeeds */
        0;
    }
  }
  /* Last chance verification.  If the sector size isn't a multiple of 512
  ** then it isn't valid.*/
  if( pFile->sectorSize % 512 != 0 ){
    pFile->deviceCharacteristics = 0;
    pFile->sectorSize = SQLITE_DEFAULT_SECTOR_SIZE;
  }
  return pFile->sectorSize;
}
#endif /* __QNXNTO__ */

/*
** Return the device characteristics for the file.
**
** This VFS is set up to return SQLITE_IOCAP_POWERSAFE_OVERWRITE by default.
** However, that choice is controversial since technically the underlying
** file system does not always provide powersafe overwrites.  (In other
** words, after a power-loss event, parts of the file that were never
** written might end up being altered.)  However, non-PSOW behavior is very,
** very rare.  And asserting PSOW makes a large reduction in the amount
** of required I/O for journaling, since a lot of padding is eliminated.
**  Hence, while POWERSAFE_OVERWRITE is on by default, there is a file-control
** available to turn it off and URI query parameter available to turn it off.
*/
static int unixDeviceCharacteristics(sqlite3_file *id){
  unixFile *p = (unixFile*)id;
  int rc = 0;
#ifdef __QNXNTO__
  if( p->sectorSize==0 ) unixSectorSize(id);
  rc = p->deviceCharacteristics;
#endif
  if( p->ctrlFlags & UNIXFILE_PSOW ){
    rc |= SQLITE_IOCAP_POWERSAFE_OVERWRITE;
  }
  return rc;
}

#if !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0

/*
** Return the system page size.
**
** This function should not be called directly by other code in this file. 
** Instead, it should be called via macro osGetpagesize().
*/
static int unixGetpagesize(void){
#if OS_VXWORKS
  return 1024;
#elif defined(_BSD_SOURCE)
  return getpagesize();
#else
  return (int)sysconf(_SC_PAGESIZE);
#endif
}

#endif /* !defined(SQLITE_OMIT_WAL) || SQLITE_MAX_MMAP_SIZE>0 */

#ifndef SQLITE_OMIT_WAL

/*
** Object used to represent an shared memory buffer.  
**
** When multiple threads all reference the same wal-index, each thread
** has its own unixShm object, but they all point to a single instance
** of this unixShmNode object.  In other words, each wal-index is opened
** only once per process.
**
** Each unixShmNode object is connected to a single unixInodeInfo object.
** We could coalesce this object into unixInodeInfo, but that would mean
** every open file that does not use shared memory (in other words, most
** open files) would have to carry around this extra information.  So
** the unixInodeInfo object contains a pointer to this unixShmNode object
** and the unixShmNode object is created only when needed.
**
** unixMutexHeld() must be true when creating or destroying
** this object or while reading or writing the following fields:
**
**      nRef
**
** The following fields are read-only after the object is created:
** 
**      fid
**      zFilename
**
** Either unixShmNode.mutex must be held or unixShmNode.nRef==0 and
** unixMutexHeld() is true when reading or writing any other field
** in this structure.
*/
struct unixShmNode {
  unixInodeInfo *pInode;     /* unixInodeInfo that owns this SHM node */
  sqlite3_mutex *mutex;      /* Mutex to access this object */
  char *zFilename;           /* Name of the mmapped file */
  int h;                     /* Open file descriptor */
  int szRegion;              /* Size of shared-memory regions */
  u16 nRegion;               /* Size of array apRegion */
  u8 isReadonly;             /* True if read-only */
  char **apRegion;           /* Array of mapped shared-memory regions */
  int nRef;                  /* Number of unixShm objects pointing to this */
  unixShm *pFirst;           /* All unixShm objects pointing to this */
#ifdef SQLITE_DEBUG
  u8 exclMask;               /* Mask of exclusive locks held */
  u8 sharedMask;             /* Mask of shared locks held */
  u8 nextShmId;              /* Next available unixShm.id value */
#endif
};

/*
** Structure used internally by this VFS to record the state of an
** open shared memory connection.
**
** The following fields are initialized when this object is created and
** are read-only thereafter:
**
**    unixShm.pFile
**    unixShm.id
**
** All other fields are read/write.  The unixShm.pFile->mutex must be held
** while accessing any read/write fields.
*/
struct unixShm {
  unixShmNode *pShmNode;     /* The underlying unixShmNode object */
  unixShm *pNext;            /* Next unixShm with the same unixShmNode */
  u8 hasMutex;               /* True if holding the unixShmNode mutex */
  u8 id;                     /* Id of this connection within its unixShmNode */
  u16 sharedMask;            /* Mask of shared locks held */
  u16 exclMask;              /* Mask of exclusive locks held */
};

/*
** Constants used for locking
*/
#define UNIX_SHM_BASE   ((22+SQLITE_SHM_NLOCK)*4)         /* first lock byte */
#define UNIX_SHM_DMS    (UNIX_SHM_BASE+SQLITE_SHM_NLOCK)  /* deadman switch */

/*
** Apply posix advisory locks for all bytes from ofst through ofst+n-1.
**
** Locks block if the mask is exactly UNIX_SHM_C and are non-blocking
** otherwise.
*/
static int unixShmSystemLock(
  unixFile *pFile,       /* Open connection to the WAL file */
  int lockType,          /* F_UNLCK, F_RDLCK, or F_WRLCK */
  int ofst,              /* First byte of the locking range */
  int n                  /* Number of bytes to lock */
){
  unixShmNode *pShmNode; /* Apply locks to this open shared-memory segment */
  struct flock f;        /* The posix advisory locking structure */
  int rc = SQLITE_OK;    /* Result code form fcntl() */

  /* Access to the unixShmNode object is serialized by the caller */
  pShmNode = pFile->pInode->pShmNode;
  assert( sqlite3_mutex_held(pShmNode->mutex) || pShmNode->nRef==0 );

  /* Shared locks never span more than one byte */
  assert( n==1 || lockType!=F_RDLCK );

  /* Locks are within range */
  assert( n>=1 && n<SQLITE_SHM_NLOCK );

  if( pShmNode->h>=0 ){
    int lkType;
    /* Initialize the locking parameters */
    memset(&f, 0, sizeof(f));
    f.l_type = lockType;
    f.l_whence = SEEK_SET;
    f.l_start = ofst;
    f.l_len = n;

    lkType = (pFile->ctrlFlags & UNIXFILE_BLOCK)!=0 ? F_SETLKW : F_SETLK;
    rc = osFcntl(pShmNode->h, lkType, &f);
    rc = (rc!=(-1)) ? SQLITE_OK : SQLITE_BUSY;
    pFile->ctrlFlags &= ~UNIXFILE_BLOCK;
  }

  /* Update the global lock state and do debug tracing */
#ifdef SQLITE_DEBUG
  { u16 mask;
  OSTRACE(("SHM-LOCK "));
  mask = ofst>31 ? 0xffff : (1<<(ofst+n)) - (1<<ofst);
  if( rc==SQLITE_OK ){
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask &= ~mask;
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock %d ok", ofst));
      pShmNode->exclMask &= ~mask;
      pShmNode->sharedMask |= mask;
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d ok", ofst));
      pShmNode->exclMask |= mask;
      pShmNode->sharedMask &= ~mask;
    }
  }else{
    if( lockType==F_UNLCK ){
      OSTRACE(("unlock %d failed", ofst));
    }else if( lockType==F_RDLCK ){
      OSTRACE(("read-lock failed"));
    }else{
      assert( lockType==F_WRLCK );
      OSTRACE(("write-lock %d failed", ofst));
    }
  }
  OSTRACE((" - afterwards %03x,%03x\n",
           pShmNode->sharedMask, pShmNode->exclMask));
  }
#endif

  return rc;        
}

/*
** Return the minimum number of 32KB shm regions that should be mapped at
** a time, assuming that each mapping must be an integer multiple of the
** current system page-size.
**
** Usually, this is 1. The exception seems to be systems that are configured
** to use 64KB pages - in this case each mapping must cover at least two
** shm regions.
*/
static int unixShmRegionPerMap(void){
  int shmsz = 32*1024;            /* SHM region size */
  int pgsz = osGetpagesize();   /* System page size */
  assert( ((pgsz-1)&pgsz)==0 );   /* Page size must be a power of 2 */
  if( pgsz<shmsz ) return 1;
  return pgsz/shmsz;
}

/*
** Purge the unixShmNodeList list of all entries with unixShmNode.nRef==0.
**
** This is not a VFS shared-memory method; it is a utility function called
** by VFS shared-memory methods.
*/
static void unixShmPurge(unixFile *pFd){
  unixShmNode *p = pFd->pInode->pShmNode;
  assert( unixMutexHeld() );
  if( p && p->nRef==0 ){
    int nShmPerMap = unixShmRegionPerMap();
    int i;
    assert( p->pInode==pFd->pInode );
    sqlite3_mutex_free(p->mutex);
    for(i=0; i<p->nRegion; i+=nShmPerMap){
      if( p->h>=0 ){
        osMunmap(p->apRegion[i], p->szRegion);
      }else{
        sqlite3_free(p->apRegion[i]);
      }
    }
    sqlite3_free(p->apRegion);
    if( p->h>=0 ){
      robust_close(pFd, p->h, __LINE__);
      p->h = -1;
    }
    p->pInode->pShmNode = 0;
    sqlite3_free(p);
  }
}

/*
** Open a shared-memory area associated with open database file pDbFd.  
** This particular implementation uses mmapped files.
**
** The file used to implement shared-memory is in the same directory
** as the open database file and has the same name as the open database
** file with the "-shm" suffix added.  For example, if the database file
** is "/home/user1/config.db" then the file that is created and mmapped
** for shared memory will be called "/home/user1/config.db-shm".  
**
** Another approach to is to use files in /dev/shm or /dev/tmp or an
** some other tmpfs mount. But if a file in a different directory
** from the database file is used, then differing access permissions
** or a chroot() might cause two different processes on the same
** database to end up using different files for shared memory - 
** meaning that their memory would not really be shared - resulting
** in database corruption.  Nevertheless, this tmpfs file usage
** can be enabled at compile-time using -DSQLITE_SHM_DIRECTORY="/dev/shm"
** or the equivalent.  The use of the SQLITE_SHM_DIRECTORY compile-time
** option results in an incompatible build of SQLite;  builds of SQLite
** that with differing SQLITE_SHM_DIRECTORY settings attempt to use the
** same database file at the same time, database corruption will likely
** result. The SQLITE_SHM_DIRECTORY compile-time option is considered
** "unsupported" and may go away in a future SQLite release.
**
** When opening a new shared-memory file, if no other instances of that
** file are currently open, in this process or in other processes, then
** the file must be truncated to zero length or have its header cleared.
**
** If the original database file (pDbFd) is using the "unix-excl" VFS
** that means that an exclusive lock is held on the database file and
** that no other processes are able to read or write the database.  In
** that case, we do not really need shared memory.  No shared memory
** file is created.  The shared memory will be simulated with heap memory.
*/
static int unixOpenSharedMemory(unixFile *pDbFd){
  struct unixShm *p = 0;          /* The connection to be opened */
  struct unixShmNode *pShmNode;   /* The underlying mmapped file */
  int rc;                         /* Result code */
  unixInodeInfo *pInode;          /* The inode of fd */
  char *zShmFilename;             /* Name of the file used for SHM */
  int nShmFilename;               /* Size of the SHM filename in bytes */

  /* Allocate space for the new unixShm object. */
  p = sqlite3_malloc64( sizeof(*p) );
  if( p==0 ) return SQLITE_NOMEM;
  memset(p, 0, sizeof(*p));
  assert( pDbFd->pShm==0 );

  /* Check to see if a unixShmNode object already exists. Reuse an existing
  ** one if present. Create a new one if necessary.
  */
  unixEnterMutex();
  pInode = pDbFd->pInode;
  pShmNode = pInode->pShmNode;
  if( pShmNode==0 ){
    struct stat sStat;                 /* fstat() info for database file */
#ifndef SQLITE_SHM_DIRECTORY
    const char *zBasePath = pDbFd->zPath;
#endif

    /* Call fstat() to figure out the permissions on the database file. If
    ** a new *-shm file is created, an attempt will be made to create it
    ** with the same permissions.
    */
    if( osFstat(pDbFd->h, &sStat) && pInode->bProcessLock==0 ){
      rc = SQLITE_IOERR_FSTAT;
      goto shm_open_err;
    }

#ifdef SQLITE_SHM_DIRECTORY
    nShmFilename = sizeof(SQLITE_SHM_DIRECTORY) + 31;
#else
    nShmFilename = 6 + (int)strlen(zBasePath);
#endif
    pShmNode = sqlite3_malloc64( sizeof(*pShmNode) + nShmFilename );
    if( pShmNode==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }
    memset(pShmNode, 0, sizeof(*pShmNode)+nShmFilename);
    zShmFilename = pShmNode->zFilename = (char*)&pShmNode[1];
#ifdef SQLITE_SHM_DIRECTORY
    sqlite3_snprintf(nShmFilename, zShmFilename, 
                     SQLITE_SHM_DIRECTORY "/sqlite-shm-%x-%x",
                     (u32)sStat.st_ino, (u32)sStat.st_dev);
#else
    sqlite3_snprintf(nShmFilename, zShmFilename, "%s-shm", zBasePath);
    sqlite3FileSuffix3(pDbFd->zPath, zShmFilename);
#endif
    pShmNode->h = -1;
    pDbFd->pInode->pShmNode = pShmNode;
    pShmNode->pInode = pDbFd->pInode;
    pShmNode->mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
    if( pShmNode->mutex==0 ){
      rc = SQLITE_NOMEM;
      goto shm_open_err;
    }

    if( pInode->bProcessLock==0 ){
      int openFlags = O_RDWR | O_CREAT;
      if( sqlite3_uri_boolean(pDbFd->zPath, "readonly_shm", 0) ){
        openFlags = O_RDONLY;
        pShmNode->isReadonly = 1;
      }
      pShmNode->h = robust_open(zShmFilename, openFlags, (sStat.st_mode&0777));
      if( pShmNode->h<0 ){
        rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zShmFilename);
        goto shm_open_err;
      }

      /* If this process is running as root, make sure that the SHM file
      ** is owned by the same user that owns the original database.  Otherwise,
      ** the original owner will not be able to connect.
      */
      osFchown(pShmNode->h, sStat.st_uid, sStat.st_gid);
  
      /* Check to see if another process is holding the dead-man switch.
      ** If not, truncate the file to zero length. 
      */
      rc = SQLITE_OK;
      if( unixShmSystemLock(pDbFd, F_WRLCK, UNIX_SHM_DMS, 1)==SQLITE_OK ){
        if( robust_ftruncate(pShmNode->h, 0) ){
          rc = unixLogError(SQLITE_IOERR_SHMOPEN, "ftruncate", zShmFilename);
        }
      }
      if( rc==SQLITE_OK ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, UNIX_SHM_DMS, 1);
      }
      if( rc ) goto shm_open_err;
    }
  }

  /* Make the new connection a child of the unixShmNode */
  p->pShmNode = pShmNode;
#ifdef SQLITE_DEBUG
  p->id = pShmNode->nextShmId++;
#endif
  pShmNode->nRef++;
  pDbFd->pShm = p;
  unixLeaveMutex();

  /* The reference count on pShmNode has already been incremented under
  ** the cover of the unixEnterMutex() mutex and the pointer from the
  ** new (struct unixShm) object to the pShmNode has been set. All that is
  ** left to do is to link the new object into the linked list starting
  ** at pShmNode->pFirst. This must be done while holding the pShmNode->mutex 
  ** mutex.
  */
  sqlite3_mutex_enter(pShmNode->mutex);
  p->pNext = pShmNode->pFirst;
  pShmNode->pFirst = p;
  sqlite3_mutex_leave(pShmNode->mutex);
  return SQLITE_OK;

  /* Jump here on any error */
shm_open_err:
  unixShmPurge(pDbFd);       /* This call frees pShmNode if required */
  sqlite3_free(p);
  unixLeaveMutex();
  return rc;
}

/*
** This function is called to obtain a pointer to region iRegion of the 
** shared-memory associated with the database file fd. Shared-memory regions 
** are numbered starting from zero. Each shared-memory region is szRegion 
** bytes in size.
**
** If an error occurs, an error code is returned and *pp is set to NULL.
**
** Otherwise, if the bExtend parameter is 0 and the requested shared-memory
** region has not been allocated (by any client, including one running in a
** separate process), then *pp is set to NULL and SQLITE_OK returned. If 
** bExtend is non-zero and the requested shared-memory region has not yet 
** been allocated, it is allocated by this function.
**
** If the shared-memory region has already been allocated or is allocated by
** this call as described above, then it is mapped into this processes 
** address space (if it is not already), *pp is set to point to the mapped 
** memory and SQLITE_OK returned.
*/
static int unixShmMap(
  sqlite3_file *fd,               /* Handle open on database file */
  int iRegion,                    /* Region to retrieve */
  int szRegion,                   /* Size of regions */
  int bExtend,                    /* True to extend file if necessary */
  void volatile **pp              /* OUT: Mapped memory */
){
  unixFile *pDbFd = (unixFile*)fd;
  unixShm *p;
  unixShmNode *pShmNode;
  int rc = SQLITE_OK;
  int nShmPerMap = unixShmRegionPerMap();
  int nReqRegion;

  /* If the shared-memory file has not yet been opened, open it now. */
  if( pDbFd->pShm==0 ){
    rc = unixOpenSharedMemory(pDbFd);
    if( rc!=SQLITE_OK ) return rc;
  }

  p = pDbFd->pShm;
  pShmNode = p->pShmNode;
  sqlite3_mutex_enter(pShmNode->mutex);
  assert( szRegion==pShmNode->szRegion || pShmNode->nRegion==0 );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  /* Minimum number of regions required to be mapped. */
  nReqRegion = ((iRegion+nShmPerMap) / nShmPerMap) * nShmPerMap;

  if( pShmNode->nRegion<nReqRegion ){
    char **apNew;                      /* New apRegion[] array */
    int nByte = nReqRegion*szRegion;   /* Minimum required file size */
    struct stat sStat;                 /* Used by fstat() */

    pShmNode->szRegion = szRegion;

    if( pShmNode->h>=0 ){
      /* The requested region is not mapped into this processes address space.
      ** Check to see if it has been allocated (i.e. if the wal-index file is
      ** large enough to contain the requested region).
      */
      if( osFstat(pShmNode->h, &sStat) ){
        rc = SQLITE_IOERR_SHMSIZE;
        goto shmpage_out;
      }
  
      if( sStat.st_size<nByte ){
        /* The requested memory region does not exist. If bExtend is set to
        ** false, exit early. *pp will be set to NULL and SQLITE_OK returned.
        */
        if( !bExtend ){
          goto shmpage_out;
        }

        /* Alternatively, if bExtend is true, extend the file. Do this by
        ** writing a single byte to the end of each (OS) page being
        ** allocated or extended. Technically, we need only write to the
        ** last page in order to extend the file. But writing to all new
        ** pages forces the OS to allocate them immediately, which reduces
        ** the chances of SIGBUS while accessing the mapped region later on.
        */
        else{
          static const int pgsz = 4096;
          int iPg;

          /* Write to the last byte of each newly allocated or extended page */
          assert( (nByte % pgsz)==0 );
          for(iPg=(sStat.st_size/pgsz); iPg<(nByte/pgsz); iPg++){
            if( seekAndWriteFd(pShmNode->h, iPg*pgsz + pgsz-1, "", 1, 0)!=1 ){
              const char *zFile = pShmNode->zFilename;
              rc = unixLogError(SQLITE_IOERR_SHMSIZE, "write", zFile);
              goto shmpage_out;
            }
          }
        }
      }
    }

    /* Map the requested memory region into this processes address space. */
    apNew = (char **)sqlite3_realloc(
        pShmNode->apRegion, nReqRegion*sizeof(char *)
    );
    if( !apNew ){
      rc = SQLITE_IOERR_NOMEM;
      goto shmpage_out;
    }
    pShmNode->apRegion = apNew;
    while( pShmNode->nRegion<nReqRegion ){
      int nMap = szRegion*nShmPerMap;
      int i;
      void *pMem;
      if( pShmNode->h>=0 ){
        pMem = osMmap(0, nMap,
            pShmNode->isReadonly ? PROT_READ : PROT_READ|PROT_WRITE, 
            MAP_SHARED, pShmNode->h, szRegion*(i64)pShmNode->nRegion
        );
        if( pMem==MAP_FAILED ){
          rc = unixLogError(SQLITE_IOERR_SHMMAP, "mmap", pShmNode->zFilename);
          goto shmpage_out;
        }
      }else{
        pMem = sqlite3_malloc64(szRegion);
        if( pMem==0 ){
          rc = SQLITE_NOMEM;
          goto shmpage_out;
        }
        memset(pMem, 0, szRegion);
      }

      for(i=0; i<nShmPerMap; i++){
        pShmNode->apRegion[pShmNode->nRegion+i] = &((char*)pMem)[szRegion*i];
      }
      pShmNode->nRegion += nShmPerMap;
    }
  }

shmpage_out:
  if( pShmNode->nRegion>iRegion ){
    *pp = pShmNode->apRegion[iRegion];
  }else{
    *pp = 0;
  }
  if( pShmNode->isReadonly && rc==SQLITE_OK ) rc = SQLITE_READONLY;
  sqlite3_mutex_leave(pShmNode->mutex);
  return rc;
}

/*
** Change the lock state for a shared-memory segment.
**
** Note that the relationship between SHAREd and EXCLUSIVE locks is a little
** different here than in posix.  In xShmLock(), one can go from unlocked
** to shared and back or from unlocked to exclusive and back.  But one may
** not go from shared to exclusive or from exclusive to shared.
*/
static int unixShmLock(
  sqlite3_file *fd,          /* Database file holding the shared memory */
  int ofst,                  /* First lock to acquire or release */
  int n,                     /* Number of locks to acquire or release */
  int flags                  /* What to do with the lock */
){
  unixFile *pDbFd = (unixFile*)fd;      /* Connection holding shared memory */
  unixShm *p = pDbFd->pShm;             /* The shared memory being locked */
  unixShm *pX;                          /* For looping over all siblings */
  unixShmNode *pShmNode = p->pShmNode;  /* The underlying file iNode */
  int rc = SQLITE_OK;                   /* Result code */
  u16 mask;                             /* Mask of locks to take or release */

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );
  assert( ofst>=0 && ofst+n<=SQLITE_SHM_NLOCK );
  assert( n>=1 );
  assert( flags==(SQLITE_SHM_LOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_LOCK | SQLITE_SHM_EXCLUSIVE)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_SHARED)
       || flags==(SQLITE_SHM_UNLOCK | SQLITE_SHM_EXCLUSIVE) );
  assert( n==1 || (flags & SQLITE_SHM_EXCLUSIVE)!=0 );
  assert( pShmNode->h>=0 || pDbFd->pInode->bProcessLock==1 );
  assert( pShmNode->h<0 || pDbFd->pInode->bProcessLock==0 );

  mask = (1<<(ofst+n)) - (1<<ofst);
  assert( n>1 || mask==(1<<ofst) );
  sqlite3_mutex_enter(pShmNode->mutex);
  if( flags & SQLITE_SHM_UNLOCK ){
    u16 allMask = 0; /* Mask of locks held by siblings */

    /* See if any siblings hold this same lock */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( pX==p ) continue;
      assert( (pX->exclMask & (p->exclMask|p->sharedMask))==0 );
      allMask |= pX->sharedMask;
    }

    /* Unlock the system-level locks */
    if( (mask & allMask)==0 ){
      rc = unixShmSystemLock(pDbFd, F_UNLCK, ofst+UNIX_SHM_BASE, n);
    }else{
      rc = SQLITE_OK;
    }

    /* Undo the local locks */
    if( rc==SQLITE_OK ){
      p->exclMask &= ~mask;
      p->sharedMask &= ~mask;
    } 
  }else if( flags & SQLITE_SHM_SHARED ){
    u16 allShared = 0;  /* Union of locks held by connections other than "p" */

    /* Find out which shared locks are already held by sibling connections.
    ** If any sibling already holds an exclusive lock, go ahead and return
    ** SQLITE_BUSY.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
      allShared |= pX->sharedMask;
    }

    /* Get shared locks at the system level, if necessary */
    if( rc==SQLITE_OK ){
      if( (allShared & mask)==0 ){
        rc = unixShmSystemLock(pDbFd, F_RDLCK, ofst+UNIX_SHM_BASE, n);
      }else{
        rc = SQLITE_OK;
      }
    }

    /* Get the local shared locks */
    if( rc==SQLITE_OK ){
      p->sharedMask |= mask;
    }
  }else{
    /* Make sure no sibling connections hold locks that will block this
    ** lock.  If any do, return SQLITE_BUSY right away.
    */
    for(pX=pShmNode->pFirst; pX; pX=pX->pNext){
      if( (pX->exclMask & mask)!=0 || (pX->sharedMask & mask)!=0 ){
        rc = SQLITE_BUSY;
        break;
      }
    }
  
    /* Get the exclusive locks at the system level.  Then if successful
    ** also mark the local connection as being locked.
    */
    if( rc==SQLITE_OK ){
      rc = unixShmSystemLock(pDbFd, F_WRLCK, ofst+UNIX_SHM_BASE, n);
      if( rc==SQLITE_OK ){
        assert( (p->sharedMask & mask)==0 );
        p->exclMask |= mask;
      }
    }
  }
  sqlite3_mutex_leave(pShmNode->mutex);
  OSTRACE(("SHM-LOCK shmid-%d, pid-%d got %03x,%03x\n",
           p->id, osGetpid(0), p->sharedMask, p->exclMask));
  return rc;
}

/*
** Implement a memory barrier or memory fence on shared memory.  
**
** All loads and stores begun before the barrier must complete before
** any load or store begun after the barrier.
*/
static void unixShmBarrier(
  sqlite3_file *fd                /* Database file holding the shared memory */
){
  UNUSED_PARAMETER(fd);
  sqlite3MemoryBarrier();         /* compiler-defined memory barrier */
  unixEnterMutex();               /* Also mutex, for redundancy */
  unixLeaveMutex();
}

/*
** Close a connection to shared-memory.  Delete the underlying 
** storage if deleteFlag is true.
**
** If there is no shared memory associated with the connection then this
** routine is a harmless no-op.
*/
static int unixShmUnmap(
  sqlite3_file *fd,               /* The underlying database file */
  int deleteFlag                  /* Delete shared-memory if true */
){
  unixShm *p;                     /* The connection to be closed */
  unixShmNode *pShmNode;          /* The underlying shared-memory file */
  unixShm **pp;                   /* For looping over sibling connections */
  unixFile *pDbFd;                /* The underlying database file */

  pDbFd = (unixFile*)fd;
  p = pDbFd->pShm;
  if( p==0 ) return SQLITE_OK;
  pShmNode = p->pShmNode;

  assert( pShmNode==pDbFd->pInode->pShmNode );
  assert( pShmNode->pInode==pDbFd->pInode );

  /* Remove connection p from the set of connections associated
  ** with pShmNode */
  sqlite3_mutex_enter(pShmNode->mutex);
  for(pp=&pShmNode->pFirst; (*pp)!=p; pp = &(*pp)->pNext){}
  *pp = p->pNext;

  /* Free the connection p */
  sqlite3_free(p);
  pDbFd->pShm = 0;
  sqlite3_mutex_leave(pShmNode->mutex);

  /* If pShmNode->nRef has reached 0, then close the underlying
  ** shared-memory file, too */
  unixEnterMutex();
  assert( pShmNode->nRef>0 );
  pShmNode->nRef--;
  if( pShmNode->nRef==0 ){
    if( deleteFlag && pShmNode->h>=0 ){
      osUnlink(pShmNode->zFilename);
    }
    unixShmPurge(pDbFd);
  }
  unixLeaveMutex();

  return SQLITE_OK;
}


#else
# define unixShmMap     0
# define unixShmLock    0
# define unixShmBarrier 0
# define unixShmUnmap   0
#endif /* #ifndef SQLITE_OMIT_WAL */

#if SQLITE_MAX_MMAP_SIZE>0
/*
** If it is currently memory mapped, unmap file pFd.
*/
static void unixUnmapfile(unixFile *pFd){
  assert( pFd->nFetchOut==0 );
  if( pFd->pMapRegion ){
    osMunmap(pFd->pMapRegion, pFd->mmapSizeActual);
    pFd->pMapRegion = 0;
    pFd->mmapSize = 0;
    pFd->mmapSizeActual = 0;
  }
}

/*
** Attempt to set the size of the memory mapping maintained by file 
** descriptor pFd to nNew bytes. Any existing mapping is discarded.
**
** If successful, this function sets the following variables:
**
**       unixFile.pMapRegion
**       unixFile.mmapSize
**       unixFile.mmapSizeActual
**
** If unsuccessful, an error message is logged via sqlite3_log() and
** the three variables above are zeroed. In this case SQLite should
** continue accessing the database using the xRead() and xWrite()
** methods.
*/
static void unixRemapfile(
  unixFile *pFd,                  /* File descriptor object */
  i64 nNew                        /* Required mapping size */
){
  const char *zErr = "mmap";
  int h = pFd->h;                      /* File descriptor open on db file */
  u8 *pOrig = (u8 *)pFd->pMapRegion;   /* Pointer to current file mapping */
  i64 nOrig = pFd->mmapSizeActual;     /* Size of pOrig region in bytes */
  u8 *pNew = 0;                        /* Location of new mapping */
  int flags = PROT_READ;               /* Flags to pass to mmap() */

  assert( pFd->nFetchOut==0 );
  assert( nNew>pFd->mmapSize );
  assert( nNew<=pFd->mmapSizeMax );
  assert( nNew>0 );
  assert( pFd->mmapSizeActual>=pFd->mmapSize );
  assert( MAP_FAILED!=0 );

  if( (pFd->ctrlFlags & UNIXFILE_RDONLY)==0 ) flags |= PROT_WRITE;

  if( pOrig ){
#if HAVE_MREMAP
    i64 nReuse = pFd->mmapSize;
#else
    const int szSyspage = osGetpagesize();
    i64 nReuse = (pFd->mmapSize & ~(szSyspage-1));
#endif
    u8 *pReq = &pOrig[nReuse];

    /* Unmap any pages of the existing mapping that cannot be reused. */
    if( nReuse!=nOrig ){
      osMunmap(pReq, nOrig-nReuse);
    }

#if HAVE_MREMAP
    pNew = osMremap(pOrig, nReuse, nNew, MREMAP_MAYMOVE);
    zErr = "mremap";
#else
    pNew = osMmap(pReq, nNew-nReuse, flags, MAP_SHARED, h, nReuse);
    if( pNew!=MAP_FAILED ){
      if( pNew!=pReq ){
        osMunmap(pNew, nNew - nReuse);
        pNew = 0;
      }else{
        pNew = pOrig;
      }
    }
#endif

    /* The attempt to extend the existing mapping failed. Free it. */
    if( pNew==MAP_FAILED || pNew==0 ){
      osMunmap(pOrig, nReuse);
    }
  }

  /* If pNew is still NULL, try to create an entirely new mapping. */
  if( pNew==0 ){
    pNew = osMmap(0, nNew, flags, MAP_SHARED, h, 0);
  }

  if( pNew==MAP_FAILED ){
    pNew = 0;
    nNew = 0;
    unixLogError(SQLITE_OK, zErr, pFd->zPath);

    /* If the mmap() above failed, assume that all subsequent mmap() calls
    ** will probably fail too. Fall back to using xRead/xWrite exclusively
    ** in this case.  */
    pFd->mmapSizeMax = 0;
  }
  pFd->pMapRegion = (void *)pNew;
  pFd->mmapSize = pFd->mmapSizeActual = nNew;
}

/*
** Memory map or remap the file opened by file-descriptor pFd (if the file
** is already mapped, the existing mapping is replaced by the new). Or, if 
** there already exists a mapping for this file, and there are still 
** outstanding xFetch() references to it, this function is a no-op.
**
** If parameter nByte is non-negative, then it is the requested size of 
** the mapping to create. Otherwise, if nByte is less than zero, then the 
** requested size is the size of the file on disk. The actual size of the
** created mapping is either the requested size or the value configured 
** using SQLITE_FCNTL_MMAP_LIMIT, whichever is smaller.
**
** SQLITE_OK is returned if no error occurs (even if the mapping is not
** recreated as a result of outstanding references) or an SQLite error
** code otherwise.
*/
static int unixMapfile(unixFile *pFd, i64 nByte){
  i64 nMap = nByte;
  int rc;

  assert( nMap>=0 || pFd->nFetchOut==0 );
  if( pFd->nFetchOut>0 ) return SQLITE_OK;

  if( nMap<0 ){
    struct stat statbuf;          /* Low-level file information */
    rc = osFstat(pFd->h, &statbuf);
    if( rc!=SQLITE_OK ){
      return SQLITE_IOERR_FSTAT;
    }
    nMap = statbuf.st_size;
  }
  if( nMap>pFd->mmapSizeMax ){
    nMap = pFd->mmapSizeMax;
  }

  if( nMap!=pFd->mmapSize ){
    if( nMap>0 ){
      unixRemapfile(pFd, nMap);
    }else{
      unixUnmapfile(pFd);
    }
  }

  return SQLITE_OK;
}
#endif /* SQLITE_MAX_MMAP_SIZE>0 */

/*
** If possible, return a pointer to a mapping of file fd starting at offset
** iOff. The mapping must be valid for at least nAmt bytes.
**
** If such a pointer can be obtained, store it in *pp and return SQLITE_OK.
** Or, if one cannot but no error occurs, set *pp to 0 and return SQLITE_OK.
** Finally, if an error does occur, return an SQLite error code. The final
** value of *pp is undefined in this case.
**
** If this function does return a pointer, the caller must eventually 
** release the reference by calling unixUnfetch().
*/
static int unixFetch(sqlite3_file *fd, i64 iOff, int nAmt, void **pp){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
#endif
  *pp = 0;

#if SQLITE_MAX_MMAP_SIZE>0
  if( pFd->mmapSizeMax>0 ){
    if( pFd->pMapRegion==0 ){
      int rc = unixMapfile(pFd, -1);
      if( rc!=SQLITE_OK ) return rc;
    }
    if( pFd->mmapSize >= iOff+nAmt ){
      *pp = &((u8 *)pFd->pMapRegion)[iOff];
      pFd->nFetchOut++;
    }
  }
#endif
  return SQLITE_OK;
}

/*
** If the third argument is non-NULL, then this function releases a 
** reference obtained by an earlier call to unixFetch(). The second
** argument passed to this function must be the same as the corresponding
** argument that was passed to the unixFetch() invocation. 
**
** Or, if the third argument is NULL, then this function is being called 
** to inform the VFS layer that, according to POSIX, any existing mapping 
** may now be invalid and should be unmapped.
*/
static int unixUnfetch(sqlite3_file *fd, i64 iOff, void *p){
#if SQLITE_MAX_MMAP_SIZE>0
  unixFile *pFd = (unixFile *)fd;   /* The underlying database file */
  UNUSED_PARAMETER(iOff);

  /* If p==0 (unmap the entire file) then there must be no outstanding 
  ** xFetch references. Or, if p!=0 (meaning it is an xFetch reference),
  ** then there must be at least one outstanding.  */
  assert( (p==0)==(pFd->nFetchOut==0) );

  /* If p!=0, it must match the iOff value. */
  assert( p==0 || p==&((u8 *)pFd->pMapRegion)[iOff] );

  if( p ){
    pFd->nFetchOut--;
  }else{
    unixUnmapfile(pFd);
  }

  assert( pFd->nFetchOut>=0 );
#else
  UNUSED_PARAMETER(fd);
  UNUSED_PARAMETER(p);
  UNUSED_PARAMETER(iOff);
#endif
  return SQLITE_OK;
}

/*
** Here ends the implementation of all sqlite3_file methods.
**
********************** End sqlite3_file Methods *******************************
******************************************************************************/

/*
** This division contains definitions of sqlite3_io_methods objects that
** implement various file locking strategies.  It also contains definitions
** of "finder" functions.  A finder-function is used to locate the appropriate
** sqlite3_io_methods object for a particular database file.  The pAppData
** field of the sqlite3_vfs VFS objects are initialized to be pointers to
** the correct finder-function for that VFS.
**
** Most finder functions return a pointer to a fixed sqlite3_io_methods
** object.  The only interesting finder-function is autolockIoFinder, which
** looks at the filesystem type and tries to guess the best locking
** strategy from that.
**
** For finder-function F, two objects are created:
**
**    (1) The real finder-function named "FImpt()".
**
**    (2) A constant pointer to this function named just "F".
**
**
** A pointer to the F pointer is used as the pAppData value for VFS
** objects.  We have to do this instead of letting pAppData point
** directly at the finder-function since C90 rules prevent a void*
** from be cast into a function pointer.
**
**
** Each instance of this macro generates two objects:
**
**   *  A constant sqlite3_io_methods object call METHOD that has locking
**      methods CLOSE, LOCK, UNLOCK, CKRESLOCK.
**
**   *  An I/O method finder function called FINDER that returns a pointer
**      to the METHOD object in the previous bullet.
*/
#define IOMETHODS(FINDER,METHOD,VERSION,CLOSE,LOCK,UNLOCK,CKLOCK,SHMMAP)     \
static const sqlite3_io_methods METHOD = {                                   \
   VERSION,                    /* iVersion */                                \
   CLOSE,                      /* xClose */                                  \
   unixRead,                   /* xRead */                                   \
   unixWrite,                  /* xWrite */                                  \
   unixTruncate,               /* xTruncate */                               \
   unixSync,                   /* xSync */                                   \
   unixFileSize,               /* xFileSize */                               \
   LOCK,                       /* xLock */                                   \
   UNLOCK,                     /* xUnlock */                                 \
   CKLOCK,                     /* xCheckReservedLock */                      \
   unixFileControl,            /* xFileControl */                            \
   unixSectorSize,             /* xSectorSize */                             \
   unixDeviceCharacteristics,  /* xDeviceCapabilities */                     \
   SHMMAP,                     /* xShmMap */                                 \
   unixShmLock,                /* xShmLock */                                \
   unixShmBarrier,             /* xShmBarrier */                             \
   unixShmUnmap,               /* xShmUnmap */                               \
   unixFetch,                  /* xFetch */                                  \
   unixUnfetch,                /* xUnfetch */                                \
};                                                                           \
static const sqlite3_io_methods *FINDER##Impl(const char *z, unixFile *p){   \
  UNUSED_PARAMETER(z); UNUSED_PARAMETER(p);                                  \
  return &METHOD;                                                            \
}                                                                            \
static const sqlite3_io_methods *(*const FINDER)(const char*,unixFile *p)    \
    = FINDER##Impl;

/*
** Here are all of the sqlite3_io_methods objects for each of the
** locking strategies.  Functions that return pointers to these methods
** are also created.
*/
IOMETHODS(
  posixIoFinder,            /* Finder function name */
  posixIoMethods,           /* sqlite3_io_methods object name */
  3,                        /* shared memory and mmap are enabled */
  unixClose,                /* xClose method */
  unixLock,                 /* xLock method */
  unixUnlock,               /* xUnlock method */
  unixCheckReservedLock,    /* xCheckReservedLock method */
  unixShmMap                /* xShmMap method */
)
IOMETHODS(
  nolockIoFinder,           /* Finder function name */
  nolockIoMethods,          /* sqlite3_io_methods object name */
  3,                        /* shared memory is disabled */
  nolockClose,              /* xClose method */
  nolockLock,               /* xLock method */
  nolockUnlock,             /* xUnlock method */
  nolockCheckReservedLock,  /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
IOMETHODS(
  dotlockIoFinder,          /* Finder function name */
  dotlockIoMethods,         /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  dotlockClose,             /* xClose method */
  dotlockLock,              /* xLock method */
  dotlockUnlock,            /* xUnlock method */
  dotlockCheckReservedLock, /* xCheckReservedLock method */
  0                         /* xShmMap method */
)

#if SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  flockIoFinder,            /* Finder function name */
  flockIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  flockClose,               /* xClose method */
  flockLock,                /* xLock method */
  flockUnlock,              /* xUnlock method */
  flockCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if OS_VXWORKS
IOMETHODS(
  semIoFinder,              /* Finder function name */
  semIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  semXClose,                /* xClose method */
  semXLock,                 /* xLock method */
  semXUnlock,               /* xUnlock method */
  semXCheckReservedLock,    /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  afpIoFinder,              /* Finder function name */
  afpIoMethods,             /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  afpClose,                 /* xClose method */
  afpLock,                  /* xLock method */
  afpUnlock,                /* xUnlock method */
  afpCheckReservedLock,     /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/*
** The proxy locking method is a "super-method" in the sense that it
** opens secondary file descriptors for the conch and lock files and
** it uses proxy, dot-file, AFP, and flock() locking methods on those
** secondary files.  For this reason, the division that implements
** proxy locking is located much further down in the file.  But we need
** to go ahead and define the sqlite3_io_methods and finder function
** for proxy locking here.  So we forward declare the I/O methods.
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
static int proxyClose(sqlite3_file*);
static int proxyLock(sqlite3_file*, int);
static int proxyUnlock(sqlite3_file*, int);
static int proxyCheckReservedLock(sqlite3_file*, int*);
IOMETHODS(
  proxyIoFinder,            /* Finder function name */
  proxyIoMethods,           /* sqlite3_io_methods object name */
  1,                        /* shared memory is disabled */
  proxyClose,               /* xClose method */
  proxyLock,                /* xLock method */
  proxyUnlock,              /* xUnlock method */
  proxyCheckReservedLock,   /* xCheckReservedLock method */
  0                         /* xShmMap method */
)
#endif

/* nfs lockd on OSX 10.3+ doesn't clear write locks when a read lock is set */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
IOMETHODS(
  nfsIoFinder,               /* Finder function name */
  nfsIoMethods,              /* sqlite3_io_methods object name */
  1,                         /* shared memory is disabled */
  unixClose,                 /* xClose method */
  unixLock,                  /* xLock method */
  nfsUnlock,                 /* xUnlock method */
  unixCheckReservedLock,     /* xCheckReservedLock method */
  0                          /* xShmMap method */
)
#endif

#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
/* 
** This "finder" function attempts to determine the best locking strategy 
** for the database file "filePath".  It then returns the sqlite3_io_methods
** object that implements that strategy.
**
** This is for MacOSX only.
*/
static const sqlite3_io_methods *autolockIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* open file object for the database file */
){
  static const struct Mapping {
    const char *zFilesystem;              /* Filesystem type name */
    const sqlite3_io_methods *pMethods;   /* Appropriate locking method */
  } aMap[] = {
    { "hfs",    &posixIoMethods },
    { "ufs",    &posixIoMethods },
    { "afpfs",  &afpIoMethods },
    { "smbfs",  &afpIoMethods },
    { "webdav", &nolockIoMethods },
    { 0, 0 }
  };
  int i;
  struct statfs fsInfo;
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }
  if( statfs(filePath, &fsInfo) != -1 ){
    if( fsInfo.f_flags & MNT_RDONLY ){
      return &nolockIoMethods;
    }
    for(i=0; aMap[i].zFilesystem; i++){
      if( strcmp(fsInfo.f_fstypename, aMap[i].zFilesystem)==0 ){
        return aMap[i].pMethods;
      }
    }
  }

  /* Default case. Handles, amongst others, "nfs".
  ** Test byte-range lock using fcntl(). If the call succeeds, 
  ** assume that the file-system supports POSIX style locks. 
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    if( strcmp(fsInfo.f_fstypename, "nfs")==0 ){
      return &nfsIoMethods;
    } else {
      return &posixIoMethods;
    }
  }else{
    return &dotlockIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const autolockIoFinder)(const char*,unixFile*) = autolockIoFinderImpl;

#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */

#if OS_VXWORKS
/*
** This "finder" function for VxWorks checks to see if posix advisory
** locking works.  If it does, then that is what is used.  If it does not
** work, then fallback to named semaphore locking.
*/
static const sqlite3_io_methods *vxworksIoFinderImpl(
  const char *filePath,    /* name of the database file */
  unixFile *pNew           /* the open file object */
){
  struct flock lockInfo;

  if( !filePath ){
    /* If filePath==NULL that means we are dealing with a transient file
    ** that does not need to be locked. */
    return &nolockIoMethods;
  }

  /* Test if fcntl() is supported and use POSIX style locks.
  ** Otherwise fall back to the named semaphore method.
  */
  lockInfo.l_len = 1;
  lockInfo.l_start = 0;
  lockInfo.l_whence = SEEK_SET;
  lockInfo.l_type = F_RDLCK;
  if( osFcntl(pNew->h, F_GETLK, &lockInfo)!=-1 ) {
    return &posixIoMethods;
  }else{
    return &semIoMethods;
  }
}
static const sqlite3_io_methods 
  *(*const vxworksIoFinder)(const char*,unixFile*) = vxworksIoFinderImpl;

#endif /* OS_VXWORKS */

/*
** An abstract type for a pointer to an IO method finder function:
*/
typedef const sqlite3_io_methods *(*finder_type)(const char*,unixFile*);


/****************************************************************************
**************************** sqlite3_vfs methods ****************************
**
** This division contains the implementation of methods on the
** sqlite3_vfs object.
*/

/*
** Initialize the contents of the unixFile structure pointed to by pId.
*/
static int fillInUnixFile(
  sqlite3_vfs *pVfs,      /* Pointer to vfs object */
  int h,                  /* Open file descriptor of file being opened */
  sqlite3_file *pId,      /* Write to the unixFile structure here */
  const char *zFilename,  /* Name of the file being opened */
  int ctrlFlags           /* Zero or more UNIXFILE_* values */
){
  const sqlite3_io_methods *pLockingStyle;
  unixFile *pNew = (unixFile *)pId;
  int rc = SQLITE_OK;

  assert( pNew->pInode==NULL );

  /* Usually the path zFilename should not be a relative pathname. The
  ** exception is when opening the proxy "conch" file in builds that
  ** include the special Apple locking styles.
  */
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
  assert( zFilename==0 || zFilename[0]=='/' 
    || pVfs->pAppData==(void*)&autolockIoFinder );
#else
  assert( zFilename==0 || zFilename[0]=='/' );
#endif

  /* No locking occurs in temporary files */
  assert( zFilename!=0 || (ctrlFlags & UNIXFILE_NOLOCK)!=0 );

  OSTRACE(("OPEN    %-3d %s\n", h, zFilename));
  pNew->h = h;
  pNew->pVfs = pVfs;
  pNew->zPath = zFilename;
  pNew->ctrlFlags = (u8)ctrlFlags;
#if SQLITE_MAX_MMAP_SIZE>0
  pNew->mmapSizeMax = sqlite3GlobalConfig.szMmap;
#endif
  if( sqlite3_uri_boolean(((ctrlFlags & UNIXFILE_URI) ? zFilename : 0),
                           "psow", SQLITE_POWERSAFE_OVERWRITE) ){
    pNew->ctrlFlags |= UNIXFILE_PSOW;
  }
  if( strcmp(pVfs->zName,"unix-excl")==0 ){
    pNew->ctrlFlags |= UNIXFILE_EXCL;
  }

#if OS_VXWORKS
  pNew->pId = vxworksFindFileId(zFilename);
  if( pNew->pId==0 ){
    ctrlFlags |= UNIXFILE_NOLOCK;
    rc = SQLITE_NOMEM;
  }
#endif

  if( ctrlFlags & UNIXFILE_NOLOCK ){
    pLockingStyle = &nolockIoMethods;
  }else{
    pLockingStyle = (**(finder_type*)pVfs->pAppData)(zFilename, pNew);
#if SQLITE_ENABLE_LOCKING_STYLE
    /* Cache zFilename in the locking context (AFP and dotlock override) for
    ** proxyLock activation is possible (remote proxy is based on db name)
    ** zFilename remains valid until file is closed, to support */
    pNew->lockingContext = (void*)zFilename;
#endif
  }

  if( pLockingStyle == &posixIoMethods
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE
    || pLockingStyle == &nfsIoMethods
#endif
  ){
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( rc!=SQLITE_OK ){
      /* If an error occurred in findInodeInfo(), close the file descriptor
      ** immediately, before releasing the mutex. findInodeInfo() may fail
      ** in two scenarios:
      **
      **   (a) A call to fstat() failed.
      **   (b) A malloc failed.
      **
      ** Scenario (b) may only occur if the process is holding no other
      ** file descriptors open on the same file. If there were other file
      ** descriptors on this file, then no malloc would be required by
      ** findInodeInfo(). If this is the case, it is quite safe to close
      ** handle h - as it is guaranteed that no posix locks will be released
      ** by doing so.
      **
      ** If scenario (a) caused the error then things are not so safe. The
      ** implicit assumption here is that if fstat() fails, things are in
      ** such bad shape that dropping a lock or two doesn't matter much.
      */
      robust_close(pNew, h, __LINE__);
      h = -1;
    }
    unixLeaveMutex();
  }

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
  else if( pLockingStyle == &afpIoMethods ){
    /* AFP locking uses the file path so it needs to be included in
    ** the afpLockingContext.
    */
    afpLockingContext *pCtx;
    pNew->lockingContext = pCtx = sqlite3_malloc64( sizeof(*pCtx) );
    if( pCtx==0 ){
      rc = SQLITE_NOMEM;
    }else{
      /* NB: zFilename exists and remains valid until the file is closed
      ** according to requirement F11141.  So we do not need to make a
      ** copy of the filename. */
      pCtx->dbPath = zFilename;
      pCtx->reserved = 0;
      srandomdev();
      unixEnterMutex();
      rc = findInodeInfo(pNew, &pNew->pInode);
      if( rc!=SQLITE_OK ){
        sqlite3_free(pNew->lockingContext);
        robust_close(pNew, h, __LINE__);
        h = -1;
      }
      unixLeaveMutex();        
    }
  }
#endif

  else if( pLockingStyle == &dotlockIoMethods ){
    /* Dotfile locking uses the file path so it needs to be included in
    ** the dotlockLockingContext 
    */
    char *zLockFile;
    int nFilename;
    assert( zFilename!=0 );
    nFilename = (int)strlen(zFilename) + 6;
    zLockFile = (char *)sqlite3_malloc64(nFilename);
    if( zLockFile==0 ){
      rc = SQLITE_NOMEM;
    }else{
      sqlite3_snprintf(nFilename, zLockFile, "%s" DOTLOCK_SUFFIX, zFilename);
    }
    pNew->lockingContext = zLockFile;
  }

#if OS_VXWORKS
  else if( pLockingStyle == &semIoMethods ){
    /* Named semaphore locking uses the file path so it needs to be
    ** included in the semLockingContext
    */
    unixEnterMutex();
    rc = findInodeInfo(pNew, &pNew->pInode);
    if( (rc==SQLITE_OK) && (pNew->pInode->pSem==NULL) ){
      char *zSemName = pNew->pInode->aSemName;
      int n;
      sqlite3_snprintf(MAX_PATHNAME, zSemName, "/%s.sem",
                       pNew->pId->zCanonicalName);
      for( n=1; zSemName[n]; n++ )
        if( zSemName[n]=='/' ) zSemName[n] = '_';
      pNew->pInode->pSem = sem_open(zSemName, O_CREAT, 0666, 1);
      if( pNew->pInode->pSem == SEM_FAILED ){
        rc = SQLITE_NOMEM;
        pNew->pInode->aSemName[0] = '\0';
      }
    }
    unixLeaveMutex();
  }
#endif
  
  storeLastErrno(pNew, 0);
#if OS_VXWORKS
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
    h = -1;
    osUnlink(zFilename);
    pNew->ctrlFlags |= UNIXFILE_DELETE;
  }
#endif
  if( rc!=SQLITE_OK ){
    if( h>=0 ) robust_close(pNew, h, __LINE__);
  }else{
    pNew->pMethod = pLockingStyle;
    OpenCounter(+1);
    verifyDbFile(pNew);
  }
  return rc;
}

/*
** Return the name of a directory in which to put temporary files.
** If no suitable temporary file directory can be found, return NULL.
*/
static const char *unixTempFileDir(void){
  static const char *azDirs[] = {
     0,
     0,
     0,
     "/var/tmp",
     "/usr/tmp",
     "/tmp",
     0        /* List terminator */
  };
  unsigned int i;
  struct stat buf;
  const char *zDir = 0;

  azDirs[0] = sqlite3_temp_directory;
  if( !azDirs[1] ) azDirs[1] = getenv("SQLITE_TMPDIR");
  if( !azDirs[2] ) azDirs[2] = getenv("TMPDIR");
  for(i=0; i<sizeof(azDirs)/sizeof(azDirs[0]); zDir=azDirs[i++]){
    if( zDir==0 ) continue;
    if( osStat(zDir, &buf) ) continue;
    if( !S_ISDIR(buf.st_mode) ) continue;
    if( osAccess(zDir, 07) ) continue;
    break;
  }
  return zDir;
}

/*
** Create a temporary file name in zBuf.  zBuf must be allocated
** by the calling process and must be big enough to hold at least
** pVfs->mxPathname bytes.
*/
static int unixGetTempname(int nBuf, char *zBuf){
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  unsigned int i, j;
  const char *zDir;

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. 
  */
  SimulateIOError( return SQLITE_IOERR );

  zDir = unixTempFileDir();
  if( zDir==0 ) zDir = ".";

  /* Check that the output buffer is large enough for the temporary file 
  ** name. If it is not, return SQLITE_ERROR.
  */
  if( (strlen(zDir) + strlen(SQLITE_TEMP_FILE_PREFIX) + 18) >= (size_t)nBuf ){
    return SQLITE_ERROR;
  }

  do{
    sqlite3_snprintf(nBuf-18, zBuf, "%s/"SQLITE_TEMP_FILE_PREFIX, zDir);
    j = (int)strlen(zBuf);
    sqlite3_randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
    zBuf[j+1] = 0;
  }while( osAccess(zBuf,0)==0 );
  return SQLITE_OK;
}

#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
/*
** Routine to transform a unixFile into a proxy-locking unixFile.
** Implementation in the proxy-lock division, but used by unixOpen()
** if SQLITE_PREFER_PROXY_LOCKING is defined.
*/
static int proxyTransformUnixFile(unixFile*, const char*);
#endif

/*
** Search for an unused file descriptor that was opened on the database 
** file (not a journal or master-journal file) identified by pathname
** zPath with SQLITE_OPEN_XXX flags matching those passed as the second
** argument to this function.
**
** Such a file descriptor may exist if a database connection was closed
** but the associated file descriptor could not be closed because some
** other file descriptor open on the same file is holding a file-lock.
** Refer to comments in the unixClose() function and the lengthy comment
** describing "Posix Advisory Locking" at the start of this file for 
** further details. Also, ticket #4018.
**
** If a suitable file descriptor is found, then it is returned. If no
** such file descriptor is located, -1 is returned.
*/
static UnixUnusedFd *findReusableFd(const char *zPath, int flags){
  UnixUnusedFd *pUnused = 0;

  /* Do not search for an unused file descriptor on vxworks. Not because
  ** vxworks would not benefit from the change (it might, we're not sure),
  ** but because no way to test it is currently available. It is better 
  ** not to risk breaking vxworks support for the sake of such an obscure 
  ** feature.  */
#if !OS_VXWORKS
  struct stat sStat;                   /* Results of stat() call */

  /* A stat() call may fail for various reasons. If this happens, it is
  ** almost certain that an open() call on the same path will also fail.
  ** For this reason, if an error occurs in the stat() call here, it is
  ** ignored and -1 is returned. The caller will try to open a new file
  ** descriptor on the same path, fail, and return an error to SQLite.
  **
  ** Even if a subsequent open() call does succeed, the consequences of
  ** not searching for a reusable file descriptor are not dire.  */
  if( 0==osStat(zPath, &sStat) ){
    unixInodeInfo *pInode;

    unixEnterMutex();
    pInode = inodeList;
    while( pInode && (pInode->fileId.dev!=sStat.st_dev
                     || pInode->fileId.ino!=sStat.st_ino) ){
       pInode = pInode->pNext;
    }
    if( pInode ){
      UnixUnusedFd **pp;
      for(pp=&pInode->pUnused; *pp && (*pp)->flags!=flags; pp=&((*pp)->pNext));
      pUnused = *pp;
      if( pUnused ){
        *pp = pUnused->pNext;
      }
    }
    unixLeaveMutex();
  }
#endif    /* if !OS_VXWORKS */
  return pUnused;
}

/*
** This function is called by unixOpen() to determine the unix permissions
** to create new files with. If no error occurs, then SQLITE_OK is returned
** and a value suitable for passing as the third argument to open(2) is
** written to *pMode. If an IO error occurs, an SQLite error code is 
** returned and the value of *pMode is not modified.
**
** In most cases, this routine sets *pMode to 0, which will become
** an indication to robust_open() to create the file using
** SQLITE_DEFAULT_FILE_PERMISSIONS adjusted by the umask.
** But if the file being opened is a WAL or regular journal file, then 
** this function queries the file-system for the permissions on the 
** corresponding database file and sets *pMode to this value. Whenever 
** possible, WAL and journal files are created using the same permissions 
** as the associated database file.
**
** If the SQLITE_ENABLE_8_3_NAMES option is enabled, then the
** original filename is unavailable.  But 8_3_NAMES is only used for
** FAT filesystems and permissions do not matter there, so just use
** the default permissions.
*/
static int findCreateFileMode(
  const char *zPath,              /* Path of file (possibly) being created */
  int flags,                      /* Flags passed as 4th argument to xOpen() */
  mode_t *pMode,                  /* OUT: Permissions to open file with */
  uid_t *pUid,                    /* OUT: uid to set on the file */
  gid_t *pGid                     /* OUT: gid to set on the file */
){
  int rc = SQLITE_OK;             /* Return Code */
  *pMode = 0;
  *pUid = 0;
  *pGid = 0;
  if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
    char zDb[MAX_PATHNAME+1];     /* Database file path */
    int nDb;                      /* Number of valid bytes in zDb */
    struct stat sStat;            /* Output of stat() on database file */

    /* zPath is a path to a WAL or journal file. The following block derives
    ** the path to the associated database file from zPath. This block handles
    ** the following naming conventions:
    **
    **   "<path to db>-journal"
    **   "<path to db>-wal"
    **   "<path to db>-journalNN"
    **   "<path to db>-walNN"
    **
    ** where NN is a decimal number. The NN naming schemes are 
    ** used by the test_multiplex.c module.
    */
    nDb = sqlite3Strlen30(zPath) - 1; 
#ifdef SQLITE_ENABLE_8_3_NAMES
    while( nDb>0 && sqlite3Isalnum(zPath[nDb]) ) nDb--;
    if( nDb==0 || zPath[nDb]!='-' ) return SQLITE_OK;
#else
    while( zPath[nDb]!='-' ){
      assert( nDb>0 );
      assert( zPath[nDb]!='\n' );
      nDb--;
    }
#endif
    memcpy(zDb, zPath, nDb);
    zDb[nDb] = '\0';

    if( 0==osStat(zDb, &sStat) ){
      *pMode = sStat.st_mode & 0777;
      *pUid = sStat.st_uid;
      *pGid = sStat.st_gid;
    }else{
      rc = SQLITE_IOERR_FSTAT;
    }
  }else if( flags & SQLITE_OPEN_DELETEONCLOSE ){
    *pMode = 0600;
  }
  return rc;
}

/*
** Open the file zPath.
** 
** Previously, the SQLite OS layer used three functions in place of this
** one:
**
**     sqlite3OsOpenReadWrite();
**     sqlite3OsOpenReadOnly();
**     sqlite3OsOpenExclusive();
**
** These calls correspond to the following combinations of flags:
**
**     ReadWrite() ->     (READWRITE | CREATE)
**     ReadOnly()  ->     (READONLY) 
**     OpenExclusive() -> (READWRITE | CREATE | EXCLUSIVE)
**
** The old OpenExclusive() accepted a boolean argument - "delFlag". If
** true, the file was configured to be automatically deleted when the
** file handle closed. To achieve the same effect using this new 
** interface, add the DELETEONCLOSE flag to those specified above for 
** OpenExclusive().
*/
static int unixOpen(
  sqlite3_vfs *pVfs,           /* The VFS for which this is the xOpen method */
  const char *zPath,           /* Pathname of file to be opened */
  sqlite3_file *pFile,         /* The file descriptor to be filled in */
  int flags,                   /* Input flags to control the opening */
  int *pOutFlags               /* Output flags returned to SQLite core */
){
  unixFile *p = (unixFile *)pFile;
  int fd = -1;                   /* File descriptor returned by open() */
  int openFlags = 0;             /* Flags to pass to open() */
  int eType = flags&0xFFFFFF00;  /* Type of file to open */
  int noLock;                    /* True to omit locking primitives */
  int rc = SQLITE_OK;            /* Function Return Code */
  int ctrlFlags = 0;             /* UNIXFILE_* flags */

  int isExclusive  = (flags & SQLITE_OPEN_EXCLUSIVE);
  int isDelete     = (flags & SQLITE_OPEN_DELETEONCLOSE);
  int isCreate     = (flags & SQLITE_OPEN_CREATE);
  int isReadonly   = (flags & SQLITE_OPEN_READONLY);
  int isReadWrite  = (flags & SQLITE_OPEN_READWRITE);
#if SQLITE_ENABLE_LOCKING_STYLE
  int isAutoProxy  = (flags & SQLITE_OPEN_AUTOPROXY);
#endif
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  struct statfs fsInfo;
#endif

  /* If creating a master or main-file journal, this function will open
  ** a file-descriptor on the directory too. The first time unixSync()
  ** is called the directory file descriptor will be fsync()ed and close()d.
  */
  int syncDir = (isCreate && (
        eType==SQLITE_OPEN_MASTER_JOURNAL 
     || eType==SQLITE_OPEN_MAIN_JOURNAL 
     || eType==SQLITE_OPEN_WAL
  ));

  /* If argument zPath is a NULL pointer, this function is required to open
  ** a temporary file. Use this buffer to store the file name in.
  */
  char zTmpname[MAX_PATHNAME+2];
  const char *zName = zPath;

  /* Check the following statements are true: 
  **
  **   (a) Exactly one of the READWRITE and READONLY flags must be set, and 
  **   (b) if CREATE is set, then READWRITE must also be set, and
  **   (c) if EXCLUSIVE is set, then CREATE must also be set.
  **   (d) if DELETEONCLOSE is set, then CREATE must also be set.
  */
  assert((isReadonly==0 || isReadWrite==0) && (isReadWrite || isReadonly));
  assert(isCreate==0 || isReadWrite);
  assert(isExclusive==0 || isCreate);
  assert(isDelete==0 || isCreate);

  /* The main DB, main journal, WAL file and master journal are never 
  ** automatically deleted. Nor are they ever temporary files.  */
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_DB );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MASTER_JOURNAL );
  assert( (!isDelete && zName) || eType!=SQLITE_OPEN_WAL );

  /* Assert that the upper layer has set one of the "file-type" flags. */
  assert( eType==SQLITE_OPEN_MAIN_DB      || eType==SQLITE_OPEN_TEMP_DB 
       || eType==SQLITE_OPEN_MAIN_JOURNAL || eType==SQLITE_OPEN_TEMP_JOURNAL 
       || eType==SQLITE_OPEN_SUBJOURNAL   || eType==SQLITE_OPEN_MASTER_JOURNAL 
       || eType==SQLITE_OPEN_TRANSIENT_DB || eType==SQLITE_OPEN_WAL
  );

  /* Detect a pid change and reset the PRNG.  There is a race condition
  ** here such that two or more threads all trying to open databases at
  ** the same instant might all reset the PRNG.  But multiple resets
  ** are harmless.
  */
  if( randomnessPid!=osGetpid(0) ){
    randomnessPid = osGetpid(0);
    sqlite3_randomness(0,0);
  }

  memset(p, 0, sizeof(unixFile));

  if( eType==SQLITE_OPEN_MAIN_DB ){
    UnixUnusedFd *pUnused;
    pUnused = findReusableFd(zName, flags);
    if( pUnused ){
      fd = pUnused->fd;
    }else{
      pUnused = sqlite3_malloc64(sizeof(*pUnused));
      if( !pUnused ){
        return SQLITE_NOMEM;
      }
    }
    p->pUnused = pUnused;

    /* Database filenames are double-zero terminated if they are not
    ** URIs with parameters.  Hence, they can always be passed into
    ** sqlite3_uri_parameter(). */
    assert( (flags & SQLITE_OPEN_URI) || zName[strlen(zName)+1]==0 );

  }else if( !zName ){
    /* If zName is NULL, the upper layer is requesting a temp file. */
    assert(isDelete && !syncDir);
    rc = unixGetTempname(MAX_PATHNAME+2, zTmpname);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    zName = zTmpname;

    /* Generated temporary filenames are always double-zero terminated
    ** for use by sqlite3_uri_parameter(). */
    assert( zName[strlen(zName)+1]==0 );
  }

  /* Determine the value of the flags parameter passed to POSIX function
  ** open(). These must be calculated even if open() is not called, as
  ** they may be stored as part of the file handle and used by the 
  ** 'conch file' locking functions later on.  */
  if( isReadonly )  openFlags |= O_RDONLY;
  if( isReadWrite ) openFlags |= O_RDWR;
  if( isCreate )    openFlags |= O_CREAT;
  if( isExclusive ) openFlags |= (O_EXCL|O_NOFOLLOW);
  openFlags |= (O_LARGEFILE|O_BINARY);

  if( fd<0 ){
    mode_t openMode;              /* Permissions to create file with */
    uid_t uid;                    /* Userid for the file */
    gid_t gid;                    /* Groupid for the file */
    rc = findCreateFileMode(zName, flags, &openMode, &uid, &gid);
    if( rc!=SQLITE_OK ){
      assert( !p->pUnused );
      assert( eType==SQLITE_OPEN_WAL || eType==SQLITE_OPEN_MAIN_JOURNAL );
      return rc;
    }
    fd = robust_open(zName, openFlags, openMode);
    OSTRACE(("OPENX   %-3d %s 0%o\n", fd, zName, openFlags));
    if( fd<0 && errno!=EISDIR && isReadWrite && !isExclusive ){
      /* Failed to open the file for read/write access. Try read-only. */
      flags &= ~(SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
      openFlags &= ~(O_RDWR|O_CREAT);
      flags |= SQLITE_OPEN_READONLY;
      openFlags |= O_RDONLY;
      isReadonly = 1;
      fd = robust_open(zName, openFlags, openMode);
    }
    if( fd<0 ){
      rc = unixLogError(SQLITE_CANTOPEN_BKPT, "open", zName);
      goto open_finished;
    }

    /* If this process is running as root and if creating a new rollback
    ** journal or WAL file, set the ownership of the journal or WAL to be
    ** the same as the original database.
    */
    if( flags & (SQLITE_OPEN_WAL|SQLITE_OPEN_MAIN_JOURNAL) ){
      osFchown(fd, uid, gid);
    }
  }
  assert( fd>=0 );
  if( pOutFlags ){
    *pOutFlags = flags;
  }

  if( p->pUnused ){
    p->pUnused->fd = fd;
    p->pUnused->flags = flags;
  }

  if( isDelete ){
#if OS_VXWORKS
    zPath = zName;
#elif defined(SQLITE_UNLINK_AFTER_CLOSE)
    zPath = sqlite3_mprintf("%s", zName);
    if( zPath==0 ){
      robust_close(p, fd, __LINE__);
      return SQLITE_NOMEM;
    }
#else
    osUnlink(zName);
#endif
  }
#if SQLITE_ENABLE_LOCKING_STYLE
  else{
    p->openFlags = openFlags;
  }
#endif

  noLock = eType!=SQLITE_OPEN_MAIN_DB;

  
#if defined(__APPLE__) || SQLITE_ENABLE_LOCKING_STYLE
  if( fstatfs(fd, &fsInfo) == -1 ){
    storeLastErrno(p, errno);
    robust_close(p, fd, __LINE__);
    return SQLITE_IOERR_ACCESS;
  }
  if (0 == strncmp("msdos", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
  if (0 == strncmp("exfat", fsInfo.f_fstypename, 5)) {
    ((unixFile*)pFile)->fsFlags |= SQLITE_FSFLAGS_IS_MSDOS;
  }
#endif

  /* Set up appropriate ctrlFlags */
  if( isDelete )                ctrlFlags |= UNIXFILE_DELETE;
  if( isReadonly )              ctrlFlags |= UNIXFILE_RDONLY;
  if( noLock )                  ctrlFlags |= UNIXFILE_NOLOCK;
  if( syncDir )                 ctrlFlags |= UNIXFILE_DIRSYNC;
  if( flags & SQLITE_OPEN_URI ) ctrlFlags |= UNIXFILE_URI;

#if SQLITE_ENABLE_LOCKING_STYLE
#if SQLITE_PREFER_PROXY_LOCKING
  isAutoProxy = 1;
#endif
  if( isAutoProxy && (zPath!=NULL) && (!noLock) && pVfs->xOpen ){
    char *envforce = getenv("SQLITE_FORCE_PROXY_LOCKING");
    int useProxy = 0;

    /* SQLITE_FORCE_PROXY_LOCKING==1 means force always use proxy, 0 means 
    ** never use proxy, NULL means use proxy for non-local files only.  */
    if( envforce!=NULL ){
      useProxy = atoi(envforce)>0;
    }else{
      useProxy = !(fsInfo.f_flags&MNT_LOCAL);
    }
    if( useProxy ){
      rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);
      if( rc==SQLITE_OK ){
        rc = proxyTransformUnixFile((unixFile*)pFile, ":auto:");
        if( rc!=SQLITE_OK ){
          /* Use unixClose to clean up the resources added in fillInUnixFile 
          ** and clear all the structure's references.  Specifically, 
          ** pFile->pMethods will be NULL so sqlite3OsClose will be a no-op 
          */
          unixClose(pFile);
          return rc;
        }
      }
      goto open_finished;
    }
  }
#endif
  
  rc = fillInUnixFile(pVfs, fd, pFile, zPath, ctrlFlags);

open_finished:
  if( rc!=SQLITE_OK ){
    sqlite3_free(p->pUnused);
  }
  return rc;
}


/*
** Delete the file at zPath. If the dirSync argument is true, fsync()
** the directory after deleting the file.
*/
static int unixDelete(
  sqlite3_vfs *NotUsed,     /* VFS containing this as the xDelete method */
  const char *zPath,        /* Name of file to be deleted */
  int dirSync               /* If true, fsync() directory after deleting file */
){
  int rc = SQLITE_OK;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError(return SQLITE_IOERR_DELETE);
  if( osUnlink(zPath)==(-1) ){
    if( errno==ENOENT
#if OS_VXWORKS
        || osAccess(zPath,0)!=0
#endif
    ){
      rc = SQLITE_IOERR_DELETE_NOENT;
    }else{
      rc = unixLogError(SQLITE_IOERR_DELETE, "unlink", zPath);
    }
    return rc;
  }
#ifndef SQLITE_DISABLE_DIRSYNC
  if( (dirSync & 1)!=0 ){
    int fd;
    rc = osOpenDirectory(zPath, &fd);
    if( rc==SQLITE_OK ){
#if OS_VXWORKS
      if( fsync(fd)==-1 )
#else
      if( fsync(fd) )
#endif
      {
        rc = unixLogError(SQLITE_IOERR_DIR_FSYNC, "fsync", zPath);
      }
      robust_close(0, fd, __LINE__);
    }else if( rc==SQLITE_CANTOPEN ){
      rc = SQLITE_OK;
    }
  }
#endif
  return rc;
}

/*
** Test the existence of or access permissions of file zPath. The
** test performed depends on the value of flags:
**
**     SQLITE_ACCESS_EXISTS: Return 1 if the file exists
**     SQLITE_ACCESS_READWRITE: Return 1 if the file is read and writable.
**     SQLITE_ACCESS_READONLY: Return 1 if the file is readable.
**
** Otherwise return 0.
*/
static int unixAccess(
  sqlite3_vfs *NotUsed,   /* The VFS containing this xAccess method */
  const char *zPath,      /* Path of the file to examine */
  int flags,              /* What do we want to learn about the zPath file? */
  int *pResOut            /* Write result boolean here */
){
  int amode = 0;
  UNUSED_PARAMETER(NotUsed);
  SimulateIOError( return SQLITE_IOERR_ACCESS; );
  switch( flags ){
    case SQLITE_ACCESS_EXISTS:
      amode = F_OK;
      break;
    case SQLITE_ACCESS_READWRITE:
      amode = W_OK|R_OK;
      break;
    case SQLITE_ACCESS_READ:
      amode = R_OK;
      break;

    default:
      assert(!"Invalid flags argument");
  }
  *pResOut = (osAccess(zPath, amode)==0);
  if( flags==SQLITE_ACCESS_EXISTS && *pResOut ){
    struct stat buf;
    if( 0==osStat(zPath, &buf) && buf.st_size==0 ){
      *pResOut = 0;
    }
  }
  return SQLITE_OK;
}


/*
** Turn a relative pathname into a full pathname. The relative path
** is stored as a nul-terminated string in the buffer pointed to by
** zPath. 
**
** zOut points to a buffer of at least sqlite3_vfs.mxPathname bytes 
** (in this case, MAX_PATHNAME bytes). The full-path is written to
** this buffer before returning.
*/
static int unixFullPathname(
  sqlite3_vfs *pVfs,            /* Pointer to vfs object */
  const char *zPath,            /* Possibly relative input path */
  int nOut,                     /* Size of output buffer in bytes */
  char *zOut                    /* Output buffer */
){

  /* It's odd to simulate an io-error here, but really this is just
  ** using the io-error infrastructure to test that SQLite handles this
  ** function failing. This function could fail if, for example, the
  ** current working directory has been unlinked.
  */
  SimulateIOError( return SQLITE_ERROR );

  assert( pVfs->mxPathname==MAX_PATHNAME );
  UNUSED_PARAMETER(pVfs);

  zOut[nOut-1] = '\0';
  if( zPath[0]=='/' ){
    sqlite3_snprintf(nOut, zOut, "%s", zPath);
  }else{
    int nCwd;
    if( osGetcwd(zOut, nOut-1)==0 ){
      return unixLogError(SQLITE_CANTOPEN_BKPT, "getcwd", zPath);
    }
    nCwd = (int)strlen(zOut);
    sqlite3_snprintf(nOut-nCwd, &zOut[nCwd], "/%s", zPath);
  }
  return SQLITE_OK;
}


#ifndef SQLITE_OMIT_LOAD_EXTENSION
/*
** Interfaces for opening a shared library, finding entry points
** within the shared library, and closing the shared library.
*/
#include <dlfcn.h>
static void *unixDlOpen(sqlite3_vfs *NotUsed, const char *zFilename){
  UNUSED_PARAMETER(NotUsed);
  return dlopen(zFilename, RTLD_NOW | RTLD_GLOBAL);
}

/*
** SQLite calls this function immediately after a call to unixDlSym() or
** unixDlOpen() fails (returns a null pointer). If a more detailed error
** message is available, it is written to zBufOut. If no error message
** is available, zBufOut is left unmodified and SQLite uses a default
** error message.
*/
static void unixDlError(sqlite3_vfs *NotUsed, int nBuf, char *zBufOut){
  const char *zErr;
  UNUSED_PARAMETER(NotUsed);
  unixEnterMutex();
  zErr = dlerror();
  if( zErr ){
    sqlite3_snprintf(nBuf, zBufOut, "%s", zErr);
  }
  unixLeaveMutex();
}
static void (*unixDlSym(sqlite3_vfs *NotUsed, void *p, const char*zSym))(void){
  /* 
  ** GCC with -pedantic-errors says that C90 does not allow a void* to be
  ** cast into a pointer to a function.  And yet the library dlsym() routine
  ** returns a void* which is really a pointer to a function.  So how do we
  ** use dlsym() with -pedantic-errors?
  **
  ** Variable x below is defined to be a pointer to a function taking
  ** parameters void* and const char* and returning a pointer to a function.
  ** We initialize x by assigning it a pointer to the dlsym() function.
  ** (That assignment requires a cast.)  Then we call the function that
  ** x points to.  
  **
  ** This work-around is unlikely to work correctly on any system where
  ** you really cannot cast a function pointer into void*.  But then, on the
  ** other hand, dlsym() will not work on such a system either, so we have
  ** not really lost anything.
  */
  void (*(*x)(void*,const char*))(void);
  UNUSED_PARAMETER(NotUsed);
  x = (void(*(*)(void*,const char*))(void))dlsym;
  return (*x)(p, zSym);
}
static void unixDlClose(sqlite3_vfs *NotUsed, void *pHandle){
  UNUSED_PARAMETER(NotUsed);
  dlclose(pHandle);
}
#else /* if SQLITE_OMIT_LOAD_EXTENSION is defined: */
  #define unixDlOpen  0
  #define unixDlError 0
  #define unixDlSym   0
  #define unixDlClose 0
#endif

/*
** Write nBuf bytes of random data to the supplied buffer zBuf.
*/
static int unixRandomness(sqlite3_vfs *NotUsed, int nBuf, char *zBuf){
  UNUSED_PARAMETER(NotUsed);
  assert((size_t)nBuf>=(sizeof(time_t)+sizeof(int)));

  /* We have to initialize zBuf to prevent valgrind from reporting
  ** errors.  The reports issued by valgrind are incorrect - we would
  ** prefer that the randomness be increased by making use of the
  ** uninitialized space in zBuf - but valgrind errors tend to worry
  ** some users.  Rather than argue, it seems easier just to initialize
  ** the whole array and silence valgrind, even if that means less randomness
  ** in the random seed.
  **
  ** When testing, initializing zBuf[] to zero is all we do.  That means
  ** that we always use the same random number sequence.  This makes the
  ** tests repeatable.
  */
  memset(zBuf, 0, nBuf);
  randomnessPid = osGetpid(0);  
#if !defined(SQLITE_TEST) && !defined(SQLITE_OMIT_RANDOMNESS)
  {
    int fd, got;
    fd = robust_open("/dev/urandom", O_RDONLY, 0);
    if( fd<0 ){
      time_t t;
      time(&t);
      memcpy(zBuf, &t, sizeof(t));
      memcpy(&zBuf[sizeof(t)], &randomnessPid, sizeof(randomnessPid));
      assert( sizeof(t)+sizeof(randomnessPid)<=(size_t)nBuf );
      nBuf = sizeof(t) + sizeof(randomnessPid);
    }else{
      do{ got = osRead(fd, zBuf, nBuf); }while( got<0 && errno==EINTR );
      robust_close(0, fd, __LINE__);
    }
  }
#endif
  return nBuf;
}


/*
** Sleep for a little while.  Return the amount of time slept.
** The argument is the number of microseconds we want to sleep.
** The return value is the number of microseconds of sleep actually
** requested from the underlying operating system, a number which
** might be greater than or equal to the argument, but not less
** than the argument.
*/
static int unixSleep(sqlite3_vfs *NotUsed, int microseconds){
#if OS_VXWORKS
  struct timespec sp;

  sp.tv_sec = microseconds / 1000000;
  sp.tv_nsec = (microseconds % 1000000) * 1000;
  nanosleep(&sp, NULL);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#elif defined(HAVE_USLEEP) && HAVE_USLEEP
  usleep(microseconds);
  UNUSED_PARAMETER(NotUsed);
  return microseconds;
#else
  int seconds = (microseconds+999999)/1000000;
  sleep(seconds);
  UNUSED_PARAMETER(NotUsed);
  return seconds*1000000;
#endif
}

/*
** The following variable, if set to a non-zero value, is interpreted as
** the number of seconds since 1970 and is used to set the result of
** sqlite3OsCurrentTime() during testing.
*/
#ifdef SQLITE_TEST
SQLITE_API int sqlite3_current_time = 0;  /* Fake system time in seconds since 1970. */
#endif

/*
** Find the current time (in Universal Coordinated Time).  Write into *piNow
** the current time and date as a Julian Day number times 86_400_000.  In
** other words, write into *piNow the number of milliseconds since the Julian
** epoch of noon in Greenwich on November 24, 4714 B.C according to the
** proleptic Gregorian calendar.
**
** On success, return SQLITE_OK.  Return SQLITE_ERROR if the time and date 
** cannot be found.
*/
static int unixCurrentTimeInt64(sqlite3_vfs *NotUsed, sqlite3_int64 *piNow){
  static const sqlite3_int64 unixEpoch = 24405875*(sqlite3_int64)8640000;
  int rc = SQLITE_OK;
#if defined(NO_GETTOD)
  time_t t;
  time(&t);
  *piNow = ((sqlite3_int64)t)*1000 + unixEpoch;
#elif OS_VXWORKS
  struct timespec sNow;
  clock_gettime(CLOCK_REALTIME, &sNow);
  *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_nsec/1000000;
#else
  struct timeval sNow;
  if( gettimeofday(&sNow, 0)==0 ){
    *piNow = unixEpoch + 1000*(sqlite3_int64)sNow.tv_sec + sNow.tv_usec/1000;
  }else{
    rc = SQLITE_ERROR;
  }
#endif

#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *piNow = 1000*(sqlite3_int64)sqlite3_current_time + unixEpoch;
  }
#endif
  UNUSED_PARAMETER(NotUsed);
  return rc;
}

/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
static int unixCurrentTime(sqlite3_vfs *NotUsed, double *prNow){
  sqlite3_int64 i = 0;
  int rc;
  UNUSED_PARAMETER(NotUsed);
  rc = unixCurrentTimeInt64(0, &i);
  *prNow = i/86400000.0;
  return rc;
}

/*
** We added the xGetLastError() method with the intention of providing
** better low-level error messages when operating-system problems come up
** during SQLite operation.  But so far, none of that has been implemented
** in the core.  So this routine is never called.  For now, it is merely
** a place-holder.
*/
static int unixGetLastError(sqlite3_vfs *NotUsed, int NotUsed2, char *NotUsed3){
  UNUSED_PARAMETER(NotUsed);
  UNUSED_PARAMETER(NotUsed2);
  UNUSED_PARAMETER(NotUsed3);
  return 0;
}


/*
************************ End of sqlite3_vfs methods ***************************
******************************************************************************/

/******************************************************************************
************************** Begin Proxy Locking ********************************
**
** Proxy locking is a "uber-locking-method" in this sense:  It uses the
** other locking methods on secondary lock files.  Proxy locking is a
** meta-layer over top of the primitive locking implemented above.  For
** this reason, the division that implements of proxy locking is deferred
** until late in the file (here) after all of the other I/O methods have
** been defined - so that the primitive locking methods are available
** as services to help with the implementation of proxy locking.
**
****
**
** The default locking schemes in SQLite use byte-range locks on the
** database file to coordinate safe, concurrent access by multiple readers
** and writers [http://sqlite.org/lockingv3.html].  The five file locking
** states (UNLOCKED, PENDING, SHARED, RESERVED, EXCLUSIVE) are implemented
** as POSIX read & write locks over fixed set of locations (via fsctl),
** on AFP and SMB only exclusive byte-range locks are available via fsctl
** with _IOWR('z', 23, struct ByteRangeLockPB2) to track the same 5 states.
** To simulate a F_RDLCK on the shared range, on AFP a randomly selected
** address in the shared range is taken for a SHARED lock, the entire
** shared range is taken for an EXCLUSIVE lock):
**
**      PENDING_BYTE        0x40000000
**      RESERVED_BYTE       0x40000001
**      SHARED_RANGE        0x40000002 -> 0x40000200
**
** This works well on the local file system, but shows a nearly 100x
** slowdown in read performance on AFP because the AFP client disables
** the read cache when byte-range locks are present.  Enabling the read
** cache exposes a cache coherency problem that is present on all OS X
** supported network file systems.  NFS and AFP both observe the
** close-to-open semantics for ensuring cache coherency
** [http://nfs.sourceforge.net/#faq_a8], which does not effectively
** address the requirements for concurrent database access by multiple
** readers and writers
** [http://www.nabble.com/SQLite-on-NFS-cache-coherency-td15655701.html].
**
** To address the performance and cache coherency issues, proxy file locking
** changes the way database access is controlled by limiting access to a
** single host at a time and moving file locks off of the database file
** and onto a proxy file on the local file system.  
**
**
** Using proxy locks
** -----------------
**
** C APIs
**
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_SET_LOCKPROXYFILE,
**                       <proxy_path> | ":auto:");
**  sqlite3_file_control(db, dbname, SQLITE_FCNTL_GET_LOCKPROXYFILE,
**                       &<proxy_path>);
**
**
** SQL pragmas
**
**  PRAGMA [database.]lock_proxy_file=<proxy_path> | :auto:
**  PRAGMA [database.]lock_proxy_file
**
** Specifying ":auto:" means that if there is a conch file with a matching
** host ID in it, the proxy path in the conch file will be used, otherwise
** a proxy path based on the user's temp dir
** (via confstr(_CS_DARWIN_USER_TEMP_DIR,...)) will be used and the
** actual proxy file name is generated from the name and path of the
** database file.  For example:
**
**       For database path "/Users/me/foo.db" 
**       The lock path will be "<tmpdir>/sqliteplocks/_Users_me_foo.db:auto:")
**
** Once a lock proxy is configured for a database connection, it can not
** be removed, however it may be switched to a different proxy path via
** the above APIs (assuming the conch file is not being held by another
** connection or process). 
**
**
** How proxy locking works
** -----------------------
**
** Proxy file locking relies primarily on two new supporting files: 
**
**   *  conch file to limit access to the database file to a single host
**      at a time
**
**   *  proxy file to act as a proxy for the advisory locks normally
**      taken on the database
**
** The conch file - to use a proxy file, sqlite must first "hold the conch"
** by taking an sqlite-style shared lock on the conch file, reading the
** contents and comparing the host's unique host ID (see below) and lock
** proxy path against the values stored in the conch.  The conch file is
** stored in the same directory as the database file and the file name
** is patterned after the database file name as ".<databasename>-conch".
** If the conch file does not exist, or its contents do not match the
** host ID and/or proxy path, then the lock is escalated to an exclusive
** lock and the conch file contents is updated with the host ID and proxy
** path and the lock is downgraded to a shared lock again.  If the conch
** is held by another process (with a shared lock), the exclusive lock
** will fail and SQLITE_BUSY is returned.
**
** The proxy file - a single-byte file used for all advisory file locks
** normally taken on the database file.   This allows for safe sharing
** of the database file for multiple readers and writers on the same
** host (the conch ensures that they all use the same local lock file).
**
** Requesting the lock proxy does not immediately take the conch, it is
** only taken when the first request to lock database file is made.  
** This matches the semantics of the traditional locking behavior, where
** opening a connection to a database file does not take a lock on it.
** The shared lock and an open file descriptor are maintained until 
** the connection to the database is closed. 
**
** The proxy file and the lock file are never deleted so they only need
** to be created the first time they are used.
**
** Configuration options
** ---------------------
**
**  SQLITE_PREFER_PROXY_LOCKING
**
**       Database files accessed on non-local file systems are
**       automatically configured for proxy locking, lock files are
**       named automatically using the same logic as
**       PRAGMA lock_proxy_file=":auto:"
**    
**  SQLITE_PROXY_DEBUG
**
**       Enables the logging of error messages during host id file
**       retrieval and creation
**
**  LOCKPROXYDIR
**
**       Overrides the default directory used for lock proxy files that
**       are named automatically via the ":auto:" setting
**
**  SQLITE_DEFAULT_PROXYDIR_PERMISSIONS
**
**       Permissions to use when creating a directory for storing the
**       lock proxy files, only used when LOCKPROXYDIR is not set.
**    
**    
** As mentioned above, when compiled with SQLITE_PREFER_PROXY_LOCKING,
** setting the environment variable SQLITE_FORCE_PROXY_LOCKING to 1 will
** force proxy locking to be used for every database file opened, and 0
** will force automatic proxy locking to be disabled for all database
** files (explicitly calling the SQLITE_FCNTL_SET_LOCKPROXYFILE pragma or
** sqlite_file_control API is not affected by SQLITE_FORCE_PROXY_LOCKING).
*/

/*
** Proxy locking is only available on MacOSX 
*/
#if defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE

/*
** The proxyLockingContext has the path and file structures for the remote 
** and local proxy files in it
*/
typedef struct proxyLockingContext proxyLockingContext;
struct proxyLockingContext {
  unixFile *conchFile;         /* Open conch file */
  char *conchFilePath;         /* Name of the conch file */
  unixFile *lockProxy;         /* Open proxy lock file */
  char *lockProxyPath;         /* Name of the proxy lock file */
  char *dbPath;                /* Name of the open file */
  int conchHeld;               /* 1 if the conch is held, -1 if lockless */
  int nFails;                  /* Number of conch taking failures */
  void *oldLockingContext;     /* Original lockingcontext to restore on close */
  sqlite3_io_methods const *pOldMethod;     /* Original I/O methods for close */
};

/* 
** The proxy lock file path for the database at dbPath is written into lPath, 
** which must point to valid, writable memory large enough for a maxLen length
** file path. 
*/
static int proxyGetLockPath(const char *dbPath, char *lPath, size_t maxLen){
  int len;
  int dbLen;
  int i;

#ifdef LOCKPROXYDIR
  len = strlcpy(lPath, LOCKPROXYDIR, maxLen);
#else
# ifdef _CS_DARWIN_USER_TEMP_DIR
  {
    if( !confstr(_CS_DARWIN_USER_TEMP_DIR, lPath, maxLen) ){
      OSTRACE(("GETLOCKPATH  failed %s errno=%d pid=%d\n",
               lPath, errno, osGetpid(0)));
      return SQLITE_IOERR_LOCK;
    }
    len = strlcat(lPath, "sqliteplocks", maxLen);    
  }
# else
  len = strlcpy(lPath, "/tmp/", maxLen);
# endif
#endif

  if( lPath[len-1]!='/' ){
    len = strlcat(lPath, "/", maxLen);
  }
  
  /* transform the db path to a unique cache name */
  dbLen = (int)strlen(dbPath);
  for( i=0; i<dbLen && (i+len+7)<(int)maxLen; i++){
    char c = dbPath[i];
    lPath[i+len] = (c=='/')?'_':c;
  }
  lPath[i+len]='\0';
  strlcat(lPath, ":auto:", maxLen);
  OSTRACE(("GETLOCKPATH  proxy lock path=%s pid=%d\n", lPath, osGetpid(0)));
  return SQLITE_OK;
}

/* 
 ** Creates the lock file and any missing directories in lockPath
 */
static int proxyCreateLockPath(const char *lockPath){
  int i, len;
  char buf[MAXPATHLEN];
  int start = 0;
  
  assert(lockPath!=NULL);
  /* try to create all the intermediate directories */
  len = (int)strlen(lockPath);
  buf[0] = lockPath[0];
  for( i=1; i<len; i++ ){
    if( lockPath[i] == '/' && (i - start > 0) ){
      /* only mkdir if leaf dir != "." or "/" or ".." */
      if( i-start>2 || (i-start==1 && buf[start] != '.' && buf[start] != '/') 
         || (i-start==2 && buf[start] != '.' && buf[start+1] != '.') ){
        buf[i]='\0';
        if( osMkdir(buf, SQLITE_DEFAULT_PROXYDIR_PERMISSIONS) ){
          int err=errno;
          if( err!=EEXIST ) {
            OSTRACE(("CREATELOCKPATH  FAILED creating %s, "
                     "'%s' proxy lock path=%s pid=%d\n",
                     buf, strerror(err), lockPath, osGetpid(0)));
            return err;
          }
        }
      }
      start=i+1;
    }
    buf[i] = lockPath[i];
  }
  OSTRACE(("CREATELOCKPATH  proxy lock path=%s pid=%d\n", lockPath, osGetpid(0)));
  return 0;
}

/*
** Create a new VFS file descriptor (stored in memory obtained from
** sqlite3_malloc) and open the file named "path" in the file descriptor.
**
** The caller is responsible not only for closing the file descriptor
** but also for freeing the memory associated with the file descriptor.
*/
static int proxyCreateUnixFile(
    const char *path,        /* path for the new unixFile */
    unixFile **ppFile,       /* unixFile created and returned by ref */
    int islockfile           /* if non zero missing dirs will be created */
) {
  int fd = -1;
  unixFile *pNew;
  int rc = SQLITE_OK;
  int openFlags = O_RDWR | O_CREAT;
  sqlite3_vfs dummyVfs;
  int terrno = 0;
  UnixUnusedFd *pUnused = NULL;

  /* 1. first try to open/create the file
  ** 2. if that fails, and this is a lock file (not-conch), try creating
  ** the parent directories and then try again.
  ** 3. if that fails, try to open the file read-only
  ** otherwise return BUSY (if lock file) or CANTOPEN for the conch file
  */
  pUnused = findReusableFd(path, openFlags);
  if( pUnused ){
    fd = pUnused->fd;
  }else{
    pUnused = sqlite3_malloc64(sizeof(*pUnused));
    if( !pUnused ){
      return SQLITE_NOMEM;
    }
  }
  if( fd<0 ){
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
    if( fd<0 && errno==ENOENT && islockfile ){
      if( proxyCreateLockPath(path) == SQLITE_OK ){
        fd = robust_open(path, openFlags, 0);
      }
    }
  }
  if( fd<0 ){
    openFlags = O_RDONLY;
    fd = robust_open(path, openFlags, 0);
    terrno = errno;
  }
  if( fd<0 ){
    if( islockfile ){
      return SQLITE_BUSY;
    }
    switch (terrno) {
      case EACCES:
        return SQLITE_PERM;
      case EIO: 
        return SQLITE_IOERR_LOCK; /* even though it is the conch */
      default:
        return SQLITE_CANTOPEN_BKPT;
    }
  }
  
  pNew = (unixFile *)sqlite3_malloc64(sizeof(*pNew));
  if( pNew==NULL ){
    rc = SQLITE_NOMEM;
    goto end_create_proxy;
  }
  memset(pNew, 0, sizeof(unixFile));
  pNew->openFlags = openFlags;
  memset(&dummyVfs, 0, sizeof(dummyVfs));
  dummyVfs.pAppData = (void*)&autolockIoFinder;
  dummyVfs.zName = "dummy";
  pUnused->fd = fd;
  pUnused->flags = openFlags;
  pNew->pUnused = pUnused;
  
  rc = fillInUnixFile(&dummyVfs, fd, (sqlite3_file*)pNew, path, 0);
  if( rc==SQLITE_OK ){
    *ppFile = pNew;
    return SQLITE_OK;
  }
end_create_proxy:    
  robust_close(pNew, fd, __LINE__);
  sqlite3_free(pNew);
  sqlite3_free(pUnused);
  return rc;
}

#ifdef SQLITE_TEST
/* simulate multiple hosts by creating unique hostid file paths */
SQLITE_API int sqlite3_hostid_num = 0;
#endif

#define PROXY_HOSTIDLEN    16  /* conch file host id length */

#ifdef HAVE_GETHOSTUUID
/* Not always defined in the headers as it ought to be */
extern int gethostuuid(uuid_t id, const struct timespec *wait);
#endif

/* get the host ID via gethostuuid(), pHostID must point to PROXY_HOSTIDLEN 
** bytes of writable memory.
*/
static int proxyGetHostID(unsigned char *pHostID, int *pError){
  assert(PROXY_HOSTIDLEN == sizeof(uuid_t));
  memset(pHostID, 0, PROXY_HOSTIDLEN);
#ifdef HAVE_GETHOSTUUID
  {
    struct timespec timeout = {1, 0}; /* 1 sec timeout */
    if( gethostuuid(pHostID, &timeout) ){
      int err = errno;
      if( pError ){
        *pError = err;
      }
      return SQLITE_IOERR;
    }
  }
#else
  UNUSED_PARAMETER(pError);
#endif
#ifdef SQLITE_TEST
  /* simulate multiple hosts by creating unique hostid file paths */
  if( sqlite3_hostid_num != 0){
    pHostID[0] = (char)(pHostID[0] + (char)(sqlite3_hostid_num & 0xFF));
  }
#endif
  
  return SQLITE_OK;
}

/* The conch file contains the header, host id and lock file path
 */
#define PROXY_CONCHVERSION 2   /* 1-byte header, 16-byte host id, path */
#define PROXY_HEADERLEN    1   /* conch file header length */
#define PROXY_PATHINDEX    (PROXY_HEADERLEN+PROXY_HOSTIDLEN)
#define PROXY_MAXCONCHLEN  (PROXY_HEADERLEN+PROXY_HOSTIDLEN+MAXPATHLEN)

/* 
** Takes an open conch file, copies the contents to a new path and then moves 
** it back.  The newly created file's file descriptor is assigned to the
** conch file structure and finally the original conch file descriptor is 
** closed.  Returns zero if successful.
*/
static int proxyBreakConchLock(unixFile *pFile, uuid_t myHostID){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  char tPath[MAXPATHLEN];
  char buf[PROXY_MAXCONCHLEN];
  char *cPath = pCtx->conchFilePath;
  size_t readLen = 0;
  size_t pathLen = 0;
  char errmsg[64] = "";
  int fd = -1;
  int rc = -1;
  UNUSED_PARAMETER(myHostID);

  /* create a new path by replace the trailing '-conch' with '-break' */
  pathLen = strlcpy(tPath, cPath, MAXPATHLEN);
  if( pathLen>MAXPATHLEN || pathLen<6 || 
     (strlcpy(&tPath[pathLen-5], "break", 6) != 5) ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"path error (len %d)",(int)pathLen);
    goto end_breaklock;
  }
  /* read the conch content */
  readLen = osPread(conchFile->h, buf, PROXY_MAXCONCHLEN, 0);
  if( readLen<PROXY_PATHINDEX ){
    sqlite3_snprintf(sizeof(errmsg),errmsg,"read error (len %d)",(int)readLen);
    goto end_breaklock;
  }
  /* write it out to the temporary break file */
  fd = robust_open(tPath, (O_RDWR|O_CREAT|O_EXCL), 0);
  if( fd<0 ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "create failed (%d)", errno);
    goto end_breaklock;
  }
  if( osPwrite(fd, buf, readLen, 0) != (ssize_t)readLen ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "write failed (%d)", errno);
    goto end_breaklock;
  }
  if( rename(tPath, cPath) ){
    sqlite3_snprintf(sizeof(errmsg), errmsg, "rename failed (%d)", errno);
    goto end_breaklock;
  }
  rc = 0;
  fprintf(stderr, "broke stale lock on %s\n", cPath);
  robust_close(pFile, conchFile->h, __LINE__);
  conchFile->h = fd;
  conchFile->openFlags = O_RDWR | O_CREAT;

end_breaklock:
  if( rc ){
    if( fd>=0 ){
      osUnlink(tPath);
      robust_close(pFile, fd, __LINE__);
    }
    fprintf(stderr, "failed to break stale lock on %s, %s\n", cPath, errmsg);
  }
  return rc;
}

/* Take the requested lock on the conch file and break a stale lock if the 
** host id matches.
*/
static int proxyConchLock(unixFile *pFile, uuid_t myHostID, int lockType){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  unixFile *conchFile = pCtx->conchFile;
  int rc = SQLITE_OK;
  int nTries = 0;
  struct timespec conchModTime;
  
  memset(&conchModTime, 0, sizeof(conchModTime));
  do {
    rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
    nTries ++;
    if( rc==SQLITE_BUSY ){
      /* If the lock failed (busy):
       * 1st try: get the mod time of the conch, wait 0.5s and try again. 
       * 2nd try: fail if the mod time changed or host id is different, wait 
       *           10 sec and try again
       * 3rd try: break the lock unless the mod time has changed.
       */
      struct stat buf;
      if( osFstat(conchFile->h, &buf) ){
        storeLastErrno(pFile, errno);
        return SQLITE_IOERR_LOCK;
      }
      
      if( nTries==1 ){
        conchModTime = buf.st_mtimespec;
        usleep(500000); /* wait 0.5 sec and try the lock again*/
        continue;  
      }

      assert( nTries>1 );
      if( conchModTime.tv_sec != buf.st_mtimespec.tv_sec || 
         conchModTime.tv_nsec != buf.st_mtimespec.tv_nsec ){
        return SQLITE_BUSY;
      }
      
      if( nTries==2 ){  
        char tBuf[PROXY_MAXCONCHLEN];
        int len = osPread(conchFile->h, tBuf, PROXY_MAXCONCHLEN, 0);
        if( len<0 ){
          storeLastErrno(pFile, errno);
          return SQLITE_IOERR_LOCK;
        }
        if( len>PROXY_PATHINDEX && tBuf[0]==(char)PROXY_CONCHVERSION){
          /* don't break the lock if the host id doesn't match */
          if( 0!=memcmp(&tBuf[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN) ){
            return SQLITE_BUSY;
          }
        }else{
          /* don't break the lock on short read or a version mismatch */
          return SQLITE_BUSY;
        }
        usleep(10000000); /* wait 10 sec and try the lock again */
        continue; 
      }
      
      assert( nTries==3 );
      if( 0==proxyBreakConchLock(pFile, myHostID) ){
        rc = SQLITE_OK;
        if( lockType==EXCLUSIVE_LOCK ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, SHARED_LOCK);
        }
        if( !rc ){
          rc = conchFile->pMethod->xLock((sqlite3_file*)conchFile, lockType);
        }
      }
    }
  } while( rc==SQLITE_BUSY && nTries<3 );
  
  return rc;
}

/* Takes the conch by taking a shared lock and read the contents conch, if 
** lockPath is non-NULL, the host ID and lock file path must match.  A NULL 
** lockPath means that the lockPath in the conch file will be used if the 
** host IDs match, or a new lock path will be generated automatically 
** and written to the conch file.
*/
static int proxyTakeConch(unixFile *pFile){
  proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext; 
  
  if( pCtx->conchHeld!=0 ){
    return SQLITE_OK;
  }else{
    unixFile *conchFile = pCtx->conchFile;
    uuid_t myHostID;
    int pError = 0;
    char readBuf[PROXY_MAXCONCHLEN];
    char lockPath[MAXPATHLEN];
    char *tempLockPath = NULL;
    int rc = SQLITE_OK;
    int createConch = 0;
    int hostIdMatch = 0;
    int readLen = 0;
    int tryOldLockPath = 0;
    int forceNewLockPath = 0;
    
    OSTRACE(("TAKECONCH  %d for %s pid=%d\n", conchFile->h,
             (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"),
             osGetpid(0)));

    rc = proxyGetHostID(myHostID, &pError);
    if( (rc&0xff)==SQLITE_IOERR ){
      storeLastErrno(pFile, pError);
      goto end_takeconch;
    }
    rc = proxyConchLock(pFile, myHostID, SHARED_LOCK);
    if( rc!=SQLITE_OK ){
      goto end_takeconch;
    }
    /* read the existing conch file */
    readLen = seekAndRead((unixFile*)conchFile, 0, readBuf, PROXY_MAXCONCHLEN);
    if( readLen<0 ){
      /* I/O error: lastErrno set by seekAndRead */
      storeLastErrno(pFile, conchFile->lastErrno);
      rc = SQLITE_IOERR_READ;
      goto end_takeconch;
    }else if( readLen<=(PROXY_HEADERLEN+PROXY_HOSTIDLEN) || 
             readBuf[0]!=(char)PROXY_CONCHVERSION ){
      /* a short read or version format mismatch means we need to create a new 
      ** conch file. 
      */
      createConch = 1;
    }
    /* if the host id matches and the lock path already exists in the conch
    ** we'll try to use the path there, if we can't open that path, we'll 
    ** retry with a new auto-generated path 
    */
    do { /* in case we need to try again for an :auto: named lock file */

      if( !createConch && !forceNewLockPath ){
        hostIdMatch = !memcmp(&readBuf[PROXY_HEADERLEN], myHostID, 
                                  PROXY_HOSTIDLEN);
        /* if the conch has data compare the contents */
        if( !pCtx->lockProxyPath ){
          /* for auto-named local lock file, just check the host ID and we'll
           ** use the local lock file path that's already in there
           */
          if( hostIdMatch ){
            size_t pathLen = (readLen - PROXY_PATHINDEX);
            
            if( pathLen>=MAXPATHLEN ){
              pathLen=MAXPATHLEN-1;
            }
            memcpy(lockPath, &readBuf[PROXY_PATHINDEX], pathLen);
            lockPath[pathLen] = 0;
            tempLockPath = lockPath;
            tryOldLockPath = 1;
            /* create a copy of the lock path if the conch is taken */
            goto end_takeconch;
          }
        }else if( hostIdMatch
               && !strncmp(pCtx->lockProxyPath, &readBuf[PROXY_PATHINDEX],
                           readLen-PROXY_PATHINDEX)
        ){
          /* conch host and lock path match */
          goto end_takeconch; 
        }
      }
      
      /* if the conch isn't writable and doesn't match, we can't take it */
      if( (conchFile->openFlags&O_RDWR) == 0 ){
        rc = SQLITE_BUSY;
        goto end_takeconch;
      }
      
      /* either the conch didn't match or we need to create a new one */
      if( !pCtx->lockProxyPath ){
        proxyGetLockPath(pCtx->dbPath, lockPath, MAXPATHLEN);
        tempLockPath = lockPath;
        /* create a copy of the lock path _only_ if the conch is taken */
      }
      
      /* update conch with host and path (this will fail if other process
      ** has a shared lock already), if the host id matches, use the big
      ** stick.
      */
      futimes(conchFile->h, NULL);
      if( hostIdMatch && !createConch ){
        if( conchFile->pInode && conchFile->pInode->nShared>1 ){
          /* We are trying for an exclusive lock but another thread in this
           ** same process is still holding a shared lock. */
          rc = SQLITE_BUSY;
        } else {          
          rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
        }
      }else{
        rc = proxyConchLock(pFile, myHostID, EXCLUSIVE_LOCK);
      }
      if( rc==SQLITE_OK ){
        char writeBuffer[PROXY_MAXCONCHLEN];
        int writeSize = 0;
        
        writeBuffer[0] = (char)PROXY_CONCHVERSION;
        memcpy(&writeBuffer[PROXY_HEADERLEN], myHostID, PROXY_HOSTIDLEN);
        if( pCtx->lockProxyPath!=NULL ){
          strlcpy(&writeBuffer[PROXY_PATHINDEX], pCtx->lockProxyPath,
                  MAXPATHLEN);
        }else{
          strlcpy(&writeBuffer[PROXY_PATHINDEX], tempLockPath, MAXPATHLEN);
        }
        writeSize = PROXY_PATHINDEX + strlen(&writeBuffer[PROXY_PATHINDEX]);
        robust_ftruncate(conchFile->h, writeSize);
        rc = unixWrite((sqlite3_file *)conchFile, writeBuffer, writeSize, 0);
        fsync(conchFile->h);
        /* If we created a new conch file (not just updated the contents of a 
         ** valid conch file), try to match the permissions of the database 
         */
        if( rc==SQLITE_OK && createConch ){
          struct stat buf;
          int err = osFstat(pFile->h, &buf);
          if( err==0 ){
            mode_t cmode = buf.st_mode&(S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP |
                                        S_IROTH|S_IWOTH);
            /* try to match the database file R/W permissions, ignore failure */
#ifndef SQLITE_PROXY_DEBUG
            osFchmod(conchFile->h, cmode);
#else
            do{
              rc = osFchmod(conchFile->h, cmode);
            }while( rc==(-1) && errno==EINTR );
            if( rc!=0 ){
              int code = errno;
              fprintf(stderr, "fchmod %o FAILED with %d %s\n",
                      cmode, code, strerror(code));
            } else {
              fprintf(stderr, "fchmod %o SUCCEDED\n",cmode);
            }
          }else{
            int code = errno;
            fprintf(stderr, "STAT FAILED[%d] with %d %s\n", 
                    err, code, strerror(code));
#endif
          }
        }
      }
      conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, SHARED_LOCK);
      
    end_takeconch:
      OSTRACE(("TRANSPROXY: CLOSE  %d\n", pFile->h));
      if( rc==SQLITE_OK && pFile->openFlags ){
        int fd;
        if( pFile->h>=0 ){
          robust_close(pFile, pFile->h, __LINE__);
        }
        pFile->h = -1;
        fd = robust_open(pCtx->dbPath, pFile->openFlags, 0);
        OSTRACE(("TRANSPROXY: OPEN  %d\n", fd));
        if( fd>=0 ){
          pFile->h = fd;
        }else{
          rc=SQLITE_CANTOPEN_BKPT; /* SQLITE_BUSY? proxyTakeConch called
           during locking */
        }
      }
      if( rc==SQLITE_OK && !pCtx->lockProxy ){
        char *path = tempLockPath ? tempLockPath : pCtx->lockProxyPath;
        rc = proxyCreateUnixFile(path, &pCtx->lockProxy, 1);
        if( rc!=SQLITE_OK && rc!=SQLITE_NOMEM && tryOldLockPath ){
          /* we couldn't create the proxy lock file with the old lock file path
           ** so try again via auto-naming 
           */
          forceNewLockPath = 1;
          tryOldLockPath = 0;
          continue; /* go back to the do {} while start point, try again */
        }
      }
      if( rc==SQLITE_OK ){
        /* Need to make a copy of path if we extracted the value
         ** from the conch file or the path was allocated on the stack
         */
        if( tempLockPath ){
          pCtx->lockProxyPath = sqlite3DbStrDup(0, tempLockPath);
          if( !pCtx->lockProxyPath ){
            rc = SQLITE_NOMEM;
          }
        }
      }
      if( rc==SQLITE_OK ){
        pCtx->conchHeld = 1;
        
        if( pCtx->lockProxy->pMethod == &afpIoMethods ){
          afpLockingContext *afpCtx;
          afpCtx = (afpLockingContext *)pCtx->lockProxy->lockingContext;
          afpCtx->dbPath = pCtx->lockProxyPath;
        }
      } else {
        conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
      }
      OSTRACE(("TAKECONCH  %d %s\n", conchFile->h,
               rc==SQLITE_OK?"ok":"failed"));
      return rc;
    } while (1); /* in case we need to retry the :auto: lock file - 
                 ** we should never get here except via the 'continue' call. */
  }
}

/*
** If pFile holds a lock on a conch file, then release that lock.
*/
static int proxyReleaseConch(unixFile *pFile){
  int rc = SQLITE_OK;         /* Subroutine return code */
  proxyLockingContext *pCtx;  /* The locking context for the proxy lock */
  unixFile *conchFile;        /* Name of the conch file */

  pCtx = (proxyLockingContext *)pFile->lockingContext;
  conchFile = pCtx->conchFile;
  OSTRACE(("RELEASECONCH  %d for %s pid=%d\n", conchFile->h,
           (pCtx->lockProxyPath ? pCtx->lockProxyPath : ":auto:"), 
           osGetpid(0)));
  if( pCtx->conchHeld>0 ){
    rc = conchFile->pMethod->xUnlock((sqlite3_file*)conchFile, NO_LOCK);
  }
  pCtx->conchHeld = 0;
  OSTRACE(("RELEASECONCH  %d %s\n", conchFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}

/*
** Given the name of a database file, compute the name of its conch file.
** Store the conch filename in memory obtained from sqlite3_malloc64().
** Make *pConchPath point to the new name.  Return SQLITE_OK on success
** or SQLITE_NOMEM if unable to obtain memory.
**
** The caller is responsible for ensuring that the allocated memory
** space is eventually freed.
**
** *pConchPath is set to NULL if a memory allocation error occurs.
*/
static int proxyCreateConchPathname(char *dbPath, char **pConchPath){
  int i;                        /* Loop counter */
  int len = (int)strlen(dbPath); /* Length of database filename - dbPath */
  char *conchPath;              /* buffer in which to construct conch name */

  /* Allocate space for the conch filename and initialize the name to
  ** the name of the original database file. */  
  *pConchPath = conchPath = (char *)sqlite3_malloc64(len + 8);
  if( conchPath==0 ){
    return SQLITE_NOMEM;
  }
  memcpy(conchPath, dbPath, len+1);
  
  /* now insert a "." before the last / character */
  for( i=(len-1); i>=0; i-- ){
    if( conchPath[i]=='/' ){
      i++;
      break;
    }
  }
  conchPath[i]='.';
  while ( i<len ){
    conchPath[i+1]=dbPath[i];
    i++;
  }

  /* append the "-conch" suffix to the file */
  memcpy(&conchPath[i+1], "-conch", 7);
  assert( (int)strlen(conchPath) == len+7 );

  return SQLITE_OK;
}


/* Takes a fully configured proxy locking-style unix file and switches
** the local lock file path 
*/
static int switchLockProxyPath(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
  char *oldPath = pCtx->lockProxyPath;
  int rc = SQLITE_OK;

  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }  

  /* nothing to do if the path is NULL, :auto: or matches the existing path */
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ||
    (oldPath && !strncmp(oldPath, path, MAXPATHLEN)) ){
    return SQLITE_OK;
  }else{
    unixFile *lockProxy = pCtx->lockProxy;
    pCtx->lockProxy=NULL;
    pCtx->conchHeld = 0;
    if( lockProxy!=NULL ){
      rc=lockProxy->pMethod->xClose((sqlite3_file *)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
    }
    sqlite3_free(oldPath);
    pCtx->lockProxyPath = sqlite3DbStrDup(0, path);
  }
  
  return rc;
}

/*
** pFile is a file that has been opened by a prior xOpen call.  dbPath
** is a string buffer at least MAXPATHLEN+1 characters in size.
**
** This routine find the filename associated with pFile and writes it
** int dbPath.
*/
static int proxyGetDbPathForUnixFile(unixFile *pFile, char *dbPath){
#if defined(__APPLE__)
  if( pFile->pMethod == &afpIoMethods ){
    /* afp style keeps a reference to the db path in the filePath field 
    ** of the struct */
    assert( (int)strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, ((afpLockingContext *)pFile->lockingContext)->dbPath,
            MAXPATHLEN);
  } else
#endif
  if( pFile->pMethod == &dotlockIoMethods ){
    /* dot lock style uses the locking context to store the dot lock
    ** file path */
    int len = strlen((char *)pFile->lockingContext) - strlen(DOTLOCK_SUFFIX);
    memcpy(dbPath, (char *)pFile->lockingContext, len + 1);
  }else{
    /* all other styles use the locking context to store the db file path */
    assert( strlen((char*)pFile->lockingContext)<=MAXPATHLEN );
    strlcpy(dbPath, (char *)pFile->lockingContext, MAXPATHLEN);
  }
  return SQLITE_OK;
}

/*
** Takes an already filled in unix file and alters it so all file locking 
** will be performed on the local proxy lock file.  The following fields
** are preserved in the locking context so that they can be restored and 
** the unix structure properly cleaned up at close time:
**  ->lockingContext
**  ->pMethod
*/
static int proxyTransformUnixFile(unixFile *pFile, const char *path) {
  proxyLockingContext *pCtx;
  char dbPath[MAXPATHLEN+1];       /* Name of the database file */
  char *lockPath=NULL;
  int rc = SQLITE_OK;
  
  if( pFile->eFileLock!=NO_LOCK ){
    return SQLITE_BUSY;
  }
  proxyGetDbPathForUnixFile(pFile, dbPath);
  if( !path || path[0]=='\0' || !strcmp(path, ":auto:") ){
    lockPath=NULL;
  }else{
    lockPath=(char *)path;
  }
  
  OSTRACE(("TRANSPROXY  %d for %s pid=%d\n", pFile->h,
           (lockPath ? lockPath : ":auto:"), osGetpid(0)));

  pCtx = sqlite3_malloc64( sizeof(*pCtx) );
  if( pCtx==0 ){
    return SQLITE_NOMEM;
  }
  memset(pCtx, 0, sizeof(*pCtx));

  rc = proxyCreateConchPathname(dbPath, &pCtx->conchFilePath);
  if( rc==SQLITE_OK ){
    rc = proxyCreateUnixFile(pCtx->conchFilePath, &pCtx->conchFile, 0);
    if( rc==SQLITE_CANTOPEN && ((pFile->openFlags&O_RDWR) == 0) ){
      /* if (a) the open flags are not O_RDWR, (b) the conch isn't there, and
      ** (c) the file system is read-only, then enable no-locking access.
      ** Ugh, since O_RDONLY==0x0000 we test for !O_RDWR since unixOpen asserts
      ** that openFlags will have only one of O_RDONLY or O_RDWR.
      */
      struct statfs fsInfo;
      struct stat conchInfo;
      int goLockless = 0;

      if( osStat(pCtx->conchFilePath, &conchInfo) == -1 ) {
        int err = errno;
        if( (err==ENOENT) && (statfs(dbPath, &fsInfo) != -1) ){
          goLockless = (fsInfo.f_flags&MNT_RDONLY) == MNT_RDONLY;
        }
      }
      if( goLockless ){
        pCtx->conchHeld = -1; /* read only FS/ lockless */
        rc = SQLITE_OK;
      }
    }
  }  
  if( rc==SQLITE_OK && lockPath ){
    pCtx->lockProxyPath = sqlite3DbStrDup(0, lockPath);
  }

  if( rc==SQLITE_OK ){
    pCtx->dbPath = sqlite3DbStrDup(0, dbPath);
    if( pCtx->dbPath==NULL ){
      rc = SQLITE_NOMEM;
    }
  }
  if( rc==SQLITE_OK ){
    /* all memory is allocated, proxys are created and assigned, 
    ** switch the locking context and pMethod then return.
    */
    pCtx->oldLockingContext = pFile->lockingContext;
    pFile->lockingContext = pCtx;
    pCtx->pOldMethod = pFile->pMethod;
    pFile->pMethod = &proxyIoMethods;
  }else{
    if( pCtx->conchFile ){ 
      pCtx->conchFile->pMethod->xClose((sqlite3_file *)pCtx->conchFile);
      sqlite3_free(pCtx->conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath); 
    sqlite3_free(pCtx);
  }
  OSTRACE(("TRANSPROXY  %d %s\n", pFile->h,
           (rc==SQLITE_OK ? "ok" : "failed")));
  return rc;
}


/*
** This routine handles sqlite3_file_control() calls that are specific
** to proxy locking.
*/
static int proxyFileControl(sqlite3_file *id, int op, void *pArg){
  switch( op ){
    case SQLITE_FCNTL_GET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      if( pFile->pMethod == &proxyIoMethods ){
        proxyLockingContext *pCtx = (proxyLockingContext*)pFile->lockingContext;
        proxyTakeConch(pFile);
        if( pCtx->lockProxyPath ){
          *(const char **)pArg = pCtx->lockProxyPath;
        }else{
          *(const char **)pArg = ":auto: (not held)";
        }
      } else {
        *(const char **)pArg = NULL;
      }
      return SQLITE_OK;
    }
    case SQLITE_FCNTL_SET_LOCKPROXYFILE: {
      unixFile *pFile = (unixFile*)id;
      int rc = SQLITE_OK;
      int isProxyStyle = (pFile->pMethod == &proxyIoMethods);
      if( pArg==NULL || (const char *)pArg==0 ){
        if( isProxyStyle ){
          /* turn off proxy locking - not supported.  If support is added for
          ** switching proxy locking mode off then it will need to fail if
          ** the journal mode is WAL mode. 
          */
          rc = SQLITE_ERROR /*SQLITE_PROTOCOL? SQLITE_MISUSE?*/;
        }else{
          /* turn off proxy locking - already off - NOOP */
          rc = SQLITE_OK;
        }
      }else{
        const char *proxyPath = (const char *)pArg;
        if( isProxyStyle ){
          proxyLockingContext *pCtx = 
            (proxyLockingContext*)pFile->lockingContext;
          if( !strcmp(pArg, ":auto:") 
           || (pCtx->lockProxyPath &&
               !strncmp(pCtx->lockProxyPath, proxyPath, MAXPATHLEN))
          ){
            rc = SQLITE_OK;
          }else{
            rc = switchLockProxyPath(pFile, proxyPath);
          }
        }else{
          /* turn on proxy file locking */
          rc = proxyTransformUnixFile(pFile, proxyPath);
        }
      }
      return rc;
    }
    default: {
      assert( 0 );  /* The call assures that only valid opcodes are sent */
    }
  }
  /*NOTREACHED*/
  return SQLITE_ERROR;
}

/*
** Within this division (the proxying locking implementation) the procedures
** above this point are all utilities.  The lock-related methods of the
** proxy-locking sqlite3_io_method object follow.
*/


/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, set *pResOut
** to a non-zero value otherwise *pResOut is set to zero.  The return value
** is set to SQLITE_OK unless an I/O error occurs during lock checking.
*/
static int proxyCheckReservedLock(sqlite3_file *id, int *pResOut) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      return proxy->pMethod->xCheckReservedLock((sqlite3_file*)proxy, pResOut);
    }else{ /* conchHeld < 0 is lockless */
      pResOut=0;
    }
  }
  return rc;
}

/*
** Lock the file with the lock specified by parameter eFileLock - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  Use the sqlite3OsUnlock()
** routine to lower a locking level.
*/
static int proxyLock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xLock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}


/*
** Lower the locking level on file descriptor pFile to eFileLock.  eFileLock
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
*/
static int proxyUnlock(sqlite3_file *id, int eFileLock) {
  unixFile *pFile = (unixFile*)id;
  int rc = proxyTakeConch(pFile);
  if( rc==SQLITE_OK ){
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    if( pCtx->conchHeld>0 ){
      unixFile *proxy = pCtx->lockProxy;
      rc = proxy->pMethod->xUnlock((sqlite3_file*)proxy, eFileLock);
      pFile->eFileLock = proxy->eFileLock;
    }else{
      /* conchHeld < 0 is lockless */
    }
  }
  return rc;
}

/*
** Close a file that uses proxy locks.
*/
static int proxyClose(sqlite3_file *id) {
  if( id ){
    unixFile *pFile = (unixFile*)id;
    proxyLockingContext *pCtx = (proxyLockingContext *)pFile->lockingContext;
    unixFile *lockProxy = pCtx->lockProxy;
    unixFile *conchFile = pCtx->conchFile;
    int rc = SQLITE_OK;
    
    if( lockProxy ){
      rc = lockProxy->pMethod->xUnlock((sqlite3_file*)lockProxy, NO_LOCK);
      if( rc ) return rc;
      rc = lockProxy->pMethod->xClose((sqlite3_file*)lockProxy);
      if( rc ) return rc;
      sqlite3_free(lockProxy);
      pCtx->lockProxy = 0;
    }
    if( conchFile ){
      if( pCtx->conchHeld ){
        rc = proxyReleaseConch(pFile);
        if( rc ) return rc;
      }
      rc = conchFile->pMethod->xClose((sqlite3_file*)conchFile);
      if( rc ) return rc;
      sqlite3_free(conchFile);
    }
    sqlite3DbFree(0, pCtx->lockProxyPath);
    sqlite3_free(pCtx->conchFilePath);
    sqlite3DbFree(0, pCtx->dbPath);
    /* restore the original locking context and pMethod then close it */
    pFile->lockingContext = pCtx->oldLockingContext;
    pFile->pMethod = pCtx->pOldMethod;
    sqlite3_free(pCtx);
    return pFile->pMethod->xClose(id);
  }
  return SQLITE_OK;
}



#endif /* defined(__APPLE__) && SQLITE_ENABLE_LOCKING_STYLE */
/*
** The proxy locking style is intended for use with AFP filesystems.
** And since AFP is only supported on MacOSX, the proxy locking is also
** restricted to MacOSX.
** 
**
******************* End of the proxy lock implementation **********************
******************************************************************************/

/*
** Initialize the operating system interface.
**
** This routine registers all VFS implementations for unix-like operating
** systems.  This routine, and the sqlite3_os_end() routine that follows,
** should be the only routines in this file that are visible from other
** files.
**
** This routine is called once during SQLite initialization and by a
** single thread.  The memory allocation and mutex subsystems have not
** necessarily been initialized when this routine is called, and so they
** should not be used.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_init(void){ 
  /* 
  ** The following macro defines an initializer for an sqlite3_vfs object.
  ** The name of the VFS is NAME.  The pAppData is a pointer to a pointer
  ** to the "finder" function.  (pAppData is a pointer to a pointer because
  ** silly C90 rules prohibit a void* from being cast to a function pointer
  ** and so we have to go through the intermediate pointer to avoid problems
  ** when compiling with -pedantic-errors on GCC.)
  **
  ** The FINDER parameter to this macro is the name of the pointer to the
  ** finder-function.  The finder-function returns a pointer to the
  ** sqlite_io_methods object that implements the desired locking
  ** behaviors.  See the division above that contains the IOMETHODS
  ** macro for addition information on finder-functions.
  **
  ** Most finders simply return a pointer to a fixed sqlite3_io_methods
  ** object.  But the "autolockIoFinder" available on MacOSX does a little
  ** more than that; it looks at the filesystem type that hosts the 
  ** database file and tries to choose an locking method appropriate for
  ** that filesystem time.
  */
  #define UNIXVFS(VFSNAME, FINDER) {                        \
    3,                    /* iVersion */                    \
    sizeof(unixFile),     /* szOsFile */                    \
    MAX_PATHNAME,         /* mxPathname */                  \
    0,                    /* pNext */                       \
    VFSNAME,              /* zName */                       \
    (void*)&FINDER,       /* pAppData */                    \
    unixOpen,             /* xOpen */                       \
    unixDelete,           /* xDelete */                     \
    unixAccess,           /* xAccess */                     \
    unixFullPathname,     /* xFullPathname */               \
    unixDlOpen,           /* xDlOpen */                     \
    unixDlError,          /* xDlError */                    \
    unixDlSym,            /* xDlSym */                      \
    unixDlClose,          /* xDlClose */                    \
    unixRandomness,       /* xRandomness */                 \
    unixSleep,            /* xSleep */                      \
    unixCurrentTime,      /* xCurrentTime */                \
    unixGetLastError,     /* xGetLastError */               \
    unixCurrentTimeInt64, /* xCurrentTimeInt64 */           \
    unixSetSystemCall,    /* xSetSystemCall */              \
    unixGetSystemCall,    /* xGetSystemCall */              \
    unixNextSystemCall,   /* xNextSystemCall */             \
  }

  /*
  ** All default VFSes for unix are contained in the following array.
  **
  ** Note that the sqlite3_vfs.pNext field of the VFS object is modified
  ** by the SQLite core when the VFS is registered.  So the following
  ** array cannot be const.
  */
  static sqlite3_vfs aVfs[] = {
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix",          autolockIoFinder ),
#elif OS_VXWORKS
    UNIXVFS("unix",          vxworksIoFinder ),
#else
    UNIXVFS("unix",          posixIoFinder ),
#endif
    UNIXVFS("unix-none",     nolockIoFinder ),
    UNIXVFS("unix-dotfile",  dotlockIoFinder ),
    UNIXVFS("unix-excl",     posixIoFinder ),
#if OS_VXWORKS
    UNIXVFS("unix-namedsem", semIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE || OS_VXWORKS
    UNIXVFS("unix-posix",    posixIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE
    UNIXVFS("unix-flock",    flockIoFinder ),
#endif
#if SQLITE_ENABLE_LOCKING_STYLE && defined(__APPLE__)
    UNIXVFS("unix-afp",      afpIoFinder ),
    UNIXVFS("unix-nfs",      nfsIoFinder ),
    UNIXVFS("unix-proxy",    proxyIoFinder ),
#endif
  };
  unsigned int i;          /* Loop counter */

  /* Double-check that the aSyscall[] array has been constructed
  ** correctly.  See ticket [bb3a86e890c8e96ab] */
  assert( ArraySize(aSyscall)==25 );

  /* Register all VFSes defined in the aVfs[] array */
  for(i=0; i<(sizeof(aVfs)/sizeof(sqlite3_vfs)); i++){
    sqlite3_vfs_register(&aVfs[i], i==0);
  }
  return SQLITE_OK; 
}

/*
** Shutdown the operating system interface.
**
** Some operating systems might need to do some cleanup in this routine,
** to release dynamically allocated objects.  But not on unix.
** This routine is a no-op for unix.
*/
SQLITE_API int SQLITE_STDCALL sqlite3_os_end(void){ 
  return SQLITE_OK; 
}
 
#endif /* SQLITE_OS_UNIX */

/************** End of os_unix.c *********************************************/
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
/******************************************************************************
** This file is an amalgamation of many separate C source files from SQLite
** version 3.9.2.  By combining all the individual C code files into this 
** single large file, the entire code can be compiled as a single translation
** unit.  This allows many compilers to do optimizations that would not be
** possible if the files were compiled separately.  Performance improvements
** of 5% or more are commonly seen when SQLite is compiled as a single
** translation unit.
**
** This file is all you need to compile SQLite.  To use SQLite in other
** programs, you need this file and the "sqlite3.h" header file that defines
** the programming interface to the SQLite library.  (If you do not have 
** the "sqlite3.h" header file at hand, you will find a copy embedded within
** the text of this file.  Search for "Begin file sqlite3.h" to find the start
** of the embedded sqlite3.h header file.) Additional code files may be needed
** if you want a wrapper to interface SQLite with your choice of programming
** language. The code for the "sqlite3" command-line shell is also in a
** separate file. This file contains only code for the core SQLite library.
*/
#define SQLITE_CORE 1
#define SQLITE_AMALGAMATION 1
#ifndef SQLITE_PRIVATE
# define SQLITE_PRIVATE static
#endif
