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

/* jmath.c - AWeb js internal Math object */

#include "awebjs.h"
#include "jprotos.h"
#include <math.h>

static BOOL seeded=FALSE;

/*-----------------------------------------------------------------------*/

/* Add a constant property */
static void Addmathproperty(struct Jobject *jo,UBYTE *name,double n)
{  struct Variable *var;
   if(var=Addproperty(jo,name))
   {  Asgnumber(&var->val,VNA_VALID,n);
      var->hook=Constantvhook;
   }
}

/* Find the numeric value of Nth argument */
static double Argument(struct Jcontext *jc,long n)
{  struct Variable *var;
   for(var=jc->functions.first->local.first;n && var->next;var=var->next,n--);
   if(var->next)
   {  Tonumber(&var->val,jc);
      if(var->val.attr==VNA_VALID)
      {  return var->val.nvalue;
      }
   }
   return 0;
}

/*-----------------------------------------------------------------------*/

static void Mathabs(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=abs(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathacos(struct Jcontext *jc)
{  double n=Argument(jc,0);
   if(n>=-1.0 && n<=1.0) n=acos(n);
   else n=0.0;
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathasin(struct Jcontext *jc)
{  double n=Argument(jc,0);
   if(n>=-1.0 && n<=1.0) n=asin(n);
   else n=0.0;
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathatan(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=atan(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathatan2(struct Jcontext *jc)
{  double x=Argument(jc,0);
   double y=Argument(jc,1);
   double n;
   if(y==0.0) n=PID2;
   else n=atan2(x,y);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathceil(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=ceil(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathcos(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=cos(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathexp(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=exp(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathfloor(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=floor(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathlog(struct Jcontext *jc)
{  double n=Argument(jc,0);
   if(n>0.0) n=log(n);
   else n=-1.797693134862316e308;
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathmax(struct Jcontext *jc)
{  double n1=Argument(jc,0);
   double n2=Argument(jc,1);
   if(n2>n1) n1=n2;
   Asgnumber(RETVAL(jc),VNA_VALID,n1);
}

static void Mathmin(struct Jcontext *jc)
{  double n1=Argument(jc,0);
   double n2=Argument(jc,1);
   if(n2<n1) n1=n2;
   Asgnumber(RETVAL(jc),VNA_VALID,n1);
}

static void Mathpow(struct Jcontext *jc)
{  double n=Argument(jc,0);
   double e=Argument(jc,1);
   n=pow(n,e);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathrandom(struct Jcontext *jc)
{  double n;
   n=drand48();
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathround(struct Jcontext *jc)
{  double n=Argument(jc,0);
   double i=floor(n);
   if(n>=0)
   {  if(n-i>=0.5) i+=1.0;
   }
   else
   {  if(n-i>0.5) i+=1.0;
   }
   Asgnumber(RETVAL(jc),VNA_VALID,i);
}

static void Mathsin(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=sin(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathsqrt(struct Jcontext *jc)
{  double n=Argument(jc,0);
   if(n>0.0) n=sqrt(n);
   else n=0.0;
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

static void Mathtan(struct Jcontext *jc)
{  double n=Argument(jc,0);
   n=tan(n);
   Asgnumber(RETVAL(jc),VNA_VALID,n);
}

/*-----------------------------------------------------------------------*/

void Initmath(struct Jcontext *jc)
{  struct Jobject *jo,*jp;
   struct Variable *var;
   if(!seeded)
   {  srand48((long)fmod(Today(),(double)0x7fffffff));
      seeded=TRUE;
   }
   if(jo=Newobject(jc))
   {  Addmathproperty(jo,"E",exp(1.0));
      Addmathproperty(jo,"LN2",log(2.0));
      Addmathproperty(jo,"LN10",log(10.0));
      Addmathproperty(jo,"LOG2E",1.0/log(2.0));
      Addmathproperty(jo,"LOG10E",1.0/log(10.0));
      Addmathproperty(jo,"PI",PI);
      Addmathproperty(jo,"SQRT1_2",sqrt(0.5));
      Addmathproperty(jo,"SQRT2",sqrt(2.0));
      
      if(jp=Internalfunction(jc,"abs",Mathabs,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"acos",Mathacos,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"asin",Mathasin,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"atan",Mathatan,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"atan2",Mathatan2,"xCoord","yCoord",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"ceil",Mathceil,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"cos",Mathcos,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"exp",Mathexp,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"floor",Mathfloor,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"log",Mathlog,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"max",Mathmax,"number1","number2",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"min",Mathmin,"number1","number2",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"pow",Mathpow,"base","exponent",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"random",Mathrandom,NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"round",Mathround,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"sin",Mathsin,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"sqrt",Mathsqrt,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      if(jp=Internalfunction(jc,"tan",Mathtan,"number",NULL))
      {  Addinternalproperty(jc,jo,jp);
      }
      
      if(var=Newvar("Math",jc->pool))
      {  ADDTAIL(&jc->functions.first->local,var);
         Asgobject(&var->val,jo);
         Keepobject(jo,TRUE);
      }
   }
}
