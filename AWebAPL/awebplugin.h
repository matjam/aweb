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

/* awebplugin.h - AWebLib modules private awebplugin.library definitions */

#ifndef AWEBPLUGIN_H
#define AWEBPLUGIN_H

/*--- copy of api/include/clib/awebplugin_protos.h ---*/

ULONG AmethodA(struct Aobject *object,struct Amessage *amessage);
ULONG Amethod(struct Aobject *object,...);
ULONG AmethodasA(ULONG objtype,struct Aobject *object,
   struct Amessage *amessage);
ULONG Amethodas(ULONG objtype,struct Aobject *object,...);

APTR AnewobjectA(ULONG objtype,struct TagItem *taglist);
APTR Anewobject(ULONG objtype,...);
void Adisposeobject(struct Aobject *object);
ULONG AsetattrsA(struct Aobject *object,struct TagItem *taglist);
ULONG Asetattrs(struct Aobject *object,...);
ULONG AgetattrsA(struct Aobject *object,struct TagItem *taglist);
ULONG Agetattrs(struct Aobject *object,...);
ULONG Agetattr(struct Aobject *object,ULONG attrid);
ULONG AupdateattrsA(struct Aobject *object,struct TagItem *maplist,
   struct TagItem *taglist);
ULONG Aupdateattrs(struct Aobject *object,struct TagItem *maplist,...);
ULONG Arender(struct Aobject *object,struct Coords *coords,LONG minx,
   LONG miny,LONG maxx,LONG maxy,USHORT flags,APTR text);
ULONG Aaddchild(struct Aobject *object,struct Aobject *child,ULONG relation);
ULONG Aremchild(struct Aobject *object,struct Aobject *child,ULONG relation);
ULONG Anotify(struct Aobject *object,struct Amessage *amessage);

APTR Allocobject(ULONG objtype,LONG size,struct Amset *amset);
ULONG AnotifysetA(struct Aobject *object,struct TagItem *taglist);
ULONG Anotifyset(struct Aobject *object,...);

APTR Allocmem(ULONG size,ULONG flags);
STRPTR Dupstr(STRPTR string,LONG length);
void Freemem(APTR mem);

struct Coords *Clipcoords(struct Aobject *frame,struct Coords *coords);
void Unclipcoords(struct Coords *coords);
void Erasebg(struct Aobject *frame,struct Coords *coords,LONG minx,LONG miny,
   LONG maxx,LONG maxy);

struct Aobject *Aweb(void);

void AsetattrsasyncA(struct Aobject *task,struct TagItem *taglist);
void Asetattrsasync(struct Aobject *task,...);
ULONG Waittask(ULONG signals);
struct Taskmsg *Gettaskmsg(void);
void Replytaskmsg(struct Taskmsg *taskmsg);
ULONG Checktaskbreak(void);
ULONG Updatetask(struct Amessage *amessage);
ULONG UpdatetaskattrsA(struct TagItem *taglist);
ULONG Updatetaskattrs(ULONG attrid,...);
ULONG Obtaintasksemaphore(struct SignalSemaphore *semaphore);

LONG Avprintf(char *format,ULONG *args);
LONG Aprintf(char *format,...);

/*--- private ---*/

UBYTE *Awebstr(ULONG id);
void TcperrorA(struct Fetchdriver *fd,ULONG err,ULONG *args);
void Tcperror(struct Fetchdriver *fd,ULONG err,...);
void TcpmessageA(struct Fetchdriver *fd,ULONG msg,ULONG *args);
void Tcpmessage(struct Fetchdriver *fd,ULONG msg,...);
struct Library *Opentcp(struct Library **base,struct Fetchdriver *fd,BOOL autocon);

struct hostent *Lookup(UBYTE *name,struct Library *base);

BOOL Expandbuffer(struct Buffer *buf,long size);
BOOL Insertinbuffer(struct Buffer *buf,UBYTE *text,long length,long pos);
BOOL Addtobuffer(struct Buffer *buf,UBYTE *text,long length);
void Deleteinbuffer(struct Buffer *buf,long pos,long length);
void Freebuffer(struct Buffer *buf);

struct Locale *Locale(void);
long Lprintdate(UBYTE *buffer,UBYTE *fmt,struct DateStamp *ds);

/*--- public ---*/

struct RastPort *Obtainbgrp(struct Aobject *frame,struct Coords *coords,
   LONG minx,LONG miny,LONG maxx,LONG maxy);
