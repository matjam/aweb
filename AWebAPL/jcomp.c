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

/* jcomp.c - AWeb js compiler */

#include "awebjs.h"
#include "jprotos.h"

/*-----------------------------------------------------------------------*/

/* Create an element */
static struct Element *Newelement(struct Jcontext *jc,void *pa,USHORT type,
   void *sub1,void *sub2,void *sub3,void *sub4)
{  struct Element *elt=ALLOCSTRUCT(Element,1,0,jc->pool);
   if(elt)
   {  elt->type=type;
      elt->generation=jc->generation;
      elt->linenr=jc->linenr+Plinenr(pa);
      elt->sub1=sub1;
      elt->sub2=sub2;
      elt->sub3=sub3;
      elt->sub4=sub4;
   }
   return elt;
}

/* Create a node for this element */
static struct Elementnode *Newnode(void *pool,void *elt)
{  struct Elementnode *enode=NULL;
   if(elt)
   {  if(enode=ALLOCSTRUCT(Elementnode,1,0,pool))
      {  enode->sub=elt;
      }
   }
   return enode;
}

/* Skip this token, or any token */
static void Skiptoken(struct Jcontext *jc,void *pa,USHORT id)
{  if(jc->nexttoken->id)
   {  if(!id || id==jc->nexttoken->id)
      {  jc->nexttoken=Nexttoken(pa);
      }
      else if(id)
      {  Errormsg(pa,"%s expected",Tokenname(id));
      }
   }
}

/*-----------------------------------------------------------------------*/

/* Root elements */

static void *Identifier(struct Jcontext *jc,void *pa)
{  struct Elementstring *elt=ALLOCSTRUCT(Elementstring,1,0,jc->pool);
   elt->type=ET_IDENTIFIER;
   elt->generation=jc->generation;
   elt->linenr=Plinenr(pa);
   elt->svalue=Jdupstr(jc->nexttoken->svalue,-1,jc->pool);
   return elt;
}

static void *Integer(struct Jcontext *jc,void *pa)
{  struct Elementint *elt=ALLOCSTRUCT(Elementint,1,0,jc->pool);
   elt->type=ET_INTEGER;
   elt->generation=jc->generation;
   elt->linenr=Plinenr(pa);
   elt->ivalue=jc->nexttoken->ivalue;
   return elt;
}

static void *Float(struct Jcontext *jc,void *pa)
{  struct Elementfloat *elt=ALLOCSTRUCT(Elementfloat,1,0,jc->pool);
   elt->type=ET_FLOAT;
   elt->generation=jc->generation;
   elt->linenr=Plinenr(pa);
   elt->fvalue=jc->nexttoken->fvalue;
   return elt;
}

static void *Boolean(struct Jcontext *jc,void *pa)
{  struct Elementint *elt=ALLOCSTRUCT(Elementint,1,0,jc->pool);
   elt->type=ET_BOOLEAN;
   elt->generation=jc->generation;
   elt->linenr=Plinenr(pa);
   elt->ivalue=jc->nexttoken->ivalue;
   return elt;
}

static void *String(struct Jcontext *jc,void *pa)
{  struct Elementstring *elt=ALLOCSTRUCT(Elementstring,1,0,jc->pool);
   elt->type=ET_STRING;
   elt->generation=jc->generation;
   elt->linenr=Plinenr(pa);
   elt->svalue=Jdupstr(jc->nexttoken->svalue,-1,jc->pool);
   return elt;
}

/*-----------------------------------------------------------------------*/

static void *Expression(struct Jcontext *jc,void *pa);
static void *Assignmentexpression(struct Jcontext *jc,void *pa);
static void *Compoundstatement(struct Jcontext *jc,void *pa);

static void *Primaryexpression(struct Jcontext *jc,void *pa)
{  void *elt=NULL;
   switch(jc->nexttoken->id)
   {  case JT_LEFTPAR:
         Skiptoken(jc,pa,0);
         elt=Expression(jc,pa);
         Skiptoken(jc,pa,JT_RIGHTPAR);
         break;
      case JT_IDENTIFIER:
         elt=Identifier(jc,pa);
         Skiptoken(jc,pa,0);
         break;
      case JT_INTEGERLIT:
         elt=Integer(jc,pa);
         Skiptoken(jc,pa,0);
         break;
      case JT_FLOATLIT:
         elt=Float(jc,pa);
         Skiptoken(jc,pa,0);
         break;
      case JT_BOOLEANLIT:
         elt=Boolean(jc,pa);
         Skiptoken(jc,pa,0);
         break;
      case JT_STRINGLIT:
         elt=String(jc,pa);
         Skiptoken(jc,pa,0);
         break;
      case JT_NULLLIT:
         elt=Newelement(jc,pa,ET_NULL,NULL,NULL,NULL,NULL);
         Skiptoken(jc,pa,0);
         break;
      case JT_THIS:
         elt=Newelement(jc,pa,ET_THIS,NULL,NULL,NULL,NULL);
         Skiptoken(jc,pa,0);
         break;
      case JT_RIGHTBRACE:
      case JT_RIGHTPAR:
      case JT_SEMICOLON:
         break;
      default:
         Errormsg(pa,"Expression expected");
         break;
   }
   return elt;
}

