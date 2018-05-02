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

/* authorlib.o - AWeb authorize AwebLib module */

#include "aweblib.h"
#include "application.h"
#include "task.h"
#include <exec/resident.h>
#include <intuition/intuition.h>
#include <classact.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/utility_protos.h>

#include "authorlib.h"

enum AUTHGADGET_IDS
{  AGID_OK=1,AGID_CANCEL,
};

struct Aewinfo    /* UserData */
{  struct Authnode *authnode;
   UBYTE *server;
   UBYTE *userid;       /* Also buffer */
   UBYTE *password;     /* points into userid buffer */
   struct Node *node;   /* LB node */
};

enum AUTHEDIT_GADGETIDS
{  EGID_LIST=1,EGID_DEL,EGID_USERID,EGID_PASSWORD,
};

static struct ColumnInfo columninfo[]=
{  50,NULL,0,
   50,NULL,0,
   -1,NULL,0,
};

static struct Hook idcmphook;

void *AwebPluginBase;
struct ExecBase *SysBase;
void *DOSBase,*IntuitionBase,*UtilityBase;
struct ClassLibrary *WindowBase,*LayoutBase,*ButtonBase,*ListBrowserBase,
   *StringBase,*ChooserBase,*CheckBoxBase,*SpaceBase,*LabelBase,*GlyphBase;

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

__asm __saveds void Authorreq(
   register __a0 struct Authorreq *areq);

__asm __saveds void Authedittask(
   register __a0 struct Authedit *aew);

__asm __saveds void Authorset(
   register __a0 struct Authorize *auth,
   register __a1 UBYTE *userid,
   register __a2 UBYTE *passwd);

/* Function declarations for project dependent hook functions */
static ULONG Initaweblib(struct Library *libbase);
static void Expungeaweblib(struct Library *libbase);

struct Library *AuthorizeBase;

static APTR libseglist;

LONG __saveds __asm Libstart(void)
{  return -1;
}

static APTR functable[]=
{  Openlib,
   Closelib,
   Expungelib,
   Extfunclib,
   Authorreq,
   Authedittask,
   Authorset,
   (APTR)-1
};

/* Init table used in library initialization. */
static ULONG inittab[]=
{  sizeof(struct Library),
   (ULONG) functable,
   0,
   (ULONG) Initlib
};

static char __aligned libname[]="authorize.aweblib";
static char __aligned libid[]="authorize.aweblib " AWEBLIBVSTRING " " __AMIGADATE__;

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
   AuthorizeBase=libbase;
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
   return TRUE;
}

