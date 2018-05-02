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

/* jexe.c - AWeb js executor */

#include "awebjs.h"
#include "jprotos.h"
#include <exec/tasks.h>
#include <math.h>

/*-----------------------------------------------------------------------*/

static UBYTE *emsg_noproperty="No such property '%s'";
static UBYTE *emsg_nofunction="Not a function";
static UBYTE *emsg_notallowed="Access to '%s' not allowed";
static UBYTE *emsg_stackoverflow="Stack overflow";

/* Display the run time error message */
static void Runtimeerror(struct Jcontext *jc,struct Element *elt,UBYTE *msg,...)
{  struct Jbuffer *jb=NULL;
   BOOL debugger;
   if(!(jc->flags&(EXF_STOP|EXF_DONTSTOP)))
   {  if(jc->flags&EXF_ERRORS)
      {  jb=Jdecompile(jc,elt);
         debugger=Errorrequester(jc,elt->linenr,jb?jb->buffer:NULL,-1,msg,VARARG(msg));
      }
      else
      {  debugger=FALSE;
      }
      if(debugger)
      {  Startdebugger(jc);
         Setdebugger(jc,elt);
      }
      else
      {  jc->flags|=EXF_STOP;
      }
      if(jb) Freejbuffer(jb);
   }
}

/* Check for stack overflow; keep a margin for CA requester. Return TRUE if ok. */
static BOOL Stackcheck(struct Jcontext *jc,struct Element *elt)
{  struct Task *task=FindTask(NULL);
   BOOL ok=TRUE;
   /* Since (task) is a stack variable, (&task) is a good approximation of
    * the current stack pointer */
   if((ULONG)&task<(ULONG)task->tc_SPLower+8192)
   {  
      Runtimeerror(jc,elt,emsg_stackoverflow);
      ok=FALSE;
   }
   return ok;
}

/*-----------------------------------------------------------------------*/

/* Add a variable to this list */
static struct Variable *Addvar(void *vlist,UBYTE *name,void *pool)
{  struct Variable *var=Newvar(name,pool);
   if(var)
   {  ADDTAIL(vlist,var);
   }
   return var;
}

/* Find a variable in the current scope. Create a global one if not found. 
 * *(pthis) is set to object from with stack */
static struct Variable *Findvar(struct Jcontext *jc,UBYTE *name,struct Jobject **pthis)
{  struct Variable *var;
   struct With *w;
   /* Search the with stack for this function */
   for(w=jc->functions.first->with.first;w->next;w=w->next)
   {  if(var=Findproperty(w->jo,name))
      {  if(pthis) *pthis=w->jo;
         return var;
      }
   }
   if(jc->functions.first->next->next)
   {  /* Try local variables for function */
      for(var=jc->functions.first->local.first;var->next;var=var->next)
      {  if(var->name && STREQUAL(var->name,name)) return var;
      }
   }
   /* Try global data scope for this function. */
   if(var=Findproperty(jc->functions.first->fscope,name))
   {  return var;
   }
   /* Try properties of this */
   if(var=Findproperty(jc->jthis,name))
   {  return var;
   }
   /* Try top level global data scopes. */
   for(w=jc->functions.last->with.first;w->next;w=w->next)
   {  if((w->flags&WITHF_GLOBAL) && (var=Findproperty(w->jo,name)))
      {  if(pthis) *pthis=w->jo;
         return var;
      }
   }
   /* Try JS global variables */
   for(var=jc->functions.last->local.first;var->next;var=var->next)
   {  if(var->name && STREQUAL(var->name,name)) return var;
   }
   /* Not found, add variable to the global scope. */
   var=Addproperty(jc->functions.last->fscope,name);
   return var;
}

/* Find a local variable. Create a new one if not found.
 * Falls back to global variables if not within function scope. */
static struct Variable *Findlocalvar(struct Jcontext *jc,UBYTE *name)
{  struct Variable *var;
   if(jc->functions.first->next->next)
   {  /* Within function scope */
      for(var=jc->functions.first->local.first;var->next;var=var->next)
      {  if(var->name && STREQUAL(var->name,name)) return var;
      }
      /* Not found, add variable. */
      return Addvar(&jc->functions.first->local,name,jc->pool);
   }
   else
   {  /* Use global variables */
      if(var=Findproperty(jc->functions.last->fscope,name)) return var;
      return Addproperty(jc->functions.last->fscope,name);
   }
}

/*-----------------------------------------------------------------------*/

/* Add a with level */
static struct With *Addwith(struct Jcontext *jc,struct Jobject *jo)
{  struct With *w;
   if(w=ALLOCSTRUCT(With,1,0,jc->pool))
   {  w->jo=jo;
      ADDHEAD(&jc->functions.first->with,w);
   }
   return w;
}

/* Dispose a with level */
static void Disposewith(struct With *w)
{  if(w)
   {  FREE(w);
   }
}

/*-----------------------------------------------------------------------*/

/* Create a new function. Add to head of function stack yourself! */
static struct Function *Newfunction(struct Jcontext *jc,struct Elementfunc *func)
{  struct Function *f=ALLOCSTRUCT(Function,1,0,jc->pool);
   if(f)
   {  NEWLIST(&f->local);
      NEWLIST(&f->with);
      f->arguments=Newarray(jc);
      if(func)
      {  f->fscope=func->fscope;
      }
   }
   return f;
}

/* Dispose a function */
static void Disposefunction(struct Function *f)
{  struct Variable *var;
   struct With *w;
   if(f)
   {  while(var=REMHEAD(&f->local)) Disposevar(var);
      while(w=REMHEAD(&f->with)) Disposewith(w);
      Clearvalue(&f->retval);
      FREE(f);
   }
}

/*-----------------------------------------------------------------------*/

/* Evaluate first argument */
static void Eval(struct Jcontext *jc)
{  struct Variable *var;
   struct Function *f;
   var=jc->functions.first->local.first;
   if(var->next)
   {  Tostring(&var->val,jc);
   }
   /* Compile and execute the thing. Execution must take place in the
    * caller's context, not the eval() context. So pop the top function
    * and put it back later */
   f=REMHEAD(&jc->functions);
   Jeval(jc,var->val.svalue);
   ADDHEAD(&jc->functions,f);
   Asgvalue(&f->retval,jc->val);
}

/* Parse string to integer */
static void Parseint(struct Jcontext *jc)
{  struct Variable *var;
   UBYTE *s="";
   long radix=0;
   long n=0,d;
   short sign=1;
   UBYTE attr=VNA_NAN;
   var=jc->functions.first->local.first;
   if(var->next)
   {  Tostring(&var->val,jc);
      s=var->val.svalue;
      var=var->next;
      if(var->next)
      {  Tonumber(&var->val,jc);
         if(var->val.attr==VNA_VALID)
         {  radix=(long)var->val.nvalue;
         }
      }
   }
   if(*s=='-')
   {  sign=-1;
      s++;
   }
   if(radix==0)
   {  if(s[0]=='0')
      {  if(toupper(s[1])=='X')
         {  radix=16;
            s+=2;
         }
         else
         {  radix=8;
            s++;
            attr=VNA_VALID;
         }
      }
      else radix=10;
   }
   if(radix>=2 && radix<=36)
   {  for(;;)
      {  if(isdigit(*s)) d=*s-'0';
         else
         {  d=toupper(*s)-'A'+10;
            if(d<10) d=radix;
         }
         if(d>=radix) break;
         n=radix*n+d;
         attr=VNA_VALID;
         s++;
      }
      n*=sign;
   }
   Asgnumber(RETVAL(jc),attr,(double)n);
}