void Releasebgrp(struct RastPort *bgrp);

/*--- private ---*/

void Setstemvar(struct Arexxcmd *ac,UBYTE *stem,long index,UBYTE *field,UBYTE *value);
void Copyprefs(struct Prefs *from,struct Prefs *to);
void Saveprefs(struct Prefs *prefs);
void Disposeprefs(struct Prefs *prefs);
void Freemenuentry(struct Menuentry *me);
struct Menuentry *Addmenuentry(void *list,USHORT type,UBYTE *title,UBYTE scut,UBYTE *cmd);
void Freemimeinfo(struct Mimeinfo *mi);
struct Mimeinfo *Addmimeinfo(LIST(Mimeinfo) *list,UBYTE *type,UBYTE *subtype,
   UBYTE *extensions,USHORT driver,UBYTE *cmd,UBYTE *args);
void Freenocache(struct Nocache *nc);
struct Nocache *Addnocache(void *list,UBYTE *name);
UBYTE *Getstemvar(struct Arexxcmd *ac,UBYTE *stem,long index,UBYTE *field);
void Freeuserbutton(struct Userbutton *ub);
struct Userbutton *Adduserbutton(void *list,UBYTE *label,UBYTE *cmd);
void Freepopupitem(struct Popupitem *pi);
struct Popupitem *Addpopupitem(void *list,USHORT flags,UBYTE *title,UBYTE *cmd);
UBYTE *Awebversion(void);
long Syncrequest(UBYTE *title,UBYTE *text,UBYTE *labels);

/* Allocate buffer and formatted print */
UBYTE *PprintfA(UBYTE *fmt,UBYTE *argspec,UBYTE **params);
UBYTE *Pprintf(UBYTE *fmt,UBYTE *argspec,...);

ULONG Today(void);

void Freeuserkey(struct Userkey *uk);
struct Userkey *Adduserkey(void *list,USHORT key,UBYTE *cmd);

long LprintfA(UBYTE *buffer,UBYTE *fmt,void *params);
long Lprintf(UBYTE *buffer,UBYTE *fmt,...);

BOOL Fullversion(void);

BOOL Awebactive(void);

void Freefontalias(struct Fontalias *fa);
struct Fontalias *Addfontalias(void *list,UBYTE *alias);

/*--- copy of api/include/pragmas/awebplugin_pragmas.h ---*/

/**/
/*--- object dispatcher*/
/**/
#pragma libcall AwebPluginBase AmethodA 1e 9802
#pragma libcall AwebPluginBase AmethodasA 24 98003
/* */
/*--- object methods*/
/* */
#pragma libcall AwebPluginBase AnewobjectA 2a 8002
#pragma libcall AwebPluginBase Adisposeobject 30 801
#pragma libcall AwebPluginBase AsetattrsA 36 9802
#pragma libcall AwebPluginBase AgetattrsA 3c 9802
#pragma libcall AwebPluginBase Agetattr 42 0802
#pragma libcall AwebPluginBase AupdateattrsA 48 A9803
#pragma libcall AwebPluginBase Arender 4e A432109808
#pragma libcall AwebPluginBase Aaddchild 54 09803
#pragma libcall AwebPluginBase Aremchild 5a 09803
#pragma libcall AwebPluginBase Anotify 60 9802
/* */
/*--- object support*/
/* */
#pragma libcall AwebPluginBase Allocobject 66 81003
#pragma libcall AwebPluginBase AnotifysetA 6c 9802
/* */
/*--- memory*/
/* */
#pragma libcall AwebPluginBase Allocmem 72 1002
#pragma libcall AwebPluginBase Dupstr 78 0802
#pragma libcall AwebPluginBase Freemem 7e 801
/* */
/*--- render*/
/* */
#pragma libcall AwebPluginBase Clipcoords 84 9802
#pragma libcall AwebPluginBase Unclipcoords 8a 801
#pragma libcall AwebPluginBase Erasebg 90 32109806
/* */
/*--- application*/
/* */
#pragma libcall AwebPluginBase Aweb 96 0
/* */
/*--- task*/
/* */
#pragma libcall AwebPluginBase AsetattrsasyncA 9c 9802
#pragma libcall AwebPluginBase Waittask a2 001
#pragma libcall AwebPluginBase Gettaskmsg a8 0
#pragma libcall AwebPluginBase Replytaskmsg ae 801
#pragma libcall AwebPluginBase Checktaskbreak b4 0
#pragma libcall AwebPluginBase Updatetask ba 801
#pragma libcall AwebPluginBase UpdatetaskattrsA c0 801
#pragma libcall AwebPluginBase Obtaintasksemaphore c6 801
/**/
/*--- debug*/
/**/
#pragma libcall AwebPluginBase Avprintf cc 9802
/**/

