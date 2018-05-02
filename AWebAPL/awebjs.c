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

/* awebjs.c - AWeb js main */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <stdio.h>
#include <string.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <pragmas/exec_pragmas.h>
#include <pragmas/locale_pragmas.h>
#include <pragmas/dos_pragmas.h>

#include "jslib.h"
#include "keyfile.h"

char version[]="\0$VER:AWebJS " AWEBVERSION " " __AMIGADATE__;

__near long __stack=16348;

static void *AWebJSBase;
extern void *DOSBase;

/* Standard I/O */
static __saveds __asm void aWrite(register __a0 struct Jcontext *jc)
{  struct Jvar *jv;
   long n;
   for(n=0;jv=Jfargument(jc,n);n++)
   {  printf("%s",Jtostring(jc,jv));
   }
}

static __saveds __asm void aWriteln(register __a0 struct Jcontext *jc)
{  aWrite(jc);
   printf("\n");
}

static __saveds __asm void aReadln(register __a0 struct Jcontext *jc)
{  UBYTE buf[80];
   long l;
   if(!fgets(buf,80,stdin)) *buf='\0';
   l=strlen(buf);
   if(l && buf[l-1]=='\n') buf[l-1]='\0';
   Jasgstring(jc,NULL,buf);
}

static __saveds __asm BOOL Feedback(register __a0 struct Jcontext *jc)
{  if(SetSignal(0,0)&SIGBREAKF_CTRL_C) return FALSE;
   else return TRUE;
}

void main()
{  long args[4]={0};
   UBYTE *argtemplate="FILES/M/A,PUBSCREEN/K,-D=DEBUG/S";
   struct RDArgs *rda;
   UBYTE **p;
   UBYTE *source;
   FILE *f;
   long l;
   void *jc;
   void *jo;
   void *dtbase;
/* window.class bug workaround */
dtbase=OpenLibrary("datatypes.library",0);
   if(rda=ReadArgs(argtemplate,args,NULL))
   {  if((AWebJSBase=OpenLibrary("aweblib/awebjs.aweblib",0))
      || (AWebJSBase=OpenLibrary("awebjs.aweblib",0))
      || (AWebJSBase=OpenLibrary("PROGDIR:aweblib/awebjs.aweblib",0))
      || (AWebJSBase=OpenLibrary("PROGDIR:awebjs.aweblib",0)))
      {  if(jc=Newjcontext((UBYTE *)args[1]))
         {  Jsetfeedback(jc,Feedback);
            Jerrors(jc,TRUE,JERRORS_ON,TRUE);
            if(jo=Newjobject(jc))
            {  Addjfunction(jc,jo,"write",aWrite,"string",NULL);
               Addjfunction(jc,jo,"writeln",aWriteln,"string",NULL);
               Addjfunction(jc,jo,"readln",aReadln,NULL);
               if(args[2])
               {  Jdebug(jc,TRUE);
               }
               for(p=(UBYTE **)args[0];*p;p++)
               {  if(f=fopen(*p,"r"))
                  {  fseek(f,0,SEEK_END);
                     l=ftell(f);
                     fseek(f,0,SEEK_SET);
                     if(source=AllocVec(l+1,MEMF_PUBLIC|MEMF_CLEAR))
                     {  fread(source,l,1,f);
                        Runjprogram(jc,jo,source,jo,NULL,0,0);
                        FreeVec(source);
                     }
                     fclose(f);
                  }
                  else fprintf(stderr,"Can't open %s\n",*p);
               }
            }
            Freejcontext(jc);
         }
         CloseLibrary(AWebJSBase);
      }
      FreeArgs(rda);
   }
/* window.class bug workaround */
if(dtbase) CloseLibrary(dtbase);
}