/* Parse string to float */
static void Parsefloat(struct Jcontext *jc)
{  struct Variable *var;
   UBYTE *s="";
   long exp=0;
   double ffrac,f;
   short sign=1,exps=1;
   UBYTE attr=VNA_NAN;
   var=jc->functions.first->local.first;
   if(var->next)
   {  Tostring(&var->val,jc);
      s=var->val.svalue;
   }
   f=0.0;
   if(*s=='-')
   {  sign=-1;
      s++;
   }
   while(isdigit(*s))
   {  f=f*10.0+(*s-'0');
      attr=VNA_VALID;
      s++;
   }
   if(*s=='.')
   {  s++;
      ffrac=1.0;
      while(isdigit(*s))
      {  ffrac/=10.0;
         f+=ffrac*(*s-'0');
         s++;
         attr=VNA_VALID;
      }
   }
   if(toupper(*s)=='E')
   {  s++;
      switch(*s)
      {  case '-':   exps=-1;s++;break;
         case '+':   s++;break;
      }
      while(isdigit(*s))
      {  exp=10*exp+(*s-'0');
         s++;
      }
   }
   f*=pow(10.0,(double)(exps*exp))*sign;
   Asgnumber(RETVAL(jc),attr,f);
}

/* Escape string */
static void Escape(struct Jcontext *jc)
{  struct Variable *var;
   UBYTE *s="";
   struct Jbuffer *jb;
   UBYTE buf[6];
   var=jc->functions.first->local.first;
   if(var->next)
   {  Tostring(&var->val,jc);
      s=var->val.svalue;
   }
   if(jb=Newjbuffer(jc->pool))
   {  for(;*s;s++)
      {  if(*s==' ')
         {  Addtojbuffer(jb,"+",1);
         }
         else if(isalnum(*s))
         {  Addtojbuffer(jb,s,1);
         }
         else
         {  sprintf(buf,"%%%02X",*s);
            Addtojbuffer(jb,buf,-1);
         }
      }
      Addtojbuffer(jb,"",1);
      Asgstring(RETVAL(jc),jb->buffer,jc->pool);
      Freejbuffer(jb);
   }
}

/* Unescape string */
static void Unescape(struct Jcontext *jc)
{  struct Variable *var;
   UBYTE *s="",c;
   struct Jbuffer *jb;
   var=jc->functions.first->local.first;
   if(var->next)
   {  Tostring(&var->val,jc);
      s=var->val.svalue;
   }
   if(jb=Newjbuffer(jc->pool))
   {  for(;*s;s++)
      {  if(*s=='%')
         {  c=0;
            if(isxdigit(s[1]))
            {  s++;
               c=16*c+(isdigit(*s)?(*s-'0'):(toupper(*s)-'A'+10));
               if(isxdigit(s[1]))
               {  s++;
                  c=16*c+(isdigit(*s)?(*s-'0'):(toupper(*s)-'A'+10));
               }
            }
            Addtojbuffer(jb,&c,1);
         }
         else if(*s=='+')
         {  Addtojbuffer(jb," ",1);
         }
         else
         {  Addtojbuffer(jb,s,1);
         }
      }
      Addtojbuffer(jb,"",1);
      Asgstring(RETVAL(jc),jb->buffer,jc->pool);
      Freejbuffer(jb);
   }
}

/* Check if value is NaN */
static void Isnan(struct Jcontext *jc)
{  struct Variable *var;
   BOOL nan=FALSE;
   var=jc->functions.first->local.first;
   if(var->next && var->type==VTP_NUMBER && var->attr==VNA_NAN)
   {  nan=TRUE;
   }
   Asgboolean(RETVAL(jc),nan);
}

static BOOL Member(struct Jcontext *jc,struct Element *elt,struct Jobject *jo,
   UBYTE *mbrname,BOOL asgonly)
{  struct Variable *mbr;
   BOOL ok=FALSE;
   mbr=Findproperty(jo,mbrname);
   if(!mbr)
   {  if(Callohook(jo,jc,OHC_ADDPROPERTY,mbrname))
      {  mbr=Findproperty(jo,mbrname);
      }
      else
      {  mbr=Addproperty(jo,mbrname);
      }
   }
   if(mbr)
   {  /* Resolve synonym reference */
      while((mbr->flags&VARF_SYNONYM) && mbr->hookdata) mbr=mbr->hookdata;

      /* Only access member if protection allows it. But keep the reference
       * even without protection to allow assignments (e.g. location.href) */
      if(!jc->protkey || !mbr->protkey || jc->protkey==mbr->protkey || asgonly)
      {  if(mbr->val.type==VTP_OBJECT && mbr->val.ovalue && mbr->val.ovalue->function)
         {  Asgfunction(jc->val,mbr->val.ovalue,jo);
         }
         else
         {  if(!Callvhook(mbr,jc,VHC_GET,jc->val))
            {  Asgvalue(jc->val,&mbr->val);
            }
         }
      }
      else
      {  Runtimeerror(jc,elt,emsg_notallowed,mbrname);
         Clearvalue(jc->val);
      }
      jc->varref=mbr;
      jc->flags|=EXF_KEEPREF;
      ok=TRUE;
   }
   else Runtimeerror(jc,elt,emsg_noproperty,mbrname);
   return ok;
}

/*-----------------------------------------------------------------------*/

static void Execute(struct Jcontext *jc,struct Element *elt);

static void Exeprogram(struct Jcontext *jc,struct Elementlist *elist)
{  struct Elementnode *enode,*enext;
   struct Element *elt;
   long generation=jc->generation;
   for(enode=elist->subs.first;enode->next;enode=enext)
   {  enext=enode->next;
      elt=enode->sub;
      if(elt && elt->generation==generation)
      {  Execute(jc,enode->sub);
         /* When used in eval() (not top level call), break out after return */
         if(jc->functions.first->next->next && jc->complete) break;
      }
   }
   /* Finally, dispose all statements for this generation */
   for(enode=elist->subs.first;enode->next;enode=enext)
   {  enext=enode->next;
      elt=enode->sub;
      if(elt && elt->generation==generation)
      {  REMOVE(enode);
         FREE(enode);
         Jdispose(elt);
      }
   }
}

/* Call this function without parameters with this object as "this" */
void Callfunctionbody(struct Jcontext *jc,struct Elementfunc *func,struct Jobject *jthis)
{  struct Function *f;
   struct Jobject *oldthis;
   USHORT oldflags;
   if(f=Newfunction(jc,func))
   {  ADDHEAD(&jc->functions,f);
      oldthis=jc->jthis;
      jc->jthis=jthis;
      oldflags=jc->flags;
      jc->flags&=~EXF_CONSTRUCT;
      Execute(jc,func);
      jc->jthis=oldthis;
      jc->flags=oldflags /*&~EXF_ERRORS*/ ;  /* Why did I put that here????? */
      REMOVE(f);
      Disposefunction(f);
   }
}

/* Call this function with supplied arguments (must be struct Value *, NULL terminated) */
void Callfunctionargs(struct Jcontext *jc,struct Elementfunc *func,struct Jobject *jthis,...)
{  struct Function *f;
   struct Jobject *oldthis;
   struct Value **val;
   struct Variable *var;
   USHORT oldflags;
   if(f=Newfunction(jc,func))
   {  for(val=VARARG(jthis);*val;val++)
      {  if(var=Newvar(NULL,jc->pool))
         {  ADDTAIL(&f->local,var);
            Asgvalue(&var->val,*val);
         }
      }
      for(val=VARARG(jthis);*val;val++)
      {  if(var=Addarrayelt(jc,f->arguments))
         {  Asgvalue(&var->val,*val);
         }
      }
      ADDHEAD(&jc->functions,f);
      oldthis=jc->jthis;
      jc->jthis=jthis;
      oldflags=jc->flags;
      jc->flags&=~EXF_CONSTRUCT;
      Execute(jc,func);
      jc->jthis=oldthis;
      jc->flags=oldflags /*&~EXF_ERRORS*/ ;
      REMOVE(f);
      Disposefunction(f);
   }
}

