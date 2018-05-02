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

/* whistask.c - AWeb window history AWebLib module */

#include "aweblib.h"
#include "task.h"
#include "whisprivate.h"
#include "url.h"
#include <exec/resident.h>
#include <intuition/intuition.h>
#include <classact.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/utility_protos.h>

enum WINHIS_GADGETIDS
{  WGID_LIST=1,WGID_FILTER,WGID_WIN,WGID_ORDER,WGID_DISPLAY,
};

static struct Image *mainimg;

static struct ColumnInfo columninfo[]=
{  8,NULL,0,
   4,NULL,0,
   4,NULL,0,
   84,NULL,0,
   -1,NULL,0,
};

static UBYTE *orderlabels[6];

enum WHIS_ORDER
{  WHIS_NATURAL=0,WHIS_MAINLINE,WHIS_RETRIEVED,WHIS_TITLE,WHIS_URL
};

static struct Hook idcmphook;

static void *AwebPluginBase;
void *IntuitionBase,*UtilityBase;
struct ClassLibrary *WindowBase,*LayoutBase,*ButtonBase,*ListBrowserBase,
   *ChooserBase,*CheckBoxBase,*IntegerBase,*SpaceBase,*LabelBase,*GlyphBase;

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

__asm __saveds void Winhistask(
   register __a0 struct Whiswindow *whw);

/* Function declarations for project dependent hook functions */
static ULONG Initaweblib(struct Library *libbase);
static void Expungeaweblib(struct Library *libbase);

struct Library *WinhisBase;

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
   Winhistask,
   (APTR)-1
};

/* Init table used in library initialization. */
static ULONG inittab[]=
{  sizeof(struct Library),
   (ULONG) functable,
   0,
   (ULONG) Initlib
};

static char __aligned libname[]="history.aweblib";
static char __aligned libid[]="history.aweblib " AWEBLIBVSTRING " " __AMIGADATE__;

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
   WinhisBase=libbase;
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
{  if(!(IntuitionBase=OpenLibrary("intuition.library",39))) return FALSE;
   if(!(UtilityBase=OpenLibrary("utility.library",39))) return FALSE;
   if(!(WindowBase=(struct ClassLibrary *)OpenLibrary("window.class",OSNEED(0,44)))) return FALSE;
   if(!(LayoutBase=(struct ClassLibrary *)OpenLibrary("gadgets/layout.gadget",OSNEED(0,44)))) return FALSE;
   if(!(ButtonBase=(struct ClassLibrary *)OpenLibrary("gadgets/button.gadget",OSNEED(0,44)))) return FALSE;
   if(!(ChooserBase=(struct ClassLibrary *)OpenLibrary("gadgets/chooser.gadget",OSNEED(0,44)))) return FALSE;
   if(!(CheckBoxBase=(struct ClassLibrary *)OpenLibrary("gadgets/checkbox.gadget",OSNEED(0,44)))) return FALSE;
   if(!(IntegerBase=(struct ClassLibrary *)OpenLibrary("gadgets/integer.gadget",OSNEED(0,44)))) return FALSE;
   if(!(ListBrowserBase=(struct ClassLibrary *)OpenLibrary("gadgets/listbrowser.gadget",OSNEED(0,44)))) return FALSE;
   if(!(SpaceBase=(struct ClassLibrary *)OpenLibrary("gadgets/space.gadget",OSNEED(0,44)))) return FALSE;
   if(!(LabelBase=(struct ClassLibrary *)OpenLibrary("images/label.image",OSNEED(0,44)))) return FALSE;
   if(!(GlyphBase=(struct ClassLibrary *)OpenLibrary("images/glyph.image",OSNEED(0,44)))) return FALSE;
   if(!(mainimg=NewObject(GLYPH_GetClass(),NULL,
      IA_Width,8,
      IA_Height,8,
      GLYPH_Glyph,GLYPH_DOWNARROW,
      TAG_END))) return FALSE;
   return TRUE;
}

