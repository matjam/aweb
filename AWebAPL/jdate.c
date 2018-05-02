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

/* jdate.c - AWeb js internal Date object */

#include "awebjs.h"
#include "jprotos.h"
#include <libraries/locale.h>
#include <dos/dos.h>
#include <time.h>

struct Date             /* Used as internal object value */
{  double date;         /* ms since 1-1-1970, local time */
};

struct Brokentime
{  struct tm tm;
   int tm_millis;
};

/*-----------------------------------------------------------------------*/

/* SAS/C system function replacement to avoid ENV access and bogus local
 * time offsets */
void __tzset(void)
{  __tzstn[0]='\0';
   __tzname[0]=__tzstn;
   __timezone=0;
   __tzdtn[0]='\0';
   __tzname[1]=__tzdtn;
   __daylight=0;
}

/*-----------------------------------------------------------------------*/

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

/* Get a pointer to broken-down time from (this) */
static void Gettime(struct Jcontext *jc,struct Brokentime *bt)
{  struct Jobject *jo=jc->jthis;
   struct tm *tm=NULL;
   double d;
   time_t time;
   if(jo && jo->internal)
   {  d=((struct Date *)jo->internal)->date;
      time=(long)(d/1000);
      bt->tm_millis=d-(double)time*1000;
      tm=gmtime(&time);
      bt->tm=*tm;
   }
}

/* Set (this) to new date */
static void Settime(struct Jcontext *jc,struct Brokentime *bt)
{  struct Jobject *jo=jc->jthis;
   time_t time;
   if(jo && jo->internal)
   {  time=mktime(&bt->tm);
      ((struct Date *)jo->internal)->date=(double)time*1000+bt->tm_millis;
   }
}