/* Call this function with this object as "this" */
static void Callfunction(struct Jcontext *jc,struct Elementlist *elist,
   struct Jobject *jthis,BOOL construct)
{  struct Elementnode *enode;
   struct Elementfunc *func=NULL;
   struct Function *f;
   struct Variable *arg,*argv;
   struct Jobject *oldthis,*fdef;
   USHORT oldflags;
   if(!Stackcheck(jc,(struct Element *)elist)) return;
   Execute(jc,(struct Element *)elist->subs.first->sub);
   Tofunction(jc->val,jc);
   if(jc->val->ovalue)
   {  func=jc->val->ovalue->function;
      fdef=jc->val->ovalue;
   }
   if(jc->val->fthis && !construct)
   {  jthis=jc->val->fthis;
   }
   if(construct && jthis && jc->val->ovalue)
   {  Initconstruct(jc,jthis,jc->val->ovalue);
   }
   /* Call constructor */
   if(func)
   {  if(f=Newfunction(jc,func))
      {  f->def=fdef;
         oldflags=jc->flags;
         jc->flags&=~EXF_CONSTRUCT;
         /* Create local variables for all actual parameters */
         for(enode=elist->subs.first->next;enode && enode->next;enode=enode->next)
         {  Execute(jc,enode->sub);
            if(arg=Newvar(NULL,jc->pool))
            {  ADDTAIL(&f->local,arg);
               Asgvalue(&arg->val,jc->val);
            }
         }
         /* Create the arguments array */
         for(argv=f->local.first;argv->next;argv=argv->next)
         {  if(arg=Addarrayelt(jc,f->arguments))
            {  Asgvalue(&arg->val,&argv->val);
            }
         }
         ADDHEAD(&jc->functions,f);
         oldthis=jc->jthis;
         jc->jthis=jthis;
         if(construct) jc->flags|=EXF_CONSTRUCT;
         Execute(jc,func);
         jc->jthis=oldthis;
         jc->flags=oldflags;
         REMOVE(f);
         Disposefunction(f);
      }
   }
   else if(jc->val->ovalue && (jc->val->ovalue->flags&OBJF_ASFUNCTION))
   {  /* Copy of Exeindex() here: */
      struct Value val,sval;
      BOOL ok=FALSE;
      Asgvalue(&val,jc->val);
      enode=elist->subs.first->next;
      if(enode && enode->next)
      {  Execute(jc,enode->sub);
         Asgvalue(&sval,jc->val);
         Tostring(&sval,jc);
         ok=Member(jc,(struct Element *)elist,val.ovalue,sval.svalue,FALSE);
         Clearvalue(&val);
         if(!ok) Clearvalue(jc->val);
         Clearvalue(&sval);
      }
      else
      {  Runtimeerror(jc,(struct Element *)elist,emsg_nofunction);
      }
   }
   else
   {  Runtimeerror(jc,(struct Element *)elist,emsg_nofunction);
   }
}

static void Execall(struct Jcontext *jc,struct Elementlist *elist)
{  Callfunction(jc,elist,jc->jthis,FALSE);
}

static void Execompound(struct Jcontext *jc,struct Elementlist *elist)
{  struct Elementnode *enode;
   for(enode=elist->subs.first;enode->next;enode=enode->next)
   {  if(enode->sub)
      {  Execute(jc,enode->sub);
         if(jc->complete) break;
      }
   }
}

static void Exevarlist(struct Jcontext *jc,struct Elementlist *elist)
{  struct Elementnode *enode;
   for(enode=elist->subs.first;enode->next;enode=enode->next)
   {  if(enode->sub)
      {  Execute(jc,enode->sub);
         /* Keep reference to last variable */
         jc->flags|=EXF_KEEPREF;
      }
   }
}

static void Exefunction(struct Jcontext *jc,struct Elementfunc *func)
{  struct Elementnode *enode;
   struct Elementstring *arg;
   struct Function *f;
   struct Variable *var,*avar;
   f=jc->functions.first;
   /* Map formal parameter names to pre-allocated local variables */
   for(enode=func->subs.first,var=f->local.first;
      enode->next && var->next;
      enode=enode->next,var=var->next)
   {  arg=enode->sub;
      if(arg && arg->type==ET_IDENTIFIER)
      {  if(var->name) FREE(var->name);
         var->name=Jdupstr(arg->svalue,-1,jc->pool);
      }
   }
   /* Add empty local variables for unassigned formal parameters */
   for(;enode && enode->next;enode=enode->next)
   {  arg=enode->sub;
      if(arg && arg->type==ET_IDENTIFIER)
      {  if(var=Newvar(arg->svalue,jc->pool))
         {  ADDTAIL(&f->local,var);
         }
      }
   }
   /* Link the arguments array to a local variable
    * and to the function object .arguments property */
   if(var=Newvar("arguments",jc->pool))
   {  ADDTAIL(&f->local,var);
      Asgobject(&var->val,f->arguments);
   }
   if((avar=Findproperty(f->def,"arguments"))
   || (avar=Addproperty(f->def,"arguments")))
   {  Asgobject(&avar->val,f->arguments);
   }
   /* Link the caller to a variable */
   if(var=Newvar("caller",jc->pool))
   {  ADDTAIL(&f->local,var);
      Asgobject(&var->val,f->next->def);
   }
   /* Create function call object */
   if(var=Newvar(func->name,jc->pool))
   {  ADDTAIL(&f->local,var);
      Asgfunction(&var->val,f->def,NULL);
   }
   /* Execute the function body */
   Execute(jc,func->body);
   if(jc->complete<=ECO_RETURN)
   {  jc->complete=ECO_NORMAL;
   }
   /* Clear the .arguments property */
   if(avar)
   {  Clearvalue(&avar->val);
   }
   /* Set the function result. */
   Asgvalue(jc->val,&f->retval);
}

static void Exebreak(struct Jcontext *jc,struct Element *elt)
{  jc->complete=ECO_BREAK;
}

static void Execontinue(struct Jcontext *jc,struct Element *elt)
{  jc->complete=ECO_CONTINUE;
}

static void Exethis(struct Jcontext *jc,struct Element *elt)
{  Asgobject(jc->val,jc->jthis);
}

static void Exenull(struct Jcontext *jc,struct Element *elt)
{  Asgobject(jc->val,NULL);
}

static void Exenegative(struct Jcontext *jc,struct Element *elt)
{  double n;
   UBYTE v;
   Execute(jc,elt->sub1);
   Tonumber(jc->val,jc);
   switch(jc->val->attr)
   {  case VNA_VALID:
         n=-jc->val->nvalue;
         v=VNA_VALID;
         break;
      case VNA_INFINITY:
         v=VNA_NEGINFINITY;
         break;
      case VNA_NEGINFINITY:
         v=VNA_INFINITY;
         break;
      default:
         v=VNA_NAN;
         break;
   }
   Asgnumber(jc->val,v,n);      
}

static void Exenot(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Toboolean(jc->val,jc);
   Asgboolean(jc->val,!jc->val->bvalue);
}