static void Expungeaweblib(struct Library *libbase)
{  if(mainimg) DisposeObject(mainimg);
   if(GlyphBase) CloseLibrary(GlyphBase);
   if(LabelBase) CloseLibrary(LabelBase);
   if(SpaceBase) CloseLibrary(SpaceBase);
   if(ListBrowserBase) CloseLibrary(ListBrowserBase);
   if(IntegerBase) CloseLibrary(IntegerBase);
   if(CheckBoxBase) CloseLibrary(CheckBoxBase);
   if(ChooserBase) CloseLibrary(ChooserBase);
   if(ButtonBase) CloseLibrary(ButtonBase);
   if(LayoutBase) CloseLibrary(LayoutBase);
   if(WindowBase) CloseLibrary(WindowBase);
   if(UtilityBase) CloseLibrary(UtilityBase);
   if(IntuitionBase) CloseLibrary(IntuitionBase);
}

/*-----------------------------------------------------------------------*/

static void Setgadgetattrs(struct Gadget *gad,struct Window *win,struct Requester *req,...)
{  struct TagItem *tags=(struct TagItem *)((ULONG *)&req+1);
   if(SetGadgetAttrsA(gad,win,req,tags)) RefreshGList(gad,win,req,1);
}

static long Getvalue(void *gad,ULONG tag)
{  long value=0;
   GetAttr(tag,gad,(ULONG *)&value);
   return value;
}

static struct Node *Getnode(struct List *list,long n)
{  struct Node *node=list->lh_Head;
   while(node->ln_Succ && n)
   {  node=node->ln_Succ;
      n--;
   }
   if(node->ln_Succ) return node;
   else return NULL;
}

static void Freechnodes(struct List *list)
{  struct Node *node;
   while(node=RemHead(list)) FreeChooserNode(node);
}

static void Freelbnodes(struct List *list)
{  struct Node *node;
   while(node=RemHead(list)) FreeListBrowserNode(node);
}

static void Makeorderlist(struct Whiswindow *whw)
{  struct Node *node;
   short i;
   for(i=0;orderlabels[i];i++)
   {  if(node=AllocChooserNode(
         CNA_Text,orderlabels[i],
         TAG_END))
         AddTail(&whw->orderlist,node);
   }
}

/* Find the leading frame history */
static struct Framehis *Findfhis(struct Winhis *whis)
{  struct Framehis *fhis;
   for(fhis=whis->frames.first;fhis->next;fhis=fhis->next)
   {  if(STREQUAL(fhis->id,whis->frameid)) return fhis;
   }
   return NULL;
}

/* Return the Urlname to be used as title */
static UBYTE *Urltitle(struct Winhis *whis)
{  if(whis->titleurl) return (UBYTE *)Agetattr(whis->titleurl,AOURL_Url);
   return "";
}

static UBYTE *Buildurlfrag(struct Whiswindow *whw,struct Framehis *fhis)
{  long l;
   if(fhis)
   {  strncpy(whw->urlbuf,
         fhis->url?(UBYTE *)Agetattr(fhis->url,AOURL_Url):NULLSTRING,
         URLBUFSIZE-1);
      l=strlen(whw->urlbuf);
      if(fhis->fragment && l<URLBUFSIZE-1)
      {  strcat(whw->urlbuf,"#");
         l++;
         strncat(whw->urlbuf,fhis->fragment,URLBUFSIZE-l-1);
      }
   }
   else
   {  *whw->urlbuf='\0';
   }
   return whw->urlbuf;
}

static struct Node *Allocnode(struct Whiswindow *whw,struct Winhis *whis)
{  struct Node *node;
   UBYTE buffer[8];
   BOOL skip;
   UBYTE *name;
   struct Framehis *fhis=Findfhis(whis);
   sprintf(buffer,"%d",whis->windownr);
   skip=whw->order>WHIS_MAINLINE || (whis->wflags&WINHF_SKIP)!=0;
   if(whw->order==WHIS_URL) name=Urltitle(whis);
   else if(whis->title) name=whis->title;
   else if(fhis) name=Buildurlfrag(whw,fhis);
   else name="";
   node=AllocListBrowserNode(4,
      LBNA_UserData,whis,
      LBNA_Column,0,
         LBNCA_CopyText,TRUE,
         LBNCA_Text,buffer,
         LBNCA_Justification,LCJ_RIGHT,
      LBNA_Column,1,
         LBNCA_Text,(*whis->frameid)?"+":"",
      LBNA_Column,2,
         skip?TAG_IGNORE:LBNCA_Image,mainimg,
         skip?LBNCA_Text:TAG_IGNORE,"",
      LBNA_Column,3,
         LBNCA_CopyText,TRUE,
         LBNCA_Text,name,
      TAG_END);
   return node;
}

