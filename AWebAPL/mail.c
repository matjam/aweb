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

/* mail.c - AWeb mailer */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <clib/exec_protos.h>
#include "aweblib.h"
#include "tcperr.h"
#include "fetchdriver.h"
#include "task.h"
#include "application.h"
#include "awebtcp.h"
#include <exec/resident.h>
#include <libraries/asl.h>
#include <graphics/displayinfo.h>
#include <clib/asl_protos.h>
#include <clib/graphics_protos.h>
#include <classact.h>

#ifndef LOCALONLY

struct Library *DOSBase,*UtilityBase,*AslBase,*GfxBase,*AwebTcpBase;
void *AwebPluginBase;

/*-----------------------------------------------------------------------*/
/* AWebLib module startup */

__asm __saveds struct Library *Initlib(
   register __a6 struct ExecBase *sysbase,
   register __a0 struct SegList *seglist,
   register __d0 struct Library *libbase);

__asm __saveds struct Library *Openlib(
   register __a6 struct Library *libbase);

__asm __saveds struct SegList *Closelib(
   register __a6 struct Library *libbase);

__asm __saveds struct SegList *Expungelib(
   register __a6 struct Library *libbase);

__asm __saveds ULONG Extfunclib(void);

__asm __saveds void Fetchdrivertask(
   register __a0 struct Fetchdriver *fd);

/* Function declarations for project dependent hook functions */
static ULONG Initaweblib(struct Library *libbase);
static void Expungeaweblib(struct Library *libbase);

struct Library *MailBase;

static APTR libseglist;

struct ExecBase *SysBase;

LONG __saveds __asm Libstart(void)
{  return -1;
}

static APTR functable[]=
{  Openlib,
   Closelib,
   Expungelib,
   Extfunclib,
   Fetchdrivertask,
   (APTR)-1
};

/* Init table used in library initialization. */
static ULONG inittab[]=
{  sizeof(struct Library),
   (ULONG) functable,
   0,
   (ULONG) Initlib
};

static char __aligned libname[]="mail.aweblib";
static char __aligned libid[]="mail.aweblib " AWEBLIBVSTRING " " __AMIGADATE__;

/* The ROM tag */
struct Resident __aligned romtag=
{  RTC_MATCHWORD,
   &romtag,
   &romtag+1,
   RTF_AUTOINIT,
   AWEBLIBVERSION,
   NT_LIBRARY,
   0,
   libname,
   libid,
   inittab
};

__asm __saveds struct Library *Initlib(
   register __a6 struct ExecBase *sysbase,
   register __a0 struct SegList *seglist,
   register __d0 struct Library *libbase)
{  SysBase=sysbase;
   MailBase=libbase;
   libbase->lib_Revision=AWEBLIBREVISION;
   libseglist=seglist;
   if(!Initaweblib(libbase))
   {  Expungeaweblib(libbase);
      libbase=NULL;
   }
   return libbase;
}

__asm __saveds struct Library *Openlib(
   register __a6 struct Library *libbase)
{  libbase->lib_OpenCnt++;
   libbase->lib_Flags&=~LIBF_DELEXP;
   if(libbase->lib_OpenCnt==1)
   {  AwebPluginBase=OpenLibrary("awebplugin.library",0);
   }
#ifndef DEMOVERSION
   if(!Fullversion())
   {  Closelib(libbase);
      return NULL;
   }
#endif
   return libbase;
}

__asm __saveds struct SegList *Closelib(
   register __a6 struct Library *libbase)
{  libbase->lib_OpenCnt--;
   if(libbase->lib_OpenCnt==0)
   {  if(AwebPluginBase)
      {  CloseLibrary(AwebPluginBase);
         AwebPluginBase=NULL;
      }
      if(libbase->lib_Flags&LIBF_DELEXP)
      {  return Expungelib(libbase);
      }
   }
   return NULL;
}

__asm __saveds struct SegList *Expungelib(
   register __a6 struct Library *libbase)
{  if(libbase->lib_OpenCnt==0)
   {  ULONG size=libbase->lib_NegSize+libbase->lib_PosSize;
      UBYTE *ptr=(UBYTE *)libbase-libbase->lib_NegSize;
      Remove((struct Node *)libbase);
      Expungeaweblib(libbase);
      FreeMem(ptr,size);
      return libseglist;
   }
   libbase->lib_Flags|=LIBF_DELEXP;
   return NULL;
}