static void Expungeaweblib(struct Library *libbase)
{  if(LabelBase) CloseLibrary(LabelBase);
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
   if(IntuitionBase) CloseLibrary(IntuitionBase);
   if(DOSBase) CloseLibrary(DOSBase);
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

static UBYTE base64[]=
   "ABCDEFGHIJKLMNOP"
   "QRSTUVWXYZabcdef"
   "ghijklmnopqrstuv"
   "wxyz0123456789+/";

/* Encodes userid:password into Authorize structure */
static void Bakecookie(struct Authorize *auth,UBYTE *userid,UBYTE *password)
{  UBYTE *cookie,*temp;
   long length1=strlen(userid)+1+strlen(password);
   long length2=length1*4/3+5;
   UBYTE *src,*dest,c;
   short t;
   if(temp=ALLOCTYPE(UBYTE,length1+1,0))
   {  strcpy(temp,userid);
      strcat(temp,":");
      strcat(temp,password);
      if(cookie=ALLOCTYPE(UBYTE,length2,MEMF_PUBLIC))
      {  src=temp;
         dest=cookie;
         t=0;
         while(*src)
         {  switch(t)
            {  case 0:  /* xxxxxx.. */
                  c=(src[0]&0xfc)>>2;
                  break;
               case 1:  /* ......xx xxxx.... */
                  c=((src[0]&0x03)<<4) | ((src[1]&0xf0)>>4);
                  src++;
                  break;
               case 2:  /* ....xxxx xx...... */
                  c=((src[0]&0x0f)<<2) | ((src[1]&0xc0)>>6);
                  src++;
                  break;
               case 3:  /* ..xxxxxx */
                  c=(src[0]&0x3f);
                  src++;
                  break;
            }
            *dest++=base64[c];
            if(++t>3) t=0;
         }
         if(t)
         {  while(t++<4) *dest++='=';
         }
         *dest='\0';
         if(auth->cookie) FREE(auth->cookie);
         auth->cookie=cookie;
      }
      FREE(temp);
   }
}

/* Returns 6bits data, or 0xff if invalid. */
static UBYTE Unbase64(UBYTE c)
{  if(c>='A' && c<='Z') c-=0x41;
   else if(c>='a' && c<='z') c-=0x47;
   else if(c>='0' && c<='9') c+=4;
   else if(c=='+') c=0x3e;
   else if(c=='/') c=0x3f;
   else c=0xff;
   return c;
}

/* Decodes cookie from Authorize. Creates dynamic string "userid\0password"*/
static UBYTE *Solvecookie(struct Authorize *auth)
{  UBYTE *temp,*src,*dest,*p;
   UBYTE c;
   short t;
   long len=strlen(auth->cookie)  /* *3/4+5; */   *2;
   if(temp=ALLOCTYPE(UBYTE,len,MEMF_CLEAR))
   {  src=auth->cookie;
      dest=temp;
      t=0;
      while(*src)
      {  c=Unbase64(*src);
         if(c==0xff) break;
         switch(t)
         {  case 0:  /* xxxxxx.. */
               *dest=c<<2;
               break;
            case 1:  /* ......xx xxxx.... */
               *dest|=(c>>4)&0x03;
               dest++;
               *dest=c<<4;
               break;
            case 2:  /* ....xxxx xx...... */
               *dest|=(c>>2)&0x0f;
               dest++;
               *dest=c<<6;
               break;
            case 3:  /* ..xxxxxx */
               *dest|=c&0x3f;
               dest++;
               break;
         }
         if(++t>3) t=0;
         src++;
      }
      if(p=strchr(temp,':'))
      {  *p='\0';
      }
   }
   return temp;
}

static void Freeauthinfo(struct Aewinfo *ai)
{  if(ai)
   {  if(ai->userid) FREE(ai->userid);
      if(ai->server) FREE(ai->server);
      FREE(ai);
   }
}

static struct Aewinfo *Newauthinfo(struct Authnode *an)
{  struct Aewinfo *ai=NULL;
   struct Authorize *auth=&an->auth;
   long len;
   if(auth->server && auth->realm && auth->cookie)
   {  if(ai=ALLOCSTRUCT(Aewinfo,1,MEMF_CLEAR))
      {  ai->authnode=an;
         len=strlen(auth->server)+strlen(auth->realm)+4;
         if(ai->server=ALLOCTYPE(UBYTE,len,0))
         {  sprintf(ai->server,"%s (%s)",auth->server,auth->realm);
         }
         if(ai->userid=Solvecookie(auth))
         {  ai->password=ai->userid+strlen(ai->userid)+1;
         }
         if(!ai->server || !ai->userid)
         {  Freeauthinfo(ai);
            ai=NULL;
         }
      }
   }
   return ai;
}

static void Freelbnodes(struct List *list)
{  struct Node *node;
   struct Aewinfo *ai;
   while(node=RemHead(list))
   {  ai=NULL;
      GetListBrowserNodeAttrs(node,LBNA_UserData,&ai,TAG_END);
      Freeauthinfo(ai);
      FreeListBrowserNode(node);
   }
}

static struct Node *Allocaewnode(struct Authedit *aew,struct Authnode *an)
{  struct Node *node=NULL;
   struct Aewinfo *ai;
   if(ai=Newauthinfo(an))
   {  if(node=AllocListBrowserNode(3,
         LBNA_UserData,ai,
         LBNA_Column,0,
            LBNCA_Text,ai->server,
         LBNA_Column,1,
            LBNCA_Text,ai->userid,
         TAG_END))
      {  ai->node=node;
      }
      else
      {  Freeauthinfo(ai);
      }
   }
   return node;
}

/* returns number of selected node */
static long Allocnodes(struct Authedit *aew,struct List *list,struct Authnode *select)
{  struct Node *node;
   struct Authnode *an;
   long selected=-1,n;
   ObtainSemaphore(aew->authsema);
   for(an=aew->auths->first,n=0;an->next;an=an->next,n++)
   {  if(an->auth.cookie)
      {  if(node=Allocaewnode(aew,an))
         {  AddTail(list,node);
            if(an==select) selected=n;
         }
      }
   }
   ReleaseSemaphore(aew->authsema);
   return selected;
}

static struct Aewinfo *Selectedinfo(struct Authedit *aew)
{  long selected=Getvalue(aew->listgad,LISTBROWSER_Selected);
   struct Node *node=Getnode(&aew->gadlist,selected);
   struct Aewinfo *ai=NULL;
   if(node) GetListBrowserNodeAttrs(node,LBNA_UserData,&ai,TAG_END);
   return ai;
}

static void Setgads(struct Authedit *aew)
{  struct Aewinfo *ai=Selectedinfo(aew);
   Setgadgetattrs(aew->servergad,aew->window,NULL,
      GA_Text,ai?ai->server:NULLSTRING,
      TAG_END);
   Setgadgetattrs(aew->delgad,aew->window,NULL,
      GA_Disabled,!ai,
      TAG_END);
   Setgadgetattrs(aew->useridgad,aew->window,NULL,
      STRINGA_TextVal,ai?ai->userid:NULLSTRING,
      STRINGA_DispPos,0,
      GA_Disabled,!ai,
      TAG_END);
   Setgadgetattrs(aew->passwordgad,aew->window,NULL,
      STRINGA_TextVal,ai?ai->password:NULLSTRING,
      STRINGA_DispPos,0,
      GA_Disabled,!ai,
      TAG_END);
}

/* List has changed. Build new gadgetlist, but remember selection */
static void Modifiedlist(struct Authedit *aew)
{  long selected;
   struct Aewinfo *ai=Selectedinfo(aew);
   struct Authnode *anselect=NULL;
   if(ai) anselect=ai->authnode;
   Setgadgetattrs(aew->listgad,aew->window,NULL,LISTBROWSER_Labels,~0,TAG_END);
   Freelbnodes(&aew->gadlist);
   selected=Allocnodes(aew,&aew->gadlist,anselect);
   Setgadgetattrs(aew->listgad,aew->window,NULL,
      LISTBROWSER_Labels,&aew->gadlist,
      LISTBROWSER_Selected,selected,
      LISTBROWSER_MakeVisible,selected,
      TAG_END);
   Setgads(aew);
}

static void Delentry(struct Authedit *aew)
{  struct Aewinfo *ai=Selectedinfo(aew);
   if(ai)
   {  Updatetaskattrs(
         AOAEW_Authnode,ai->authnode,
         AOAEW_Delete,TRUE,
         TAG_END);
      Setgadgetattrs(aew->listgad,aew->window,NULL,LISTBROWSER_Labels,~0,TAG_END);
      Freelbnodes(&aew->gadlist);
      Allocnodes(aew,&aew->gadlist,NULL);
      Setgadgetattrs(aew->listgad,aew->window,NULL,
         LISTBROWSER_Labels,&aew->gadlist,
         TAG_END);
      Setgads(aew);
   }
}

static void Changeentry(struct Authedit *aew)
{  struct Aewinfo *ai=Selectedinfo(aew);
   UBYTE *userid,*password;
   struct Authorize auth={0};
   struct Authnode *anselect;
   long selected;
   if(ai)
   {  userid=(UBYTE *)Getvalue(aew->useridgad,STRINGA_TextVal);
      password=(UBYTE *)Getvalue(aew->passwordgad,STRINGA_TextVal);
      Bakecookie(&auth,userid,password);
      Updatetaskattrs(
         AOAEW_Authnode,ai->authnode,
         AOAEW_Change,&auth,
         TAG_END);
      anselect=ai->authnode;
      Setgadgetattrs(aew->listgad,aew->window,NULL,LISTBROWSER_Labels,~0,TAG_END);
      Freelbnodes(&aew->gadlist);
      selected=Allocnodes(aew,&aew->gadlist,anselect);
      Setgadgetattrs(aew->listgad,aew->window,NULL,
         LISTBROWSER_Labels,&aew->gadlist,
         LISTBROWSER_Selected,selected,
         LISTBROWSER_MakeVisible,selected,
         TAG_END);
   }
}

static void Moveselected(struct Authedit *aew,short n)
{  long selected;
   selected=Getvalue(aew->listgad,LISTBROWSER_Selected);
   if(n<0)
   {  if(selected>0)
      {  selected--;
      }
      else
      {  selected=Getvalue(aew->listgad,LISTBROWSER_TotalNodes)-1;
      }
   }
   else if(n>0)
   {  struct Node *node=Getnode(&aew->gadlist,selected);
      if(node && node->ln_Succ->ln_Succ)
      {  selected++;
      }
      else
      {  selected=0;
      }
   }
   Setgadgetattrs(aew->listgad,aew->window,NULL,
      LISTBROWSER_Selected,selected,
      LISTBROWSER_MakeVisible,selected,
      TAG_END);
   Setgads(aew);
}

static long __saveds __asm Idcmphook(register __a0 struct Hook *hook,
   register __a1 struct IntuiMessage *msg)
{  struct Authedit *aew=hook->h_Data;
   switch(msg->Class)
   {  case IDCMP_CHANGEWINDOW:
         Updatetaskattrs(AOAEW_Dimx,aew->window->LeftEdge,
            AOAEW_Dimy,aew->window->TopEdge,
            AOAEW_Dimw,aew->window->Width-aew->window->BorderLeft-aew->window->BorderRight,
            AOAEW_Dimh,aew->window->Height-aew->window->BorderTop-aew->window->BorderBottom,
            TAG_END);
         break;
   }
   return 0;
}

/*-----------------------------------------------------------------------*/

static void Buildautheditwindow(struct Authedit *aew)
{  NewList(&aew->gadlist);
   columninfo[0].ci_Title=AWEBSTR(MSG_AUTHEDIT_TITLE_SERVER);
   columninfo[1].ci_Title=AWEBSTR(MSG_AUTHEDIT_TITLE_USERID);
   idcmphook.h_Entry=(HOOKFUNC)Idcmphook;
   idcmphook.h_Data=aew;
   if(aew->screen=LockPubScreen(aew->screenname))
   {  ObtainSemaphore(aew->authsema);
      Allocnodes(aew,&aew->gadlist,NULL);
      ReleaseSemaphore(aew->authsema);
      if(!aew->w)
      {  aew->x=aew->screen->Width/4;
         aew->y=aew->screen->Height/4;
         aew->w=aew->screen->Width/2;
         aew->h=aew->screen->Height/2;
      }
      aew->winobj=WindowObject,
         WA_Title,AWEBSTR(MSG_AUTHEDIT_TITLE),
         WA_Left,aew->x,
         WA_Top,aew->y,
         WA_InnerWidth,aew->w,
         WA_InnerHeight,aew->h,
         WA_SizeGadget,TRUE,
         WA_DepthGadget,TRUE,
         WA_DragBar,TRUE,
         WA_CloseGadget,TRUE,
         WA_Activate,TRUE,
         WA_AutoAdjust,TRUE,
         WA_SimpleRefresh,TRUE,
         WA_PubScreen,aew->screen,
         WA_IDCMP,IDCMP_RAWKEY,
         WINDOW_IDCMPHook,&idcmphook,
         WINDOW_IDCMPHookBits,IDCMP_CHANGEWINDOW,
         WINDOW_Layout,aew->toplayout=VLayoutObject,
            LAYOUT_SpaceOuter,TRUE,
            LAYOUT_DeferLayout,TRUE,
            StartMember,aew->listgad=ListBrowserObject,
               GA_ID,EGID_LIST,
               GA_RelVerify,TRUE,
               LISTBROWSER_ShowSelected,TRUE,
               LISTBROWSER_ColumnInfo,columninfo,
               LISTBROWSER_Labels,&aew->gadlist,
               LISTBROWSER_ColumnTitles,TRUE,
               LISTBROWSER_AutoFit,TRUE,
               LISTBROWSER_HorizontalProp,TRUE,
            EndMember,
            StartMember,HLayoutObject,
               StartMember,VLayoutObject,
                  StartMember,aew->servergad=ButtonObject,
                     GA_ReadOnly,TRUE,
                     GA_Text,"",
                     BUTTON_Justification,BCJ_LEFT,
                     CLASSACT_Underscore,0,
                  EndMember,
                  MemberLabel(AWEBSTR(MSG_AUTHEDIT_SERVER)),
                  StartMember,aew->useridgad=StringObject,
                     GA_ID,EGID_USERID,
                     GA_RelVerify,TRUE,
                     GA_TabCycle,TRUE,
                     STRINGA_TextVal,"",
                     STRINGA_MaxChars,127,
                     GA_Disabled,TRUE,
                  EndMember,
                  MemberLabel(AWEBSTR(MSG_AUTHEDIT_USERID)),
                  StartMember,aew->passwordgad=StringObject,
                     GA_ID,EGID_PASSWORD,
                     GA_RelVerify,TRUE,
                     GA_TabCycle,TRUE,
                     STRINGA_TextVal,"",
                     STRINGA_MaxChars,127,
                     STRINGA_Font,aew->pwfont,
                     GA_Disabled,TRUE,
                  EndMember,
                  MemberLabel(AWEBSTR(MSG_AUTHEDIT_PASSWORD)),
               EndMember,
               StartMember,VLayoutObject,
                  StartMember,aew->delgad=ButtonObject,
                     GA_ID,EGID_DEL,
                     GA_RelVerify,TRUE,
                     GA_Text,AWEBSTR(MSG_AUTHEDIT_DEL),
                     GA_Disabled,TRUE,
                  EndMember,
                  CHILD_WeightedHeight,0,
               EndMember,
               CHILD_WeightedWidth,0,
            EndMember,
            CHILD_WeightedHeight,0,
         EndMember,
      EndWindow;
      if(aew->winobj)
      {  if(aew->window=(struct Window *)CA_OpenWindow(aew->winobj))
         {  GetAttr(WINDOW_SigMask,aew->winobj,&aew->winsigmask);
         }
      }
   }
}

static BOOL Handleautheditwindow(struct Authedit *aew)
{  ULONG result;
   BOOL done=FALSE;
   USHORT click;
   while((result=CA_HandleInput(aew->winobj,&click))!=WMHI_LASTMSG)
   {  switch(result&WMHI_CLASSMASK)
      {  case WMHI_CLOSEWINDOW:
            done=TRUE;
            break;
         case WMHI_GADGETUP:
            switch(result&WMHI_GADGETMASK)
            {  case EGID_LIST:
                  Setgads(aew);
                  break;
               case EGID_DEL:
                  Delentry(aew);
                  break;
               case EGID_USERID:
               case EGID_PASSWORD:
                  Changeentry(aew);
                  break;
            }
            break;
         case WMHI_RAWKEY:
            switch(result&WMHI_GADGETMASK)
            {  case 0x45:  /* esc */
                  done=TRUE;
                  break;
               case 0x4c:  /* up */
                  Moveselected(aew,-1);
                  break;
               case 0x4d:  /* down */
                  Moveselected(aew,1);
                  break;
            }
            break;
/*
         case WMHI_CHANGEWINDOW:
            Updatetaskattrs(AOAEW_Dimx,aew->window->LeftEdge,
               AOAEW_Dimy,aew->window->TopEdge,
               AOAEW_Dimw,aew->window->Width-aew->window->BorderLeft-aew->window->BorderRight,
               AOAEW_Dimh,aew->window->Height-aew->window->BorderTop-aew->window->BorderBottom,
               TAG_END);
            break;
*/
      }
   }
   return done;
}

static void Closeautheditwindow(struct Authedit *aew)
{  if(aew->winobj)
   {  if(aew->window)
      {  Updatetaskattrs(AOAEW_Dimx,aew->window->LeftEdge,
            AOAEW_Dimy,aew->window->TopEdge,
            AOAEW_Dimw,aew->window->Width-aew->window->BorderLeft-aew->window->BorderRight,
            AOAEW_Dimh,aew->window->Height-aew->window->BorderTop-aew->window->BorderBottom,
            TAG_END);
      }
      DisposeObject(aew->winobj);
   }
   if(aew->screen) UnlockPubScreen(NULL,aew->screen);aew->screen=NULL;
   Freelbnodes(&aew->gadlist);
}

__asm __saveds void Authedittask(register __a0 struct Authedit *aew)
{  struct Taskmsg *em;
   BOOL done=FALSE;
   ULONG getmask;
   struct TagItem *tag,*tstate;
   Buildautheditwindow(aew);
   if(aew->window)
   {  while(!done)
      {  getmask=Waittask(aew->winsigmask);
         while(!done && (em=Gettaskmsg()))
         {  if(em->amsg && em->amsg->method==AOM_SET)
            {  tstate=((struct Amset *)em->amsg)->tags;
               while(tag=NextTagItem(&tstate))
               {  switch(tag->ti_Tag)
                  {  case AOTSK_Stop:
                        if(tag->ti_Data) done=TRUE;
                        break;
                     case AOAEW_Tofront:
                        if(tag->ti_Data)
                        {  WindowToFront(aew->window);
                           ActivateWindow(aew->window);
                        }
                        break;
                     case AOAEW_Modified:
                        if(tag->ti_Data)
                        {  Modifiedlist(aew);
                        }
                        break;
                  }
               }
            }
            Replytaskmsg(em);
         }
         if(!done && (getmask&aew->winsigmask))
         {  done=Handleautheditwindow(aew);
         }
      }
      Closeautheditwindow(aew);
   }
   Updatetaskattrs(AOTSK_Async,TRUE,
      AOAEW_Close,TRUE,
      TAG_END);
}

/*-----------------------------------------------------------------------*/

__asm __saveds void Authorreq(register __a0 struct Authorreq *areq)
{  void *useridgad,*passwordgad,*toplayout;
   UBYTE *userid,*password;
   Object *winobj;
   struct Window *window;
   UBYTE *screenname;
   struct TextFont *pwfont;
   struct Screen *screen;
   struct Buffer namebuf={0};
   UBYTE *p;
   Agetattrs(Aweb(),
      AOAPP_Screenname,&screenname,
      AOAPP_Pwfont,&pwfont,
      TAG_END);
   screen=LockPubScreen(screenname);
   p=areq->name;
   while(strlen(p)>70)
   {  Addtobuffer(&namebuf,p,70);
      Addtobuffer(&namebuf,"\n",1);
      p+=70;
   }
   Addtobuffer(&namebuf,p,-1);
   Addtobuffer(&namebuf,"",1);
   if(screen)
   {  winobj=WindowObject,
         WA_Title,AWEBSTR(MSG_AUTH_TITLE),
         WA_Left,40,
         WA_Top,60,
         WA_AutoAdjust,TRUE,
         WA_CloseGadget,TRUE,
         WA_DragBar,TRUE,
         WA_DepthGadget,TRUE,
         WA_SizeGadget,TRUE,
         WA_Activate,Awebactive(),
         WA_PubScreen,screen,
         WINDOW_Position,WPOS_CENTERSCREEN,
         WINDOW_Layout,toplayout=VLayoutObject,
            LAYOUT_VertAlignment,LALIGN_TOP,
            LAYOUT_FixedVert,FALSE,
            StartMember,VLayoutObject,
               LAYOUT_SpaceOuter,TRUE,
               StartMember,HLayoutObject,
                  LAYOUT_HorizAlignment,LALIGN_CENTER,
                  StartImage,LabelObject,
                     LABEL_Text,AWEBSTR(areq->proxy?MSG_AUTH_PROMPT_PROXY:MSG_AUTH_PROMPT),
                     LABEL_Text,"\n",
                     LABEL_Text,namebuf.buffer,
                     LABEL_Text,"\n(",
                     LABEL_Text,areq->auth->realm,
                     LABEL_Text,")",
                     LABEL_Justification,LJ_CENTRE,
                     LABEL_Underscore,0,
                  EndImage,
               EndMember,
               StartMember,useridgad=StringObject,
                  STRINGA_TextVal,"",
                  STRINGA_MaxChars,127,
                  GA_TabCycle,TRUE,
               EndMember,
               CHILD_Label,LabelObject,
                  LABEL_Text,AWEBSTR(MSG_AUTH_USERID),
               EndMember,
               StartMember,passwordgad=StringObject,
                  STRINGA_TextVal,"",
                  STRINGA_MaxChars,127,
                  STRINGA_Font,pwfont,
                  GA_TabCycle,TRUE,
               EndMember,
               CHILD_Label,LabelObject,
                  LABEL_Text,AWEBSTR(MSG_AUTH_PASSWORD),
               EndMember,
            EndMember,
            StartMember,HLayoutObject,
               LAYOUT_SpaceOuter,TRUE,
               LAYOUT_BevelStyle,BVS_SBAR_VERT,
               StartMember,ButtonObject,
                  GA_Text,AWEBSTR(MSG_AUTH_OK),
                  GA_ID,AGID_OK,
                  GA_RelVerify,TRUE,
               EndMember,
               StartMember,SpaceObject,
               EndMember,
               StartMember,ButtonObject,
                  GA_Text,AWEBSTR(MSG_AUTH_CANCEL),
                  GA_ID,AGID_CANCEL,
                  GA_RelVerify,TRUE,
               EndMember,
            EndMember,
         End,
      EndWindow;
      if(winobj)
      {  if(window=CA_OpenWindow(winobj))
         {  ULONG sigmask=0,getmask;
            ULONG result;
            BOOL done=FALSE;
            GetAttr(WINDOW_SigMask,winobj,&sigmask);
            ActivateLayoutGadget(toplayout,window,NULL,useridgad);
            while(!done)
            {  getmask=Wait(sigmask|SIGBREAKF_CTRL_C);
               if(getmask&SIGBREAKF_CTRL_C) break;
               while((result=CA_HandleInput(winobj,NULL))!=WMHI_LASTMSG)
               {  switch(result&WMHI_CLASSMASK)
                  {  case WMHI_CLOSEWINDOW:
                        done=TRUE;
                        break;
                     case WMHI_GADGETUP:
                        switch(result&WMHI_GADGETMASK)
                        {  case AGID_OK:
                              GetAttr(STRINGA_TextVal,useridgad,
                                 (ULONG *)&userid);
                              GetAttr(STRINGA_TextVal,passwordgad,
                                 (ULONG *)&password);
                              Bakecookie(areq->auth,userid,password);
                              areq->Rememberauth(areq->auth);
                              done=TRUE;
                              break;
                           case AGID_CANCEL:
                              done=TRUE;
                              break;
                        }
                        break;
                     case WMHI_RAWKEY:
                        if((result&WMHI_GADGETMASK)==0x45) done=TRUE;
                        break;
                  }
               }
            }
         }
         DisposeObject(winobj);
      }
   }
   Freebuffer(&namebuf);
   if(screen) UnlockPubScreen(NULL,screen);
}

/*-----------------------------------------------------------------------*/

__asm __saveds void Authorset(
   register __a0 struct Authorize *auth,
   register __a1 UBYTE *userid,
   register __a2 UBYTE *passwd)
{  Bakecookie(auth,userid,passwd);
}