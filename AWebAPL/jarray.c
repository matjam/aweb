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

/* jarray.c - AWeb js internal Array object */

#include "awebjs.h"
#include "jprotos.h"

struct Array            /* Used as internal object value */
{  long length;
};

struct Tabelt           /* Array of this is used to reverse or sort the array */
{  struct Asortinfo *asi;  /* All tabelts point to the same Asortinfo */
   struct Value val;
   UBYTE *svalue;       /* Result of toString or NULL */
};

struct Asortinfo        /* Common info while sorting */
{  struct Jcontext *jc;
   struct Elementfunc *cfunc; /* Sort compare function or NULL */
   struct Jobject *cthis;     /* (this) for compare function */
};

/*-----------------------------------------------------------------------*/

/* Find the string value of Nth argument or NULL */
static UBYTE *Strargument(struct Jcontext *jc,long n)
{  struct Variable *var;
   for(var=jc->functions.first->local.first;n && var->next;var=var->next,n--);
   if(var->next && var->type!=VTP_UNDEFINED)
   {  Tostring(&var->val,jc);
      return var->val.svalue;
   }
   return NULL;
}

/* Find the arguments array */
static struct Jobject *Findarguments(struct Jcontext *jc)
{  struct Variable *var;
   for(var=jc->functions.first->local.first;var && var->next;var=var->next)
   {  if(var->name && STRIEQUAL(var->name,"arguments")
      && var->type==VTP_OBJECT) return var->ovalue;
   }
   return NULL;
}

/* Make a Tabelt array from an Array object. (asi->jc) must be set. */
struct Tabelt *Maketabelts(struct Asortinfo *asi,struct Jobject *jo,long n)
{  struct Variable *elt;
   struct Tabelt *tabelts;
   long i;
   if(tabelts=ALLOCSTRUCT(Tabelt,n,0,asi->jc->pool))
   {  for(i=0;i<n;i++)
      {  if(elt=Arrayelt(jo,i))
         {  Asgvalue(&tabelts[i].val,&elt->val);
         }
         tabelts[i].asi=asi;
      }
   }
   return tabelts;
}

/* Assign the Tabelt elements back to the Array object */
void Returntabelts(struct Jobject *jo,struct Tabelt *tabelts,long n)
{  struct Variable *elt;
   UBYTE nname[16];
   long i;
   for(i=0;i<n;i++)
   {  sprintf(nname,"%d",i);
      if(!(elt=Findproperty(jo,nname)))
      {  elt=Addproperty(jo,nname);
      }
      if(elt)
      {  Asgvalue(&elt->val,&tabelts[i].val);
      }
      Clearvalue(&tabelts[i].val);
      if(tabelts[i].svalue) FREE(tabelts[i].svalue);
   }
   FREE(tabelts);
}

/* General qsort compare function */
static int Asortcompare(struct Tabelt *ta,struct Tabelt *tb)
{  struct Value val;
   int c;
   if(ta->asi->cfunc)
   {  /* Compare function supplied, use it */
      Callfunctionargs(ta->asi->jc,ta->asi->cfunc,ta->asi->cthis,&ta->val,&tb->val,NULL);
      Tonumber(ta->asi->jc->val,ta->asi->jc);
      c=(int)ta->asi->jc->val->nvalue;
   }
   else
   {  /* Convert both elements to string and do string compare */
      if(!ta->svalue)
      {  val.type=0;
         Asgvalue(&val,&ta->val);
         Tostring(&val,ta->asi->jc);
         ta->svalue=Jdupstr(val.svalue,-1,ta->asi->jc->pool);
         Clearvalue(&val);
      }
      if(!tb->svalue)
      {  val.type=0;
         Asgvalue(&val,&tb->val);
         Tostring(&val,tb->asi->jc);
         tb->svalue=Jdupstr(val.svalue,-1,tb->asi->jc->pool);
         Clearvalue(&val);
      }
      c=strcmp(ta->svalue,tb->svalue);
   }
   return c;
}

/*-----------------------------------------------------------------------*/

/* Do the actual array joining */
static void Joinarray(struct Jcontext *jc,UBYTE *sep)
{  struct Jobject *jo=jc->jthis;
   struct Variable *elt;
   struct Value val;
   struct Jbuffer *buf;
   long n,length,sepl;
   if(!sep) sep=",";
   sepl=strlen(sep);
   val.type=0;
   if(jo && jo->internal)
   {  length=((struct Array *)jo->internal)->length;
      if(buf=Newjbuffer(jc->pool))
      {  for(n=0;n<length;n++)
         {  if(elt=Arrayelt(jo,n))
            {  Asgvalue(&val,&elt->val);
               Tostring(&val,jc);
               if(n>0) Addtojbuffer(buf,sep,sepl);
               Addtojbuffer(buf,val.svalue,strlen(val.svalue));
            }
         }
         Asgstring(RETVAL(jc),buf->buffer?buf->buffer:(UBYTE *)"",jc->pool);
         Freejbuffer(buf);
      }
   }
}

static void Arraytostring(struct Jcontext *jc)
{  Joinarray(jc,",");
}

static void Arrayjoin(struct Jcontext *jc)
{  UBYTE *sep=Strargument(jc,0);
   Joinarray(jc,sep);
}

static void Arrayreverse(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Tabelt *tabelts;
   struct Tabelt te;
   struct Asortinfo asi={0};
   long length,i;
   if(jo && jo->internal)
   {  length=((struct Array *)jo->internal)->length;
      asi.jc=jc;
      if(tabelts=Maketabelts(&asi,jo,length))
      {  /* reverse the table */
         for(i=0;i<length/2;i++)
         {  te=tabelts[i];
            tabelts[i]=tabelts[length-i-1];
            tabelts[length-i-1]=te;
         }
         Returntabelts(jo,tabelts,length);
      }
      Asgobject(RETVAL(jc),jo);
   }
}