__asm __saveds ULONG Extfunclib(void)
{  return 0;
}

/*-----------------------------------------------------------------------*/

/* Convert hexadecimal */
static UBYTE Hextodec(UBYTE c)
{  UBYTE ch=0;
   if(c>='0' && c<='9') ch=c-'0';
   else
   {  c=toupper(c);
      if(c>='A' && c<='F') ch=c+10-'A';
   }
   return ch;
}

/* Remove URL escape from block. Return new length (always <= old length) */
static long Unescape(UBYTE *block,long len)
{  UBYTE *start,*p,*end;
   long newlen=len;
   start=block;
   end=block+len;
   while(start<end)
   {  for(p=start;p<end && *p!='%';p++)
      {  if(*p=='+') *p=' ';
      }
      if(p<end)
      {  /* Now *p=='%' */
         if(p<end-2)
         {  *p=(Hextodec(p[1])<<4)+Hextodec(p[2]);
            memmove(p+1,p+3,end-p-3);
            newlen-=2;
         }
      }
      start=p+1;
   }
   return newlen;
}

/* Append this block with dangerous characters HTML escaped. */
static void Addblock(struct Buffer *buf,UBYTE *block,long len)
{  UBYTE *start,*p,*end,*esc;
   start=block;
   end=block+len;
   while(start<end)
   {  for(p=start;p<end && *p!='<' && *p!='&' && *p!='\'';p++);
      Addtobuffer(buf,start,p-start);
      if(p<end)
      {  switch(*p)
         {  case '<':esc="&lt;";break;
            case '&':esc="&amp;";break;
            case '\'':esc="&#39;";break;
            default: esc="";break;
         }
         Addtobuffer(buf,esc,strlen(esc));
         p++;  /* Skip over dangerous character */
      }
      start=p;
   }
}

