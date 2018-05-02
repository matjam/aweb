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

/* printwin.c - AWeb off-screen print window object*/

#include "aweb.h"
#include "printwin.h"
#include "window.h"
#include "frame.h"
#include "application.h"
#include <intuition/intuition.h>

#include <clib/exec_protos.h>
#include <clib/utility_protos.h>
#include <clib/intuition_protos.h>
#include <clib/layers_protos.h>
#include <clib/graphics_protos.h>

/*---------------------------------------------------------------------*/

struct Pwindow
{  struct Aobject object;
   void *frame;
   long width,height,layoutheight;
   void *whis;
   struct BitMap *bitmap;
   struct Layer_Info *layerinfo;
   struct Layer *layer;
   struct RastPort *rp;
   USHORT flags;
};

#define PRWF_CLEARTOP      0x0001   /* Don't use top row */
#define PRWF_NOBACKGROUND  0x0002   /* Don't use backgrounds */
#define PRWF_TURBOPRINT    0x0004   /* Turboprint device */

/*---------------------------------------------------------------------*/

static long Setprintwindow(struct Pwindow *win,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOPRW_Width:
            win->width=tag->ti_Data;
            break;
         case AOPRW_Height:
            win->height=tag->ti_Data;
            if(win->frame)
            {  Asetattrs(win->frame,AOBJ_Height,win->height,TAG_END);
            }
            break;
         case AOPRW_Layoutheight:
            win->layoutheight=tag->ti_Data;
            break;
         case AOBJ_Winhis:
            if(!win->whis)
            {  win->whis=(void *)tag->ti_Data;
               Asetattrs(win->frame,AOBJ_Winhis,win->whis,TAG_END);
            }
            break;
         case AOBJ_Top:
            Asetattrs(win->frame,AOFRM_Toppos,tag->ti_Data,TAG_END);
            break;
         case AOPRW_Cleartop:
            SETFLAG(win->flags,PRWF_CLEARTOP,tag->ti_Data);
            Asetattrs(win->frame,AOBJ_Height,win->height-(tag->ti_Data?1:0),TAG_END);
            break;
         case AOPRW_Update:
            Asetattrs(win->frame,AOFRM_Updatecopy,TRUE,TAG_END);
            break;
         case AOBJ_Nobackground:
            SETFLAG(win->flags,PRWF_NOBACKGROUND,tag->ti_Data);
            break;
         case AOPRW_Turboprint:
            SETFLAG(win->flags,PRWF_TURBOPRINT,tag->ti_Data);
            break;
      }
   }
   return 0;
}

static void Disposeprintwindow(struct Pwindow *win)
{  if(win->frame)
   {  Asetattrs(win->frame,AOBJ_Window,NULL,TAG_END);
      Adisposeobject(win->frame);
   }
   if(win->layer)
   {  DeleteLayer(0,win->layer);
      WaitBlit();
   }
   if(win->layerinfo) DisposeLayerInfo(win->layerinfo);
   if(win->bitmap) FreeBitMap(win->bitmap);
   Amethodas(AOTP_OBJECT,win,AOM_DISPOSE);
}

static struct Pwindow *Newprintwindow(struct Amset *ams)
{  struct Pwindow *win;
   struct Screen *screen=(struct Screen *)Agetattr(Aweb(),AOAPP_Screen);
   if(win=Allocobject(AOTP_PRINTWINDOW,sizeof(struct Pwindow),ams))
   {  Setprintwindow(win,ams);
      /* Allocate off-screen bitmap. If turboprint is used, create a friend
       * of the screen, else attempt to create a standard bitmap. */
      if(!(win->bitmap=AllocBitMap(win->width,win->height,
         GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH),
         (win->flags&PRWF_TURBOPRINT)?BMF_MINPLANES:0,
         (win->flags&PRWF_TURBOPRINT)?screen->RastPort.BitMap:NULL))) goto err;
      if(!(win->layerinfo=NewLayerInfo())) goto err;
      if(!(win->layer=CreateUpfrontLayer(win->layerinfo,win->bitmap,
         0,0,win->width-1,win->height-1,LAYERSIMPLE|LAYERBACKDROP,NULL))) goto err;
      win->rp=win->layer->rp;
      SetDrMd(win->rp,JAM1);
      if(win->flags&PRWF_NOBACKGROUND)
      {  SetRast(win->rp,Agetattr(Aweb(),AOAPP_Whitepen));
      }
      if(!(win->frame=Anewobject(AOTP_FRAME,
         AOBJ_Window,win,
         AOFRM_Topframe,TRUE,
         AOFRM_Dumbframe,TRUE,
         AOFRM_Layoutheight,win->layoutheight,
         AOBJ_Nobackground,BOOLVAL(win->flags&PRWF_NOBACKGROUND),
         TAG_END))) goto err;
   }
   return win;

err:
   Disposeprintwindow(win);
   return NULL;
}

static long Getprintwindow(struct Pwindow *win,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOBJ_Frame:
            PUTATTR(tag,win->frame);
            break;
         case AOWIN_Rastport:
            PUTATTR(tag,win->rp);
            break;
         case AOWIN_Innerleft:
            PUTATTR(tag,0);
            break;
         case AOWIN_Innertop:
            PUTATTR(tag,(win->flags&PRWF_CLEARTOP)?1:0);
            break;
         case AOWIN_Innerwidth:
            PUTATTR(tag,win->width);
            break;
         case AOWIN_Innerheight:
            PUTATTR(tag,win->height-((win->flags&PRWF_CLEARTOP)?1:0));
            break;
         case AOPRW_Totalheight:
            PUTATTR(tag,Agetattr(win->frame,AOFRM_Contentheight));
            break;
      }
   }
   return 0;
}

static long Dispatch(struct Pwindow *win,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newprintwindow((struct Amset *)amsg);
         break;
      case AOM_SET:
         result=Setprintwindow(win,(struct Amset *)amsg);
         break;
      case AOM_GET:
         result=Getprintwindow(win,(struct Amset *)amsg);
         break;
      case AOM_DISPOSE:
         Disposeprintwindow(win);
         break;
      case AOM_DEINSTALL:
         break;
   }
   return result;
}

/*------------------------------------------------------------------------*/

BOOL Installprintwindow(void)
{  if(!Amethod(NULL,AOM_INSTALL,AOTP_PRINTWINDOW,Dispatch)) return FALSE;
   return TRUE;
}

