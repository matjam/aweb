/**********************************************************************
 * 
 * This file is part of the AWeb-II distribution
 *
 * Copyright (C) 2002 Yvon Rozijn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the AWeb Public License as included in this
 * distribution.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * AWeb Public License for more details.
 *
 **********************************************************************/

/* amissl.c - AWeb SSL function library. Compile this with AmiSSL SDK */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <clib/exec_protos.h>
#include "aweb.h"
#include "awebssl.h"
#include "task.h"
#include <proto/amissl.h>
#include <pragmas/amissl_pragmas.h>
#include <amissl.h>
//#define NO_BLOWFISH
#include <ssl.h>
#include <err.h>
#include <crypto.h>

/*-----------------------------------------------------------------------*/

struct Assl
{  struct Library *amisslbase;
   SSL_CTX *sslctx;
   SSL *ssl;
   UBYTE *hostname;
   BOOL denied;
};

/*-----------------------------------------------------------------------*/

struct Assl *Assl_initamissl(struct Library *socketbase)
{  struct Library *AmiSSLBase;
   struct Assl *assl=NULL;
#ifndef DEMOVERSION
   if(assl=ALLOCSTRUCT(Assl,1,MEMF_CLEAR))
   {  if(AmiSSLBase=assl->amisslbase=OpenLibrary("amissl.library",1))
      {  if(InitAmiSSL(AmiSSL_Version,AmiSSL_CurrentVersion,
            AmiSSL_Revision,AmiSSL_CurrentRevision,
            AmiSSL_SocketBase,socketbase,
            TAG_END))
         {  CloseLibrary(AmiSSLBase);
            assl->amisslbase=NULL;
         }
      }
      if(!assl->amisslbase)
      {  FREE(assl);
         assl=NULL;
      }
   }
#endif
   return assl;
}

/*-----------------------------------------------------------------------*/

static int __saveds __stdargs
   Certcallback(int ok,X509_STORE_CTX *sctx)
{  char *s=NULL,*u=NULL;
   struct Assl *assl;
   X509 *xs;
   int err;
   char buf[256];
   if(!ok)
   {  assl=Gettaskuserdata();
      if(assl)
      {  struct Library *AmiSSLBase=assl->amisslbase;
         xs=X509_STORE_CTX_get_current_cert(sctx);
         err=X509_STORE_CTX_get_error(sctx);
         X509_NAME_oneline(X509_get_subject_name(xs),buf,sizeof(buf));
         s=buf;         
         u=assl->hostname;
         ok=Httpcertaccept(u,s);
         if(!ok) assl->denied=TRUE;
      }
   }
   return ok;
}

__asm void Assl_cleanup(register __a0 struct Assl *assl)
{  if(assl)
   {  if(assl->amisslbase)
      {  struct Library *AmiSSLBase=assl->amisslbase;
         CleanupAmiSSL(TAG_END);
         CloseLibrary(assl->amisslbase);
      }
      FREE(assl);
   }
}

__asm BOOL Assl_openssl(register __a0 struct Assl *assl)
{  if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      SSLeay_add_ssl_algorithms();
      SSL_load_error_strings();
      if(assl->sslctx=SSL_CTX_new(SSLv23_client_method()))
      {  SSL_CTX_set_default_verify_paths(assl->sslctx);
         SSL_CTX_set_verify(assl->sslctx,/*SSL_VERIFY_PEER|*/SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
            Certcallback);
         if(assl->ssl=SSL_new(assl->sslctx))
         {  Settaskuserdata(assl);
         }
      }
   }
   return (BOOL)(assl && assl->ssl);
}

__asm void Assl_closessl(register __a0 struct Assl *assl)
{  if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      if(assl->ssl)
      {  /* SSL_Shutdown(assl->ssl); */
         SSL_free(assl->ssl);
         assl->ssl=NULL;
      }
      if(assl->sslctx)
      {  SSL_CTX_free(assl->sslctx);
         assl->sslctx=NULL;
      }
   }
}

__asm long Assl_connect(register __a0 struct Assl *assl,
   register __d0 long sock,
   register __a1 UBYTE *hostname)
{  long result=ASSLCONNECT_FAIL;
   if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      assl->hostname=hostname;
      if(SSL_set_fd(assl->ssl,sock))
      {  if(SSL_connect(assl->ssl)>=0)
         {  result=ASSLCONNECT_OK;
         }
         else if(assl->denied)
         {  result=ASSLCONNECT_DENIED;
         }
      }
   }
   return result;
}

__asm char *Assl_geterror(register __a0 struct Assl *assl,
   register __a1 char *errbuf)
{  long err;
   UBYTE *p=NULL;
   short i;
   if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      ERR_load_SSL_strings();
      err=ERR_get_error();
      ERR_error_string(err,errbuf);
      /* errbuf now contains something like: 
         "error:1408806E:SSL routines:SSL_SET_CERTIFICATE:certificate verify failed"
         Find the descriptive text after the 4th colon. */
      for(i=0,p=errbuf;i<4;i++)
      {  p=strchr(p,':');
         if(!p) break;
         p++;
      }
   }
   if(!p) p=errbuf;
   return (char *)p;
}

__asm long Assl_write(register __a0 struct Assl *assl,
   register __a1 char *buffer,
   register __d0 long length)
{  long result=-1;
   if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      result=SSL_write(assl->ssl,buffer,length);
   }
   return result;
}

__asm long Assl_read(register __a0 struct Assl *assl,
   register __a1 char *buffer,
   register __d0 long length)
{  long result=-1;
   if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      result=SSL_read(assl->ssl,buffer,length);
   }
   return result;
}

__asm char *Assl_getcipher(register __a0 struct Assl *assl)
{  char *result=NULL;
   if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      result=(char *)SSL_get_cipher(assl->ssl);
   }
   return result;
}

__asm char *Assl_libname(register __a0 struct Assl *assl)
{  char *result=NULL;
   if(assl && assl->amisslbase)
   {  struct Library *AmiSSLBase=assl->amisslbase;
      result=(char *)AmiSSLBase->lib_IdString;
   }
   return result;
}

__asm void Assl_dummy(void)
{  return;
}

/*-----------------------------------------------------------------------*/

static UBYTE version[]="AwebAmiSSL.library";

struct Jumptab
{  UWORD jmp;
   void *function;
};
#define JMP 0x4ef9

static struct Jumptab jumptab[]=
{
   JMP,Assl_libname,
   JMP,Assl_getcipher,
   JMP,Assl_read,
   JMP,Assl_write,
   JMP,Assl_geterror,
   JMP,Assl_connect,
   JMP,Assl_closessl,
   JMP,Assl_openssl,
   JMP,Assl_cleanup,
   JMP,Assl_dummy, /* Extfunc */
   JMP,Assl_dummy, /* Expunge */
   JMP,Assl_dummy, /* Close */
   JMP,Assl_dummy, /* Open */
};
static struct Library awebamissllib=
{  {  NULL,NULL,NT_LIBRARY,0,version },
   0,0,
   sizeof(jumptab),
   sizeof(struct Library),
   1,0,
   version,
   0,0
};

struct Library *AwebAmisslBase=&awebamissllib;