/* Build mail form in HTML format */
static void Buildmailform(struct Fetchdriver *fd)
{  UBYTE *to,*p,*q,*subject=NULL;
   UBYTE *params[2];
   ULONG file;
   struct Buffer buf={0};
   long len,subjlen;
   short i;
   BOOL first,formmail;
   Updatetaskattrs(
      AOURL_Contenttype,"text/html",
      TAG_END);
   to=fd->name+7; /* skip mailto: */
   if(p=strchr(to,'?'))
   {  *p++=0;
      params[0]=p;
   }
   else params[0]=NULL;
   params[1]=fd->postmsg;
   for(i=0;i<2;i++)
   {  if(params[i])
      {  p=params[i];
         for(;;)
         {  if(STRNIEQUAL(p,"SUBJECT=",8))
            {  subject=p+8;
               if(q=strchr(subject,'&')) subjlen=q-subject;
               else
               {  subjlen=strlen(subject);
                  if(p==params[i]) params[i]=NULL; /* subject is only form parameter */
               }
               break;
            }
            if(q=strchr(p,'&')) p=q+1;
            else break;
         }
      }
   }
   formmail=(params[0] || params[1]);
   p=fd->block;
   p+=sprintf(p,"<HTML>\n<HEAD>\n<TITLE>");
   p+=sprintf(p,AWEBSTR(MSG_MAIL_MAILTO_TITLE),to);
   p+=sprintf(p,"</TITLE>\n</HEAD>\n<BODY>\n<H1>");
   p+=sprintf(p,AWEBSTR(MSG_MAIL_MAILTO_HEADER),to);
   p+=sprintf(p,"</H1>\n<FORM ACTION='x-aweb:mail/1/mail' METHOD=POST>\n");
   p+=sprintf(p,"<TABLE>\n<TR>\n<TD ALIGN='right'>%s\n"
      "<TD><INPUT NAME='to' VALUE='%s' SIZE=60>\n",AWEBSTR(MSG_MAIL_TO),to);
   p+=sprintf(p,"<TR>\n<TD ALIGN='right'>%s\n"
      "<TD><INPUT NAME='subject' VALUE='",AWEBSTR(MSG_MAIL_SUBJECT));
   if(subject)
   {  Addtobuffer(&buf,fd->block,p-fd->block);
      subjlen=Unescape(subject,subjlen);
      Addblock(&buf,subject,subjlen);
      p=fd->block;
   }
   else
   {  p+=sprintf(p,"mailto:%s",to);
   }
   p+=sprintf(p,"' SIZE=60>\n");
   p+=sprintf(p,"</TABLE>\n%s"
      " (<INPUT TYPE=CHECKBOX NAME='extra' VALUE='Y'%s> %s)"
      "<BR>\n<TEXTAREA NAME='data' COLS=70 ROWS=20>\n",
      AWEBSTR(MSG_MAIL_MESSAGE_BODY),
      formmail?" CHECKED":"",
      AWEBSTR(MSG_MAIL_EXTRA_HEADERS));
   if(formmail)
   {  p+=sprintf(p,"Content-type: application/x-www-form-urlencoded\n\n");
      Addtobuffer(&buf,fd->block,p-fd->block);
      first=TRUE;
      for(i=0;i<2;i++)
      {  while(params[i] && *params[i])
         {  if(!(q=strchr(params[i],'&'))) q=params[i]+strlen(params[i]);
            if(!STRNIEQUAL(params[i],"SUBJECT=",8))
            {  if(!first) Addblock(&buf,"&",1);
               first=FALSE;
               Addblock(&buf,params[i],q-params[i]);
            }
            params[i]=q;
            if(*params[i]) params[i]++;
         }
         p=fd->block;
      }
   }
   else
   {  ObtainSemaphore(fd->prefssema);
      if(fd->prefs->sigfile)
      {  p+=sprintf(p,"&#10;\n--\n");
         Addtobuffer(&buf,fd->block,p-fd->block);
         if(file=Open(fd->prefs->sigfile,MODE_OLDFILE))
         {  first=TRUE;
            while(len=Read(file,fd->block,fd->blocksize))
            {  if(first && len>3 && STRNEQUAL(fd->block,"--\n",3))
               {  Addblock(&buf,fd->block+3,len-3);
               }
               else
               {  Addblock(&buf,fd->block,len);
               }
               first=FALSE;
            }
            Close(file);
         }
         p=fd->block;
      }
      ReleaseSemaphore(fd->prefssema);
   }
   p+=sprintf(p,"</TEXTAREA>\n<BR><BR>\n<INPUT TYPE=SUBMIT VALUE='%s'> "
      "<INPUT TYPE=RESET VALUE='%s'> "
      "<INPUT TYPE=SUBMIT NAME='Return' VALUE='%s'>\n",
      AWEBSTR(MSG_MAIL_SEND),AWEBSTR(MSG_MAIL_RESET),AWEBSTR(MSG_MAIL_RETURN));
   p+=sprintf(p,"</FORM>\n</BODY>\n</HTML>\n");
   Addtobuffer(&buf,fd->block,p-fd->block);
   Updatetaskattrs(
      AOURL_Data,buf.buffer,
      AOURL_Datalength,buf.length,
      TAG_END);
   Freebuffer(&buf);
}

/*-----------------------------------------------------------------------*/

struct Mailinfo
{  struct Buffer buf;
   UBYTE *to;
};

/* See if mail form was submitted from RETURN button */
static BOOL Findreturn(struct Fetchdriver *fd)
{  UBYTE *p;
   p=fd->postmsg;
   while(p=strchr(p,'&'))
   {  if(STRNIEQUAL(p,"&RETURN=",8)) return TRUE;
      p++;
   }
   return FALSE;
}

/* Build the mail message. Expects form fields in order:
 * To, Subject, [Inreplyto], [extra=y], Data. */
