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

/* version.c - AWeb version info */

#include "aweb.h"

static UBYTE *versionstring=
   "$VER: AWeb-II " AWEBVERSION RELEASECLASS " " __AMIGADATE__;
static UBYTE *internalstring=
   "AWeb " BETARELEASE;

UBYTE *aboutversion;

UBYTE *awebversion=
#ifdef BETAKEYFILE
   FULLRELEASE " (" BETARELEASE " beta)";
#else
   AWEBVERSION RELEASECLASS;
#endif

/*-----------------------------------------------------------------------*/

BOOL Initversion(void)
{  aboutversion=versionstring+6;
   return TRUE;
}

void Initialrequester(void (*about)(UBYTE *),UBYTE *p)
{
#ifdef POPABOUT
   about(p);
#endif
}