static void *Functioncall(struct Jcontext *jc,void *pa,void *elt)
{  struct Elementlist *elist=ALLOCSTRUCT(Elementlist,1,0,jc->pool);
   struct Elementnode *enode;
   struct Element *arg;
   if(elist)
   {  elist->type=ET_CALL;
      elist->generation=jc->generation;
      elist->linenr=Plinenr(pa);
      NEWLIST(&elist->subs);
      if(enode=Newnode(jc->pool,elt))
      {  ADDTAIL(&elist->subs,enode);
      }
      Skiptoken(jc,pa,JT_LEFTPAR);
      while(arg=Assignmentexpression(jc,pa))
      {  if(enode=Newnode(jc->pool,arg))
         {  ADDTAIL(&elist->subs,enode);
         }
         if(jc->nexttoken->id==JT_RIGHTPAR) break;
         Skiptoken(jc,pa,JT_COMMA);
      }
      Skiptoken(jc,pa,JT_RIGHTPAR);
   }
   return elist;
}

static void *Memberexpression(struct Jcontext *jc,void *pa)
{  BOOL done=FALSE;
   void *elt=Primaryexpression(jc,pa),*sub2;
   while(!done)
   {  switch(jc->nexttoken->id)
      {  case JT_DOT:
            Skiptoken(jc,pa,0);
            sub2=Primaryexpression(jc,pa);
            elt=Newelement(jc,pa,ET_DOT,elt,sub2,NULL,NULL);
            break;
         case JT_LEFTBRACKET:
            Skiptoken(jc,pa,0);
            sub2=Expression(jc,pa);
            elt=Newelement(jc,pa,ET_INDEX,elt,sub2,NULL,NULL);
            Skiptoken(jc,pa,JT_RIGHTBRACKET);
            break;
         case JT_LEFTPAR:
            elt=Functioncall(jc,pa,elt);
            break;
         default:
            done=TRUE;
            break;
      }
   }
   return elt;
}

static void *Unaryexpression(struct Jcontext *jc,void *pa)
{  void *elt;
   USHORT preop=0;
   switch(jc->nexttoken->id)
   {  case JT_MINUS:    preop=ET_NEGATIVE;break;
      case JT_NOT:      preop=ET_NOT;break;
      case JT_BITNEG:   preop=ET_BITNEG;break;
      case JT_INC:      preop=ET_PREINC;break;
      case JT_DEC:      preop=ET_PREDEC;break;
      case JT_DELETE:   preop=ET_DELETE;break;
      case JT_TYPEOF:   preop=ET_TYPEOF;break;
      case JT_VOID:     preop=ET_VOID;break;
      case JT_NEW:      preop=ET_NEW;break;
#ifdef JSADDRESS
      case JT_ADDRESS:  preop=ET_ADDRESS;break;
#endif
   }
   if(preop)
   {  Skiptoken(jc,pa,0);
      elt=Unaryexpression(jc,pa);
   }
   else
   {  elt=Memberexpression(jc,pa);
   }
   if(!preop)
   {  switch(jc->nexttoken->id)
      {  case JT_INC:   preop=ET_POSTINC;break;
         case JT_DEC:   preop=ET_POSTDEC;break;
      }
      if(preop) Skiptoken(jc,pa,0);
   }
   if(preop)
   {  elt=Newelement(jc,pa,preop,elt,NULL,NULL,NULL);
   }
   return elt;
}

