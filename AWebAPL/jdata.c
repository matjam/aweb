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

/* jdata.c - AWeb js data model */

#include "awebjs.h"
#include "jprotos.h"

/*-----------------------------------------------------------------------*/

/* Call this property function */
BOOL Callproperty(struct Jcontext *jc,struct Jobject *jo,UBYTE *name)
{  struct Variable *prop;
   if(jo && (prop=Findproperty(jo,name))
   && prop->type==VTP_OBJECT && prop->ovalue && prop->ovalue->function)
   {  Callfunctionbody(jc,prop->ovalue->function,jo);
      return TRUE;
   }
   return FALSE;
}

/*-----------------------------------------------------------------------*/

/* Clear out a value */
void Clearvalue(struct Value *v)
{  switch(v->type)
   {  case VTP_STRING:
         FREE(v->svalue);
         break;
   }
   v->type=VTP_UNDEFINED;
}

/* Assign a value */
void Asgvalue(struct Value *to,struct Value *from)
{  struct Value val={0};
   /* First build up duplicate, then clear (to) to prevent object
    * being removed from under our ass.. */
   val.type=from->type;
   val.attr=from->attr;
   switch(from->type)
   {  case VTP_NUMBER:
         val.nvalue=from->nvalue;
         break;
      case VTP_BOOLEAN:
         val.bvalue=from->bvalue;
         break;
      case VTP_STRING:
         val.svalue=Jdupstr(from->svalue,-1,Getpool(from->svalue));
         break;
      case VTP_OBJECT:
         val.ovalue=from->ovalue;
         val.fthis=from->fthis;
         break;
   }
   Clearvalue(to);
   *to=val;
}

/* Assign a number */
void Asgnumber(struct Value *to,UBYTE attr,double n)
{  switch(attr)
   {  case VNA_NAN:
         n=0.0;
         break;
      case VNA_INFINITY:
         n=1.0;
         break;
      case VNA_NEGINFINITY:
         n=-1.0;
         break;
   }
   Clearvalue(to);
   to->type=VTP_NUMBER;
   to->attr=attr;
   to->nvalue=n;
}

/* Assign a boolean */
void Asgboolean(struct Value *to,BOOL b)
{  Clearvalue(to);
   to->type=VTP_BOOLEAN;
   to->attr=0;
   to->bvalue=b;
}

/* Assign a string */
void Asgstring(struct Value *to,UBYTE *s,void *pool)
{  Clearvalue(to);
   to->type=VTP_STRING;
   to->attr=0;
   to->svalue=Jdupstr(s,-1,pool);
}

/* Assign an object */
void Asgobject(struct Value *to,struct Jobject *jo)
{  Clearvalue(to);
   to->type=VTP_OBJECT;
   to->attr=0;
   to->ovalue=jo;
   to->fthis=NULL;
}

/* Assign a function */
void Asgfunction(struct Value *to,struct Jobject *f,struct Jobject *fthis)
{  Clearvalue(to);
   to->type=VTP_OBJECT;
   to->attr=0;
   to->ovalue=f;
   to->fthis=fthis;
}

/* Make this value a string */
void Tostring(struct Value *v,struct Jcontext *jc)
{  switch(v->type)
   {  case VTP_NUMBER:
         {  UBYTE buffer[32];
            switch(v->attr)
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
                  sprintf(buffer,"%.20lg",v->nvalue);
                  break;
            }
            Asgstring(v,buffer,jc->pool);
         }
         break;
      case VTP_BOOLEAN:
         Asgstring(v,v->bvalue?"true":"false",jc->pool);
         break;
      case VTP_STRING:
         break;
      case VTP_OBJECT:
         if(v->ovalue && v->ovalue->function)
         {  struct Jbuffer *jb;
            if(jb=Jdecompile(jc,v->ovalue->function))
            {  Asgstring(v,jb->buffer,jc->pool);
               Freejbuffer(jb);
            }
            else
            {  Asgstring(v,"function",jc->pool);
            }
         }
         else
         {  struct Jobject *oldthis;
            if(Callproperty(jc,v->ovalue,"toString") && jc->val->type==VTP_STRING)
            {  Asgstring(v,jc->val->svalue,jc->pool);
            }
            else
            {  oldthis=jc->jthis;
               jc->jthis=v->ovalue;
               Defaulttostring(jc);
               jc->jthis=oldthis;
               Asgvalue(v,jc->val);
            }
         }
         break;
      default:
         Asgstring(v,"undefined",jc->pool);
         break;
   }
}

