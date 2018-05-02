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

/* tcp.c - AWeb TCP interface */

#ifndef LOCALONLY

#include "aweb.h"
#include "tcperr.h"
#include "application.h"
#include "awebtcp.h"
#include <dos/dostags.h>

struct SignalSemaphore tcpsema;
BOOL startedtcp=FALSE;

/*-----------------------------------------------------------------------*/

#ifndef DEMOVERSION
static long Spawnsync(UBYTE *cmd,UBYTE *args)
{  UBYTE *scrname,*cmdbuf;
   long result=100;
   long out;
   UBYTE conbuf[64];
   long pl=Copypathlist();
   scrname=(UBYTE *)Agetattr(Aweb(),AOAPP_Screenname);
   if(!args) args="";
   if(!scrname) scrname="";
   if(cmdbuf=ALLOCTYPE(UBYTE,strlen(cmd)+Pformatlength(args,"n",&scrname)+32,0))
   {  sprintf(cmdbuf,"%s ",cmd);
      Pformat(cmdbuf+strlen(cmdbuf),args,"n",&scrname,TRUE);
      strcpy(conbuf,"CON:////");
      strcat(conbuf,AWEBSTR(MSG_AWEB_EXTWINTITLE));
      strcat(conbuf,"/AUTO/CLOSE");
      out=Open(conbuf,MODE_NEWFILE);
      if(out)
      {  result=(0<=SystemTags(cmdbuf,
            SYS_Input,out,
            SYS_Output,NULL,
            NP_Path,pl,
            TAG_END));
         if(!result) Freepathlist(pl);
         Close(out);
      }
      FREE(cmdbuf);
   }
   return result;
}
#endif

/*-----------------------------------------------------------------------*/

BOOL Inittcp(void)
{  InitSemaphore(&tcpsema);
   return TRUE;
}

void Freetcp(void)
{  
#ifndef DEMOVERSION
   if(startedtcp && prefs.endtcpcmd)
   {  if(Syncrequest(AWEBSTR(MSG_REQUEST_TITLE),haiku?HAIKU21:AWEBSTR(MSG_TCP_ENDTCP),
         AWEBSTR(MSG_TCP_BUTTONS),0))
      {  Spawnsync(prefs.endtcpcmd,prefs.endtcpargs);
      }
   }
#endif
}

struct Library *Opentcp(struct Library **base,struct Fetchdriver *fd,BOOL autocon)
{  BOOL started=FALSE;
   UBYTE *cmd=NULL,*args=NULL;
   struct Process *proc=(struct Process *)FindTask(NULL);
   APTR windowptr=proc->pr_WindowPtr;
   proc->pr_WindowPtr=(APTR)-1;
#ifdef DEMOVERSION
   if(*base=Tcpopenlib())
#else
   if(!AttemptSemaphore(&tcpsema))
   {  if(autocon) Tcpmessage(fd,TCPMSG_TCPSTART);
      ObtainSemaphore(&tcpsema);
   }
   for(;;)
   {  if(*base=Tcpopenlib())
      if(*base) break;
      if(started) break;
      if(!autocon) break;
      ObtainSemaphore(&prefssema);
      if(prefs.starttcpcmd && *prefs.starttcpcmd)
      {  cmd=Dupstr(prefs.starttcpcmd,-1);
         args=Dupstr(prefs.starttcpargs,-1);
      }
      ReleaseSemaphore(&prefssema);
      if(!cmd) break;
      Tcpmessage(fd,TCPMSG_TCPSTART);
      Spawnsync(cmd,args);
      started=startedtcp=TRUE;
   }
   ReleaseSemaphore(&tcpsema);
   if(cmd) FREE(cmd);
   if(args) FREE(args);
#endif
   proc->pr_WindowPtr=windowptr;
   return ((*base)?AwebTcpBase:NULL);
}

#endif /* LOCALONLY */