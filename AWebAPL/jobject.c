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

/* jobject.c - AWeb js generic Object object */

#include "awebjs.h"
#include "jprotos.h"

/*-----------------------------------------------------------------------*/

/* Make (jthis) a new Object object */
static void Constructor(struct Jcontext *jc)
{
}

/*-----------------------------------------------------------------------*/

void Initobject(struct Jcontext *jc)
{  struct Jobject *jo;
   if(jo=Internalfunction(jc,"Object",Constructor,NULL))
   {  Addprototype(jc,jo);
      Addglobalfunction(jc,jo);
      jc->string=jo;
      Keepobject(jo,TRUE);
   }
}