/* Make this value a number */
void Tonumber(struct Value *v,struct Jcontext *jc)
{  switch(v->type)
   {  case VTP_NUMBER:
         break;
      case VTP_BOOLEAN:
         Asgnumber(v,VNA_VALID,v->bvalue?1.0:0.0);
         break;
      case VTP_STRING:
         {  double n;
            if(sscanf(v->svalue,"%lg",&n))
            {  Asgnumber(v,VNA_VALID,n);
            }
            else
            {  Asgnumber(v,VNA_NAN,0.0);
            }
         }
         break;
      case VTP_OBJECT:
         if(Callproperty(jc,v->ovalue,"valueOf") && jc->val->type==VTP_NUMBER)
         {  Asgvalue(v,jc->val);
         }
         else
         {  Asgnumber(v,VNA_NAN,0.0);
         }
         break;
      default:
         Asgnumber(v,VNA_NAN,0.0);
         break;
   }
}

/* Make this value a boolean */
void Toboolean(struct Value *v,struct Jcontext *jc)
{  switch(v->type)
   {  case VTP_NUMBER:
         Asgboolean(v,v->nvalue!=0.0);
         break;
      case VTP_BOOLEAN:
         break;
      case VTP_STRING:
         Asgboolean(v,*v->svalue!='\0');
         break;
      case VTP_OBJECT:
         if(Callproperty(jc,v->ovalue,"valueOf") && jc->val->type==VTP_BOOLEAN)
         {  Asgvalue(v,jc->val);
         }
         else
         {  Asgboolean(v,v->ovalue?TRUE:FALSE);
         }
         break;
      default:
         Asgboolean(v,FALSE);
         break;
   }
}

/* Make this value an object */
void Toobject(struct Value *v,struct Jcontext *jc)
{  struct Jobject *jo;
   switch(v->type)
   {  case VTP_NUMBER:
         Asgobject(v,jo=Newnumber(jc,v->attr,v->nvalue));
         break;
      case VTP_BOOLEAN:
         Asgobject(v,jo=Newboolean(jc,v->bvalue));
         break;
      case VTP_STRING:
         Asgobject(v,jo=Newstring(jc,v->svalue));
         break;
      case VTP_OBJECT:
         break;
      default:
         Asgobject(v,NULL);
         break;
   }
}

/* Make this value a function */
void Tofunction(struct Value *v,struct Jcontext *jc)
{  Toobject(v,jc);
}

/*-----------------------------------------------------------------------*/

/* Default toString property function */
void Defaulttostring(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *p,*buf;
   if(Callproperty(jc,jo,"valueOf") && jc->val->type==VTP_STRING)
   {  /* We're done */
   }
   else
   {  if(!jo)
      {  Asgstring(jc->val,"null",jc->pool);
      }
      else
      {  if(jo->constructor && jo->constructor->function
         && jo->constructor->function->name)
         {  p=jo->constructor->function->name;
         }
         else
         {  p="";
         }
         if(buf=ALLOCTYPE(UBYTE,10+strlen(p),0,jc->pool))
         {  sprintf(buf,"[object %s]",p);
            Asgstring(jc->val,buf,jc->pool);
            FREE(buf);
         }
         else
         {  Asgstring(jc->val,"[object]",jc->pool);
         }
      }
   }
}

/*-----------------------------------------------------------------------*/

/* Create a new variable */
struct Variable *Newvar(UBYTE *name,void *pool)
{  struct Variable *var;
   if(var=ALLOCSTRUCT(Variable,1,0,pool))
   {  if(name)
      {  var->name=Jdupstr(name,-1,pool);
      }
      var->val.type=0;
   }
   return var;
}