typedef int whissortf(void *,void *);

static int Whissortretrieved(struct Winhis **wa,struct Winhis **wb)
{  int c=0;
   if((*wa)->loadnr && (*wb)->loadnr) c=(*wa)->loadnr-(*wb)->loadnr;
   else c=(int)(*wa)->whisnr-(int)(*wb)->whisnr;
   if(c==0) c=(*wa)->windownr-(*wb)->windownr;
   return c;
}

static int Whissorttitle(struct Winhis **wa,struct Winhis **wb)
{  UBYTE *pa,*pb;
   int c=0;
   pa=(*wa)->title?(*wa)->title:Urltitle(*wa);
   pb=(*wb)->title?(*wb)->title:Urltitle(*wb);
   c=stricmp(pa,pb);
   if(c==0) c=(*wa)->windownr-(*wb)->windownr;
   if(c==0) c=(int)(*wa)->whisnr-(int)(*wb)->whisnr;
   return c;
}

static int Whissorturl(struct Winhis **wa,struct Winhis **wb)
{  int c=0;
   c=stricmp(Urltitle(*wa),Urltitle(*wb));
   if(c==0) c=(*wa)->windownr-(*wb)->windownr;
   if(c==0) c=(int)(*wa)->whisnr-(int)(*wb)->whisnr;
   return c;
}

static void Allocnodes(struct Whiswindow *whw,struct List *list)
{  struct Node *node;
   struct Winhis *whis;
   long windownr=0;
   long nnodes=0;
   if(whw->filter)
   {  if(whw->wingad) windownr=Getvalue(whw->wingad,INTEGER_Number);
      else windownr=whw->windownr;
   }
   if(whw->order<=WHIS_MAINLINE)  /* NATURAL or MAINLINE */
   {  whw->currentnode=-1;
      for(whis=whw->winhislist->first;whis->next;whis=whis->next)
      {  if(whis->key)
         {  if(!whw->filter || whis->windownr==windownr)
            {  if(whw->order==WHIS_NATURAL || !(whis->wflags&WINHF_SKIP))
               {  if(node=Allocnode(whw,whis))
                  {  AddTail(list,node);
                     if(whis==whw->current) whw->currentnode=nnodes;
                     nnodes++;
                  }
               }
            }
         }
      }
   }
   else
   {  struct Winhis **whwab;
      long i,n,lastnr=0;
      whissortf *f;
      if(whw->winhislist->last->prev) n=whw->winhislist->last->whisnr;
      else return;
      if(whwab=ALLOCTYPE(struct Winhis *,n,0))
      {  for(whis=whw->winhislist->first,i=0;whis->next;whis=whis->next)
         {  if(whis->key)
            {  if(!whw->filter || whis->windownr==windownr) whwab[i++]=whis;
            }
         }
         n=i;
         switch(whw->order)
         {  case WHIS_RETRIEVED:
               f=(whissortf *)Whissortretrieved;
               break;
            case WHIS_TITLE:
               f=(whissortf *)Whissorttitle;
               break;
            case WHIS_URL:
               f=(whissortf *)Whissorturl;
               break;
            default:
               f=NULL;
         }
         if(f) qsort(whwab,n,sizeof(struct Winhis *),f);
         for(i=0;i<n;i++)
         {  if(!whwab[i]->loadnr || whwab[i]->loadnr!=lastnr)
            {  if(node=Allocnode(whw,whwab[i])) AddTail(list,node);
               if(whwab[i]->loadnr) lastnr=whwab[i]->loadnr;
            }
         }
         FREE(whwab);
      }
   }
}