static void Buildmessage(struct Fetchdriver *fd,struct Mailinfo *mi)
{  UBYTE *p,*q,*start;
   long len;
   p=fd->postmsg;
   if(!STRNIEQUAL(p,"TO=",3)) return;
   p+=3;
   if(!(q=strchr(p,'&'))) return;
   *q++='\0';
   len=Unescape(p,q-p);
   mi->to=Dupstr(p,len);
   ObtainSemaphore(fd->prefssema);
   len=sprintf(fd->block,"From: %s (%s)\r\n",fd->prefs->emailaddr,fd->prefs->fullname);
   Addtobuffer(&mi->buf,fd->block,len);
   len=sprintf(fd->block,"To: %s\r\n",mi->to);
   Addtobuffer(&mi->buf,fd->block,len);
   p=q;
   if(!STRNIEQUAL(p,"SUBJECT=",8))
   {  ReleaseSemaphore(fd->prefssema);
      return;
   }
   p+=8;
   if(!(q=strchr(p,'&')))
   {  ReleaseSemaphore(fd->prefssema);
      return;
   }
   *q++='\0';
   Unescape(p,q-p);
   len=sprintf(fd->block,"Subject: %s\r\n",p);
   Addtobuffer(&mi->buf,fd->block,len);
   if(*fd->prefs->replyaddr)
   {  len=sprintf(fd->block,"Reply-to: %s\r\n",fd->prefs->replyaddr);
      Addtobuffer(&mi->buf,fd->block,len);
   }
   ReleaseSemaphore(fd->prefssema);
   p=q;
   if(STRNIEQUAL(p,"INREPLYTO=",10))
   {  p+=10;
      if(!(q=strchr(p,'&'))) return;
      *q++='\0';
      Unescape(p,q-p);
      len=sprintf(fd->block,"In-Reply-To: %s\r\n",p);
      Addtobuffer(&mi->buf,fd->block,len);
      p=q;
   }
   len=sprintf(fd->block,"X-Mailer: Amiga-AWeb/%s\r\n",Awebversion());
   Addtobuffer(&mi->buf,fd->block,len);
   if(STRNIEQUAL(p,"EXTRA=Y&",8))
   {  p+=8;
   }
   else
   {  Addtobuffer(&mi->buf,"\r\n",2);
   }
   if(!STRNIEQUAL(p,"DATA=",5)) return;
   p+=5;
   len=Unescape(p,strlen(p));
   p[len]='\0';
   start=p;
   for(;;)
   {  if(*p=='.')
      {  Addtobuffer(&mi->buf,start,p-start);
         Addtobuffer(&mi->buf,".",1);
         start=p;
      }
      if(!(q=strchr(p,'\r'))) break;
      q+=2; /* Skip over \r\n */
      p=q;
   }
   Addtobuffer(&mi->buf,start,strlen(start));
   p=mi->buf.buffer+mi->buf.length-2;
   if(!(p[0]=='\r' && p[1]=='\n'))
   {  Addtobuffer(&mi->buf,"\r\n",2);
   }
}

/* Get a response code */
static long Getresponse(struct Fetchdriver *fd,long sock,struct Library *SocketBase)
{  long len,result;
   len=a_recv(sock,fd->block,fd->blocksize,0,SocketBase);
   if(len<0) return 999;
   fd->block[MIN(len,fd->blocksize-1)]='\0';
   result=atoi(fd->block);
   return result;
}

/* Send a command, return the response. */
static long Sendcommand(struct Fetchdriver *fd,long sock,struct Library *SocketBase,
   UBYTE *fmt,...)
{  va_list args;
   long len;
   va_start(args,fmt);
   vsprintf(fd->block,fmt,args);
   va_end(args);
   strcat(fd->block,"\r\n");
   len=strlen(fd->block);
   if(a_send(sock,fd->block,len,0,SocketBase)!=len) return 999;
   return Getresponse(fd,sock,SocketBase);
}

enum SENDMESSAGE_RETURNS
{  SMR_FAIL,SMR_NOCONNECT,SMR_OK,
};