static void *Multiplicativeexpression(struct Jcontext *jc,void *pa)
{  void *elt=Unaryexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_MULT:     op=ET_MULT;break;
         case JT_DIV:      op=ET_DIV;break;
         case JT_REM:      op=ET_REM;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Unaryexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Additiveexpression(struct Jcontext *jc,void *pa)
{  void *elt=Multiplicativeexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_PLUS:     op=ET_PLUS;break;
         case JT_MINUS:    op=ET_MINUS;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Multiplicativeexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Shiftexpression(struct Jcontext *jc,void *pa)
{  void *elt=Additiveexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_SHLEFT:   op=ET_SHLEFT;break;
         case JT_SHRIGHT:  op=ET_SHRIGHT;break;
         case JT_USHRIGHT: op=ET_USHRIGHT;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Additiveexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Relationalexpression(struct Jcontext *jc,void *pa)
{  void *elt=Shiftexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_LT:       op=ET_LT;break;
         case JT_GT:       op=ET_GT;break;
         case JT_LE:       op=ET_LE;break;
         case JT_GE:       op=ET_GE;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Shiftexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Equalityexpression(struct Jcontext *jc,void *pa)
{  void *elt=Relationalexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_EQ:       op=ET_EQ;break;
         case JT_NE:       op=ET_NE;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Relationalexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Bitwiseandexpression(struct Jcontext *jc,void *pa)
{  void *elt=Equalityexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_BITAND:   op=ET_BITAND;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Equalityexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Bitwisexorexpression(struct Jcontext *jc,void *pa)
{  void *elt=Bitwiseandexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_BITXOR:   op=ET_BITXOR;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Bitwiseandexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Bitwiseorexpression(struct Jcontext *jc,void *pa)
{  void *elt=Bitwisexorexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_BITOR:    op=ET_BITOR;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Bitwisexorexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Andexpression(struct Jcontext *jc,void *pa)
{  void *elt=Bitwiseorexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_AND:      op=ET_AND;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Bitwiseorexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Orexpression(struct Jcontext *jc,void *pa)
{  void *elt=Andexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_OR:       op=ET_OR;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Andexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Conditionalexpression(struct Jcontext *jc,void *pa)
{  struct Element *elt=Orexpression(jc,pa);
   if(jc->nexttoken->id==JT_CONDIF)
   {  Skiptoken(jc,pa,0);
      elt=Newelement(jc,pa,ET_COND,elt,Assignmentexpression(jc,pa),NULL,NULL);
      if(elt && jc->nexttoken->id==JT_CONDELSE)
      {  Skiptoken(jc,pa,0);
         elt->sub3=Assignmentexpression(jc,pa);
      }
      else
      {  Errormsg(pa,"':' expected");
      }
   }
   return elt;
}

static void *Assignmentexpression(struct Jcontext *jc,void *pa)
{  void *elt=Conditionalexpression(jc,pa);
   USHORT op;
   switch(jc->nexttoken->id)
   {  case JT_ASSIGN:   op=ET_ASSIGN;break;
      case JT_APLUS:    op=ET_APLUS;break;
      case JT_AMINUS:   op=ET_AMINUS;break;
      case JT_AMULT:    op=ET_AMULT;break;
      case JT_ADIV:     op=ET_ADIV;break;
      case JT_AREM:     op=ET_AREM;break;
      case JT_ABITAND:  op=ET_ABITAND;break;
      case JT_ABITOR:   op=ET_ABITOR;break;
      case JT_ABITXOR:  op=ET_ABITXOR;break;
      case JT_ASHLEFT:  op=ET_ASHLEFT;break;
      case JT_ASHRIGHT: op=ET_ASHRIGHT;break;
      case JT_AUSHRIGHT:op=ET_AUSHRIGHT;break;
      default:          op=0;break;
   }
   if(op)
   {  Skiptoken(jc,pa,0);
      elt=Newelement(jc,pa,op,elt,Assignmentexpression(jc,pa),NULL,NULL);
   }
   return elt;
}

static void *Expression(struct Jcontext *jc,void *pa)
{  void *elt=Assignmentexpression(jc,pa);
   USHORT op;
   do
   {  switch(jc->nexttoken->id)
      {  case JT_COMMA:    op=ET_COMMA;break;
         default:          op=0;break;
      }
      if(op)
      {  Skiptoken(jc,pa,0);
         elt=Newelement(jc,pa,op,elt,Assignmentexpression(jc,pa),NULL,NULL);
      }
   } while(op);
   return elt;
}

static void *Condition(struct Jcontext *jc,void *pa)
{  void *elt=NULL;
   if(jc->nexttoken->id==JT_LEFTPAR)
   {  Skiptoken(jc,pa,0);
      elt=Expression(jc,pa);
      Skiptoken(jc,pa,JT_RIGHTPAR);
   }
   else Errormsg(pa,"'(' expected");
   return elt;
}

static void *Varlist(struct Jcontext *jc,void *pa)
{  struct Elementlist *elist;
   struct Elementnode *enode;
   struct Element *elt;
   if(elist=ALLOCSTRUCT(Elementlist,1,0,jc->pool))
   {  elist->type=ET_VARLIST;
      elist->generation=jc->generation;
      elist->linenr=Plinenr(pa);
      NEWLIST(&elist->subs);
      if(jc->nexttoken->id!=JT_IDENTIFIER)
      {  Errormsg(pa,"Identifier expected");
      }
      while(jc->nexttoken->id==JT_IDENTIFIER)
      {  elt=Identifier(jc,pa);
         Skiptoken(jc,pa,0);
         if(jc->nexttoken->id==JT_ASSIGN)
         {  Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_VAR,elt,Assignmentexpression(jc,pa),NULL,NULL);
         }
         else
         {  elt=Newelement(jc,pa,ET_VAR,elt,NULL,NULL,NULL);
         }
         if(enode=Newnode(jc->pool,elt))
         {  ADDTAIL(&elist->subs,enode);
         }
         if(jc->nexttoken->id!=JT_COMMA) break;
         Skiptoken(jc,pa,JT_COMMA);
      }
   }
   return elist;
}

static void *Variablesorexpression(struct Jcontext *jc,void *pa)
{  void *elt;
   if(jc->nexttoken->id==JT_VAR)
   {  Skiptoken(jc,pa,0);
      elt=Varlist(jc,pa);
   }
   else
   {  elt=Expression(jc,pa);
   }
   return elt;
}

static void *Statement(struct Jcontext *jc,void *pa)
{  struct Element *elt=NULL;
   if(!Feedback(jc))
   {  jc->flags|=JCF_IGNORE;
   }
   if(!(jc->flags&JCF_IGNORE))
   {  switch(jc->nexttoken->id)
      {  case JT_IF:
            Skiptoken(jc,pa,0);
            if(elt=Newelement(jc,pa,ET_IF,Condition(jc,pa),Statement(jc,pa),NULL,NULL))
            if(elt && jc->nexttoken->id==JT_ELSE)
            {  Skiptoken(jc,pa,0);
               elt->sub3=Statement(jc,pa);
            }
            break;
         case JT_WHILE:
            Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_WHILE,Condition(jc,pa),Statement(jc,pa),NULL,NULL);
            break;
         case JT_DO:
            Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_DO,Statement(jc,pa),NULL,NULL,NULL);
            Skiptoken(jc,pa,JT_WHILE);
            elt->sub2=Condition(jc,pa);
            break;
         case JT_FOR:
            Skiptoken(jc,pa,0);
            Skiptoken(jc,pa,JT_LEFTPAR);
            elt=Variablesorexpression(jc,pa);
            if(elt && jc->nexttoken->id==JT_IN)
            {  Skiptoken(jc,pa,0);
               elt=Newelement(jc,pa,ET_FORIN,elt,Expression(jc,pa),NULL,NULL);
               Skiptoken(jc,pa,JT_RIGHTPAR);
               elt->sub3=Statement(jc,pa);
            }
            else if(jc->nexttoken->id==JT_SEMICOLON)
            {  Skiptoken(jc,pa,0);
               elt=Newelement(jc,pa,ET_FOR,elt,Expression(jc,pa),NULL,NULL);
               Skiptoken(jc,pa,JT_SEMICOLON);
               elt->sub3=Expression(jc,pa);
               Skiptoken(jc,pa,JT_RIGHTPAR);
               elt->sub4=Statement(jc,pa);
            }
            else
            {  Errormsg(pa,"'in' or ';' expected");
            }
            break;
         case JT_BREAK:
            Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_BREAK,NULL,NULL,NULL,NULL);
            if(jc->nexttoken->id==JT_SEMICOLON)
            {  Skiptoken(jc,pa,0);
            }
            break;
         case JT_CONTINUE:
            Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_CONTINUE,NULL,NULL,NULL,NULL);
            if(jc->nexttoken->id==JT_SEMICOLON)
            {  Skiptoken(jc,pa,0);
            }
            break;
         case JT_WITH:
            Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_WITH,Condition(jc,pa),Statement(jc,pa),NULL,NULL);
            break;
         case JT_RETURN:
            Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_RETURN,Expression(jc,pa),NULL,NULL,NULL);
            if(jc->nexttoken->id==JT_SEMICOLON)
            {  Skiptoken(jc,pa,0);
            }
            break;
         case JT_LEFTBRACE:
            elt=Compoundstatement(jc,pa);
            break;
#ifdef JSDEBUG
         case JT_DEBUG:
            Skiptoken(jc,pa,0);
            elt=Newelement(jc,pa,ET_DEBUG,Expression(jc,pa),NULL,NULL,NULL);
            if(jc->nexttoken->id==JT_SEMICOLON)
            {  Skiptoken(jc,pa,0);
            }
            break;
#endif
         default:
            elt=Variablesorexpression(jc,pa);
            if(jc->nexttoken->id==JT_SEMICOLON)
            {  Skiptoken(jc,pa,0);
            }
            break;
      }
   }
   return elt;
}

static void *Compoundstatement(struct Jcontext *jc,void *pa)
{  struct Elementlist *elist=NULL;
   struct Elementnode *enode;
   ULONG state;
   
   if(jc->nexttoken->id==JT_LEFTBRACE)
   {  Skiptoken(jc,pa,0);
      if(elist=ALLOCSTRUCT(Elementlist,1,0,jc->pool))
      {  elist->type=ET_COMPOUND;
         elist->generation=jc->generation;
         elist->linenr=Plinenr(pa);
         NEWLIST(&elist->subs);
         
         while(jc->nexttoken->id)
         {  state=Parserstate(pa);
            if(enode=Newnode(jc->pool,Statement(jc,pa)))
            {  ADDTAIL(&elist->subs,enode);
            }
            else if(jc->nexttoken->id==JT_RIGHTBRACE) break;
            if(state==Parserstate(pa)) Skiptoken(jc,pa,0);
         }
      }
      Skiptoken(jc,pa,JT_RIGHTBRACE);
   }
   else Errormsg(pa,"'{' expected");
   return elist;
}

static void *Element(struct Jcontext *jc,void *pa)
{  void *elt=NULL;
   struct Elementnode *enode;
   struct Elementfunc *func;
   struct Variable *fprop;
   struct Jobject *fobj;
   if(jc->nexttoken->id==JT_FUNCTION)
   {  Skiptoken(jc,pa,0);
      if(func=ALLOCSTRUCT(Elementfunc,1,0,jc->pool))
      {  func->type=ET_FUNCTION;
         func->generation=jc->generation;
         func->linenr=Plinenr(pa);
         NEWLIST(&func->subs);
         if(jc->nexttoken->id==JT_IDENTIFIER)
         {  func->name=Jdupstr(jc->nexttoken->svalue,-1,jc->pool);
            Skiptoken(jc,pa,0);
         }
         else Errormsg(pa,"Identifier expected");
         if(jc->nexttoken->id==JT_LEFTPAR)
         {  Skiptoken(jc,pa,0);
            for(;;)
            {  if(jc->nexttoken->id==JT_IDENTIFIER)
               {  if(enode=Newnode(jc->pool,Identifier(jc,pa)))
                  {  ADDTAIL(&func->subs,enode);
                  }
                  Skiptoken(jc,pa,0);
               }
               if(jc->nexttoken->id==JT_COMMA)
               {  Skiptoken(jc,pa,0);
               }
               else break;
            }
            Skiptoken(jc,pa,JT_RIGHTPAR);
         }
         else Errormsg(pa,"'(' expected");
         func->body=Compoundstatement(jc,pa);
         
         /* Add function to current scope */
         if(fobj=Newobject(jc))
         {  fobj->function=func;
            if((fprop=Findproperty(jc->fscope,func->name))
            || (fprop=Addproperty(jc->fscope,func->name)))
            {  Asgfunction(&fprop->val,fobj,NULL);
            }
            Addprototype(jc,fobj);
         }
         
         /* Remember current scope with function */
         func->fscope=jc->fscope;
      }
   }
   else
   {  elt=Statement(jc,pa);
   }
   return elt;
}

/* Create new program, or expand existing one */
static void Compileprogram(struct Jcontext *jc,void *pa)
{  struct Elementlist *plist;
   struct Elementnode *enode;
   struct Element *elt;
   ULONG state;
   if(!jc->program)
   {  if(plist=ALLOCSTRUCT(Elementlist,1,0,jc->pool))
      {  plist->type=ET_PROGRAM;
         plist->generation=jc->generation;
         plist->linenr=Plinenr(pa);
         NEWLIST(&plist->subs);
      }
      jc->program=plist;
   }
   if(plist=jc->program)
   {  while(jc->nexttoken->id && !(jc->flags&JCF_IGNORE))
      {  state=Parserstate(pa);
         if(elt=Element(jc,pa))
         {  if(enode=Newnode(jc->pool,elt))
            {  ADDTAIL(&plist->subs,enode);
            }
         }
         if(state==Parserstate(pa)) Skiptoken(jc,pa,0);
      }
   }
}

/*-----------------------------------------------------------------------*/

struct Decompile
{  struct Jbuffer *jb;
   short indent;
   short parlevel;
   BOOL nosemicolon;      /* If TRUE, no semicolon is needed in a compound statement */
};

static void Decompile(struct Decompile *dc,struct Element *elt);

static void Denewline(struct Decompile *dc)
{  short i;
   Addtojbuffer(dc->jb,"\n",1);
   for(i=0;i<dc->indent;i++)
   {  Addtojbuffer(dc->jb,"  ",2);
   }
}

static short Derpar(struct Decompile *dc,short level)
{  short oldlevel=dc->parlevel;
   if(level<dc->parlevel)
   {  Addtojbuffer(dc->jb,"(",1);
   }
   dc->parlevel=level;
   return oldlevel;
}

static void Delpar(struct Decompile *dc,short level)
{  if(level>dc->parlevel)
   {  Addtojbuffer(dc->jb,")",1);
   }
   dc->parlevel=level;
}

static void Desemicolon(struct Decompile *dc)
{  if(!dc->nosemicolon)
   {  Addtojbuffer(dc->jb,";",1);
   }
   dc->nosemicolon=FALSE;
}

static void Deprogram(struct Decompile *dc,struct Elementlist *elist)
{  struct Elementnode *enode;
   for(enode=elist->subs.first;enode->next;enode=enode->next)
   {  if(enode->sub)
      {  Decompile(dc,enode->sub);
         Desemicolon(dc);
      }
   }
}

static void Decall(struct Decompile *dc,struct Elementlist *elist)
{  struct Elementnode *enode;
   BOOL first;
   if(elist->subs.first->next && elist->subs.first->sub)
   {  Decompile(dc,elist->subs.first->sub);
      Addtojbuffer(dc->jb,"(",1);
      first=TRUE;
      for(enode=elist->subs.first->next;enode && enode->next;enode=enode->next)
      {  if(enode->sub)
         {  if(!first)
            {  Addtojbuffer(dc->jb,", ",-1);
            }
            first=FALSE;
            Decompile(dc,enode->sub);
         }
      }
      Addtojbuffer(dc->jb,")",1);
   }
}

static void Decompound(struct Decompile *dc,struct Elementlist *elist)
{  struct Elementnode *enode;
   Addtojbuffer(dc->jb,"{",1);
   dc->indent++;
   for(enode=elist->subs.first;enode->next;enode=enode->next)
   {  Denewline(dc);
      if(enode->sub) Decompile(dc,enode->sub);
      Desemicolon(dc);
   }
   dc->indent--;
   Denewline(dc);
   Addtojbuffer(dc->jb,"}",1);
   dc->nosemicolon=TRUE;
}

static void Devarlist(struct Decompile *dc,struct Elementlist *elist)
{  struct Elementnode *enode;
   BOOL first=TRUE;
   Addtojbuffer(dc->jb,"var ",-1);
   for(enode=elist->subs.first;enode->next;enode=enode->next)
   {  if(!first)
      {  Addtojbuffer(dc->jb,", ",-1);
      }
      first=FALSE;
      if(enode->sub) Decompile(dc,enode->sub);
   }
}

static void Defunction(struct Decompile *dc,struct Elementfunc *func)
{  struct Elementnode *enode;
   BOOL first;
   Addtojbuffer(dc->jb,"function ",-1);
   Addtojbuffer(dc->jb,func->name,-1);
   Addtojbuffer(dc->jb,"(",1);
   first=TRUE;
   for(enode=func->subs.first;enode->next;enode=enode->next)
   {  if(enode->sub)
      {  if(!first)
         {  Addtojbuffer(dc->jb,", ",-1);
         }
         first=FALSE;
         Decompile(dc,enode->sub);
      }
   }
   Addtojbuffer(dc->jb,") ",-1);
   if(func->body)
   {  Decompile(dc,func->body);
   }
   Denewline(dc);
   dc->nosemicolon=TRUE;
}

static void Debreak(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"break",-1);
}

static void Decontinue(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"continue",-1);
}

static void Dethis(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"this",-1);
}

