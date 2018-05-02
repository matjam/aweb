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

/* tcperr.c - aweb common tcp errors */

#include "aweb.h"
#include "tcperr.h"
#include "url.h"
#include "fetch.h"
#include "fetchdriver.h"
#include "task.h"

static ULONG error[]=
{  0,
   MSG_EPART_NOLIB,        /* TCPERR_NOLIB */
   MSG_EPART_NOHOST,       /* TCPERR_NOHOST */
   MSG_EPART_NOCONNECT,    /* TCPERR_NOCONNECT */
   MSG_EPART_NOFILE,       /* TCPERR_NOFILE */
   MSG_EPART_XAWEB,        /* TCPERR_XAWEB */
   MSG_EPART_NOLOGIN,      /* TCPERR_NOLOGIN */
};

static ULONG message[]=
{  0,
   MSG_AWEB_LOOKUP,        /* TCPMSG_LOOKUP */
   MSG_AWEB_CONNECT,       /* TCPMSG_CONNECT */
   MSG_AWEB_WAITING,       /* TCPMSG_WAITING */
   MSG_AWEB_TCPSTART,      /* TCPMSG_TCPSTART */
   MSG_AWEB_LOGIN,         /* TCPMSG_LOGIN */
   MSG_AWEB_NEWSGROUP,     /* TCPMSG_NEWSGROUP */
   MSG_AWEB_NEWSSCAN,      /* TCPMSG_NEWSSCAN */
   MSG_AWEB_NEWSSORT,      /* TCPMSG_NEWSSORT */
   MSG_AWEB_NEWSPOST,      /* TCPMSG_NEWSPOST */
   MSG_AWEB_MAILSEND,      /* TCPMSG_MAILSEND */
   MSG_AWEB_UPLOAD,        /* TCPMSG_UPLOAD */
};

/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/

void TcperrorA(struct Fetchdriver *fd,ULONG err,ULONG *args)
{  void *url=(void *)Agetattr(fd->fetch,AOFCH_Url);
   long length;
   if(haiku)
   {  UBYTE *p="";
      strcpy(fd->block,"<html>");
      switch(err)
      {  case TCPERR_NOLIB:p=HAIKU3;break;
         case TCPERR_NOHOST:p=HAIKU4;break;
         case TCPERR_NOCONNECT:p=HAIKU5;break;
         case TCPERR_NOFILE:p=HAIKU6;break;
         case TCPERR_XAWEB:p=HAIKU7;break;
         case TCPERR_NOLOGIN:p=HAIKU8;break;
      }
      strcat(fd->block,p);
      length=strlen(fd->block);
   }
   else
   {  length=sprintf(fd->block,"<html><h1>%s</h1>%s: %.1000s<p><strong>",
            AWEBSTR(MSG_EPART_ERROR),AWEBSTR(MSG_EPART_RETURL),Agetattr(url,AOURL_Url));
      length+=vsprintf(fd->block+length,AWEBSTR(error[err]),(va_list)args);
      length+=sprintf(fd->block+length,"</strong>");
   }
   Updatetaskattrs(
      AOURL_Contenttype,"TEXT/HTML",
      AOURL_Error,TRUE,
      AOURL_Data,fd->block,
      AOURL_Datalength,length,
      TAG_END);
}


void Tcperror(struct Fetchdriver *fd,ULONG err,...)
{  TcperrorA(fd,err,VARARG(err));
}

void TcpmessageA(struct Fetchdriver *fd,ULONG msg,ULONG *args)
{  vsprintf(fd->block,AWEBSTR(message[msg]),(va_list)args);
   Updatetaskattrs(
      AOURL_Status,fd->block,
      TAG_END);
}

void Tcpmessage(struct Fetchdriver *fd,ULONG msg,...)
{  TcpmessageA(fd,msg,VARARG(msg));
}