/* Show mail error requester. Return TRUE if retry requested. */
static BOOL Mailretry(struct Fetchdriver *fd,struct Mailinfo *mi,short err)
{  struct FileRequester *afr;
   struct Screen *scr;
   long result,file,height;
   struct DimensionInfo dim={0};
   if(err==SMR_NOCONNECT)
   {  ObtainSemaphore(fd->prefssema);
      if(*fd->prefs->smtphost)
      {  sprintf(fd->block,AWEBSTR(MSG_MAIL_NOCONNECT),fd->prefs->smtphost);
      }
      else
      {  strcpy(fd->block,AWEBSTR(MSG_MAIL_NOSMTPHOST));
      }
      ReleaseSemaphore(fd->prefssema);
   }
   else
   {  strcpy(fd->block,AWEBSTR(MSG_MAIL_MAILFAILED));
   }
   result=Syncrequest(AWEBSTR(MSG_MAIL_TITLE),fd->block,AWEBSTR(MSG_MAIL_BUTTONS));
   if(result==2) /* Save */
   {  if(scr=(struct Screen *)Agetattr(Aweb(),AOAPP_Screen))
      {  if(GetDisplayInfoData(NULL,(UBYTE *)&dim,sizeof(dim),DTAG_DIMS,
            GetVPModeID(&scr->ViewPort)))
         {  height=(dim.Nominal.MaxY-dim.Nominal.MinY+1)*4/5;
         }
         else
         {  height=scr->Height*4/5;
         }
         if(afr=AllocAslRequestTags(ASL_FileRequest,
            ASLFR_Screen,scr,
            ASLFR_TitleText,AWEBSTR(MSG_MAIL_SAVETITLE),
            ASLFR_InitialHeight,height,
            ASLFR_InitialDrawer,"RAM:",
            ASLFR_InitialFile,"MailMessage",
            ASLFR_RejectIcons,TRUE,
            ASLFR_DoSaveMode,TRUE,
            TAG_END))
         {  if(AslRequest(afr,NULL))
            {  strcpy(fd->block,afr->fr_Drawer);
               AddPart(fd->block,afr->fr_File,fd->blocksize);
               if(file=Open(fd->block,MODE_OLDFILE))
               {  Seek(file,0,OFFSET_END);
               }
               else
               {  file=Open(fd->block,MODE_NEWFILE);
               }
               if(file)
               {  Write(file,mi->buf.buffer,mi->buf.length);
                  Close(file);
               }
            }
            FreeAslRequest(afr);
         }
      }
      result=0;
   }
   return (BOOL)(result==1);
}

/* Connect and transfer the message */
static short Sendmessage(struct Fetchdriver *fd,struct Mailinfo *mi)
{  struct Library *SocketBase;
   struct hostent *hent;
   long sock;
   UBYTE name[128],*to,*p,*q;
   BOOL error=FALSE;
   short retval=SMR_FAIL;

   AwebTcpBase=Opentcp(&SocketBase,fd,TRUE);
   if(SocketBase)
   {  Updatetaskattrs(AOURL_Netstatus,NWS_LOOKUP,TAG_END);
      Tcpmessage(fd,TCPMSG_LOOKUP,fd->prefs->smtphost);
      ObtainSemaphore(fd->prefssema);
      hent=Lookup(fd->prefs->smtphost,SocketBase);
      ReleaseSemaphore(fd->prefssema);
      if(hent)
      {  if((sock=a_socket(hent->h_addrtype,SOCK_STREAM,0,SocketBase))>=0)
         {  Updatetaskattrs(AOURL_Netstatus,NWS_CONNECT,TAG_END);
            Tcpmessage(fd,TCPMSG_CONNECT,"SMTP",hent->h_name);
            if(!a_connect(sock,hent,25,SocketBase)
            && Getresponse(fd,sock,SocketBase)<400)
            {  gethostname(name,sizeof(name));
               if(Sendcommand(fd,sock,SocketBase,"HELO %s",name)<400)
               {  ObtainSemaphore(fd->prefssema);
                  strncpy(name,fd->prefs->emailaddr,sizeof(name));
                  ReleaseSemaphore(fd->prefssema);
                  Updatetaskattrs(AOURL_Netstatus,NWS_WAIT,TAG_END);
                  Tcpmessage(fd,TCPMSG_MAILSEND);
                  if(Sendcommand(fd,sock,SocketBase,"MAIL FROM:<%s>",name)<400)
                  {  to=Dupstr(mi->to,-1);
                     p=to;
                     while(p)
                     {  if(q=strchr(p,',')) *q++='\0';
                        if(Sendcommand(fd,sock,SocketBase,"RCPT TO:<%s>",p)>=400)
                        {  error=TRUE;
                           break;
                        }
                        p=q;
                     }
                     if(to) Freemem(to);
                     if(!error)
                     {  if(Sendcommand(fd,sock,SocketBase,"DATA")<400)
                        {  a_send(sock,mi->buf.buffer,mi->buf.length,0,SocketBase);
                           a_send(sock,".\r\n",3,0,SocketBase);
                           if(Getresponse(fd,sock,SocketBase)<400)
                           {  retval=SMR_OK;
                           }
                        }
                     }
                  }
               }
            }
            else retval=SMR_NOCONNECT;
            a_close(sock,SocketBase);
         }
      }
      a_cleanup(SocketBase);
      CloseLibrary(SocketBase);
   }
   else retval=SMR_NOCONNECT;
   return retval;
}