static void Denull(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"null",-1);
}

static void Deempty(struct Decompile *dc,struct Element *elt)
{
}

static void Denegative(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"-",1);
   Decompile(dc,elt->sub1);
}

static void Denot(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"!",1);
   Decompile(dc,elt->sub1);
}

static void Debitneg(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"~",1);
   Decompile(dc,elt->sub1);
}

static void Depreinc(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"++",2);
   Decompile(dc,elt->sub1);
}

static void Depredec(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"--",2);
   Decompile(dc,elt->sub1);
}

static void Depostinc(struct Decompile *dc,struct Element *elt)
{  Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,"++",2);
}

static void Depostdec(struct Decompile *dc,struct Element *elt)
{  Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,"--",2);
}

static void Denew(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"new ",-1);
   Decompile(dc,elt->sub1);
}

static void Dedelete(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"delete ",-1);
   Decompile(dc,elt->sub1);
}

static void Detypeof(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"typeof ",-1);
   Decompile(dc,elt->sub1);
}

static void Devoid(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"void ",-1);
   Decompile(dc,elt->sub1);
}

static void Dereturn(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"return ",-1);
   Decompile(dc,elt->sub1);
}

static void Deinternal(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"{",-1);
   dc->indent++;
   Denewline(dc);
   Addtojbuffer(dc->jb,"[internal code]",-1);
   dc->indent--;
   Denewline(dc);
   Addtojbuffer(dc->jb,"}",-1);
   dc->nosemicolon=TRUE;
}

