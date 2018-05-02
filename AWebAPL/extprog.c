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

/* extprog.c - AWeb spawn external program source driver object */

#include "aweb.h"
#include "source.h"
#include "sourcedriver.h"
#include "url.h"
#include "file.h"
#include "application.h"
#include <clib/utility_protos.h>

/*------------------------------------------------------------------------*/

struct Extprog
{  struct Aobject object;
   void *source;              /* The source for this driver */
   void *file;                /* File to save in */
   UBYTE *name;               /* Command to start */
   UBYTE *args;               /* Command arguments */
   UBYTE mimetype[32];        /* MIME type */
   USHORT flags;
};

#define EXPF_PIPE    0x0001   /* Should use a pipe */

/*------------------------------------------------------------------------*/

static long Setextprog(struct Extprog *exp,struct Amset *ams)
{  long result;
   struct TagItem *tag,*tstate=ams->tags;
   result=Amethodas(AOTP_OBJECT,exp,AOM_SET,ams->tags);
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOSDV_Source:
            exp->source=(void *)tag->ti_Data;
            break;
         case AOSDV_Name:
            exp->name=Dupstr((UBYTE *)tag->ti_Data,-1);
            break;
         case AOSDV_Arguments:
            exp->args=Dupstr((UBYTE *)tag->ti_Data,-1);
            break;
         case AOSDV_Pipe:
            SETFLAG(exp->flags,EXPF_PIPE,tag->ti_Data);
            break;
         case AOURL_Contenttype:
            strncpy(exp->mimetype,(UBYTE *)tag->ti_Data,31);
            break;
      }
   }
   return result;
}

static struct Extprog *Newextprog(struct Amset *ams)
{  struct Extprog *exp;
   if(exp=Allocobject(AOTP_EXTPROG,sizeof(struct Extprog),ams))
   {  Setextprog(exp,ams);
   }
   return exp;
}

static long Getextprog(struct Extprog *exp,struct Amset *ams)
{  long result;
   struct TagItem *tag,*tstate=ams->tags;
   result=AmethodasA(AOTP_OBJECT,exp,ams);
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOSDV_Source:
            PUTATTR(tag,exp->source);
            break;
         case AOSDV_Volatile:
            PUTATTR(tag,TRUE);
            break;
      }
   }
   return result;
}

static long Srcupdateextprog(struct Extprog *exp,struct Amsrcupdate *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   long length=0;
   UBYTE *data=NULL;
   BOOL eof=FALSE;
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOURL_Data:
            data=(UBYTE *)tag->ti_Data;
            break;
         case AOURL_Datalength:
            length=tag->ti_Data;
            break;
         case AOURL_Reload:
            if(exp->file)
            {  Adisposeobject(exp->file);
               exp->file=NULL;
            }
            break;
         case AOURL_Eof:
            eof=TRUE;
            break;
      }
   }
   if((data || eof) && !exp->file)
   {  void *url=(void *)Agetattr(exp->source,AOSRC_Url);
      UBYTE *urlname=(UBYTE *)Agetattr(url,AOURL_Url);
      UBYTE *ext=urlname?Urlfileext(urlname):NULLSTRING;
      if((exp->flags&EXPF_PIPE) && exp->name && exp->args)
      {  if(exp->file=Anewobject(AOTP_FILE,
            AOFIL_Pipe,TRUE,
            AOFIL_Extension,ext,
            TAG_END))
         {  Spawn(FALSE,exp->name,exp->args,"fnum",
               Agetattr(exp->file,AOFIL_Name),
               Agetattr(Aweb(),AOAPP_Screenname),
               Agetattr((void *)Agetattr(exp->source,AOSRC_Url),AOURL_Url),
               exp->mimetype);
         }
      }
      else
      {  exp->file=Anewobject(AOTP_FILE,
            AOFIL_Extension,ext,
            TAG_END);
      }
      if(ext) FREE(ext);
   }
   if(data)
   {  if(exp->file)
      {  Asetattrs(exp->file,
            AOFIL_Data,data,
            AOFIL_Datalength,length,
            TAG_END);
      }
   }
   if(eof)
   {  if(exp->file)
      {  Asetattrs(exp->file,AOFIL_Eof,TRUE,TAG_END);
         if(!(exp->flags&EXPF_PIPE)
         && exp->name && exp->args
         && Spawn(TRUE,exp->name,exp->args,"fnum",
               Agetattr(exp->file,AOFIL_Name),
               Agetattr(Aweb(),AOAPP_Screenname),
               Agetattr((void *)Agetattr(exp->source,AOSRC_Url),AOURL_Url),
               exp->mimetype))
         {  Asetattrs(exp->file,AOFIL_Delete,FALSE,TRUE);
         }
         Adisposeobject(exp->file);
         exp->file=NULL;
      }
   }
   return 0;
}

static void Disposeextprog(struct Extprog *exp)
{  if(exp->name) FREE(exp->name);
   if(exp->args) FREE(exp->args);
   if(exp->file) Adisposeobject(exp->file);
   Amethodas(AOTP_OBJECT,exp,AOM_DISPOSE);
}

static long Dispatch(struct Extprog *exp,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newextprog((struct Amset *)amsg);
         break;
      case AOM_SET:
         result=Setextprog(exp,(struct Amset *)amsg);
         break;
      case AOM_GET:
         result=Getextprog(exp,(struct Amset *)amsg);
         break;
      case AOM_SRCUPDATE:
         result=Srcupdateextprog(exp,(struct Amsrcupdate *)amsg);
         break;
      case AOM_DISPOSE:
         Disposeextprog(exp);
         break;
      case AOM_DEINSTALL:
         break;
   }
   return result;
}

/*------------------------------------------------------------------------*/

BOOL Installextprog(void)
{  if(!Amethod(NULL,AOM_INSTALL,AOTP_EXTPROG,Dispatch)) return FALSE;
   return TRUE;
}
