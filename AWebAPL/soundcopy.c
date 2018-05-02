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

/* soundcopy.c - AWeb sound copy driver object */

#include "aweb.h"
#include "soundprivate.h"
#include "copydriver.h"
#include "task.h"
#include <datatypes/datatypesclass.h>
#include <datatypes/soundclass.h>
#include <devices/audio.h>
#include <clib/utility_protos.h>
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/datatypes_protos.h>

/*------------------------------------------------------------------------*/

/* If loop is not 1, then don't use datatypes (current datatypes can't handle
 * this) but use normal audio. */

struct Sndcopy
{  struct Copydriver cdv;
   void *copy;
   struct Sndsource *source;
   long loop;                    /* How many times to loop */
   USHORT flags;
   void *task;
};

#define SNDF_WANTPLAY      0x0001   /* We want to play and wait for source */

/*------------------------------------------------------------------------*/

struct Sndtask
{  void *dto;                    /* Sound datatype object */
   UBYTE *dtsample;               /* Sample from dto */
   ULONG samplength,period,volume,cycles; /* Sound dto details */
   long loop;                    /* Nr of times yet to go */
   struct MsgPort *ioport;
   struct IOAudio *audev;        /* Main audio IO to open device */
   UBYTE *sample[2];              /* Single or double chip buffer */
   struct IOAudio *audio[2];     /* Single or double buffer io request */
   short nextio;                 /* Next buffer/ioreq to use (0 or 1) */
   long nextpos;                 /* Next buffer position to start */
   USHORT flags;
};

#define SNTF_OURSAMPLE     0x0001   /* We allocated sample */
#define SNTF_PLAYING       0x0002   /* Currently playing through audio device */

#define MAXLENGTH 131072

/* Play using single-buffering */
static void Playsbuf(struct Sndtask *st)
{  if(TypeOfMem(st->sample)&MEMF_CHIP)
   {  st->sample[0]=st->dtsample;
   }
   else
   {  st->sample[0]=ALLOCTYPE(UBYTE,st->samplength,MEMF_CHIP|MEMF_PUBLIC);
      st->flags|=SNTF_OURSAMPLE;
   }
   if(st->sample[0])
   {  memmove(st->sample[0],st->dtsample,st->samplength);
      st->audev->ioa_Request.io_Command=CMD_WRITE;
      st->audev->ioa_Request.io_Flags=ADIOF_PERVOL;
      st->audev->ioa_Data=st->sample[0];
      st->audev->ioa_Length=st->samplength;
      st->audev->ioa_Period=st->period;
      st->audev->ioa_Cycles=st->loop>0?st->loop:0;
      st->audev->ioa_Volume=st->volume;
      SetSignal(0,1<<st->ioport->mp_SigBit);
      BeginIO(st->audev);
      /* Let it play until we are killed or sound ends */
      Waittask(1<<st->ioport->mp_SigBit);
      AbortIO(st->audev);
      WaitIO(st->audev);
      if(st->flags&SNTF_OURSAMPLE) FREE(st->sample[0]);
   }
}

/* Write the next buffer */
static void Startdbuf(struct Sndtask *st,short n)
{  long len=MIN(MAXLENGTH,st->samplength-st->nextpos);
   memmove(st->sample[n],st->dtsample+st->nextpos,len);
   *st->audio[n]=*st->audev;
   st->audio[n]->ioa_Request.io_Command=CMD_WRITE;
   st->audio[n]->ioa_Request.io_Flags=ADIOF_PERVOL;
   st->audio[n]->ioa_Data=st->sample[n];
   st->audio[n]->ioa_Length=len;
   st->audio[n]->ioa_Period=st->period;
   st->audio[n]->ioa_Cycles=1;
   st->audio[n]->ioa_Volume=st->volume;
   BeginIO(st->audio[n]);
   st->nextpos+=len;
   if(st->nextpos>=st->samplength)
   {  st->nextpos=0;
      st->loop--;
   }
}