static void Deplus(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,42);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," + ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deminus(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,42);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," - ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Demult(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,44);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," * ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Dediv(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,44);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," / ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Derem(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,44);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," % ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Debitand(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,28);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," & ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Debitor(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,24);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," | ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Debitxor(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,26);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," ^ ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deshleft(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,40);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," << ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deshright(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,40);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," >> ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deushright(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,40);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," >>> ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deeq(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,30);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," == ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Dene(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,30);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," != ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Delt(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,32);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," < ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Degt(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,32);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," > ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Dele(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,32);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," <= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Dege(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,32);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," >= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deand(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,22);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," && ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deor(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,20);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," || ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deassign(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," = ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deaplus(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," += ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deaminus(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," -= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deamult(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," *= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deadiv(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," /= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Dearem(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," %= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deabitand(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," &= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deabitor(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," |= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deabitxor(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," ^= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deashleft(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," <<= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deashright(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," >>= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Deaushright(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,10);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," >>>= ",-1);
   Decompile(dc,elt->sub2);
   Delpar(dc,level);
}

static void Decomma(struct Decompile *dc,struct Element *elt)
{  Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," , ",-1);
   Decompile(dc,elt->sub2);
}

static void Dewhile(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"while(",-1);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,") ",1);
   Decompile(dc,elt->sub2);
}