static void Exebitneg(struct Jcontext *jc,struct Element *elt)
{  ULONG n=0.0;
   Execute(jc,elt->sub1);
   Tonumber(jc->val,jc);
   if(jc->val->attr==VNA_VALID)
   {  n=~((ULONG)(long)jc->val->nvalue);
   }
   Asgnumber(jc->val,VNA_VALID,(double)n);
}

static void Exepreinc(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   if(jc->varref)
   {  Tonumber(&jc->varref->val,jc);
      if(jc->varref->val.attr==VNA_VALID)
      {  jc->varref->val.nvalue+=1.0;
      }
      Asgvalue(jc->val,&jc->varref->val);
   }
}

static void Exepredec(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   if(jc->varref)
   {  Tonumber(&jc->varref->val,jc);
      if(jc->varref->val.attr==VNA_VALID)
      {  jc->varref->val.nvalue-=1.0;
      }
      Asgvalue(jc->val,&jc->varref->val);
   }
}

static void Exepostinc(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   if(jc->varref)
   {  Tonumber(&jc->varref->val,jc);
      Asgvalue(jc->val,&jc->varref->val);
      if(jc->varref->val.attr==VNA_VALID)
      {  jc->varref->val.nvalue+=1.0;
      }
   }
}

static void Exepostdec(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   if(jc->varref)
   {  Tonumber(&jc->varref->val,jc);
      Asgvalue(jc->val,&jc->varref->val);
      if(jc->varref->val.attr==VNA_VALID)
      {  jc->varref->val.nvalue-=1.0;
      }
   }
}

static void Exenew(struct Jcontext *jc,struct Element *elt)
{  struct Jobject *jo;
   struct Element *ctr=elt->sub1;
   if(jo=Newobject(jc))
   {  if(ctr)
      {  if(ctr->type==ET_CALL)
         {  Callfunction(jc,elt->sub1,jo,TRUE);
         }
         else if(ctr->type==ET_IDENTIFIER)
         {  /* "new Constructor" called, without ().
             * Build CALL type element on the fly and execute it */
            struct Elementlist call={0};
            struct Elementnode node={0};
            call.type=ET_CALL;
            call.generation=elt->generation;
            call.linenr=elt->linenr;
            NEWLIST(&call.subs);
            node.sub=ctr;
            ADDTAIL(&call.subs,&node);
            Callfunction(jc,&call,jo,TRUE);
         }
      }
      Asgobject(jc->val,jo);
   }
}

static void Exedelete(struct Jcontext *jc,struct Element *elt)
{
}

static void Exetypeof(struct Jcontext *jc,struct Element *elt)
{  UBYTE *tp;
   Execute(jc,elt->sub1);
   switch(jc->val->type)
   {  case VTP_NUMBER:  tp="number";break;
      case VTP_BOOLEAN: tp="boolean";break;
      case VTP_STRING:  tp="string";break;
      case VTP_OBJECT:
         if(jc->val->ovalue && jc->val->ovalue->function)
         {  tp="function";
         }
         else
         {  tp="object";
         }
         break;
      default:          tp="undefined";break;
   }
   Asgstring(jc->val,tp,jc->pool);
}

static void Exevoid(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Clearvalue(jc->val);
}

static void Exereturn(struct Jcontext *jc,struct Element *elt)
{  if(elt->sub1)
   {  Execute(jc,elt->sub1);
      Asgvalue(&jc->functions.first->retval,jc->val);
   }
   else
   {  Clearvalue(&jc->functions.first->retval);
   }
   jc->complete=ECO_RETURN;
}

static void Exeinternal(struct Jcontext *jc,struct Element *elt)
{  void (*f)(void *);
   if(elt->sub1)
   {  f=(void (*)(void *))elt->sub1;
      f(jc);
   }
}

static void Exefunceval(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Tostring(jc->val,jc);
   Jeval(jc,jc->val->svalue);
}

