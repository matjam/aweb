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

/* jstring.c - AWeb js internal String object */

#include "awebjs.h"
#include "jprotos.h"
#include <clib/locale_protos.h>
#include <pragmas/locale_pragmas.h>

struct String           /* Used as internal object value */
{  UBYTE *svalue;
};

/*-----------------------------------------------------------------------*/

/* Find the numeric value of Nth argument */
static double Numargument(struct Jcontext *jc,long n,BOOL *validp)
{  struct Variable *var;
   for(var=jc->functions.first->local.first;n && var->next;var=var->next,n--);
   if(var->next)
   {  Tonumber(&var->val,jc);
      if(var->val.attr==VNA_VALID)
      {  if(validp) *validp=TRUE;
         return var->val.nvalue;
      }
   }
   if(validp) *validp=FALSE;
   return 0;
}

/* Find the string value of Nth argument */
static UBYTE *Strargument(struct Jcontext *jc,long n)
{  struct Variable *var;
   for(var=jc->functions.first->local.first;n && var->next;var=var->next,n--);
   if(var->next)
   {  Tostring(&var->val,jc);
      return var->val.svalue;
   }
   return "";
}

static void Surround(struct Jcontext *jc,UBYTE *s1,UBYTE *s2)
{  struct Jobject *jo=jc->jthis;
   UBYTE *str,*sur;
   long l;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      l=strlen(str)+strlen(s1)+strlen(s2)+5;
      if(sur=ALLOCTYPE(UBYTE,l,0,jc->pool))
      {  sprintf(sur,"<%s>%s<%s>",s1,str,s2);
         Asgstring(RETVAL(jc),sur,jc->pool);
         FREE(sur);
      }
   }
}

static void Surroundattr(struct Jcontext *jc,UBYTE *s1,UBYTE *s2)
{  struct Jobject *jo=jc->jthis;
   UBYTE *attr=Strargument(jc,0);
   UBYTE *str,*sur;
   long l;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      l=strlen(str)+strlen(s1)+strlen(s2)+strlen(attr)+7;
      if(sur=ALLOCTYPE(UBYTE,l,0,jc->pool))
      {  sprintf(sur,"<%s\"%s\">%s<%s>",s1,attr,str,s2);
         Asgstring(RETVAL(jc),sur,jc->pool);
         FREE(sur);
      }
   }
}

/*-----------------------------------------------------------------------*/

/* Get string value of (jthis) */
static void Stringtostring(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *p="";
   if(jo && jo->internal)
   {  p=((struct String *)jo->internal)->svalue;
   }
   Asgstring(RETVAL(jc),p,jc->pool);
}

static void Stringindexof(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *str,*p;
   UBYTE *sval=Strargument(jc,0);
   long start=(long)
   Numargument(jc,1,NULL);
   long index=-1;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      if(str && start>=0 && start<strlen(str))
      {  p=str+start;
         if(p=strstr(p,sval))
         {  index=p-str;
         }
      }
   }
   Asgnumber(RETVAL(jc),VNA_VALID,(double)index);
}

static void Stringlastindexof(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Variable *var;
   UBYTE *str,*p;
   UBYTE *sval=Strargument(jc,0);
   long start;
   long l,n,index=-1;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      start=strlen(str)-1;
      for(n=1,var=jc->functions.first->local.first;n && var->next;var=var->next,n--);
      if(var->next)
      {  Tonumber(&var->val,jc);
         if(var->val.attr==VNA_VALID)
         {  start=(long)var->val.nvalue;
         }
      }
      if(str && start<strlen(str))
      {  p=str+start;
         l=strlen(sval);
         while(p>=str)
         {  if(STRNEQUAL(p,sval,l))
            {  index=p-str;
               break;
            }
            p--;
         }
      }
   }
   Asgnumber(RETVAL(jc),VNA_VALID,(double)index);
}