static UBYTE months[12][4]=
{  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

/* Get next token from date string */
static UBYTE *Getdatetoken(UBYTE *p,UBYTE *token)
{  while(*p && !isalnum(*p)) p++;
   while(*p && isalnum(*p)) *token++=*p++;
   *token='\0';
   return p;
}

/* Scan date, return GMT date value */
double Scandate(UBYTE *buf)
{  UBYTE *p;
   UBYTE token[16];
   struct Brokentime bt={0};
   BOOL ok,first=TRUE,gmt=FALSE;
   short i;
   time_t time;
   short offset=0;
   p=buf;
   for(;;)
   {  p=Getdatetoken(p,token);
      if(!*token) break;
      ok=FALSE;
      for(i=0;i<12;i++)
      {  if(STRNIEQUAL(token,months[i],3))
         {  bt.tm_mon=i;
            ok=TRUE;
            break;
         }
      }
      if(ok) continue;
      if(*p==':')
      {  sscanf(token,"%hd",&i);
         bt.tm_hour=i;
         p=Getdatetoken(p,token);
         sscanf(token,"%hd",&i);
         bt.tm_min=i;
         p=Getdatetoken(p,token);
         sscanf(token,"%hd",&i);
         bt.tm_sec=i;
         continue;
      }
      if(sscanf(token,"%hd",&i))
      {  if(first)
         {  bt.tm_mday=i;
            first=FALSE;
         }
         else
         {  if(i<70) i+=2000;          /* e.g. 01 -> 2001 */
            else if(i<1970) i+=1900;   /* e.g. 96 -> 1996 */
                                       /* else: e.g. 1996 */
            bt.tm_year=i-1900;
         }
         continue;
      }
      if(STRIEQUAL(token,"GMT"))
      {  gmt=TRUE;
         sscanf(p,"%hd",&offset);
         break;
      }
   }
   /****... ObtainSemaphore() here.... ****/
   time=mktime(&bt.tm);
   /****.... ReleaseSemaphore() here.... ****/
   if(offset)
   {  time-=3600*(offset/100);
      time-=60*(offset%100);
   }
   else if(!gmt)
   {  time+=locale->loc_GMTOffset*60;
   }
   return (double)time*1000.0;
}

/*-----------------------------------------------------------------------*/

/* Convert (jthis) to string */
static void Datetostring(struct Jcontext *jc)
{  struct Brokentime bt;
   UBYTE buffer[64];
   Gettime(jc,&bt);
   if(!strftime(buffer,63,"%a, %d %b %Y %H:%M:%S",&bt.tm)) *buffer='\0';
   Asgstring(RETVAL(jc),buffer,jc->pool);
}

/* Convert to GMT string */
static void Datetogmtstring(struct Jcontext *jc)
{  UBYTE buffer[64];
   struct Jobject *jo=jc->jthis;
   struct tm *tm=NULL;
   double d;
   time_t time;
   if(jo && jo->internal)
   {  d=((struct Date *)jo->internal)->date;
      time=(long)(d/1000);
      time+=locale->loc_GMTOffset*60;
      tm=gmtime(&time);
      if(!strftime(buffer,63,"%a, %d %b %Y %H:%M:%S GMT",tm)) *buffer='\0';
      Asgstring(RETVAL(jc),buffer,jc->pool);
   }
}

static void Datevalueof(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   double d;
   if(jo && jo->internal)
   {  d=((struct Date *)jo->internal)->date;
      Asgnumber(RETVAL(jc),VNA_VALID,d);
   }
}

/* Locale hook. (hook)->h_Data points at next buffer position. */
static void __saveds __asm Lputchar(register __a0 struct Hook *hook,
   register __a1 UBYTE c)
{  UBYTE *p=hook->h_Data;
   *p++=c;
   hook->h_Data=p;
}

/* Convert to locale string */
static void Datetolocalestring(struct Jcontext *jc)
{  struct Hook hook;
   UBYTE buffer[64];
   struct Jobject *jo=jc->jthis;
   double d;
   struct DateStamp ds={0};
   long t;
   hook.h_Entry=(HOOKFUNC)Lputchar;
   hook.h_Data=buffer;
   if(jo && jo->internal)
   {  d=((struct Date *)jo->internal)->date;
      t=(long)(d/1000);
      /* JS dates are from 1970, locale dates from 1978. 8 years = 8 * 356 + 2 = 2922 */
      ds.ds_Days=t/86400-2922;
      t=t%86400;
      ds.ds_Minute=t/60;
      t=t%60;
      ds.ds_Tick=t*TICKS_PER_SECOND;
      FormatDate(locale,locale->loc_ShortDateTimeFormat,&ds,&hook);
      Asgstring(RETVAL(jc),buffer,jc->pool);
   }
}

static void Datesetyear(struct Jcontext *jc)
{  double n=Argument(jc,0);
   struct Brokentime bt;
   int y=(int)n;
   if(y>1900) y-=1900;
   Gettime(jc,&bt);
   bt.tm_year=y;
   Settime(jc,&bt);
}

static void Datesetmonth(struct Jcontext *jc)
{  double n=Argument(jc,0);
   struct Brokentime bt;
   Gettime(jc,&bt);
   bt.tm_mon=(int)n;
   Settime(jc,&bt);
}

static void Datesetdate(struct Jcontext *jc)
{  double n=Argument(jc,0);
   struct Brokentime bt;
   Gettime(jc,&bt);
   bt.tm_mday=(int)n;
   Settime(jc,&bt);
}

static void Datesethours(struct Jcontext *jc)
{  double n=Argument(jc,0);
   struct Brokentime bt;
   Gettime(jc,&bt);
   bt.tm_hour=(int)n;
   Settime(jc,&bt);
}

static void Datesetminutes(struct Jcontext *jc)
{  double n=Argument(jc,0);
   struct Brokentime bt;
   Gettime(jc,&bt);
   bt.tm_min=(int)n;
   Settime(jc,&bt);
}

static void Datesetseconds(struct Jcontext *jc)
{  double n=Argument(jc,0);
   struct Brokentime bt;
   Gettime(jc,&bt);
   bt.tm_sec=(int)n;
   Settime(jc,&bt);
}

static void Datesettime(struct Jcontext *jc)
{  double n=Argument(jc,0);
   struct Jobject *jo=jc->jthis;
   if(jo && jo->internal)
   {  ((struct Date *)jo->internal)->date=n-locale->loc_GMTOffset*60000;
   }
}

static void Dategetday(struct Jcontext *jc)
{  struct Brokentime bt;
   Gettime(jc,&bt);
   Asgnumber(RETVAL(jc),VNA_VALID,(double)(bt.tm_wday));
}

static void Dategetyear(struct Jcontext *jc)
{  struct Brokentime bt;
   Gettime(jc,&bt);
   if(bt.tm_year>=100)
   {  bt.tm_year+=1900;
   }
   Asgnumber(RETVAL(jc),VNA_VALID,(double)(bt.tm_year));
}

static void Dategetmonth(struct Jcontext *jc)
{  struct Brokentime bt;
   Gettime(jc,&bt);
   Asgnumber(RETVAL(jc),VNA_VALID,(double)(bt.tm_mon));
}

static void Dategetdate(struct Jcontext *jc)
{  struct Brokentime bt;
   Gettime(jc,&bt);
   Asgnumber(RETVAL(jc),VNA_VALID,(double)(bt.tm_mday));
}

static void Dategethours(struct Jcontext *jc)
{  struct Brokentime bt;
   Gettime(jc,&bt);
   Asgnumber(RETVAL(jc),VNA_VALID,(double)(bt.tm_hour));
}

static void Dategetminutes(struct Jcontext *jc)
{  struct Brokentime bt;
   Gettime(jc,&bt);
   Asgnumber(RETVAL(jc),VNA_VALID,(double)(bt.tm_min));
}

static void Dategetseconds(struct Jcontext *jc)
{  struct Brokentime bt;
   Gettime(jc,&bt);
   Asgnumber(RETVAL(jc),VNA_VALID,(double)(bt.tm_sec));
}

static void Dategettime(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   if(jo && jo->internal)
   {  Asgnumber(RETVAL(jc),VNA_VALID,
         ((struct Date *)jo->internal)->date+locale->loc_GMTOffset*60000);
   }
}

static void Dategettimezoneoffset(struct Jcontext *jc)
{  long offset=locale->loc_GMTOffset;
   Asgnumber(RETVAL(jc),VNA_VALID,(double)offset);
}

static void Dateparse(struct Jcontext *jc)
{  struct Variable *var;
   double d=0.0;
   var=jc->functions.first->local.first;
   if(var->next)
   {  Tostring(&var->val,jc);
      d=Scandate(var->svalue);
      d-=60000.0*locale->loc_GMTOffset;
   }
   Asgnumber(RETVAL(jc),VNA_VALID,d);
}

static void Dateutc(struct Jcontext *jc)
{  struct Brokentime bt={0};
   time_t time;
   bt.tm_year=(int)Argument(jc,0);
   bt.tm_mon=(int)Argument(jc,1);
   bt.tm_mday=(int)Argument(jc,2);
   bt.tm_hour=(int)Argument(jc,3);
   bt.tm_min=(int)Argument(jc,4);
   bt.tm_sec=(int)Argument(jc,5);
   time=mktime(&bt.tm);
   Asgnumber(RETVAL(jc),VNA_VALID,(double)time*1000.0);
}

/* Dispose a Date object */
static void Destructor(struct Date *d)
{  FREE(d);
}

/* Make (jthis) a new Date object */
static void Constructor(struct Jcontext *jc)
{  struct Jobject *jo=jc->jthis;
   struct Date *d;
   if(jc->flags&EXF_CONSTRUCT)
   {  if(jo)
      {  if(d=ALLOCSTRUCT(Date,1,0,jc->pool))
         {  struct Variable *arg1;
            jo->internal=d;
            jo->dispose=Destructor;
            arg1=jc->functions.first->local.first;
            if(arg1->next && arg1->type!=VTP_UNDEFINED)
            {  if(arg1->next->next && arg1->next->type!=VTP_UNDEFINED)
               {  struct Brokentime bt={0};
                  bt.tm_year=(int)Argument(jc,0);
                  if(bt.tm_year<70) bt.tm_year+=100;
                  else if(bt.tm_year>=1900) bt.tm_year-=1900;
                  bt.tm_mon=(int)Argument(jc,1);
                  bt.tm_mday=(int)Argument(jc,2);
                  bt.tm_hour=(int)Argument(jc,3);
                  bt.tm_min=(int)Argument(jc,4);
                  bt.tm_sec=(int)Argument(jc,5);
                  Settime(jc,&bt);
               }
               else
               {  /* Only 1 argument */
                  if(arg1->type==VTP_STRING)
                  {  d->date=Scandate(arg1->svalue)-60000.0*locale->loc_GMTOffset;
                  }
                  else
                  {  d->date=Argument(jc,0)-60000.0*locale->loc_GMTOffset;
                  }
               }
            }
            else
            {  d->date=Today();
            }
         }
      }
   }
   else
   {  /* Not called as a constructor; return date string */
      time_t time;
      double date=Today();
      struct tm *tm=NULL;
      UBYTE buffer[64];
      time=(long)(date/1000);
      tm=gmtime(&time);
      if(!strftime(buffer,63,"%a, %d %b %Y %H:%M:%S",tm)) *buffer='\0';
      Asgstring(RETVAL(jc),buffer,jc->pool);
   }
}

/*-----------------------------------------------------------------------*/

double Today(void)
{  unsigned int clock[2]={ 0, 0 };
   double t;
   timer(clock);
   t=1000.0*clock[0]+clock[1]/1000;
   /* System time is since 1978, convert to 1970 (2 leap years) */
   t+=(8*365+2)*24*60*60*1000.0;
   return t;
}

void Initdate(struct Jcontext *jc)
{  struct Jobject *jo,*f;
   if(jo=Internalfunction(jc,"Date",Constructor,"arg1","month","day",
      "hours","minutes","seconds",NULL))
   {  Addprototype(jc,jo);
      if(f=Internalfunction(jc,"parse",Dateparse,"dateString",NULL))
      {  Addinternalproperty(jc,jo,f);
      }
      if(f=Internalfunction(jc,"UTC",Dateutc,"year","month","day","hrs","min","sec",NULL))
      {  Addinternalproperty(jc,jo,f);
      }
      if(f=Internalfunction(jc,"toString",Datetostring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"toGMTString",Datetogmtstring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"toLocaleString",Datetolocalestring,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"valueOf",Datevalueof,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"setYear",Datesetyear,"yearValue",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"setMonth",Datesetmonth,"monthValue",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"setDate",Datesetdate,"dayValue",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"setHours",Datesethours,"hoursValue",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"setMinutes",Datesetminutes,"minutesValue",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"setSeconds",Datesetseconds,"secondsValue",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"setTime",Datesettime,"timeValue",NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getDay",Dategetday,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getYear",Dategetyear,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getMonth",Dategetmonth,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getDate",Dategetdate,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getHours",Dategethours,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getMinutes",Dategetminutes,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getSeconds",Dategetseconds,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getTime",Dategettime,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      if(f=Internalfunction(jc,"getTimezoneOffset",Dategettimezoneoffset,NULL))
      {  Addtoprototype(jc,jo,f);
      }
      Addglobalfunction(jc,jo);
      Keepobject(jo,TRUE);
   }
}
