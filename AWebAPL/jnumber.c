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

/* jnumber.c - AWeb js internal Number object */

#include "awebjs.h"
#include "jprotos.h"

struct Number           /* Used as internal object value */
{  UBYTE attr;
   double nvalue;
};

/*-----------------------------------------------------------------------*/

/* Add a constant property */
static void Addnumberproperty(struct Jobject *jo,UBYTE *name,UBYTE attr,double n)
{  struct Variable *var;
   if(var=Addproperty(jo,name))
   {  Asgnumber(&var->val,attr,n);
      var->hook=Constantvhook;
   }
}

/*-----------------------------------------------------------------------*/

/* Convert (jthis) to string */
static void Numbertostring(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE buffer[32];
   struct Number *n;
   if(jo && (n=(struct Number *)jo->internal))
   {  switch(n->attr)
      {  case VNA_NAN:
            strcpy(buffer,"NaN");
            break;
         case VNA_INFINITY:
            strcpy(buffer,"+Infinity");
            break;
         case VNA_NEGINFINITY:
            strcpy(buffer,"-Infinity");
            break;
         default:
            sprintf(buffer,"%.20lg",n->nvalue);
            break;
      }
      Asgstring(RETVAL(jc),buffer,jc->pool);
   }
}

/* Get value of (jthis) */
static void Numbervalueof(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   double n=0.0;
   UBYTE attr=VNA_NAN;
   if(jo && jo->internal)
   {  n=((struct Number *)jo->internal)->nvalue;
      attr=((struct Number *)jo->internal)->attr;
   }
   Asgnumber(RETVAL(jc),attr,n);
}

/* Dispose a Number object */
static void Destructor(struct Number *n)
{  FREE(n);
}

/* Make (jthis) a new Number object */
static void Constructor(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Number *n;
   if(jo)
   {  if(n=ALLOCSTRUCT(Number,1,0,jc->pool))
      {  struct Variable *arg;
         jo->internal=n;
         jo->dispose=Destructor;
         arg=jc->functions.first->local.first;
         if(arg->next && arg->type!=VTP_UNDEFINED)
         {  Tonumber(&arg->val,jc);
            n->attr=jc->val->attr;
            n->nvalue=jc->val->nvalue;
         }
         else
         {  n->attr=VNA_VALID;
            n->nvalue=0.0;
         }
      }
   }
}

/*-----------------------------------------------------------------------*/

void Initnumber(struct Jcontext *jc)
{  struct Jobject *jo,*f;
   if(jo=Internalfunction(jc,"Number",Constructor,"NumericLiteral",NULL))
   {  Addprototype(jc,jo);
      Addnumberproperty(jo,"MAX_VALUE",VNA_VALID,1.797e308);
      Addnumberproperty(jo,"MIN_VALUE",VNA_VALID,2.222e-307);
      Addnumberproperty(jo,"NaN",VNA_NAN,0.0);
      Addnumberproperty(jo,"NEGATIVE_INFINITY",VNA_NEGINFINITY,0.0);
      Addnumberproperty(jo,"POSITIVE_INFINITY",VNA_INFINITY,0.0);
      if(f=Internalfunction(jc,"toString",Numbertostring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"valueOf",Numbervalueof,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      Addglobalfunction(jc,jo);
      jc->number=jo;
      Keepobject(jo,TRUE);
   }
}

/* Create a new Number object. */
struct Jobject *Newnumber(struct Jcontext *jc,UBYTE attr,double nvalue)
{  struct Jobject *jo;
   struct Number *n;
   if(jo=Newobject(jc))
   {  Initconstruct(jc,jo,jc->number);
      if(n=ALLOCSTRUCT(Number,1,0,jc->pool))
      {  jo->internal=n;
         jo->dispose=Destructor;
         n->attr=attr;
         n->nvalue=nvalue;
      }
   }
   return jo;
}