/* substring(a,b) returns substring from pos a upto b-1 inclusive */
static void Stringsubstring(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *str,*sub;
   BOOL ibvalid=FALSE;
   long ia=(long)Numargument(jc,0,NULL);
   long ib=(long)Numargument(jc,1,&ibvalid);
   long l;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      l=strlen(str);
      if(!ibvalid)
      {  ib=l;
      }
      if(ia>ib)
      {  long ic=ia;
         ia=ib;
         ib=ic;
      }
      if(ia<0) ia=0;
      if(ib>l) ib=l;
      if(sub=Jdupstr(str+ia,ib-ia,jc->pool))
      {  Asgstring(RETVAL(jc),sub,jc->pool);
         FREE(sub);
      }
   }
}

/* substr(a,b) returns substring from pos a upto a+b-1 inclusive */
static void Stringsubstr(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *str,*sub;
   BOOL ibvalid=FALSE;
   long ia=(long)Numargument(jc,0,NULL);
   long ib=(long)Numargument(jc,1,&ibvalid);
   long l;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      l=strlen(str);
      if(!ibvalid)
      {  ib=l;
      }
      if(ia<0) ia=0;
      ib+=ia;
      if(ib>l) ib=l;
      if(sub=Jdupstr(str+ia,ib-ia,jc->pool))
      {  Asgstring(RETVAL(jc),sub,jc->pool);
         FREE(sub);
      }
   }
}

static void Stringcharat(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *str;
   UBYTE buf[2];
   long i=(long)Numargument(jc,0,NULL);
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      if(i<0 || i>=strlen(str))
      {  buf[0]='\0';
      }
      else
      {  buf[0]=str[i];
         buf[1]='\0';
      }
      Asgstring(RETVAL(jc),buf,jc->pool);
   }
}

static void Stringtolowercase(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *str,*to,*p;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      if(to=Jdupstr(str,-1,jc->pool))
      {  for(p=to;*p;p++) *p=ConvToLower(locale,*p);
         Asgstring(RETVAL(jc),to,jc->pool);
         FREE(to);
      }
   }
}

static void Stringtouppercase(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   UBYTE *str,*to,*p;
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      if(to=Jdupstr(str,-1,jc->pool))
      {  for(p=to;*p;p++) *p=ConvToUpper(locale,*p);
         Asgstring(RETVAL(jc),to,jc->pool);
         FREE(to);
      }
   }
}

static void Stringsplit(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Jobject *array;
   struct Variable *elt;
   UBYTE *str,*sub,*p,*q;
   UBYTE *sep=Strargument(jc,0);
   if(jo && jo->internal)
   {  str=((struct String *)jo->internal)->svalue;
      if(array=Newarray(jc))
      {  if(*sep)
         {  p=str;
            for(;;)
            {  if(!(q=strstr(p,sep))) q=p+strlen(p);
               if(sub=Jdupstr(p,q-p,jc->pool))
               {  if(elt=Addarrayelt(jc,array))
                  {  Asgstring(&elt->val,sub,jc->pool);
                  }
                  FREE(sub);
               }
               if(*q)
               {  p=q+strlen(sep);
               }
               else break;
            }
         }
         else
         {  UBYTE c[2]={ 0,0 };
            for(p=str;*p;p++)
            {  c[0]=*p;
               if(elt=Addarrayelt(jc,array))
               {  Asgstring(&elt->val,c,jc->pool);
               }
            }
         }
         Asgobject(RETVAL(jc),array);
      }
   }
}

static void Stringanchor(struct Jcontext *jc)
{  Surroundattr(jc,"A NAME=","/A");
}

static void Stringbig(struct Jcontext *jc)
{  Surround(jc,"BIG","/BIG");
}

static void Stringblink(struct Jcontext *jc)
{  Surround(jc,"BLINK","/BLINK");
}

static void Stringbold(struct Jcontext *jc)
{  Surround(jc,"B","/B");
}

static void Stringfixed(struct Jcontext *jc)
{  Surround(jc,"TT","/TT");
}

static void Stringfontcolor(struct Jcontext *jc)
{  Surroundattr(jc,"FONT COLOR=","/FONT");
}

