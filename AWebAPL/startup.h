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

/* startup.h - AWeb startup window module */

#ifndef AWEB_STARTUP_H
#define AWEB_STARTUP_H

/* States for Startupstate() */
enum LOADREQ_STATES
{  LRQ_FONTS,LRQ_IMAGES,LRQ_CACHE
};


extern void Startupopen(struct Screen *screen,UBYTE *version);
extern void Startupstate(ULONG state);
extern void Startuplevel(long ready,long total);
extern void Startupclose(void);

#pragma libcall StartupBase Startupopen 1e 9802
#pragma libcall StartupBase Startupstate 24 001
#pragma libcall StartupBase Startuplevel 2a 1002
#pragma libcall StartupBase Startupclose 30 0

#endif
