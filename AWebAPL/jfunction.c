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

/* jfunction.c - AWeb js internal Function object */

#include "awebjs.h"
#include "jprotos.h"

/*-----------------------------------------------------------------------*/

/* Make (jthis) a new Function object */
/* Turn it into a function, create an Elementfunc with the ET_FUNCEVAL
 * element for the body string */
static void Constructor(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Jobject *args;
   struct Variable *var;
   struct Elementfunc *func;
   struct Elementnode *enode;
   struct Elementstring *eid;
   struct Element *elt;
   long n;
   if(jo)
   {  if(func=ALLOCSTRUCT(Elementfunc,1,0,jc->pool))
      {  NEWLIST(&func->subs);
         func->type=ET_FUNCTION;
         func->name=Jdupstr("Function",-1,jc->pool);
         if(args=jc->functions.first->arguments)
         {  /* All but the last element in the arguments array are formal parameters */
            for(n=0;Arrayelt(args,n+1);n++)
            {  var=Arrayelt(args,n);
               Tostring(&var->val,jc);
               if(eid=ALLOCSTRUCT(Elementstring,1,0,jc->pool))
               {  eid->type=ET_IDENTIFIER;
                  eid->svalue=Jdupstr(var->val.svalue,-1,jc->pool);
                  if(enode=ALLOCSTRUCT(Elementnode,1,0,jc->pool))
                  {  enode->sub=eid;
                     ADDTAIL(&func->subs,enode);
                  }
               }
            }
            /* The last element is the body string */
            if(var=Arrayelt(args,n))
            {  Tostring(&var->val,jc);
               if(eid=ALLOCSTRUCT(Elementstring,1,0,jc->pool))
               {  eid->type=ET_STRING;
                  eid->svalue=Jdupstr(var->val.svalue,-1,jc->pool);
               }
               if(elt=ALLOCSTRUCT(Element,1,0,jc->pool))
               {  elt->type=ET_FUNCEVAL;
                  elt->sub1=eid;
                  func->body=elt;
               }
            }
         }
         jo->function=func;
      }
   }
}

/*-----------------------------------------------------------------------*/

void Initfunction(struct Jcontext *jc)
{  struct Jobject *jo;
   if(jo=Internalfunction(jc,"Function",Constructor,NULL))
   {  Addprototype(jc,jo);
      Addglobalfunction(jc,jo);
      jc->function=jo;
      Keepobject(jo,TRUE);
   }
}
