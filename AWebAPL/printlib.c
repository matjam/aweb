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

/* printlib.c - AWeb print magic and debug aweblib module */

#include "aweblib.h"
#include "print.h"
#include <exec/resident.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/intuition_protos.h>

static void *AwebPluginBase;
static struct Library *DOSBase,*UtilityBase;

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

__asm __saveds void doPrintdebugprefs(
   register __a0 struct Print *prt);

__asm __saveds void doPrintfinddimensions(
   register __a0 struct Print *prt);

__asm __saveds void doPrintprintsection(
   register __a0 struct Print *prt);

__asm __saveds void doPrintclosedebug(
   register __a0 struct Print *prt);


/* Function declarations for project dependent hook functions */
static ULONG Initaweblib(struct Library *libbase);
static void Expungeaweblib(struct Library *libbase);

struct Library *StartupBase;

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
   doPrintdebugprefs,
   doPrintfinddimensions,
   doPrintprintsection,
   doPrintclosedebug,
   (APTR)-1
};

/* Init table used in library initialization. */
static ULONG inittab[]=
{  sizeof(struct Library),
   (ULONG) functable,
   0,
   (ULONG) Initlib
};

static char __aligned libname[]="print.aweblib";
static char __aligned libid[]="print.aweblib " AWEBLIBVSTRING " " __AMIGADATE__;

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
   StartupBase=libbase;
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

static ULONG Initaweblib(struct Library *libbase)
{  if(!(DOSBase=OpenLibrary("dos.library",39))) return FALSE;
   if(!(UtilityBase=OpenLibrary("utility.library",39))) return FALSE;
   return TRUE;
}

static void Expungeaweblib(struct Library *libbase)
{  if(UtilityBase) CloseLibrary(UtilityBase);
   if(DOSBase) CloseLibrary(DOSBase);
}

/*-----------------------------------------------------------------------*/

static void Prtdebug(long fh,char *str,...)
{  VFPrintf(fh,str,(long *)VARARG(str));
}

__asm __saveds void doPrintdebugprefs(
   register __a0 struct Print *prt)
{  if(prt->flags&PRTF_DEBUG)
   {  prt->debugfile=Open("T:AWebPrintDebug.txt",MODE_NEWFILE);
      if(prt->debugfile)
      {  long f=prt->debugfile;
         struct PrinterData *pd=(struct PrinterData *)prt->ioreq->io_Device;
         struct PrinterExtendedData *ped=(struct PrinterExtendedData *)
            &pd->pd_SegmentData->ps_PED;
         struct Preferences *pr=&pd->pd_Preferences;
         Prtdebug(f,"\n--- Start\n");
         Prtdebug(f,"\nPrinter:\n");
         Prtdebug(f,"Device=%s %ld.%ld (%s)\n",pd->lib_Node.ln_Name,
            pd->lib_Version,pd->lib_Revision,pd->lib_IdString);
         Prtdebug(f,"Driver=%s\n",pd->pd_DriverName);
         Prtdebug(f,"Printer=%s\n",ped->ped_PrinterName);
         Prtdebug(f,"NumRows=%ld\n",ped->ped_NumRows);
         Prtdebug(f,"MaxXDots=%ld\n",ped->ped_MaxXDots);
         Prtdebug(f,"MaxYDots=%ld\n",ped->ped_MaxYDots);
         Prtdebug(f,"XDotsInch=%ld\n",ped->ped_XDotsInch);
         Prtdebug(f,"YDotsInch=%ld\n",ped->ped_XDotsInch);
         Prtdebug(f,"\nPreferences:\n");
         Prtdebug(f,"PaperSize=0x%04lX\n",pr->PaperSize);
         Prtdebug(f,"PaperLength=%ld\n",pr->PaperLength);
         Prtdebug(f,"PaperType=0x%04lX\n",pr->PaperType);
         Prtdebug(f,"PrintFlags=0x%04lX\n",pr->PrintFlags);
         Prtdebug(f,"PrintMaxWidth=%ld\n",pr->PrintMaxWidth);
         Prtdebug(f,"PrintMaxHeight=%ld\n",pr->PrintMaxHeight);
         Prtdebug(f,"PrintDensity=%ld\n",pr->PrintDensity);
         Prtdebug(f,"PrintXOffset=%ld\n",pr->PrintXOffset);
      }         
   }
}

/* Find rastport height so that printer rows is muptiple of printhead's rows.
 * Find largest height that match this condition.
 * Also, if printer has MaxYDots, use the height that causes the smallest number
 * of rows unprinted. If more heights result in equal remainder, use largest.
 */