/* Dispose this variable */
void Disposevar(struct Variable *var)
{  if(var)
   {  if(var->name) FREE(var->name);
      Clearvalue(&var->val);
      FREE(var);
   }
}

/*-----------------------------------------------------------------------*/

/* Create a new empty object */
struct Jobject *Newobject(struct Jcontext *jc)
{  struct Jobject *jo;
   if(jo=ALLOCSTRUCT(Jobject,1,0,jc->pool))
   {  NEWLIST(&jo->properties);
      ADDTAIL(&jc->objects,jo);
//debug(" NEW object %08lx\n",jo);
   }
   return jo;
}

/* Dispose an object */
void Disposeobject(struct Jobject *jo)
{  struct Variable *var;
   if(jo)
   {  while(var=REMHEAD(&jo->properties))
      {  Disposevar(var);
      }
      if(jo->internal && jo->dispose)
      {  jo->dispose(jo->internal);
      }
      if(jo->function)
      {  Jdispose(jo->function);
      }
//debug ("FREE object %08lx\n",jo);
      FREE(jo);
   }
}

/* Clear all properties of this object. If a property contains a reference
 * to another object, clear that too recursively. */
void Clearobject(struct Jobject *jo,UBYTE **except)
{  struct Variable *var,*next;
   UBYTE **p;
   if(jo && !(jo->flags&OBJF_CLEARING))
   {  jo->flags|=OBJF_CLEARING;
      for(var=jo->properties.first;var->next;var=next)
      {  next=var->next;
         if(var->name && except)
         {  for(p=except;*p;p++)
            {  if(STRIEQUAL(*p,var->name)) break;
            }
         }
         else p=NULL;
         if(!p || !*p)
         {  /* not in exception list */
            REMOVE(var);
/*
            if(var->val.type==VTP_OBJECT && var->val.ovalue)
            {  Clearobject(jo,NULL);
            }
*/
            Disposevar(var);
         }
      }
      jo->flags&=~OBJF_CLEARING;
   }
}

/* Add a property to this object */
struct Variable *Addproperty(struct Jobject *jo,UBYTE *name)
{  struct Variable *var=NULL;
   if(jo)
   {  if(var=Newvar(name,Getpool(jo)))
      {  ADDTAIL(&jo->properties,var);
      }
   }
   return var;
}

/* Find a property in this object */
struct Variable *Findproperty(struct Jobject *jo,UBYTE *name)
{  struct Variable *var;
   if(jo)
   {  for(var=jo->properties.first;var->next;var=var->next)
      {  if(STREQUAL(var->name,name)) return var;
      }
   }
   return NULL;
}

/* Prototype property variable hook. (data) is (struct Jobject *constructor).
 * Change value must add/set it in all objects of this type */
BOOL Protopropvhook(struct Varhookdata *h)
{  BOOL result=FALSE;
   struct Jobject *jo;
   struct Variable *prop;
   switch(h->code)
   {  case VHC_SET:
         for(jo=h->jc->objects.first;jo->next;jo=jo->next)
         {  if(jo->constructor==h->var->hookdata)
            {  if(!(prop=Findproperty(jo,h->var->name)))
               {  prop=Addproperty(jo,h->var->name);
               }
               if(prop)
               {  Asgvalue(&prop->val,&h->value->val);
               }
            }
         }
         result=TRUE;
         break;
   }
   return result;
}

/* Prototype object hook.
 * Add property must set the prototype property hook */
BOOL Prototypeohook(struct Objhookdata *h)
{  BOOL result=FALSE;
   struct Variable *prop;
   switch(h->code)
   {  case OHC_ADDPROPERTY:
         if(!(prop=Findproperty(h->jo,h->name)))
         {  prop=Addproperty(h->jo,h->name);
         }
         if(prop)
         {  prop->hook=Protopropvhook;
            prop->hookdata=h->jo->constructor;
         }
         result=TRUE;
         break;
   }
   return result;
}

/* General hook function for constants (that cannot change their value) */
BOOL Constantvhook(struct Varhookdata *h)
{  BOOL result=FALSE;
   if(h->code==VHC_SET) result=TRUE;
   return result;
}