#ifdef __SASC
#pragma tagcall AwebPluginBase Amethod 1e 9802
#pragma tagcall AwebPluginBase Amethodas 24 98003
#pragma tagcall AwebPluginBase Anewobject 2a 8002
#pragma tagcall AwebPluginBase Asetattrs 36 9802
#pragma tagcall AwebPluginBase Agetattrs 3c 9802
#pragma tagcall AwebPluginBase Aupdateattrs 48 A9803
#pragma tagcall AwebPluginBase Anotifyset 6c 9802
#pragma tagcall AwebPluginBase Asetattrsasync 9c 9802
#pragma tagcall AwebPluginBase Updatetaskattrs c0 801
#pragma tagcall AwebPluginBase Aprintf cc 9802
#endif

/*--- private ---*/

#pragma libcall AwebPluginBase Awebstr d2 001
#pragma libcall AwebPluginBase TcperrorA d8 90803
#pragma tagcall AwebPluginBase Tcperror d8 90803
#pragma libcall AwebPluginBase TcpmessageA de 90803
#pragma tagcall AwebPluginBase Tcpmessage de 90803
#pragma libcall AwebPluginBase Opentcp e4 09803
#pragma libcall AwebPluginBase Lookup ea 9802
#pragma libcall AwebPluginBase Expandbuffer f0 0802
#pragma libcall AwebPluginBase Insertinbuffer f6 109804
#pragma libcall AwebPluginBase Addtobuffer fc 09803
#pragma libcall AwebPluginBase Deleteinbuffer 102 10803
#pragma libcall AwebPluginBase Freebuffer 108 801
#pragma libcall AwebPluginBase Locale 10e 0
#pragma libcall AwebPluginBase Lprintdate 114 A9803

/*--- public ---*/

#pragma libcall AwebPluginBase Obtainbgrp 11a 32109806
#pragma libcall AwebPluginBase Releasebgrp 120 801

/*--- private ---*/

#pragma libcall AwebPluginBase Setstemvar 126 BA09805
#pragma libcall AwebPluginBase Copyprefs 12c 9802
#pragma libcall AwebPluginBase Saveprefs 132 801
#pragma libcall AwebPluginBase Disposeprefs 138 801
#pragma libcall AwebPluginBase Freemenuentry 13e 801
#pragma libcall AwebPluginBase Addmenuentry 144 A190805
#pragma libcall AwebPluginBase Freemimeinfo 14a 801
#pragma libcall AwebPluginBase Addmimeinfo 150 DC0BA9807
#pragma libcall AwebPluginBase Freenocache 156 801
#pragma libcall AwebPluginBase Addnocache 15c 9802
#pragma libcall AwebPluginBase Getstemvar 162 A09804
#pragma libcall AwebPluginBase Freeuserbutton 168 801
#pragma libcall AwebPluginBase Adduserbutton 16e A9803
#pragma libcall AwebPluginBase Freepopupitem 174 801
#pragma libcall AwebPluginBase Addpopupitem 17a A90804
#pragma libcall AwebPluginBase Awebversion 180 0
#pragma libcall AwebPluginBase Syncrequest 186 A9803

#pragma libcall AwebPluginBase PprintfA 18c A9803
#pragma tagcall AwebPluginBase Pprintf 18c A9803

#pragma libcall AwebPluginBase Today 192 0

#pragma libcall AwebPluginBase Freeuserkey 198 801
#pragma libcall AwebPluginBase Adduserkey 19e 90803

#pragma libcall AwebPluginBase LprintfA 1a4 A9803
#pragma tagcall AwebPluginBase Lprintf 1a4 A9803

#pragma libcall AwebPluginBase Fullversion 1aa 0

#pragma libcall AwebPluginBase Setfiltertype 1b0 9802
#pragma libcall AwebPluginBase Writefilter 1b6 09803
#pragma libcall AwebPluginBase Awebcommand 1bc 198004

#pragma libcall AwebPluginBase Awebactive 1c2 0

#pragma libcall AwebPluginBase Freefontalias 1c8 801
#pragma libcall AwebPluginBase Addfontalias 1ce 9802


#endif