/* Sample too long, play using double-buffering */
static void Playdbuf(struct Sndtask *st)
{  if((st->sample[0]=ALLOCTYPE(UBYTE,MAXLENGTH,MEMF_CHIP|MEMF_PUBLIC))
   && (st->sample[1]=ALLOCTYPE(UBYTE,MAXLENGTH,MEMF_CHIP|MEMF_PUBLIC))
   && (st->audio[0]=(struct IOAudio *)CreateExtIO(st->ioport,sizeof(struct IOAudio)))
   && (st->audio[1]=(struct IOAudio *)CreateExtIO(st->ioport,sizeof(struct IOAudio))))
   {  st->nextio=0;
      st->nextpos=0;
      Startdbuf(st,0);
      Startdbuf(st,1);
      while(st->loop)
      {  Waittask(1<<st->ioport->mp_SigBit);
         if(Checktaskbreak()) break;
         WaitIO(st->audio[st->nextio]);   /* Should be this one that's ready */
         Startdbuf(st,st->nextio);
         st->nextio=1-st->nextio;
      }
      /* Now wait until both buffers are finished playing, or we are killed. */
      if(!Checktaskbreak()) Waittask(1<<st->ioport->mp_SigBit);
      if(Checktaskbreak()) AbortIO(st->audio[st->nextio]);
      WaitIO(st->audio[st->nextio]);
      st->nextio=1-st->nextio;
      if(!Checktaskbreak()) Waittask(1<<st->ioport->mp_SigBit);
      if(Checktaskbreak()) AbortIO(st->audio[st->nextio]);
      WaitIO(st->audio[st->nextio]);
   }
   if(st->audio[0]) DeleteExtIO(st->audio[0]);
   if(st->audio[1]) DeleteExtIO(st->audio[1]);
   if(st->sample[0]) FREE(st->sample[0]);
   if(st->sample[1]) FREE(st->sample[1]);
}

/* Play the sound using audio device */
static void Audioplay(struct Sndcopy *snd,struct Sndtask *st)
{  static char channels[]={ 1,2,4,8 };
   if(st->ioport=CreateMsgPort())
   {  if(st->audev=(struct IOAudio *)CreateExtIO(st->ioport,sizeof(struct IOAudio)))
      {  if(!OpenDevice(AUDIONAME,0,st->audev,0))
         {  st->audev->io_Command=ADCMD_ALLOCATE;
            st->audev->io_Flags=ADIOF_NOWAIT;
            st->audev->ioa_AllocKey=0;
            st->audev->ioa_Data=channels;
            st->audev->ioa_Length=sizeof(channels);
            BeginIO(st->audev);
            /* Wait because if IO completes quickly WaitIO doesn't Wait() and
             * doesn't clear the signal bit. */
            Waittask(1<<st->ioport->mp_SigBit);
            if(!WaitIO(st->audev))
            {  GetDTAttrs(st->dto,
                  SDTA_Sample,&st->dtsample,
                  SDTA_SampleLength,&st->samplength,
                  SDTA_Period,&st->period,
                  SDTA_Volume,&st->volume,
                  SDTA_Cycles,&st->cycles,
                  TAG_END);
               if(st->dtsample)
               {  if(st->samplength>MAXLENGTH)
                  {  Playdbuf(st);
                  }
                  else
                  {  Playsbuf(st);
                  }
               }
            }
            CloseDevice(st->audev);
         }
         DeleteExtIO(st->audev);
      }
      DeleteMsgPort(st->ioport);
   }
}

/* Subtask playing the sound */
static void Soundtask(struct Sndcopy *snd)
{  struct Sndtask st={0};
   st.dto=NewDTObject(snd->source->filename,
      DTA_SourceType,DTST_FILE,
      DTA_GroupID,GID_SOUND,
      TAG_END);
   if(st.dto)
   {  if(snd->loop==1)
      {  DoMethod(st.dto,DTM_TRIGGER,NULL,STM_PLAY,NULL);
      }
      else
      {  st.loop=snd->loop;
         Audioplay(snd,&st);
      }
      Waittask(0);
      DisposeDTObject(st.dto);
   }
}