static void Stringfontsize(struct Jcontext *jc)
{  Surroundattr(jc,"FONT SIZE=","/FONT");
}

static void Stringitalics(struct Jcontext *jc)
{  Surround(jc,"I","/I");
}

static void Stringlink(struct Jcontext *jc)
{  Surroundattr(jc,"A HREF=","/A");
}

static void Stringsmall(struct Jcontext *jc)
{  Surround(jc,"SMALL","/SMALL");
}

static void Stringstrike(struct Jcontext *jc)
{  Surround(jc,"STRIKE","/STRIKE");
}

static void Stringsub(struct Jcontext *jc)
{  Surround(jc,"SUB","/SUB");
}

static void Stringsup(struct Jcontext *jc)
{  Surround(jc,"SUP","/SUP");
}

/* Dispose a String object */
static void Destructor(struct String *s)
{  if(s->svalue) FREE(s->svalue);
   FREE(s);
}

/* Make (jthis) a new String object */
static void Constructor(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct String *s;
   struct Variable *prop;
   struct Variable *arg;
   UBYTE *p;
   arg=jc->functions.first->local.first;
   if(arg->next && arg->type!=VTP_UNDEFINED)
   {  Tostring(&arg->val,jc);
      p=arg->val.svalue;
   }
   else
   {  p="";
   }
   if(jc->flags&EXF_CONSTRUCT)
   {  if(jo)
      {  if(s=ALLOCSTRUCT(String,1,0,jc->pool))
         {  jo->internal=s;
            jo->dispose=Destructor;
            s->svalue=Jdupstr(p,-1,jc->pool);
            if(prop=Addproperty(jo,"length"))
            {  Asgnumber(&prop->val,VNA_VALID,(double)strlen(p));
               prop->hook=Constantvhook;
            }
         }
      }
   }
   else
   {  /* Not called as constructor; return argument string */
      Asgstring(RETVAL(jc),p,jc->pool);
   }
}

/*-----------------------------------------------------------------------*/

void Initstring(struct Jcontext *jc)
{  struct Jobject *jo,*f;
   if(jo=Internalfunction(jc,"String",Constructor,"stringValue",NULL))
   {  Addprototype(jc,jo);
      if(f=Internalfunction(jc,"toString",Stringtostring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"valueOf",Stringtostring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"indexOf",Stringindexof,"searchValue","fromIndex",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"lastIndexOf",Stringlastindexof,"searchValue","fromIndex",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"substring",Stringsubstring,"indexA","indexB",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"substr",Stringsubstr,"index","length",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"charAt",Stringcharat,"index",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"toLowerCase",Stringtolowercase,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"toUpperCase",Stringtouppercase,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"split",Stringsplit,"separator",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"anchor",Stringanchor,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"big",Stringbig,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"blink",Stringblink,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"bold",Stringbold,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"fixed",Stringfixed,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"fontcolor",Stringfontcolor,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"fontsize",Stringfontsize,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"italics",Stringitalics,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"link",Stringlink,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"small",Stringsmall,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"strike",Stringstrike,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"sub",Stringsub,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"sup",Stringsup,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      Addglobalfunction(jc,jo);
      jc->string=jo;
      Keepobject(jo,TRUE);
   }
}

/* Create a new String object. */
struct Jobject *Newstring(struct Jcontext *jc,UBYTE *svalue)
{  struct Jobject *jo;
   struct String *s;
   struct Variable *prop;
   if(jo=Newobject(jc))
   {  Initconstruct(jc,jo,jc->string);
      if(s=ALLOCSTRUCT(String,1,0,jc->pool))
      {  jo->internal=s;
         jo->dispose=Destructor;
         s->svalue=Jdupstr(svalue,-1,jc->pool);
         if(prop=Addproperty(jo,"length"))
         {  Asgnumber(&prop->val,VNA_VALID,(double)strlen(svalue));
            prop->hook=Constantvhook;
         }
      }
   }
   return jo;
}