static void Setgads(struct Whiswindow *whw)
{  ULONG selected=Getvalue(whw->listgad,LISTBROWSER_Selected);
   struct Node *node=Getnode(&whw->gadlist,selected);
   struct Winhis *whis;
   struct Framehis *fhis;
   UBYTE *title="",*url="";
   if(node)
   {  GetListBrowserNodeAttrs(node,LBNA_UserData,&whis,TAG_END);
      if(fhis=Findfhis(whis))
      {  if(whis->title) title=whis->title;
         url=Buildurlfrag(whw,fhis);
      }
   }
   Setgadgetattrs(whw->titlegad,whw->window,NULL,
      GA_Text,title,
      TAG_END);
   Setgadgetattrs(whw->urlgad,whw->window,NULL,
      GA_Text,url,
      TAG_END);
}

static void Rebuild(struct Whiswindow *whw,BOOL keepselected)
{  long selected=keepselected?Getvalue(whw->listgad,LISTBROWSER_Selected):~0;
   Setgadgetattrs(whw->listgad,whw->window,NULL,
      LISTBROWSER_Labels,~0,TAG_END);
   Freelbnodes(&whw->gadlist);
   Allocnodes(whw,&whw->gadlist);
   if(selected<0 && whw->order<=WHIS_MAINLINE) selected=whw->currentnode;
   Setgadgetattrs(whw->listgad,whw->window,NULL,
      LISTBROWSER_Labels,&whw->gadlist,
      LISTBROWSER_Selected,selected,
      TAG_END);
   if(selected>=0)
   {  Setgadgetattrs(whw->listgad,whw->window,NULL,
         LISTBROWSER_MakeVisible,selected,
         TAG_END);
   }
   Setgads(whw);
}

static void Dofilter(struct Whiswindow *whw)
{  whw->filter=Getvalue(whw->filtergad,GA_Selected);
   Setgadgetattrs(whw->wingad,whw->window,NULL,
      GA_Disabled,!whw->filter,
      TAG_END);
   Rebuild(whw,FALSE);
}

static void Doorder(struct Whiswindow *whw)
{  whw->order=Getvalue(whw->ordergad,CHOOSER_Active);
   Rebuild(whw,FALSE);
}

static void Setcurrentnode(struct Whiswindow *whw)
{  struct Node *node;
   long nr=0;
   struct Winhis *whis;
   whw->currentnode=-1;
   for(node=whw->gadlist.lh_Head;node->ln_Succ;node=node->ln_Succ,nr++)
   {  GetListBrowserNodeAttrs(node,LBNA_UserData,&whis,TAG_END);
      if(whis==whw->current)
      {  whw->currentnode=nr;
         break;
      }
   }
   Setgadgetattrs(whw->listgad,whw->window,NULL,
      LISTBROWSER_Selected,whw->currentnode,
      LISTBROWSER_MakeVisible,whw->currentnode,
      TAG_END);
}

static BOOL Dodisplay(struct Whiswindow *whw)
{  long selected=Getvalue(whw->listgad,LISTBROWSER_Selected);
   struct Node *node=Getnode(&whw->gadlist,selected);
   struct Winhis *whis;
   BOOL done=FALSE;
   if(node)
   {  GetListBrowserNodeAttrs(node,LBNA_UserData,&whis,TAG_END);
      ReleaseSemaphore(whw->whissema);  /* avoid deadlock, main task needs it */
      Updatetaskattrs(AOWHW_Display,whis,TAG_END);
      ObtainSemaphore(whw->whissema);
      done=whw->autoclose;
   }
   return done;
}

static void Moveselected(struct Whiswindow *whw,short n)
{  long selected=Getvalue(whw->listgad,LISTBROWSER_Selected);
   BOOL update=FALSE;
   if(n<0)
   {  if(selected>0)
      {  selected--;
         update=TRUE;
      }
      else if(selected<0)
      {  selected=0;
         update=TRUE;
      }
   }
   else if(n>0)
   {  struct Node *node=Getnode(&whw->gadlist,selected);
      if(node && node->ln_Succ->ln_Succ)
      {  selected++;
         update=TRUE;
      }
      else if(!node)
      {  selected=0;
         update=TRUE;
      }
   }
   if(update)
   {  Setgadgetattrs(whw->listgad,whw->window,NULL,
         LISTBROWSER_Selected,selected,
         LISTBROWSER_MakeVisible,selected,
         TAG_END);
      Setgads(whw);
   }
}