/*-----------------------------------------------------------------------*/

/* Call a variable hook function */
BOOL Callvhook(struct Variable *var,struct Jcontext *jc,short code,struct Value *val)
{  struct Varhookdata vh={0};
   struct Variable valvar={0};
   BOOL result=FALSE;
   if(var && var->hook)
   {  vh.jc=jc;
      vh.code=code;
      vh.var=var;
      vh.hookdata=var->hookdata;
      if(code==VHC_SET) Asgvalue(&valvar.val,val);
      vh.value=&valvar;
      vh.name=var->name;
      result=var->hook(&vh);
      if(result && code==VHC_GET) Asgvalue(val,&valvar.val);
   }
   Clearvalue(&valvar.val);
   return result;
}

/* Call an object hook function */
BOOL Callohook(struct Jobject *jo,struct Jcontext *jc,short code,UBYTE *name)
{  struct Objhookdata oh={0};
   BOOL result=FALSE;
   if(jo && jo->hook)
   {  oh.jc=jc;
      oh.code=code;
      oh.jo=jo;
      oh.name=name;
      result=jo->hook(&oh);
   }
   return result;
}

void Dumpobjects(struct Jcontext *jc)
{  struct Jobject *jo;
   struct Variable *v;
   debug("=== JS object dump ===\n");
   for(jo=jc->objects.first;jo->next;jo=jo->next)
   {  debug("%08lx :",jo);
      if(jo->constructor && jo->constructor->function && jo->constructor->function->name)
      {  debug("[%s] ",jo->constructor->function->name);
      }
      if(jo->function && jo->function->name)
      {  debug("{%s} ",jo->function->name);
      }
      for(v=jo->properties.first;v->next;v=v->next)
      {  if(v->name)
         {  debug("%s,",v->name);
         }
      }
      debug("\n");
   }
}

/*-----------------------------------------------------------------------*/

static void Garbagemark(struct Jobject *jo)
{  struct Variable *v;
   if(jo && !(jo->flags&OBJF_USED))
   {  jo->flags|=OBJF_USED;
      for(v=jo->properties.first;v->next;v=v->next)
      {  if(v->val.type==VTP_OBJECT)
         {  Garbagemark(v->val.ovalue);
            if(v->val.fthis) Garbagemark(v->val.fthis);
         }
      }
      Garbagemark(jo->constructor);
      if(jo->function)
      {  Garbagemark(jo->function->fscope);
      }
   }
}

void Garbagecollect(struct Jcontext *jc)
{  struct Jobject *jo,*jonext;
   struct Function *f;
   struct With *w;
   struct Variable *v;
   for(jo=jc->objects.first;jo->next;jo=jo->next)
   {  jo->flags&=~OBJF_USED;
   }
   for(jo=jc->objects.first;jo->next;jo=jo->next)
   {  if(jo->keepnr) Garbagemark(jo);
   }
   for(f=jc->functions.first;f->next;f=f->next)
   {  Garbagemark(f->arguments);
      Garbagemark(f->def);
      Garbagemark(f->fscope);
      if(f->retval.type==VTP_OBJECT)
      {  Garbagemark(f->retval.ovalue);
         if(f->retval.fthis) Garbagemark(f->retval.fthis);
      }
      for(v=f->local.first;v->next;v=v->next)
      {  if(v->val.type==VTP_OBJECT)
         {  Garbagemark(v->val.ovalue);
            if(v->val.fthis) Garbagemark(v->val.fthis);
         }
      }
      for(w=f->with.first;w->next;w=w->next)
      {  Garbagemark(w->jo);
      }
   }
   for(jo=jc->objects.first;jo->next;jo=jonext)
   {  jonext=jo->next;
      if(!(jo->flags&OBJF_USED))
      {  REMOVE(jo);
         Disposeobject(jo);
      }
   }
}

void Keepobject(struct Jobject *jo,BOOL used)
{  if(used) jo->keepnr++;
   else if(jo->keepnr) jo->keepnr--;
}
