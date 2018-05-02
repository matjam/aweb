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

/* timer.c - AWeb timer object */

#include "aweb.h"
#include "timer.h"
#include <devices/timer.h>
#include <clib/utility_protos.h>
#include <clib/timer_protos.h>
#include <pragmas/timer_pragmas.h>

/*------------------------------------------------------------------------*/

struct Timer
{  struct Aobject object;
   struct timerequest req;
   struct timeval val;
   USHORT flags;
   void *target;
   struct TagItem *maplist;
};

#define TIMF_IO         0x0001      /* I/O request outstanding */

static struct MsgPort *timerport;
static struct timerequest *timerequest;
static void *TimerBase;

static LIST(Timer) timers;

#define TIMERFROMREQ(tr)   (struct Timer *)((ULONG)(tr)-(ULONG)&((struct Timer *)0)->req)

/*------------------------------------------------------------------------*/

/* Process timer replies */
static void Processtimer(void)
{  struct timerequest *tr;
   struct Timer *tim;
   struct List list;
   NewList(&list);
   while(tr=(struct timerequest *)GetMsg(timerport))
   {  AddTail(&list,tr);
   }
   while(tr=(struct timerequest *)RemHead(&list))
   {  tim=TIMERFROMREQ(tr);
      tim->flags&=~TIMF_IO;
      if(tim->target)
      {  Aupdateattrs(tim->target,tim->maplist,
            AOBJ_Target,tim,
            AOTIM_Ready,TRUE,
            TAG_END);
      }
   }
}

/*------------------------------------------------------------------------*/

static long Settimer(struct Timer *tim,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   long waitsecs=0,waitmicro=0;
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOTIM_Waitseconds:
            waitsecs=tag->ti_Data;
            break;
         case AOTIM_Waitmicros:
            waitmicro=tag->ti_Data;
            break;
         case AOBJ_Target:
            tim->target=(void *)tag->ti_Data;
            break;
         case AOBJ_Map:
            tim->maplist=(struct TagItem *)tag->ti_Data;
            break;
      }
   }
   if(waitsecs || waitmicro)
   {  if(tim->flags&TIMF_IO)
      {  AbortIO(&tim->req);
         WaitIO(&tim->req);
         tim->flags&=~TIMF_IO;
      }
      tim->req.tv_secs=waitsecs;
      tim->req.tv_micro=waitmicro;
      tim->req.io_Command=TR_ADDREQUEST;
      GetSysTime(&tim->val);
      AddTime(&tim->val,&tim->req.tr_time);
      SendIO(&tim->req);
      tim->flags|=TIMF_IO;
   }
   return 0;
}

static struct Timer *Newtimer(struct Amset *ams)
{  struct Timer *tim;
   if(tim=Allocobject(AOTP_TIMER,sizeof(struct Timer),ams))
   {  tim->req=*timerequest;
      ADDTAIL(&timers,tim);
      Settimer(tim,ams);
   }
   return tim;
}

static long Gettimer(struct Timer *tim,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   struct timeval current,togo={0};
   GetSysTime(&current);
   if(tim->flags&TIMF_IO)
   {  togo=tim->val;
      SubTime(&togo,&current);
   }
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOTIM_Waitseconds:
            PUTATTR(tag,togo.tv_secs);
            break;
         case AOTIM_Waitmicros:
            PUTATTR(tag,togo.tv_micro);
            break;
         case AOTIM_Seconds:
            PUTATTR(tag,current.tv_secs);
            break;
         case AOTIM_Micros:
            PUTATTR(tag,current.tv_micro);
            break;
      }
   }
   return 0;
}

static void Disposetimer(struct Timer *tim)
{  if(tim->flags&TIMF_IO)
   {  AbortIO(&tim->req);
      WaitIO(&tim->req);
   }
   REMOVE(tim);
   Amethodas(AOTP_OBJECT,tim,AOM_DISPOSE);
}

static void Deinstalltimer(void)
{  struct Timer *tim;
   for(tim=timers.first;tim->next;tim=tim->next)
   {  if(tim->flags&TIMF_IO)
      {  AbortIO(&tim->req);
         WaitIO(&tim->req);
         tim->flags&=~TIMF_IO;
      }
   }
   if(timerequest)
   {  if(timerequest->io_Device) CloseDevice(timerequest);
      DeleteExtIO(timerequest);
   }
   if(timerport)
   {  Setprocessfun(timerport->mp_SigBit,NULL);
      DeleteMsgPort(timerport);
   }
}

static long Dispatch(struct Timer *tim,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newtimer((struct Amset *)amsg);
         break;
      case AOM_SET:
         result=Settimer(tim,(struct Amset *)amsg);
         break;
      case AOM_GET:
         result=Gettimer(tim,(struct Amset *)amsg);
         break;
      case AOM_DISPOSE:
         Disposetimer(tim);
         break;
      case AOM_DEINSTALL:
         Deinstalltimer();
         break;
   }
   return result;
}

/*------------------------------------------------------------------------*/

BOOL Installtimer(void)
{  NEWLIST(&timers);
   if(!Amethod(NULL,AOM_INSTALL,AOTP_TIMER,Dispatch)) return FALSE;
   if(!(timerport=CreateMsgPort())) return FALSE;
   Setprocessfun(timerport->mp_SigBit,Processtimer);
   if(!(timerequest=(struct timerequest *)
      CreateExtIO(timerport,sizeof(struct timerequest)))) return FALSE;
   if(OpenDevice(TIMERNAME,UNIT_MICROHZ,timerequest,0)) return FALSE;
   TimerBase=timerequest->io_Device;
   return TRUE;
}