static long __saveds __asm Idcmphook(register __a0 struct Hook *hook,
   register __a1 struct IntuiMessage *msg)
{  struct Whiswindow *whw=hook->h_Data;
   switch(msg->Class)
   {  case IDCMP_CHANGEWINDOW:
         Updatetaskattrs(AOWHW_Dimx,whw->window->LeftEdge,
            AOWHW_Dimy,whw->window->TopEdge,
            AOWHW_Dimw,whw->window->Width-whw->window->BorderLeft-whw->window->BorderRight,
            AOWHW_Dimh,whw->window->Height-whw->window->BorderTop-whw->window->BorderBottom,
            TAG_END);
         break;
   }
   return 0;
}

/*-----------------------------------------------------------------------*/

static void Buildwinhiswindow(struct Whiswindow *whw)
{  NewList(&whw->gadlist);
   NewList(&whw->orderlist);
   orderlabels[0]=AWEBSTR(MSG_WHIS_ORDER_NATURAL);
   orderlabels[1]=AWEBSTR(MSG_WHIS_ORDER_MAINLINE);
   orderlabels[2]=AWEBSTR(MSG_WHIS_ORDER_RETRIEVED);
   orderlabels[3]=AWEBSTR(MSG_WHIS_ORDER_TITLE);
   orderlabels[4]=AWEBSTR(MSG_WHIS_ORDER_URL);
   idcmphook.h_Entry=(HOOKFUNC)Idcmphook;
   idcmphook.h_Data=whw;
   if(whw->screen=LockPubScreen(whw->screenname))
   {  whw->filter=TRUE;
      whw->order=WHIS_NATURAL;
      ObtainSemaphore(whw->whissema);
      Allocnodes(whw,&whw->gadlist);
      ReleaseSemaphore(whw->whissema);
      Makeorderlist(whw);
      if(!whw->w)
      {  whw->x=whw->screen->Width/4;
         whw->y=whw->screen->Height/4;
         whw->w=whw->screen->Width/2;
         whw->h=whw->screen->Height/2;
      }
      whw->winobj=WindowObject,
         WA_Title,AWEBSTR(MSG_WHIS_TITLE),
         WA_Left,whw->x,
         WA_Top,whw->y,
         WA_InnerWidth,whw->w,
         WA_InnerHeight,whw->h,
         WA_SizeGadget,TRUE,
         WA_DepthGadget,TRUE,
         WA_DragBar,TRUE,
         WA_CloseGadget,TRUE,
         WA_Activate,TRUE,
         WA_AutoAdjust,TRUE,
         WA_SimpleRefresh,TRUE,
         WA_PubScreen,whw->screen,
         WA_IDCMP,IDCMP_RAWKEY,
         WINDOW_IDCMPHook,&idcmphook,
         WINDOW_IDCMPHookBits,IDCMP_CHANGEWINDOW,
         WINDOW_Layout,VLayoutObject,
            LAYOUT_SpaceOuter,TRUE,
            LAYOUT_DeferLayout,TRUE,
            StartMember,whw->listgad=ListBrowserObject,
               GA_ID,WGID_LIST,
               GA_RelVerify,TRUE,
               LISTBROWSER_ShowSelected,TRUE,
               LISTBROWSER_ColumnInfo,columninfo,
               LISTBROWSER_Labels,&whw->gadlist,
               LISTBROWSER_Separators,FALSE,
               LISTBROWSER_Selected,whw->currentnode,
            EndMember,
            StartMember,HLayoutObject,
               StartMember,whw->titlegad=ButtonObject,
                  GA_Text," ",
                  GA_ReadOnly,TRUE,
                  BUTTON_Justification,BCJ_LEFT,
                  CLASSACT_Underscore,0,
               EndMember,
               StartMember,ButtonObject,
                  GA_ID,WGID_DISPLAY,
                  GA_RelVerify,TRUE,
                  GA_Text,AWEBSTR(MSG_WHIS_DISPLAY),
               EndMember,
               CHILD_WeightedWidth,0,
            EndMember,
            CHILD_WeightedHeight,0,
            StartMember,whw->urlgad=ButtonObject,
               GA_Text," ",
               GA_ReadOnly,TRUE,
               BUTTON_Justification,BCJ_LEFT,
               CLASSACT_Underscore,0,
            EndMember,
            CHILD_WeightedHeight,0,
            StartMember,HLayoutObject,
               LAYOUT_HorizAlignment,LALIGN_LEFT,
               StartMember,whw->filtergad=CheckBoxObject,
                  GA_ID,WGID_FILTER,
                  GA_RelVerify,TRUE,
                  GA_Text,AWEBSTR(MSG_WHIS_FILTER),
                  GA_Selected,TRUE,
               EndMember,
               CHILD_WeightedWidth,0,
               StartMember,whw->wingad=IntegerObject,
                  GA_ID,WGID_WIN,
                  GA_RelVerify,TRUE,
                  INTEGER_Minimum,1,
                  INTEGER_Maximum,99999,
                  INTEGER_Number,whw->windownr,
               EndMember,
               MemberLabel(AWEBSTR(MSG_WHIS_WINDOW)),
               CHILD_MaxWidth,100,
               StartMember,SpaceObject,
               EndMember,
               StartMember,whw->ordergad=ChooserObject,
                  GA_ID,WGID_ORDER,
                  GA_RelVerify,TRUE,
                  CHOOSER_PopUp,TRUE,
                  CHOOSER_Labels,&whw->orderlist,
                  CHOOSER_Active,WHIS_NATURAL,
               EndMember,
               MemberLabel(AWEBSTR(MSG_WHIS_ORDER)),
               CHILD_WeightedWidth,0,
            EndMember,
            CHILD_WeightedHeight,0,
         EndMember,
      EndWindow;
      if(whw->winobj)
      {  if(whw->window=(struct Window *)CA_OpenWindow(whw->winobj))
         {  GetAttr(WINDOW_SigMask,whw->winobj,&whw->winsigmask);
            if(whw->currentnode>=0)
            {  Setgadgetattrs(whw->listgad,whw->window,NULL,
                  LISTBROWSER_MakeVisible,whw->currentnode,
                  TAG_END);
            }
            Setgads(whw);
         }
      }
   }
}

