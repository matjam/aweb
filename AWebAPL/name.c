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

/* name.c - AWeb html document name element object */

#include "aweb.h"
#include "name.h"

#include <clib/utility_protos.h>

/*------------------------------------------------------------------------*/

struct Name
{  struct Element elt;
};

/*------------------------------------------------------------------------*/

static struct Name *Newname(struct Amset *ams)
{  struct Name *nam;
   if(nam=Allocobject(AOTP_NAME,sizeof(struct Name),ams))
   {  Amethodas(AOTP_ELEMENT,nam,AOM_SET,ams->tags);
      Asetattrs(nam,
         AOELT_Valign,VALIGN_TOP,
         AOELT_Visible,FALSE,
         TAG_END);
   }
   return nam;
}

static long Dispatch(struct Name *nam,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newname((struct Amset *)amsg);
         break;
      case AOM_DEINSTALL:
         break;
      default:
         result=AmethodasA(AOTP_ELEMENT,nam,amsg);
   }
   return result;
}

/*------------------------------------------------------------------------*/

BOOL Installname(void)
{  if(!Amethod(NULL,AOM_INSTALL,AOTP_NAME,Dispatch)) return FALSE;
   return TRUE;
}