static void Dedo(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"do",-1);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,"while(",-1);
   Decompile(dc,elt->sub2);
   Addtojbuffer(dc->jb,") ",1);
}

static void Dewith(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"with(",-1);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,") ",1);
   Decompile(dc,elt->sub2);
}

static void Dedot(struct Decompile *dc,struct Element *elt)
{  Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,".",1);
   Decompile(dc,elt->sub2);
}

static void Deindex(struct Decompile *dc,struct Element *elt)
{  Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,"[",1);
   Decompile(dc,elt->sub2);
   Addtojbuffer(dc->jb,"]",1);
}

static void Devar(struct Decompile *dc,struct Element *elt)
{  Decompile(dc,elt->sub1);
   if(elt->sub2)
   {  Addtojbuffer(dc->jb,"=",1);
      Decompile(dc,elt->sub2);
   }
}

static void Decond(struct Decompile *dc,struct Element *elt)
{  short level=Derpar(dc,5);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," ? ",-1);
   Decompile(dc,elt->sub2);
   Addtojbuffer(dc->jb," : ",-1);
   Decompile(dc,elt->sub3);
   Delpar(dc,level);
}

static void Deif(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"if(",-1);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,") ",2);
   Decompile(dc,elt->sub2);
   if(elt->sub3)
   {  Desemicolon(dc);
      Denewline(dc);
      Addtojbuffer(dc->jb,"else ",-1);
      Decompile(dc,elt->sub3);
   }
}