static void Exeplus(struct Jcontext *jc,struct Element *elt)
{  static UBYTE attrtab[4][4]=
   {/* a+b   valid            nan      +inf              -inf */
   /*valid*/ VNA_VALID,       VNA_NAN, VNA_INFINITY,     VNA_NEGINFINITY,
   /* nan */ VNA_NAN,         VNA_NAN, VNA_NAN,          VNA_NAN,
   /* +inf*/ VNA_INFINITY,    VNA_NAN, VNA_INFINITY,     VNA_NAN,
   /* -inf*/ VNA_NEGINFINITY, VNA_NAN, VNA_NAN,          VNA_NEGINFINITY,
   };
   struct Value val1,val2;
   struct Variable *lhs;
   BOOL concat=FALSE;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Asgvalue(&val2,jc->val);
   /* If object, see if it string-convertible */
   if(val1.type==VTP_OBJECT && val1.ovalue && !val1.ovalue->function)
   {  if(!Callproperty(jc,val1.ovalue,"valueOf") || jc->val->type>=VTP_STRING)
      {  /* Not number-convertible */
         concat=TRUE;
      }
   }
   else if(val1.type>=VTP_STRING)
   {  concat=TRUE;
   }
   if(!concat && val2.type==VTP_OBJECT && val2.ovalue && !val2.ovalue->function)
   {  if(!Callproperty(jc,val2.ovalue,"valueOf") || jc->val->type>=VTP_STRING)
      {  /* Not number-convertible */
         concat=TRUE;
      }
   }
   else if(val2.type>=VTP_STRING)
   {  concat=TRUE;
   }
   if(concat)
   {  /* Do string concatenation */
      long l;
      UBYTE *s;
      Tostring(&val1,jc);
      Tostring(&val2,jc);
      l=strlen(val1.svalue)+strlen(val2.svalue);
      if(s=ALLOCTYPE(UBYTE,l+1,0,jc->pool))
      {  strcpy(s,val1.svalue);
         strcat(s,val2.svalue);
         Asgstring(jc->val,s,jc->pool);
         FREE(s);
      }
   }
   else
   {  /* Do numeric addition */
      double n=0.0;
      UBYTE v;
      Tonumber(&val1,jc);
      Tonumber(&val2,jc);
      if((v=attrtab[val1.attr][val2.attr])==VNA_VALID)
      {  _FPERR=0;
         n=val1.nvalue+val2.nvalue;
         if(_FPERR)
         {  if(val1.nvalue>0) v=VNA_INFINITY;
            else v=VNA_NEGINFINITY;
         }
      }
      Asgnumber(jc->val,v,n);
   }
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_APLUS && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exeminus(struct Jcontext *jc,struct Element *elt)
{  static UBYTE attrtab[4][4]=
   {/* a-b   valid            nan      +inf              -inf */
   /*valid*/ VNA_VALID,       VNA_NAN, VNA_NEGINFINITY,  VNA_INFINITY,
   /* nan */ VNA_NAN,         VNA_NAN, VNA_NAN,          VNA_NAN,
   /* +inf*/ VNA_INFINITY,    VNA_NAN, VNA_NAN,          VNA_INFINITY,
   /* -inf*/ VNA_NEGINFINITY, VNA_NAN, VNA_NEGINFINITY,  VNA_NAN,
   };
   struct Value val1,val2;
   struct Variable *lhs;
   double n=0.0;
   UBYTE v;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
   if((v=attrtab[val1.attr][val2.attr])==VNA_VALID)
   {  _FPERR=0;
      n=val1.nvalue-val2.nvalue;
      if(_FPERR)
      {  if(val1.nvalue>0) v=VNA_INFINITY;
         else v=VNA_NEGINFINITY;
      }
   }
   Asgnumber(jc->val,v,n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_AMINUS && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

#define FSIGN(d) (((d)>0.0)?1:(((d)<0.0)?-1:0))

static void Exemult(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   double n=0.0;
   UBYTE v;
   short sign=0;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
   if(val1.attr==VNA_NAN || val2.attr==VNA_NAN)
   {  v=VNA_NAN;
   }
   else
   {  sign=FSIGN(val1.nvalue)*FSIGN(val2.nvalue);
      if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
      {  _FPERR=0;
         n=val1.nvalue*val2.nvalue;
         if(_FPERR)
         {  v=(sign>0)?VNA_INFINITY:VNA_NEGINFINITY;
         }
         else
         {  v=VNA_VALID;
         }
      }
      else
      {  /* Either operand is +/- infinity */
         if(sign)
         {  v=(sign>0)?VNA_INFINITY:VNA_NEGINFINITY;
         }
         else
         {  /* infinity times zero */
            v=VNA_NAN;
         }
      }
   }
   Asgnumber(jc->val,v,n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_AMULT && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exediv(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   double n=0.0;
   UBYTE v;
   short sign=0;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
   if(val1.attr==VNA_NAN || val2.attr==VNA_NAN)
   {  v=VNA_NAN;
   }
   else
   {  sign=FSIGN(val1.nvalue)*FSIGN(val2.nvalue);
      if(val1.attr==VNA_VALID)
      {  if(val1.nvalue==0.0)
         {  if(val2.nvalue==0.0)
            {  /* 0/0 */
               v=VNA_NAN;
            }
            else
            {  /* 0/n, 0/i */
               n=0.0;
               v=VNA_VALID;
            }
         }
         else
         {  if(val2.attr==VNA_VALID)
            {  if(val2.nvalue==0.0)
               {  /* n/0 */
                  if(FSIGN(val1.nvalue)>0) v=VNA_INFINITY;
                  else v=VNA_NEGINFINITY;
               }
               else
               {  /* n/n */
                  _FPERR=0;
                  n=val1.nvalue/val2.nvalue;
                  if(_FPERR==_FPEUND)
                  {  n=0.0;
                     v=VNA_VALID;
                  }
                  else if(_FPERR)
                  {  v=(sign>0)?VNA_INFINITY:VNA_NEGINFINITY;
                  }
                  else
                  {  v=VNA_VALID;
                  }
               }
            }
            else
            {  /* n/i */
               n=0.0;
               v=VNA_VALID;
            }
         }
      }
      else
      {  if(val2.attr==VNA_VALID)
         {  if(val2.nvalue==0.0)
            {  /* i/0 */
               v=val1.attr;
            }
            else
            {  /* i/n */
               v=(sign>0)?VNA_INFINITY:VNA_NEGINFINITY;
            }
         }
         else
         {  /* i/i */
            v=VNA_NAN;
         }
      }
   }
   Asgnumber(jc->val,v,n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_ADIV && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exerem(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   double n=0.0;
   UBYTE v;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
   if(val1.attr==VNA_NAN || val2.attr==VNA_NAN)
   {  v=VNA_NAN;
   }
   else
   {  if(val1.attr!=VNA_VALID || val2.nvalue==0.0)
      {  v=VNA_NAN;
      }
      else if(val2.attr!=VNA_VALID || val1.nvalue==0.0)
      {  n=val1.nvalue;
         v=VNA_VALID;
      }
      else
      {  n=fmod(val1.nvalue,val2.nvalue);
         v=VNA_VALID;
      }
   }
   Asgnumber(jc->val,v,n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_AREM && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exebitand(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   ULONG n=0;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
   if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
   {  n=((ULONG)val1.nvalue) & ((ULONG)val2.nvalue);
   }
   Asgnumber(jc->val,VNA_VALID,(double)n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_ABITAND && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exebitor(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   ULONG n=0;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
   if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
   {  n=((ULONG)val1.nvalue) | ((ULONG)val2.nvalue);
   }
   Asgnumber(jc->val,VNA_VALID,(double)n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_ABITOR && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exebitxor(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   ULONG n=0;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
   if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
   {  n=((ULONG)val1.nvalue) ^ ((ULONG)val2.nvalue);
   }
   Asgnumber(jc->val,VNA_VALID,(double)n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_ABITXOR && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exeshleft(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   ULONG n;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
      if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
   {  n=((ULONG)val1.nvalue) << (((ULONG)val2.nvalue)&0x1f);
   }
   Asgnumber(jc->val,VNA_VALID,(double)n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_ASHLEFT && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exeshright(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   ULONG n;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
      if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
   {  n=((long)val1.nvalue) >> (((ULONG)val2.nvalue)&0x1f);
   }
   Asgnumber(jc->val,VNA_VALID,(double)n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_ASHRIGHT && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exeushright(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   struct Variable *lhs;
   ULONG n;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   Tonumber(jc->val,jc);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Tonumber(jc->val,jc);
   Asgvalue(&val2,jc->val);
      if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
   {  n=((ULONG)(long)val1.nvalue) >> (((ULONG)val2.nvalue)&0x1f);
   }
   Asgnumber(jc->val,VNA_VALID,(double)n);
   Clearvalue(&val1);
   Clearvalue(&val2);
   if(elt->type==ET_AUSHRIGHT && lhs)
   {  if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Exeequality(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   BOOL b;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Asgvalue(&val2,jc->val);
   if(val1.type==VTP_OBJECT && val2.type==VTP_OBJECT)
   {  /* Reference comparison */
      b=(BOOL)(val1.ovalue==val2.ovalue);
   }
   else if(val1.type==VTP_OBJECT && !val1.ovalue)
   {  Toobject(&val2,jc);
      b=!val2.ovalue;
   }
   else if(val2.type==VTP_OBJECT && !val2.ovalue)
   {  Toobject(&val1,jc);
      b=!val1.ovalue;
   }
   else if(val1.type>=VTP_STRING || val2.type>=VTP_STRING)
   {  /* String comparison */
      Tostring(&val1,jc);
      Tostring(&val2,jc);
      b=!strcmp(val1.svalue,val2.svalue);
   }
   else
   {  /* Numeric comparison */
      Tonumber(&val1,jc);
      Tonumber(&val2,jc);
      if(val1.attr==VNA_NAN || val2.attr==VNA_NAN)
      {  b=FALSE; /* NaN != NaN */
      }
      else if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
      {  b=(val1.nvalue==val2.nvalue);
      }
      else
      {  b=(val1.attr==val2.attr);
      }
   }
   if(elt->type==ET_NE) b=!b;
   Asgboolean(jc->val,b);
   Clearvalue(&val1);
   Clearvalue(&val2);
}

static void Exerelational(struct Jcontext *jc,struct Element *elt)
{  struct Value val1,val2;
   double dc;
   long c;
   BOOL b,gotb=FALSE;
   val1.type=val2.type=0;
   Execute(jc,elt->sub1);
   Asgvalue(&val1,jc->val);
   Execute(jc,elt->sub2);
   Asgvalue(&val2,jc->val);
   /* NS 3 heuristic: if either value is numeric, convert both to numeric */
   if(val1.type==VTP_NUMBER || val2.type==VTP_NUMBER)
   {  Tonumber(&val1,jc);
      Tonumber(&val2,jc);
   }
   if(val1.type>=VTP_STRING || val2.type>=VTP_STRING)
   {  /* String comparison */
      Tostring(&val1,jc);
      Tostring(&val2,jc);
      c=strcmp(val1.svalue,val2.svalue);
   }
   else
   {  /* Numeric comparison */
      Tonumber(&val1,jc);
      Tonumber(&val2,jc);
      if(val1.attr==VNA_NAN || val2.attr==VNA_NAN)
      {  b=FALSE;
         gotb=TRUE;
      }
      else
      {  if(val1.attr==VNA_VALID && val2.attr==VNA_VALID)
         {  dc=val1.nvalue-val2.nvalue;
            if(dc>0) c=1;
            else if(dc<0) c=-1;
            else c=0;
         }
         else if(val1.attr==val2.attr)
         {  /* both i with same sign */
            c=0;
         }
         else if(val1.attr==VNA_INFINITY)
         {  /* i > -1 */
            c=1;
         }
         else
         {  /* -1 < i */
            c=-1;
         }
      }
   }
   if(!gotb)
   {  switch(elt->type)
      {  case ET_LT: b=(c<0);break;
         case ET_GT: b=(c>0);break;
         case ET_LE: b=(c<=0);break;
         case ET_GE: b=(c>=0);break;
      }
   }
   Asgboolean(jc->val,b);
   Clearvalue(&val1);
   Clearvalue(&val2);
}

static void Exeand(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Toboolean(jc->val,jc);
   if(jc->val->bvalue)
   {  Execute(jc,elt->sub2);
      Toboolean(jc->val,jc);
   }
}

static void Exeor(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Toboolean(jc->val,jc);
   if(!jc->val->bvalue)
   {  Execute(jc,elt->sub2);
      Toboolean(jc->val,jc);
   }
}

static void Exeassign(struct Jcontext *jc,struct Element *elt)
{  struct Variable *lhs;
   jc->flags|=EXF_ASGONLY;
   Execute(jc,elt->sub1);
   jc->flags&=~EXF_ASGONLY;
   lhs=jc->varref;
   if(lhs)
   {  Execute(jc,elt->sub2);
      if(!Callvhook(lhs,jc,VHC_SET,jc->val))
      {  Asgvalue(&lhs->val,jc->val);
         lhs->flags&=~VARF_HIDDEN;
      }
   }
}

static void Execomma(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Execute(jc,elt->sub2);
}

static void Exewhile(struct Jcontext *jc,struct Element *elt)
{  for(;!(jc->flags&EXF_STOP);)
   {  Execute(jc,elt->sub1);
      Toboolean(jc->val,jc);
      if(!jc->val->bvalue) break;
      Execute(jc,elt->sub2);
      if(jc->complete>=ECO_RETURN) break;
      if(jc->complete==ECO_BREAK)
      {  jc->complete=ECO_NORMAL;
         break;
      }
      jc->complete=ECO_NORMAL;
   }
}

static void Exedo(struct Jcontext *jc,struct Element *elt)
{  for(;!(jc->flags&EXF_STOP);)
   {  Execute(jc,elt->sub1);
      if(jc->complete>=ECO_RETURN) break;
      if(jc->complete==ECO_BREAK)
      {  jc->complete=ECO_NORMAL;
         break;
      }
      jc->complete=ECO_NORMAL;
      Execute(jc,elt->sub2);
      Toboolean(jc->val,jc);
      if(!jc->val->bvalue) break;
   }
}

static void Exewith(struct Jcontext *jc,struct Element *elt)
{  struct With *w;
   Execute(jc,elt->sub1);
   Toobject(jc->val,jc);
   if(w=Addwith(jc,jc->val->ovalue))
   {  Execute(jc,elt->sub2);
      REMOVE(w);
      Disposewith(w);
   }
}

static void Exedot(struct Jcontext *jc,struct Element *elt)
{  struct Elementstring *mbrname=elt->sub2;
   BOOL ok=FALSE,temp=FALSE;
   BOOL asgonly=BOOLVAL(jc->flags&EXF_ASGONLY);
   jc->flags&=~EXF_ASGONLY;
   Execute(jc,elt->sub1);
   temp=(jc->val->type!=VTP_OBJECT);
   Toobject(jc->val,jc);
   if(temp && jc->val->ovalue)
   {  jc->val->ovalue->flags|=OBJF_TEMP;
   }
   if(mbrname && mbrname->type==ET_IDENTIFIER)
   {  ok=Member(jc,elt,jc->val->ovalue,mbrname->svalue,asgonly);
   }
   if(!ok) Clearvalue(jc->val);
}

static void Exeindex(struct Jcontext *jc,struct Element *elt)
{  struct Value val,sval;
   BOOL ok=FALSE,temp=FALSE;
   val.type=sval.type=0;
   Execute(jc,elt->sub1);
   temp=(jc->val->type!=VTP_OBJECT);
   Toobject(jc->val,jc);
   if(temp && jc->val->ovalue)
   {  jc->val->ovalue->flags|=OBJF_TEMP;
   }
   Asgvalue(&val,jc->val);
   Execute(jc,elt->sub2);
   Asgvalue(&sval,jc->val);
   Tostring(&sval,jc);
   ok=Member(jc,elt,val.ovalue,sval.svalue,FALSE);
   Clearvalue(&val);
   if(!ok) Clearvalue(jc->val);
   Clearvalue(&sval);
}

static void Exevar(struct Jcontext *jc,struct Element *elt)
{  struct Elementstring *vid=elt->sub1;
   struct Variable *var;
   if(vid && vid->type==ET_IDENTIFIER)
   {  var=Findlocalvar(jc,vid->svalue);
      if(elt->sub2)
      {  Execute(jc,elt->sub2);
         Asgvalue(&var->val,jc->val);
      }
      jc->varref=var;
      jc->flags|=EXF_KEEPREF;
   }
}

static void Execond(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Toboolean(jc->val,jc);
   if(jc->val->bvalue)
   {  Execute(jc,elt->sub2);
   }
   else
   {  Execute(jc,elt->sub3);
   }
}

static void Exeif(struct Jcontext *jc,struct Element *elt)
{  Execute(jc,elt->sub1);
   Toboolean(jc->val,jc);
   if(jc->val->bvalue)
   {  Execute(jc,elt->sub2);
   }
   else
   {  Execute(jc,elt->sub3);
   }
}

static void Exeforin(struct Jcontext *jc,struct Element *elt)
{  struct Variable *var,*lhs;
   struct Jobject *jo;
   long n,i;
   Execute(jc,elt->sub1);
   lhs=jc->varref;
   if(lhs)
   {  for(n=0;!(jc->flags&EXF_STOP);n++)
      {  Execute(jc,elt->sub2);  /* Specs say must be evaluated for each iteration */
         Toobject(jc->val,jc);
         if(jo=jc->val->ovalue)
         {  for(var=jo->properties.first,i=0;var->next && i<n;var=var->next,i++);
            if(!var->next) break;
            if(var->name && !(var->flags&VARF_HIDDEN))
            {  Asgstring(&lhs->val,var->name,jc->pool);
               lhs->flags&=~VARF_HIDDEN;
               Execute(jc,elt->sub3);
            }
         }
         if(jc->complete>=ECO_RETURN) break;
         if(jc->complete==ECO_BREAK)
         {  jc->complete=ECO_NORMAL;
            break;
         }
         jc->complete=ECO_NORMAL;
      }
   }
}

static void Exefor(struct Jcontext *jc,struct Element *elt)
{  BOOL c;
   Execute(jc,elt->sub1);
   for(;!(jc->flags&EXF_STOP);)
   {  if(elt->sub2)
      {  Execute(jc,elt->sub2);
         Toboolean(jc->val,jc);
         c=jc->val->bvalue;
      }
      else c=TRUE;
      if(!c) break;
      Execute(jc,elt->sub4);
      if(jc->complete>=ECO_RETURN) break;
      if(jc->complete==ECO_BREAK)
      {  jc->complete=ECO_NORMAL;
         break;
      }
      jc->complete=ECO_NORMAL;
      if(elt->sub3)
      {  Execute(jc,elt->sub3);
      }
   }
}

static void Exeinteger(struct Jcontext *jc,struct Elementint *elt)
{  Asgnumber(jc->val,VNA_VALID,(double)elt->ivalue);
}

static void Exefloat(struct Jcontext *jc,struct Elementfloat *elt)
{  Asgnumber(jc->val,VNA_VALID,elt->fvalue);
}

static void Exeboolean(struct Jcontext *jc,struct Elementint *elt)
{  Asgboolean(jc->val,elt->ivalue);
}

static void Exestring(struct Jcontext *jc,struct Elementstring *elt)
{  Asgstring(jc->val,elt->svalue,jc->pool);
}

static void Exeidentifier(struct Jcontext *jc,struct Elementstring *elt)
{  struct Jobject *jthis=NULL;
   struct Variable *var=Findvar(jc,elt->svalue,&jthis);
   if(var)
   {  /* Resolve synonym reference */
      while((var->flags&VARF_SYNONYM) && var->hookdata) var=var->hookdata;

      if(var->val.type==VTP_OBJECT && var->val.ovalue && var->val.ovalue->function)
      {  Asgfunction(jc->val,var->val.ovalue,jthis?jthis:var->val.fthis);
      }
      else
      {  if(!Callvhook(var,jc,VHC_GET,jc->val))
         {  Asgvalue(jc->val,&var->val);
         }
      }
      jc->varref=var;
      jc->flags|=EXF_KEEPREF;
   }
   else
   {  Clearvalue(jc->val);
   }
}

#ifdef JSDEBUG
static void Exedebug(struct Jcontext *jc,struct Element *elt)
{  struct Value val;
   val.type=0;
   Execute(jc,elt->sub1);
   Asgvalue(&val,jc->val);
   Tostring(&val,jc);
   printf("%s\n",val.svalue);
   Clearvalue(&val);
}
#endif

#ifdef JSADDRESS
static void Exeaddress(struct Jcontext *jc,struct Element *elt)
{  struct Value val;
   UBYTE buf[16];
   val.type=0;
   Execute(jc,elt->sub1);
   Asgvalue(&val,jc->val);
   Toobject(&val,jc);
   sprintf(buf,"0x%08X",val.ovalue);
   Asgstring(jc->val,buf,jc->pool);
}
#endif

/*-----------------------------------------------------------------------*/

typedef void Exeelement(void *,void *);
static Exeelement *exetab[]=
{
   NULL,
   Exeprogram,
   Execall,
   Execompound,
   Exevarlist,
   Exefunction,
   Exebreak,
   Execontinue,
   Exethis,
   Exenull,
   NULL,          /* empty */
   Exenegative,
   Exenot,
   Exebitneg,
   Exepreinc,
   Exepredec,
   Exepostinc,
   Exepostdec,
   Exenew,
   Exedelete,
   Exetypeof,
   Exevoid,
   Exereturn,
   Exeinternal,
   Exefunceval,
   Exeplus,
   Exeminus,
   Exemult,
   Exediv,
   Exerem,
   Exebitand,
   Exebitor,
   Exebitxor,
   Exeshleft,
   Exeshright,
   Exeushright,
   Exeequality,   /* eq */
   Exeequality,   /* ne */
   Exerelational, /* lt */
   Exerelational, /* gt */
   Exerelational, /* le */
   Exerelational, /* ge */
   Exeand,
   Exeor,
   Exeassign,
   Exeplus,       /* aplus */
   Exeminus,      /* aminus */
   Exemult,       /* amult */
   Exediv,        /* adiv */
   Exerem,        /* arem */
   Exebitand,     /* abitand */
   Exebitor,      /* abitor */
   Exebitxor,     /* abitxor */
   Exeshleft,     /* ashleft */
   Exeshright,    /* ashright */
   Exeushright,   /* aushright */
   Execomma,
   Exewhile,
   Exedo,
   Exewith,
   Exedot,
   Exeindex,
   Exevar,
   Execond,
   Exeif,
   Exeforin,
   Exefor,
   Exeinteger,
   Exefloat,
   Exeboolean,
   Exestring,
   Exeidentifier,
#ifdef JSDEBUG
   Exedebug,
#endif
#ifdef JSADDRESS
   Exeaddress,
#endif
};

static void Execute(struct Jcontext *jc,struct Element *elt)
{  BOOL debugover=FALSE;
   if(elt && exetab[elt->type])
   {  if(!Feedback(jc))
      {  jc->flags|=EXF_STOP;
      }
      if(!(jc->flags&EXF_STOP) && (jc->dflags&DEBF_DBREAK)
      && elt->type!=ET_PROGRAM)
      {  Setdebugger(jc,elt);
         /* Flags are set:
          * DEBUG DBREAK
          *   1      1   step into
          *   1      0   step over
          *   0      0   run or stop
          */
         debugover=(jc->dflags&DEBF_DEBUG) && !(jc->dflags&DEBF_DBREAK);
      }
      if(!(jc->flags&EXF_STOP))
      {  exetab[elt->type](jc,elt);
         if(!(jc->flags&EXF_KEEPREF))
         {  jc->varref=NULL;
         }
      }
      if(!(jc->flags&EXF_STOP) && debugover && (jc->dflags&DEBF_DEBUG))
      {  jc->dflags|=DEBF_DBREAK;
      }
      jc->flags&=~EXF_KEEPREF;
   }
}

/*-----------------------------------------------------------------------*/

/* Create a function object for this internal function.
 * Varargs are argument names (UBYTE *) terminated by NULL. */
struct Jobject *InternalfunctionA(struct Jcontext *jc,UBYTE *name,
   void (*code)(void *),UBYTE **args)
{  struct Jobject *jo=NULL;
   struct Elementfunc *func;
   struct Elementnode *enode;
   struct Element *elt;
   struct Elementstring *arg;
   UBYTE **a;
   if(func=ALLOCSTRUCT(Elementfunc,1,0,jc->pool))
   {  func->type=ET_FUNCTION;
      func->name=Jdupstr(name,-1,jc->pool);
      NEWLIST(&func->subs);
      if(elt=ALLOCSTRUCT(Element,1,0,jc->pool))
      {  elt->type=ET_INTERNAL;
         elt->sub1=(void *)code;
      }
      func->body=elt;
      for(a=args;*a;a++)
      {  if(arg=ALLOCSTRUCT(Elementstring,1,0,jc->pool))
         {  arg->type=ET_IDENTIFIER;
            arg->svalue=Jdupstr(*a,-1,jc->pool);
            if(enode=ALLOCSTRUCT(Elementnode,1,0,jc->pool))
            {  enode->sub=arg;
               ADDTAIL(&func->subs,enode);
            }
         }
      }
      if(jo=Newobject(jc))
      {  jo->function=func;
      }
   }
   return jo;
}

struct Jobject *Internalfunction(struct Jcontext *jc,UBYTE *name,
   void (*code)(void *),...)
{  return InternalfunctionA(jc,name,code,VARARG(code));
}

/* Adds a function object to global variable list */
void Addglobalfunction(struct Jcontext *jc,struct Jobject *f)
{  struct Variable *var;
   if(f && f->function && f->function->type==ET_FUNCTION)
   {  if(var=Newvar(f->function->name,jc->pool))
      {  ADDTAIL(&jc->functions.first->local,var);
         Asgobject(&var->val,f);
      }
   }
}

/* Adds a function object to an object's properties */
struct Variable *Addinternalproperty(struct Jcontext *jc,struct Jobject *jo,struct Jobject *f)
{  struct Variable *var=NULL;
   if(f && f->function && f->function->type==ET_FUNCTION)
   {  if(var=Addproperty(jo,f->function->name))
      {  Asgobject(&var->val,f);
      }
   }
   return var;
}

/* Adds the .prototype property to a function object */
void Addprototype(struct Jcontext *jc,struct Jobject *jo)
{  struct Variable *prop;
   struct Jobject *pro;
   if(prop=Addproperty(jo,"prototype"))
   {  if(pro=Newobject(jc))
      {  pro->constructor=jo;
         pro->hook=Prototypeohook;
         Asgobject(&prop->val,pro);
      }
   }
}

/* Add a function object to the object's prototype */
void Addtoprototype(struct Jcontext *jc,struct Jobject *jo,struct Jobject *f)
{  struct Variable *var,*proto;
   if(f && f->function && f->function->type==ET_FUNCTION)
   {  if((proto=Findproperty(jo,"prototype")) && proto->type==VTP_OBJECT && proto->ovalue)
      {  if(var=Addproperty(proto->ovalue,f->function->name))
         {  var->hook=Protopropvhook;
            var->hookdata=proto->ovalue->constructor;
            Asgobject(&var->val,f);
            var->flags|=VARF_HIDDEN;
         }
      }
   }
}

/* Add .constructor, default .toString() and .prototype properties */
void Initconstruct(struct Jcontext *jc,struct Jobject *jo,struct Jobject *fo)
{  struct Variable *proto,*pro,*newpro,*p;
   if(fo)
   {  if(!jo->constructor) jo->constructor=fo;
      if(newpro=Addproperty(jo,"constructor"))
      {  Asgfunction(&newpro->val,fo,NULL);
         newpro->flags|=VARF_HIDDEN;
      }
      if((proto=Findproperty(fo,"prototype"))
      && proto->type==VTP_OBJECT && proto->ovalue)
      {  for(pro=proto->ovalue->properties.first;pro->next;pro=pro->next)
         {  if(!(newpro=Findproperty(jo,pro->name)))
            {  newpro=Addproperty(jo,pro->name);
            }
            if(newpro)
            {  Asgvalue(&newpro->val,&pro->val);
               newpro->flags|=(pro->flags&VARF_HIDDEN);
            }
         }
      }
   }
   if(!(pro=Findproperty(jo,"toString")))
   {  if(p=Addinternalproperty(jc,jo,jc->tostring))
      {  p->flags|=VARF_HIDDEN;
      }
   }
   if(!(pro=Findproperty(jo,"eval")))
   {  if(p=Addinternalproperty(jc,jo,jc->eval))
      {  p->flags|=VARF_HIDDEN;
      }
   }
}

/*-----------------------------------------------------------------------*/

/* Evaluate this string */
void Jeval(struct Jcontext *jc,UBYTE *s)
{  struct Jcontext jc2={0};
   jc2.pool=jc->pool;
   NEWLIST(&jc2.objects);
   NEWLIST(&jc2.functions);
   jc2.generation=jc->generation;
   Jcompile(&jc2,s);
   if(!(jc2.flags&JCF_ERROR))
   {  Execute(jc,jc2.program);
   }
   Jdispose(jc2.program);
}

/*-----------------------------------------------------------------------*/

void Jexecute(struct Jcontext *jc,struct Jobject *jthis,struct Jobject **gwtab)
{  struct Jobject *oldjthis,*oldfscope;
   struct With *w1,*w;
   struct Jobject **jp;
   struct Value oldretval={0};
   if(jc && jc->program)
   {  if(jc->jthis) Keepobject(jc->jthis,TRUE);
      oldjthis=jc->jthis;
      jc->jthis=jthis;
      /* By setting the last (JS global) function scope to jc->fscope we ensure
       * that the correct global scope is used also when not within a function. */
      oldfscope=jc->functions.last->fscope;
      jc->functions.last->fscope=jc->fscope;
      /* Add With's for global data scopes and function scope. Remember first
       * With, and remove the entire series from that one after execution
       * (backwards because Withs are ADDHEAD'ed). */
      w1=NULL;
      if(gwtab)
      {  for(jp=gwtab;*jp;jp++)
         {  if(w=Addwith(jc,*jp))
            {  w->flags|=WITHF_GLOBAL;
               if(!w1) w1=w;
            }
         }
      }
      Asgvalue(&oldretval,&jc->functions.last->retval);
      /* Set default TRUE return value */
      Asgboolean(&jc->functions.last->retval,TRUE);
      
      Execute(jc,jc->program);
      if(w1)
      {  for(;w1->prev;w1=w)
         {  w=w1->prev;
            REMOVE(w1);
            Disposewith(w1);
         }
      }
      jc->jthis=oldjthis;
      if(jc->jthis) Keepobject(jc->jthis,FALSE);
      jc->functions.last->fscope=oldfscope;
      jc->complete=ECO_NORMAL;
      /* Keep execution result and restore old result */
      Asgvalue(&jc->result->val,jc->val);
      Asgvalue(jc->val,&jc->functions.last->retval);
      Asgvalue(&jc->functions.last->retval,&oldretval);
      Clearvalue(&oldretval);
   }
}

BOOL Newexecute(struct Jcontext *jc)
{  struct Function *f;
   struct Jobject *fo;
   NEWLIST(&jc->functions);
   if((jc->valvar=Newvar(NULL,jc->pool))
   && (jc->result=Newvar(NULL,jc->pool))
   && (jc->jthis=Newobject(jc))
   && (f=Newfunction(jc,NULL)))
   {  Keepobject(jc->jthis,TRUE);
      jc->val=&jc->valvar->val;
      ADDHEAD(&jc->functions,f);
      jc->tostring=Internalfunction(jc,"toString",Defaulttostring,NULL);
      Keepobject(jc->tostring,TRUE);
      if(jc->eval=Internalfunction(jc,"eval",Eval,"Expression",NULL))
      {  Addglobalfunction(jc,jc->eval);
      }
      if(fo=Internalfunction(jc,"parseInt",Parseint,"string","radix",NULL))
      {  Addglobalfunction(jc,fo);
      }
      if(fo=Internalfunction(jc,"parseFloat",Parsefloat,"string",NULL))
      {  Addglobalfunction(jc,fo);
      }
      if(fo=Internalfunction(jc,"escape",Escape,"string",NULL))
      {  Addglobalfunction(jc,fo);
      }
      if(fo=Internalfunction(jc,"unescape",Unescape,"string",NULL))
      {  Addglobalfunction(jc,fo);
      }
      if(fo=Internalfunction(jc,"isNaN",Isnan,"testValue",NULL))
      {  Addglobalfunction(jc,fo);
      }
      Initobject(jc);
      Initarray(jc);
      Initboolean(jc);
      Initdate(jc);
      Initfunction(jc);
      Initmath(jc);
      Initnumber(jc);
      Initstring(jc);
      return TRUE;
   }
   Freeexecute(jc);
}

void Freeexecute(struct Jcontext *jc)
{  struct Function *f;
   if(jc)
   {  if(jc->val)
      {  if(jc->valvar) Disposevar(jc->valvar);
         if(jc->result) Disposevar(jc->result);
         while(f=REMHEAD(&jc->functions)) Disposefunction(f);
         Disposeobject(jc->jthis);
         Disposeobject(jc->tostring);
      }
   }
}