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

/* awebjfif.c - AWeb jfif plugin main */

#include "pluginlib.h"
#include "awebjfif.h"
#include <libraries/awebplugin.h>
#include <clib/awebplugin_protos.h>
#include <clib/exec_protos.h>
#include <pragmas/awebplugin_pragmas.h>
#include <pragmas/exec_sysbase_pragmas.h>

void *DISBase,*GfxBase,*UtilityBase,*CyberGfxBase,*AwebPluginBase;

ULONG Initpluginlib(struct AwebJfifBase *base)
{  DOSBase=OpenLibrary("dos.library",39);
   GfxBase=OpenLibrary("graphics.library",39);
   UtilityBase=OpenLibrary("utility.library",39);
   CyberGfxBase=OpenLibrary("cybergraphics.library",0);
   AwebPluginBase=OpenLibrary("awebplugin.library",0);
   return (ULONG)(DOSBase && GfxBase && UtilityBase && AwebPluginBase);
}

void Expungepluginlib(struct AwebJfifBase *base)
{  if(base->sourcedriver) Amethod(NULL,AOM_INSTALL,base->sourcedriver,NULL);
   if(base->copydriver) Amethod(NULL,AOM_INSTALL,base->copydriver,NULL);
   if(CyberGfxBase) CloseLibrary(CyberGfxBase);
   if(AwebPluginBase) CloseLibrary(AwebPluginBase);
   if(UtilityBase) CloseLibrary(UtilityBase);
   if(GfxBase) CloseLibrary(GfxBase);
   if(DOSBase) CloseLibrary(DOSBase);
}

__asm __saveds ULONG Initplugin(register __a0 struct Plugininfo *pi)
{  if(!PluginBase->sourcedriver)
   {  PluginBase->sourcedriver=Amethod(NULL,AOM_INSTALL,0,Dispatchsource);
   }
   if(!PluginBase->copydriver)
   {  PluginBase->copydriver=Amethod(NULL,AOM_INSTALL,0,Dispatchcopy);
   }
   pi->sourcedriver=PluginBase->sourcedriver;
   pi->copydriver=PluginBase->copydriver;
   return (ULONG)(PluginBase->sourcedriver && PluginBase->copydriver);
}