static long Getdimensions(struct Print *prt,long start,long max)
{  long f=prt->debugfile;
   struct PrinterData *pd=(struct PrinterData *)prt->ioreq->io_Device;
   struct PrinterExtendedData *ped=(struct PrinterExtendedData *)&pd->pd_SegmentData->ps_PED;
   ULONG numrows=ped->ped_NumRows;
   long height,goodheight=0,minremainder,maxydots,rem;
   /* maxydots is the number of dots corresponding to the preferred page height */
   if(pd->pd_Preferences.PrintFlags&(BOUNDED_DIMENSIONS|ABSOLUTE_DIMENSIONS))
   {  maxydots=pd->pd_Preferences.PrintMaxHeight*ped->ped_YDotsInch/10;
      if(f) Prtdebug(f,"use dimensions; maxydots=%ld * %ld / 10 = %ld\n",
         pd->pd_Preferences.PrintMaxHeight,ped->ped_YDotsInch,maxydots);
   }
   else
   {  maxydots=ped->ped_MaxYDots;
      if(f) Prtdebug(f,"use default size; maxydots=%ld\n",maxydots);
   }
   if(!numrows) numrows=1;
   if(f) Prtdebug(f,"Use numrows=%ld\n",numrows);
   minremainder=maxydots;
   prt->ioreq->io_Command=PRD_DUMPRPORT;
   prt->ioreq->io_RastPort=prt->rp;
   prt->ioreq->io_ColorMap=prt->cmap;
   prt->ioreq->io_Modes=prt->screenmode;
   prt->ioreq->io_SrcX=0;
   prt->ioreq->io_SrcY=1;
   prt->ioreq->io_SrcWidth=prt->printwidth;
   for(height=max;height>=start;height--)
   {  prt->ioreq->io_SrcHeight=height-2;
      prt->ioreq->io_DestCols=(ULONG)0xffffffff/100*prt->scale;
      prt->ioreq->io_DestRows=(ULONG)0xffffffff/100*prt->scale;
      prt->ioreq->io_Special=SPECIAL_FRACCOLS|SPECIAL_FRACROWS|SPECIAL_ASPECT|SPECIAL_NOPRINT;
      DoIO(prt->ioreq);
      if(f) Prtdebug(f,"height=%ld -> DestRows=%ld\n",height,prt->ioreq->io_DestRows);
      if(maxydots && prt->ioreq->io_DestRows>maxydots) break;
      /* DestRows must be multiple of numrows, and not greater than what can maximally
       * be dumped to the printer. */
      if(prt->ioreq->io_DestRows>0 && prt->ioreq->io_DestRows%numrows==0
      && (!ped->ped_MaxYDots || prt->ioreq->io_DestRows<=ped->ped_MaxYDots))
      {  if(maxydots)
         {  /* DestRows must fit in MaxYDots with minimum remainder */
            rem=maxydots%prt->ioreq->io_DestRows;
            if(rem<minremainder)
            {  goodheight=height;
               minremainder=rem;
               prt->numstrips=maxydots/prt->ioreq->io_DestRows;
               if(f) Prtdebug(f,"  good height! remainder=%ld numstrips=%ld\n",
                  rem,prt->numstrips);
            }
            else if(f) Prtdebug(f,"  height fits but larger remainder %ld\n",rem);
         }
         else
         {  /* Use maximum height */
            goodheight=height;
            if(f) Prtdebug(f,"  good height for endless paper\n");
            break;
         }
      }
   }
   return goodheight;
}


__asm __saveds void doPrintfinddimensions(
   register __a0 struct Print *prt)
{  long f=prt->debugfile;
   short d;
   if(prt->scale<=50) d=1;
   else d=-1;
   prt->printheight=prt->numstrips=0;
   if(f) Prtdebug(f,"\n--- Calculate\n");
   while(prt->scale>0 && prt->scale<=100)
   {  if(f) Prtdebug(f,"\nScale=%ld\n",prt->scale);
      prt->printheight=Getdimensions(prt,8,PRINTWINH);
      if(prt->printheight) break;
      prt->scale+=d;
   }
   if(f)
   {  Prtdebug(f,"\nFound printheight=%ld\n",prt->printheight);
      Prtdebug(f,"\n--- Print\n");
   }
}

__asm __saveds void doPrintprintsection(
   register __a0 struct Print *prt)
{  long f=prt->debugfile;
   BOOL last=(prt->top+prt->printheight-2>=prt->totalheight);
   if(f) Prtdebug(f,"Section from line %ld\n",prt->top);
   prt->ioreq->io_Command=PRD_DUMPRPORT;
   prt->ioreq->io_RastPort=prt->rp;
   prt->ioreq->io_ColorMap=prt->cmap;
   prt->ioreq->io_Modes=prt->screenmode;
   prt->ioreq->io_SrcX=0;
   prt->ioreq->io_SrcY=1;
   prt->ioreq->io_SrcWidth=prt->printwidth;
   prt->ioreq->io_SrcHeight=last?(prt->totalheight-prt->top):prt->printheight-2;
   prt->ioreq->io_DestCols=(ULONG)0xffffffff/100*prt->scale;
   prt->ioreq->io_DestRows=(ULONG)0xffffffff/100*prt->scale;
   prt->ioreq->io_Special=SPECIAL_FRACCOLS|SPECIAL_FRACROWS|SPECIAL_ASPECT|SPECIAL_TRUSTME;
   if(prt->flags&PRTF_CENTER) prt->ioreq->io_Special|=SPECIAL_CENTER;
   if(last)
   {  if(!(prt->flags&PRTF_FORMFEED)) prt->ioreq->io_Special|=SPECIAL_NOFORMFEED;
      if(f) Prtdebug(f,"  Last section");
   }
   else
   {  if(f && prt->numstrips)
      {  Prtdebug(f,"  printing %ld of %ld per page",prt->numprinted+1,prt->numstrips);
      }
      if(!prt->numstrips || ++prt->numprinted<prt->numstrips)
      {  prt->ioreq->io_Special|=SPECIAL_NOFORMFEED;
      }
      else prt->numprinted=0;
   }
   if(f)
   {  if(prt->ioreq->io_Special&SPECIAL_NOFORMFEED) Prtdebug(f,"\n");
      else Prtdebug(f," FORM FEED\n");
   }
   SendIO(prt->ioreq);
}

__asm __saveds void doPrintclosedebug(
   register __a0 struct Print *prt)
{  if(prt->debugfile)
   {  Prtdebug(prt->debugfile,"\n--- End\n");
      Close(prt->debugfile);
      prt->debugfile=NULL;
   }
}