/* Build result page */
static void Buildresult(struct Fetchdriver *fd,struct Mailinfo *mi)
{  long len;
   Freebuffer(&mi->buf);
   len=sprintf(fd->block,"<HTML>\n<HEAD>\n<TITLE>");
   Addtobuffer(&mi->buf,fd->block,len);
   len=sprintf(fd->block,AWEBSTR(MSG_MAIL_MAIL_SENT_TITLE),mi->to);
   Addtobuffer(&mi->buf,fd->block,len);
   len=sprintf(fd->block,"</TITLE>\n</HEAD>\n<BODY>\n");
   Addtobuffer(&mi->buf,fd->block,len);
   len=sprintf(fd->block,AWEBSTR(MSG_MAIL_MAIL_SENT),mi->to);
   Addtobuffer(&mi->buf,fd->block,len);
   len=sprintf(fd->block,"<p>\n<FORM ACTION='x-aweb:mail/2'>\n<INPUT TYPE=SUBMIT VALUE='%s'>\n</FORM>\n",
      AWEBSTR(MSG_MAIL_RETURN));
   Addtobuffer(&mi->buf,fd->block,len);
   len=sprintf(fd->block,"</BODY>\n</HTML>\n");
   Addtobuffer(&mi->buf,fd->block,len);
   Updatetaskattrs(
      AOURL_Contenttype,"text/html",
      AOURL_Data,mi->buf.buffer,
      AOURL_Datalength,mi->buf.length,
      TAG_END);
}

/* Send mail */
static void Sendmail(struct Fetchdriver *fd)
{  struct Mailinfo mi={0};
   short r;
   if(Findreturn(fd))
   {  Updatetaskattrs(
         AOURL_Gohistory,-1,
         TAG_END);
   }
   else
   {  Buildmessage(fd,&mi);
      for(;;)
      {  r=Sendmessage(fd,&mi);
         if(r==SMR_OK) break;
         if(!Mailretry(fd,&mi,r)) break;
      }
      if(r==SMR_OK)
      {  Buildresult(fd,&mi);
      }
      Freebuffer(&mi.buf);
      if(mi.to) FREE(mi.to);
   }
}

/*-----------------------------------------------------------------------*/

__saveds __asm void Fetchdrivertask(register __a0 struct Fetchdriver *fd)
{  if(STRNIEQUAL(fd->name,"mailto:",7))
   {  Buildmailform(fd);
   }
   else if(STRNIEQUAL(fd->name,"x-aweb:mail/1/mail",18))
   {  if(fd->referer
      && (STRNIEQUAL(fd->referer,"mailto:",7)
         || STRNIEQUAL(fd->referer,"x-aweb:news/reply?",18)))
      {  Sendmail(fd);
      }
   }
   else if(STRNIEQUAL(fd->name,"x-aweb:mail/2",13))
   {  Updatetaskattrs(
         AOURL_Gohistory,-2,
         TAG_END);
   }
   Updatetaskattrs(AOTSK_Async,TRUE,
      AOURL_Eof,TRUE,
      AOURL_Terminate,TRUE,
      TAG_END);
}

/*-----------------------------------------------------------------------*/

static ULONG Initaweblib(struct Library *libbase)
{  if(!(DOSBase=OpenLibrary("dos.library",39))) return FALSE;
   if(!(AslBase=OpenLibrary("asl.library",OSNEED(39,44)))) return FALSE;
   if(!(GfxBase=OpenLibrary("graphics.library",39))) return FALSE;
   if(!(UtilityBase=OpenLibrary("utility.library",39))) return FALSE;
   return TRUE;
}

static void Expungeaweblib(struct Library *libbase)
{  if(UtilityBase) CloseLibrary(UtilityBase);
   if(GfxBase) CloseLibrary(GfxBase);
   if(AslBase) CloseLibrary(AslBase);
   if(DOSBase) CloseLibrary(DOSBase);
}

#endif /* !LOCALONLY */