/*------------------------------------------------------------------------*/

/* Stop playing */
static void Stopsound(struct Sndcopy *snd)
{  if(snd->task)
   {  Adisposeobject(snd->task);
      snd->task=NULL;
   }
   snd->flags&=~SNDF_WANTPLAY;
}

/* Start playing of sound. If no source available yet, set flag and
 * start to play upon receipt of AOSNP_Srcupdate. */
static void Playsound(struct Sndcopy *snd)
{  if(snd->source && snd->source->filename)
   {  snd->flags&=~SNDF_WANTPLAY;
      Stopsound(snd);
      if(snd->task=Anewobject(AOTP_TASK,
         AOTSK_Entry,Soundtask,
         AOTSK_Name,"AWeb sound player",
         AOTSK_Userdata,snd,
         TAG_END))
      {  Asetattrs(snd->task,AOTSK_Start,TRUE,TAG_END);
      }
   }
   else
   {  snd->flags|=SNDF_WANTPLAY;
   }
}

/*------------------------------------------------------------------------*/

static long Setsoundcopy(struct Sndcopy *snd,struct Amset *ams)
{  long result=0;
   struct TagItem *tag,*tstate=ams->tags;
   Amethodas(AOTP_COPYDRIVER,snd,AOM_SET,ams->tags);
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOCDV_Copy:
            snd->copy=(void *)tag->ti_Data;
            break;
         case AOCDV_Sourcedriver:
            snd->source=(struct Sndsource *)tag->ti_Data;
            break;
         case AOCDV_Soundloop:
            snd->loop=(long)tag->ti_Data;
            if(snd->loop==0) snd->loop=1;
            break;
         case AOSNP_Srcupdate:
            Playsound(snd);
            break;
         case AOBJ_Cframe:
            /* Probably a background sound of a document going in or out of display */
            if(tag->ti_Data)
            {  Playsound(snd);
            }
            else
            {  Stopsound(snd);
            }
            break;
      }
   }
   return result;
}

static struct Sndcopy *Newsoundcopy(struct Amset *ams)
{  struct Sndcopy *snd=Allocobject(AOTP_SOUNDCOPY,sizeof(struct Sndcopy),ams);
   if(snd)
   {  snd->loop=1;
      Setsoundcopy(snd,ams);
   }
   return snd;
}

static long Getsoundcopy(struct Sndcopy *snd,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   long result;
   result=AmethodasA(AOTP_COPYDRIVER,snd,ams);
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOCDV_Undisplayed:
            PUTATTR(tag,TRUE);
            break;
      }
   }
   return result;
}

static void Disposesoundcopy(struct Sndcopy *snd)
{  Stopsound(snd);
   Amethodas(AOTP_COPYDRIVER,snd,AOM_DISPOSE);
}

static long Dispatch(struct Sndcopy *snd,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newsoundcopy((struct Amset *)amsg);
         break;
      case AOM_SET:
         result=Setsoundcopy(snd,(struct Amset *)amsg);
         break;
      case AOM_GET:
         result=Getsoundcopy(snd,(struct Amset *)amsg);
         break;
      case AOM_DISPOSE:
         Disposesoundcopy(snd);
         break;
      case AOM_DEINSTALL:
         break;
      default:
         result=AmethodasA(AOTP_COPYDRIVER,snd,amsg);
   }
   return result;
}

/*------------------------------------------------------------------------*/

BOOL Installsoundcopy(void)
{  if(!Amethod(NULL,AOM_INSTALL,AOTP_SOUNDCOPY,Dispatch)) return FALSE;
   return TRUE;
}