static BOOL Handlewinhiswindow(struct Whiswindow *whw)
{  ULONG result,relevent;
   BOOL done=FALSE;
   USHORT click;
   while((result=CA_HandleInput(whw->winobj,&click))!=WMHI_LASTMSG)
   {  ObtainSemaphore(whw->whissema);
      switch(result&WMHI_CLASSMASK)
      {  case WMHI_CLOSEWINDOW:
            done=TRUE;
            break;
         case WMHI_GADGETUP:
            switch(result&WMHI_GADGETMASK)
            {  case WGID_LIST:
                  relevent=Getvalue(whw->listgad,LISTBROWSER_RelEvent);
                  if(relevent&LBRE_DOUBLECLICK)
                  {  if(click==whw->lastclick) done=Dodisplay(whw);
                  }
                  Setgads(whw);
                  whw->lastclick=click;
                  break;
               case WGID_FILTER:
                  Dofilter(whw);
                  break;
               case WGID_WIN:
                  if(whw->filter)
                  {  whw->windownr=Getvalue(whw->wingad,INTEGER_Number);
                     Updatetaskattrs(AOWHW_Getcurrent,TRUE,TAG_END);
                     Rebuild(whw,FALSE);
                  }
                  break;
               case WGID_ORDER:
                  Doorder(whw);
                  break;
               case WGID_DISPLAY:
                  done=Dodisplay(whw);
                  break;
            }
            break;
         case WMHI_RAWKEY:
            switch(result&WMHI_GADGETMASK)
            {  case 0x45:  /* esc */
                  done=TRUE;
                  break;
               case 0x43:  /* num enter */
               case 0x44:  /* enter */
                  done=Dodisplay(whw);
                  break;
               case 0x4c:  /* up */
                  Moveselected(whw,-1);
                  break;
               case 0x4d:  /* down */
                  Moveselected(whw,1);
                  break;
            }
            break;
/*
         case WMHI_CHANGEWINDOW:
            Updatetaskattrs(AOWHW_Dimx,whw->window->LeftEdge,
               AOWHW_Dimy,whw->window->TopEdge,
               AOWHW_Dimw,whw->window->Width-whw->window->BorderLeft-whw->window->BorderRight,
               AOWHW_Dimh,whw->window->Height-whw->window->BorderTop-whw->window->BorderBottom,
               TAG_END);
            break;
*/
      }
      ReleaseSemaphore(whw->whissema);
   }
   return done;
}

