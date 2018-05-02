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

/* hotlisttask.o - AWeb hotlist AwebLib module */

#include "aweblib.h"
#include <exec/resident.h>
#include <clib/exec_protos.h>

void *AwebPluginBase;
struct ExecBase *SysBase;
void *DOSBase,*IntuitionBase,*GfxBase,*UtilityBase;
struct ClassLibrary *WindowBase,*LayoutBase,*ButtonBase,*ListBrowserBase,
   *StringBase,*ChooserBase,*CheckBoxBase,*SpaceBase,*LabelBase,*GlyphBase,
   *DrawListBase;

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

__asm __saveds void Hotviewtask(
   register __a0 struct Hotwindow *how);

__asm __saveds void Hotmgrtask(
   register __a0 struct Hotwindow *how);

/* Function declarations for project dependent hook functions */
static ULONG Initaweblib(struct Library *libbase);
static void Expungeaweblib(struct Library *libbase);

struct Library *HotlistBase;

static APTR libseglist;

LONG __saveds __asm Libstart(void)
{  return -1;
}

static APTR functable[]=
{  Openlib,
   Closelib,
   Expungelib,
   Extfunclib,
   Hotviewtask,
   Hotmgrtask,
   (APTR)-1
};

/* Init table used in library initialization. */
static ULONG inittab[]=
{  sizeof(struct Library),
   (ULONG) functable,
   0,
   (ULONG) Initlib
};

static char __aligned libname[]="hotlist.aweblib";
static char __aligned libid[]="hotlist.aweblib " AWEBLIBVSTRING " " __AMIGADATE__;

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
   HotlistBase=libbase;
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
   if(!(IntuitionBase=OpenLibrary("intuition.library",39))) return FALSE;
   if(!(GfxBase=OpenLibrary("graphics.library",39))) return FALSE;
   if(!(UtilityBase=OpenLibrary("utility.library",39))) return FALSE;
   if(!(WindowBase=(struct ClassLibrary *)OpenLibrary("window.class",OSNEED(0,44)))) return FALSE;
   if(!(LayoutBase=(struct ClassLibrary *)OpenLibrary("gadgets/layout.gadget",OSNEED(0,44)))) return FALSE;
   if(!(ButtonBase=(struct ClassLibrary *)OpenLibrary("gadgets/button.gadget",OSNEED(0,44)))) return FALSE;
   if(!(ListBrowserBase=(struct ClassLibrary *)OpenLibrary("gadgets/listbrowser.gadget",OSNEED(0,44)))) return FALSE;
   if(!(StringBase=(struct ClassLibrary *)OpenLibrary("gadgets/string.gadget",OSNEED(0,44)))) return FALSE;
   if(!(ChooserBase=(struct ClassLibrary *)OpenLibrary("gadgets/chooser.gadget",OSNEED(0,44)))) return FALSE;
   if(!(CheckBoxBase=(struct ClassLibrary *)OpenLibrary("gadgets/checkbox.gadget",OSNEED(0,44)))) return FALSE;
   if(!(SpaceBase=(struct ClassLibrary *)OpenLibrary("gadgets/space.gadget",OSNEED(0,44)))) return FALSE;
   if(!(LabelBase=(struct ClassLibrary *)OpenLibrary("images/label.image",OSNEED(0,44)))) return FALSE;
   if(!(GlyphBase=(struct ClassLibrary *)OpenLibrary("images/glyph.image",OSNEED(0,44)))) return FALSE;
   if(!(DrawListBase=(struct ClassLibrary *)OpenLibrary("images/drawlist.image",OSNEED(0,44)))) return FALSE;
   return TRUE;
}

static void Expungeaweblib(struct Library *libbase)
{  if(LabelBase) CloseLibrary(LabelBase);
   if(DrawListBase) CloseLibrary(DrawListBase);
   if(GlyphBase) CloseLibrary(GlyphBase);
   if(SpaceBase) CloseLibrary(SpaceBase);
   if(CheckBoxBase) CloseLibrary(CheckBoxBase);
   if(ChooserBase) CloseLibrary(ChooserBase);
   if(StringBase) CloseLibrary(StringBase);
   if(ListBrowserBase) CloseLibrary(ListBrowserBase);
   if(ButtonBase) CloseLibrary(ButtonBase);
   if(LayoutBase) CloseLibrary(LayoutBase);
   if(WindowBase) CloseLibrary(WindowBase);
   if(UtilityBase) CloseLibrary(UtilityBase);
   if(GfxBase) CloseLibrary(GfxBase);
   if(IntuitionBase) CloseLibrary(IntuitionBase);
   if(DOSBase) CloseLibrary(DOSBase);
}