static void Deforin(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"for(",-1);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb," in ",-1);
   Decompile(dc,elt->sub2);
   Addtojbuffer(dc->jb,") ",-1);
   Decompile(dc,elt->sub3);
}

static void Defor(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"for(",-1);
   Decompile(dc,elt->sub1);
   Addtojbuffer(dc->jb,";",1);
   Decompile(dc,elt->sub2);
   Addtojbuffer(dc->jb,";",1);
   Decompile(dc,elt->sub3);
   Addtojbuffer(dc->jb,") ",-1);
   Decompile(dc,elt->sub4);
}

static void Deinteger(struct Decompile *dc,struct Elementint *elt)
{  UBYTE buf[16];
   sprintf(buf,"%d",elt->ivalue);
   Addtojbuffer(dc->jb,buf,-1);
}

static void Defloat(struct Decompile *dc,struct Elementfloat *elt)
{  UBYTE buf[24];
   sprintf(buf,"%g",elt->fvalue);
   Addtojbuffer(dc->jb,buf,-1);
}

static void Deboolean(struct Decompile *dc,struct Elementint *elt)
{  Addtojbuffer(dc->jb,elt->ivalue?"true":"false",-1);
}

static void Destring(struct Decompile *dc,struct Elementstring *elt)
{  UBYTE *p,*q;
   UBYTE buf[6];
   Addtojbuffer(dc->jb,"\"",1);
   for(p=elt->svalue;*p;p++)
   {  switch(*p)
      {  case '"':   q="\\\"";break;
         case '\b':  q="\\b";break;
         case '\t':  q="\\t";break;
         case '\n':  q="\\n";break;
         case '\f':  q="\\f";break;
         case '\r':  q="\\r";break;
         case '\\':  q="\\\\";break;
         default:
            if(!isprint(*p))
            {  sprintf(buf,"\\x%02x",*p);
               q=buf;
            }
            else
            {  Addtojbuffer(dc->jb,p,1);
               q=NULL;
            }
            break;
      }
      if(q) Addtojbuffer(dc->jb,q,-1);
   }
   Addtojbuffer(dc->jb,"\"",1);
}

static void Deidentifier(struct Decompile *dc,struct Elementstring *elt)
{  Addtojbuffer(dc->jb,elt->svalue,-1);
}

#ifdef JSDEBUG
static void Dedebug(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"debug ",-1);
   Decompile(dc,elt->sub1);
}
#endif

#ifdef JSADDRESS
static void Deaddress(struct Decompile *dc,struct Element *elt)
{  Addtojbuffer(dc->jb,"address ",-1);
   Decompile(dc,elt->sub1);
}
#endif

typedef void Decompelement(void *,void *);
static Decompelement *decomptab[]=
{
   NULL,
   Deprogram,
   Decall,
   Decompound,
   Devarlist,
   Defunction,
   Debreak,
   Decontinue,
   Dethis,
   Denull,
   Deempty,
   Denegative,
   Denot,
   Debitneg,
   Depreinc,
   Depredec,
   Depostinc,
   Depostdec,
   Denew,
   Dedelete,
   Detypeof,
   Devoid,
   Dereturn,
   Deinternal,
   NULL,          /* funceval */
   Deplus,
   Deminus,
   Demult,
   Dediv,
   Derem,
   Debitand,
   Debitor,
   Debitxor,
   Deshleft,
   Deshright,
   Deushright,
   Deeq,
   Dene,
   Delt,
   Degt,
   Dele,
   Dege,
   Deand,
   Deor,
   Deassign,
   Deaplus,
   Deaminus,
   Deamult,
   Deadiv,
   Dearem,
   Deabitand,
   Deabitor,
   Deabitxor,
   Deashleft,
   Deashright,
   Deaushright,
   Decomma,
   Dewhile,
   Dedo,
   Dewith,
   Dedot,
   Deindex,
   Devar,
   Decond,
   Deif,
   Deforin,
   Defor,
   Deinteger,
   Defloat,
   Deboolean,
   Destring,
   Deidentifier,
#ifdef JSDEBUG
   Dedebug,
#endif
#ifdef JSADDRESS
   Deaddress,
#endif
};

static void Decompile(struct Decompile *dc,struct Element *elt)
{  if(elt && decomptab[elt->type])
   {  decomptab[elt->type](dc,elt);
   }
}

/*-----------------------------------------------------------------------*/

static void Disposelt(struct Element *elt);

static void Diselement(struct Element *elt)
{  if(elt->sub1) Disposelt(elt->sub1);
   if(elt->sub2) Disposelt(elt->sub2);
   if(elt->sub3) Disposelt(elt->sub3);
   if(elt->sub4) Disposelt(elt->sub4);
   FREE(elt);
}

static void Disinternal(struct Element *elt)
{  /* Don't free the internal code! */
   FREE(elt);
}

static void Disint(struct Elementint *elt)
{  FREE(elt);
}

static void Disfloat(struct Elementfloat *elt)
{  FREE(elt);
}

static void Disstring(struct Elementstring *elt)
{  if(elt->svalue) FREE(elt->svalue);
   FREE(elt);
}

