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

/* jboolean.c - AWeb js internal Boolean object */

#include "awebjs.h"
#include "jprotos.h"

struct Boolean          /* Used as internal object value */
{  BOOL bvalue;
};

/*-----------------------------------------------------------------------*/

/* Convert (jthis) to string */
static void Booleantostring(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *p="false";
   BOOL b;
   if(jo && jo->internal)
   {  b=((struct Boolean *)jo->internal)->bvalue;
      if(b) p="true";
   }
   Asgstring(RETVAL(jc),p,jc->pool);
}

/* Get value of (jthis) */
static void Booleanvalueof(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   BOOL b=FALSE;
   if(jo && jo->internal)
   {  b=((struct Boolean *)jo->internal)->bvalue;
   }
   Asgboolean(RETVAL(jc),b);
}

/* Dispose a Boolean object */
static void Destructor(struct Boolean *b)
{  FREE(b);
}

/* Make (jthis) a new Boolean object */
static void Constructor(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Boolean *b;
   if(jo)
   {  if(b=ALLOCSTRUCT(Boolean,1,0,jc->pool))
      {  struct Variable *arg;
         jo->internal=b;
         jo->dispose=Destructor;
         arg=jc->functions.first->local.first;
         if(arg->next && arg->type!=VTP_UNDEFINED)
         {  Toboolean(&arg->val,jc);
            b->bvalue=jc->val->bvalue;
         }
         else
         {  b->bvalue=FALSE;
         }
      }
   }
}

/*-----------------------------------------------------------------------*/

void Initboolean(struct Jcontext *jc)
{  struct Jobject *jo,*f;
   if(jo=Internalfunction(jc,"Boolean",Constructor,"BooleanLiteral",NULL))
   {  Addprototype(jc,jo);
      if(f=Internalfunction(jc,"toString",Booleantostring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"valueOf",Booleanvalueof,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      Addglobalfunction(jc,jo);
      jc->boolean=jo;
      Keepobject(jo,TRUE);
   }
}

/* Create a new Boolean object. */
struct Jobject *Newboolean(struct Jcontext *jc,BOOL bvalue)
{  struct Jobject *jo;
   struct Boolean *b;
   if(jo=Newobject(jc))
   {  Initconstruct(jc,jo,jc->boolean);
      if(b=ALLOCSTRUCT(Boolean,1,0,jc->pool))
      {  jo->internal=b;
         jo->dispose=Destructor;
         b->bvalue=bvalue;
      }
   }
   return jo;
}