static void Closewinhiswindow(struct Whiswindow *whw)
{  if(whw->winobj)
   {  if(whw->window)
      {  Updatetaskattrs(AOWHW_Dimx,whw->window->LeftEdge,
            AOWHW_Dimy,whw->window->TopEdge,
            AOWHW_Dimw,whw->window->Width-whw->window->BorderLeft-whw->window->BorderRight,
            AOWHW_Dimh,whw->window->Height-whw->window->BorderTop-whw->window->BorderBottom,
            TAG_END);
      }
      DisposeObject(whw->winobj);
   }
   Freelbnodes(&whw->gadlist);
   Freechnodes(&whw->orderlist);
   if(whw->screen) UnlockPubScreen(NULL,whw->screen);whw->screen=NULL;
   whw->wingad=NULL;
}

__asm __saveds void Winhistask(register __a0 struct Whiswindow *whw)
{  struct Taskmsg *wm;
   ULONG done=FALSE;
   ULONG getmask;
   struct TagItem *tag,*tstate;
   struct Winhis *whis;
   Buildwinhiswindow(whw);
   if(whw->window)
   {  while(!done)
      {  getmask=Waittask(whw->winsigmask);
         while(!done && (wm=Gettaskmsg()))
         {  if(wm->amsg && wm->amsg->method==AOM_SET)
            {  tstate=((struct Amset *)wm->amsg)->tags;
               while(tag=NextTagItem(&tstate))
               {  switch(tag->ti_Tag)
                  {  case AOTSK_Stop:
                        if(tag->ti_Data) done=TRUE;
                        break;
                     case AOWHW_Tofront:
                        if(tag->ti_Data)
                        {  WindowToFront(whw->window);
                           ActivateWindow(whw->window);
                        }
                        if(Getvalue(whw->wingad,INTEGER_Number)!=tag->ti_Data)
                        {  Setgadgetattrs(whw->wingad,whw->window,NULL,
                              INTEGER_Number,tag->ti_Data,
                              TAG_END);
                           whw->windownr=tag->ti_Data;
                           Updatetaskattrs(AOWHW_Getcurrent,TRUE,TAG_END);
                           ObtainSemaphore(whw->whissema);
                           Rebuild(whw,FALSE);
                           ReleaseSemaphore(whw->whissema);
                        }
                        break;
                     case AOWHW_Changed:
                        if(!whw->filter || (long)tag->ti_Data<0
                        || tag->ti_Data==Getvalue(whw->wingad,INTEGER_Number))
                        {  ObtainSemaphore(whw->whissema);
                           Rebuild(whw,TRUE);
                           ReleaseSemaphore(whw->whissema);
                        }
                        break;
                     case AOWHW_Current:
                        whis=(struct Winhis *)tag->ti_Data;
                        if(!whw->filter
                        || whis->windownr==Getvalue(whw->wingad,INTEGER_Number))
                        {  ObtainSemaphore(whw->whissema);
                           whw->current=whis;
                           if(whw->order<=WHIS_MAINLINE) Setcurrentnode(whw);
                           ReleaseSemaphore(whw->whissema);
                        }
                        break;
                  }
               }
            }
            Replytaskmsg(wm);
         }
         if(!done && (getmask&whw->winsigmask))
         {  done=Handlewinhiswindow(whw);
         }
      }
      Closewinhiswindow(whw);
   }
   Updatetaskattrs(AOTSK_Async,TRUE,
      AOWHW_Close,TRUE,
      TAG_END);
}

/*-----------------------------------------------------------------------*/