static void Dislist(struct Elementlist *elt)
{  struct Elementnode *enode;
   while(enode=REMHEAD(&elt->subs))
   {  if(enode->sub) Disposelt(enode->sub);
      FREE(enode);
   }
   FREE(elt);
}

static void Disfunc(struct Elementfunc *elt)
{  struct Elementnode *enode;
   while(enode=REMHEAD(&elt->subs))
   {  if(enode->sub) Disposelt(enode->sub);
      FREE(enode);
   }
   if(elt->body)
   {  Disposelt(elt->body);
      elt->body=NULL;
   }
   if(elt->name)
   {  FREE(elt->name);
   }
   elt->fscope=NULL;
   FREE(elt);
}

typedef void Diselementf(void *);
static Diselementf *distab[]=
{
   NULL,
   Dislist,       /* program */
   Dislist,       /* call */
   Dislist,       /* compound */
   Dislist,       /* varlist */
   Disfunc,       /* function */
   Diselement,    /* break */
   Diselement,    /* continue */
   Diselement,    /* this */
   Diselement,    /* null */
   Diselement,    /* empty */
   Diselement,    /* negative */
   Diselement,    /* not */
   Diselement,    /* bitneg */
   Diselement,    /* preinc */
   Diselement,    /* predec */
   Diselement,    /* postinc */
   Diselement,    /* postdec */
   Diselement,    /* new */
   Diselement,    /* delete */
   Diselement,    /* typeof */
   Diselement,    /* void */
   Diselement,    /* return */
   Disinternal,   /* internal */
   Diselement,    /* funceval */
   Diselement,    /* plus */
   Diselement,    /* minus */
   Diselement,    /* mult */
   Diselement,    /* div */
   Diselement,    /* rem */
   Diselement,    /* bitand */
   Diselement,    /* bitor */
   Diselement,    /* bitxor */
   Diselement,    /* shleft */
   Diselement,    /* shright */
   Diselement,    /* ushright */
   Diselement,    /* eq */
   Diselement,    /* ne */
   Diselement,    /* lt */
   Diselement,    /* gt */
   Diselement,    /* le */
   Diselement,    /* ge */
   Diselement,    /* and */
   Diselement,    /* or */
   Diselement,    /* assign */
   Diselement,    /* aplus */
   Diselement,    /* aminus */
   Diselement,    /* amult */
   Diselement,    /* adiv */
   Diselement,    /* arem */
   Diselement,    /* abitand */
   Diselement,    /* abitor */
   Diselement,    /* abitxor */
   Diselement,    /* ashleft */
   Diselement,    /* ashright */
   Diselement,    /* aushright */
   Diselement,    /* comma */
   Diselement,    /* while */
   Diselement,    /* do */
   Diselement,    /* with */
   Diselement,    /* dot */
   Diselement,    /* index */
   Diselement,    /* var */
   Diselement,    /* cond */
   Diselement,    /* if */
   Diselement,    /* forin */
   Diselement,    /* for */
   Disint,        /* integer */
   Disfloat,      /* float */
   Disint,        /* boolean */
   Disstring,     /* string */
   Disstring,     /* identifier */
#ifdef JSDEBUG
   Diselement,    /* debug */
#endif
#ifdef JSADDRESS
   Diselement,    /* address */
#endif
};

static void Disposelt(struct Element *elt)
{  if(elt && distab[elt->type])
   {  distab[elt->type](elt);
   }
}

/*-----------------------------------------------------------------------*/

void Jcompile(struct Jcontext *jc,UBYTE *source)
{  void *pa;
   if(pa=Newparser(jc,source))
   {  jc->nexttoken=Nexttoken(pa);
      Compileprogram(jc,pa);
      FREE(pa);
   }
}

struct Jobject *Jcompiletofunction(struct Jcontext *jc,UBYTE *source,UBYTE *name)
{  struct Elementfunc *func;
   struct Jobject *fobj=NULL;
   struct Jcontext jc2={0};
   jc2.pool=jc->pool;
   NEWLIST(&jc2.objects);
   NEWLIST(&jc2.functions);
   jc2.generation=jc->generation;
   Jcompile(&jc2,source);
   if(!(jc2.flags&JCF_ERROR))
   {  if(func=ALLOCSTRUCT(Elementfunc,1,0,jc->pool))
      {  func->type=ET_FUNCTION;
         func->generation=jc->generation;
         func->linenr=0;
         NEWLIST(&func->subs);
         func->name=Jdupstr(name,-1,jc->pool);
         func->name[0]=toupper(func->name[0]);
         func->body=jc2.program;
         ((struct Elementlist *)func->body)->type=ET_COMPOUND;
         /* Create function object */
         if(fobj=Newobject(jc))
         {  fobj->function=func;
         }
         /* Remember current scope with function */
         func->fscope=jc->fscope;
      }
   }
   if(!fobj)
   {  Jdispose(jc2.program);
   }
   return fobj;
}

struct Jbuffer *Jdecompile(struct Jcontext *jc,struct Element *elt)
{  struct Decompile dc={0};
   if(dc.jb=Newjbuffer(jc->pool))
   {  Decompile(&dc,elt);
   }
   return dc.jb;
}

void Jdispose(struct Element *elt)
{  if(elt)
   {  Disposelt(elt);
   }
}
