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

/* memory.c AWeb memory manager */

#include "awebjs.h"

#include <clib/exec_protos.h>

/*-----------------------------------------------------------------------*/

/*
typedef char (* __asm SegTrack(register __a0 ULONG Address,
   register __a1 ULONG *SegNum,register __a2 ULONG *Offset));

struct SegSem
{  struct SignalSemaphore seg_Semaphore;
   SegTrack *seg_Find;
};

void Debugmem(UBYTE *f,void *mem,long size,void *stack)
{  UBYTE *segname;
   ULONG segnum,offset;
   ULONG *p;
   short n;
   struct SegSem *segsem;
   KPrintF("%s %08lx %6ld        ",f,mem,size);
   if(segsem=(struct SegSem *)FindSemaphore("SegTracker"))
   {  for(p=(ULONG *)stack,n=0;n<6;p++)
      {  if((segname=segsem->seg_Find(*p,&segnum,&offset)) && segnum==0)
         {  KPrintF("%ld:%08lx ",segnum,offset);
            n++;
         }
      }
   }
   KPrintF("\n");
}
*/

/*-----------------------------------------------------------------------*/

void *Pallocmem(long size,ULONG flags,void *pool)
{  void *mem;
   if(pool) mem=AllocPooled(pool,size+8);
   else mem=AllocMem(size+8,flags|MEMF_PUBLIC|MEMF_CLEAR);
//Debugmem("ALLOC",mem,size,&pool);
   if(mem)
   {  *(void **)mem=pool;
      *(long *)((ULONG)mem+4)=size+8;
      return (void *)((ULONG)mem+8);
   }
   else return NULL;
}

void Freemem(void *mem)
{  void *pool;
   long size;
   if(mem)
   {  pool=*(void **)((ULONG)mem-8);
      size=*(long *)((ULONG)mem-4);
//Debugmem("FREE ",(void *)((ULONG)mem-8),-(size-8),&mem);
      if(pool) FreePooled(pool,(void *)((ULONG)mem-8),size);
      else FreeMem((void *)((ULONG)mem-8),size);
   }
}

void *Getpool(void *p)
{  return *(void **)((ULONG)p-8);
}