static void Arraysort(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Tabelt *tabelts;
   struct Asortinfo asi={0};
   struct Variable *cvar;
   long length;
   if(jo && jo->internal)
   {  length=((struct Array *)jo->internal)->length;
      asi.jc=jc;
      if((cvar=jc->functions.first->local.first) && cvar->next)
      {  Tofunction(&cvar->val,jc);
         if(cvar->ovalue)
         {  asi.cfunc=cvar->ovalue->function;
            asi.cthis=cvar->fthis;
         }
      }
      if(tabelts=Maketabelts(&asi,jo,length))
      {  /* sort the table */
         qsort(tabelts,length,sizeof(struct Tabelt),Asortcompare);
         Returntabelts(jo,tabelts,length);
      }
      Asgobject(RETVAL(jc),jo);
   }
}


/* Add a property to an array, increase .length if necessary */
static BOOL Arrayohook(struct Objhookdata *h)
{  BOOL result=FALSE;
   struct Variable *prop;
   struct Array *a=(h->jo)?h->jo->internal:NULL;
   long n;
   switch(h->code)
   {  case OHC_ADDPROPERTY:
         if(!(prop=Findproperty(h->jo,h->name)))
         {  prop=Addproperty(h->jo,h->name);
         }
         if(prop && a)
         {  n=atoi(h->name);
            if(n>=a->length)
            {  a->length=n+1;
               if(prop=Findproperty(h->jo,"length"))
               {  Asgnumber(&prop->val,VNA_VALID,(double)a->length);
               }
            }
         }
         result=TRUE;
         break;
   }
   return result;
}

/* Dispose an Array object */
static void Destructor(struct Array *a)
{  FREE(a);
}

/* Make (jthis) a new Array object */
static void Constructor(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis,*args;
   struct Array *a;
   struct Variable *prop,*length,*arg,*narg;
   BOOL constructed=FALSE;
   long n;
   if(jo)
   {  if(a=ALLOCSTRUCT(Array,1,0,jc->pool))
      {  jo->internal=a;
         jo->dispose=Destructor;
         jo->hook=Arrayohook;
         a->length=0;
         if(args=Findarguments(jc))
         {  if((length=Findproperty(args,"length")) && length->type==VTP_NUMBER)
            {  if((long)length->nvalue==1)
               {  /* Only 1 argument, see if it's numeric */
                  if((arg=Arrayelt(args,0)) && arg->type==VTP_NUMBER)
                  {  a->length=(long)arg->nvalue;
                     constructed=TRUE;
                  }
               }
            }
            if(!constructed)
            {  /* Build new array from arguments array */
               for(n=0;arg=Arrayelt(args,n);n++)
               {  if(narg=Addarrayelt(jc,jo))
                  {  Asgvalue(&narg->val,&arg->val);
                  }
               }
            }
         }
         if(prop=Addproperty(jo,"length"))
         {  prop->flags|=VARF_HIDDEN;
            Asgnumber(&prop->val,VNA_VALID,(double)a->length);
            prop->hook=Constantvhook;
         }
      }
   }
}

/*-----------------------------------------------------------------------*/

void Initarray(struct Jcontext *jc)
{  struct Jobject *jo,*f;
   if(jo=Internalfunction(jc,"Array",Constructor,"arrayLength",NULL))
   {  Addprototype(jc,jo);
      if(f=Internalfunction(jc,"toString",Arraytostring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"join",Arrayjoin,"separator",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"reverse",Arrayreverse,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"sort",Arraysort,"compareFunction",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      Addglobalfunction(jc,jo);
      jc->array=jo;
      Keepobject(jo,TRUE);
   }
}

/* Create a new empty Array object. */
struct Jobject *Newarray(struct Jcontext *jc)
{  struct Jobject *jo;
   struct Array *a;
   struct Variable *prop;
   if(jo=Newobject(jc))
   {  Initconstruct(jc,jo,jc->array);
      if(a=ALLOCSTRUCT(Array,1,0,jc->pool))
      {  jo->internal=a;
         jo->dispose=Destructor;
         jo->hook=Arrayohook;
         a->length=0;
         if(prop=Addproperty(jo,"length"))
         {  prop->flags|=VARF_HIDDEN;
            Asgnumber(&prop->val,VNA_VALID,0.0);
            prop->hook=Constantvhook;
         }
      }
   }
   return jo;
}

/* Find the nth array element, or NULL */
struct Variable *Arrayelt(struct Jobject *jo,long n)
{  UBYTE nname[16];
   struct Array *a;
   struct Variable *prop=NULL;
   if(jo && jo->internal && jo->hook==Arrayohook)
   {  a=(struct Array *)jo->internal;
      if(n>=0 && n<a->length)
      {  sprintf(nname,"%d",n);
         prop=Findproperty(jo,nname);
      }
   }
   return prop;
}

/* Add an element to this array */
struct Variable *Addarrayelt(struct Jcontext *jc,struct Jobject *jo)
{  UBYTE nname[16];
   struct Array *a;
   struct Variable *prop=NULL,*length;
   if(jo && jo->internal && jo->hook==Arrayohook)
   {  a=(struct Array *)jo->internal;
      sprintf(nname,"%d",a->length);
      if(prop=Addproperty(jo,nname))
      {  a->length++;
         if(length=Findproperty(jo,"length"))
         {  Asgnumber(&length->val,VNA_VALID,(double)a->length);
         }
      }
   }
   return prop;
}

/* Tests if this object is an array */
BOOL Isarray(struct Jobject *jo)
{  return (BOOL)(jo && jo->internal && jo->hook==Arrayohook);
}
