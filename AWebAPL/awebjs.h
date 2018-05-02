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

/* awebjs.h - AWeb js common definitions */

#ifndef AWEBJS_H
#define AWEBJS_H

#include <exec/types.h>
#include <exec/memory.h>
#include <ezlists.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/locale_protos.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pragmas/exec_pragmas.h>
#include <pragmas/locale_pragmas.h>

// #define JSDEBUG
// #define JSADDRESS

// #define debug KPrintF
#define debug / ## /KPrintF

#define STRNIEQUAL(a,b,n)  !strnicmp(a,b,n)
#define STRNEQUAL(a,b,n)   !strncmp(a,b,n)
#define STRIEQUAL(a,b)     !stricmp(a,b)
#define STREQUAL(a,b)      !strcmp(a,b)

#define BOOLVAL(x)         ((x)?TRUE:FALSE)

#define ALLOCTYPE(t,n,f,p)    (t*)Pallocmem((n)*sizeof(t),(f)|MEMF_PUBLIC,p)
#define ALLOCSTRUCT(s,n,f,p)  ALLOCTYPE(struct s,n,f,p)
#define FREE(p)               Freemem(p)

/* get pointer to first vararg after p */
#define VARARG(p)          (void *)((ULONG)&p+sizeof(p))

#ifndef OSNEED
#ifdef NEED35
#define OSNEED(a,b) (b)
#else
#define OSNEED(a,b) (a)
#endif
#endif

extern struct Locale *locale;
extern struct Library *LocaleBase;
extern struct Hook idcmphook;

/*-----------------------------------------------------------------------*/

typedef BOOL Jfeedback(struct Jcontext *jc);

struct Jcontext
{  void *pool;
   struct Token *nexttoken;
   void *program;
   LIST(Jobject) objects;
   USHORT flags;
   USHORT dflags;             /* Debugger state */
   UBYTE *screenname;
   long generation;
   struct Jobject *fscope;    /* Scope to add functions and global variables to. */
   struct Jobject *jthis;
   ULONG protkey;             /* Runtime protection key */
   ULONG userdata;
   Jfeedback *feedback;
   long linenr;               /* Starting line # of source */
   ULONG fbtime;              /* Time (s) when to give next feedback */
   ULONG warntime;            /* Time (s) when to give warning */
   ULONG warnmem;             /* Memory amount when to give warning */
   struct Variable *valvar;   /* Variable holding execution result */
   struct Value *val;         /* Pointer to valvar->val */
   struct Variable *varref;   /* Reference to assignable variable */
   struct Variable *result;   /* Last program execution result */
   USHORT complete;           /* Normal or abrubt completion, see below */
   LIST(Function) functions;  /* Function call stack. Last one is JS global scope,
                               * for standard JS defined variables. */
   struct Jobject *tostring;  /* General default toString function */
   struct Jobject *eval;      /* General standard eval function */
   struct Jobject *boolean;   /* Standard Boolean constructor function */
   struct Jobject *function;  /* Standard Function constructor function */
   struct Jobject *number;    /* Standard Number constructor function */
   struct Jobject *string;    /* Standard String constructor function */
   struct Jobject *array;     /* Standard Array constructor function */
};

#define JCF_ERROR       0x0001   /* Compiler error occurred */
#define JCF_IGNORE      0x0002   /* Ignore all errors */
#define JCF_ERRORS      0x0004   /* Show compilation error requesters */

#define EXF_KEEPREF     0x0100   /* Variable reference is valid */
#define EXF_CONSTRUCT   0x0200   /* Function was called as constructor */
#define EXF_STOP        0x0400   /* Feedback function requested stop */
#define EXF_ASGONLY     0x0800   /* Rhs of . operator is only for assign use */
#define EXF_ERRORS      0x1000   /* Show runtime error requesters */
#define EXF_DONTSTOP    0x2000   /* Don't stop after error encountered */
#define EXF_WARNINGS    0x4000   /* Issue runtime loop warnings */

#define DEBF_DEBUG      0x0001   /* Use debugger at all */
#define DEBF_DBREAK     0x0002   /* Debug break at next element */
#define DEBF_DOPEN      0x0004   /* Debuger was opened, close when ready */

#define ECO_NORMAL   0
#define ECO_CONTINUE 1
#define ECO_BREAK    2
#define ECO_RETURN   3

/*-----------------------------------------------------------------------*/

struct Value
{  UBYTE type;       /* type, see below */
   UBYTE attr;       /* type-specific attributes, see below */
   union
   {  double nvalue; /* number */
      BOOL bvalue;   /* boolean */
      UBYTE *svalue; /* string */
      struct
      {  struct Jobject *ovalue; /* object or function. NOTE: can be NULL. */
         struct Jobject *fthis;  /* (this) object for this function */
      } obj;
   } value;
};

#define VTP_UNDEFINED   0
#define VTP_NUMBER      1
#define VTP_BOOLEAN     2
#define VTP_STRING      3
#define VTP_OBJECT      4  /* Can be function too */

/* Attributes for number */
#define VNA_VALID       0
#define VNA_NAN         1  /* nvalue=0.0 */
#define VNA_INFINITY    2  /* nvalue=1.0 */
#define VNA_NEGINFINITY 3  /* nvalue=-1.0 */

/* Hook function when variable is assigned to.
 * Returns TRUE if it understands the function, FALSE if default
 * action should be taken. */
typedef BOOL Varhookfunc(struct Varhookdata *data);

struct Varhookdata
{  struct Jcontext *jc;       /* Execution context */
   USHORT code;               /* Function code */
   struct Variable *var;      /* Variable affected (that has this hook defined) */
   void *hookdata;            /* Private data for hook */
   struct Variable *value;    /* Value to set variable to (VHC_SET) or to get into (VHC_GET) */
   UBYTE *name;               /* Name of variable to get or set */
};

#define VHC_SET         1  /* Set variable to this value */
#define VHC_GET         2  /* Get variable value */

struct Variable
{  NODE(Variable);
   UBYTE *name;
   struct Value val;
   Varhookfunc *hook;         /* Hook to call when variable is changed */
   void *hookdata;            /* Private data for use in hook */
   ULONG protkey;             /* Data protection key */
   USHORT flags;
};

#define VARF_HIDDEN     0x0001   /* Property is hidden (doesn't show up in for(in)) */
#define VARF_SYNONYM    0x0002   /* Use variable pointed to by hookdata instead. */

/* Hook function when property is added to object.
 * Returns TRUE if it understands the function, FALSE if default
 * action should be taken. */
typedef BOOL Objhookfunc(struct Objhookdata *data);

struct Objhookdata
{  struct Jcontext *jc;       /* Execution context */
   USHORT code;               /* Function code */
   struct Jobject *jo;        /* Object affected */
   UBYTE *name;               /* Name of property to add */
};

#define OHC_ADDPROPERTY 1  /* Add a property */

struct Jobject
{  NODE(Jobject);
   struct Jobject *constructor;
   LIST(Variable) properties;
   struct Elementfunc *function;
   Objhookfunc *hook;         /* Hook to call when property is added */
   void *internal;
   void (*dispose)(void *);   /* Destructor function for (internal) */
   USHORT flags;
   USHORT dumpnr;             /* Id number for debugger dump */
   long keepnr;               /* If >0, mark as used from here */
};

#define OBJF_CLEARING   0x0001   /* Clearing this object */
#define OBJF_TEMP       0x0002   /* Temporary object */
#define OBJF_USED       0x0004   /* Don't sweep away */
#define OBJF_ASFUNCTION 0x0008   /* Object may be called as function to obtain its properties */

/*-----------------------------------------------------------------------*/

struct Function
{  NODE(Function);
   LIST(Variable) local;      /* Arguments and local variables */
   LIST(With) with;           /* with() stack */
   struct Jobject *arguments; /* Arguments array */
   struct Jobject *def;       /* Function definition */
   struct Jobject *fscope;    /* Global scope for this function */
   struct Value retval;       /* Value of last return statement */
};

struct With
{  NODE(With);
   struct Jobject *jo;
   USHORT flags;
};

#define WITHF_GLOBAL    0x0001   /* Global scope on top level, visible everywhere. */

#define RETVAL(jc)      &((jc)->functions.first->retval)

/*-----------------------------------------------------------------------*/

struct Jbuffer
{  void *pool;
   UBYTE *buffer;
   long length;            /* Length of string in buffer */
   long size;              /* Size of buffer */
};

/*-----------------------------------------------------------------------*/

struct Token
{  USHORT id;
   UBYTE *svalue;
   long ivalue;
   double fvalue;
};

#define JTT_KEYWORD     0x0000
#define JTT_IDENTIFIER  0x1000
#define JTT_LITERAL     0x2000
#define JTT_SEPARATOR   0x3000
#define JTT_OPERATOR    0x4000

enum JTOKEN_IDS
{  JT_BREAK=JTT_KEYWORD+1,
   JT_CONTINUE,JT_DELETE,JT_DO,JT_ELSE,JT_FOR,JT_FUNCTION,
   JT_IF,JT_IN,JT_NEW,JT_RETURN,JT_THIS,
   JT_TYPEOF,JT_VAR,JT_VOID,JT_WHILE,JT_WITH,

   JT_IDENTIFIER=JTT_IDENTIFIER+1,
   
   JT_INTEGERLIT=JTT_LITERAL+1,
   JT_FLOATLIT,JT_BOOLEANLIT,JT_STRINGLIT,JT_NULLLIT,
   
   JT_LEFTPAR=JTT_SEPARATOR+1,JT_RIGHTPAR,
   JT_LEFTBRACE,JT_RIGHTBRACE,
   JT_LEFTBRACKET,JT_RIGHTBRACKET,
   JT_SEMICOLON,JT_COMMA,
   
   JT_ASSIGN=JTT_OPERATOR+1,
   JT_GT,JT_LT,JT_NOT,JT_BITNEG,JT_CONDIF,JT_CONDELSE,JT_DOT,
   JT_EQ,JT_LE,JT_GE,JT_NE,JT_AND,JT_OR,JT_INC,JT_DEC,
   JT_PLUS,JT_MINUS,JT_MULT,JT_DIV,JT_REM,JT_BITAND,JT_BITOR,JT_BITXOR,
   JT_SHLEFT,JT_SHRIGHT,JT_USHRIGHT,
   JT_APLUS,JT_AMINUS,JT_AMULT,JT_ADIV,JT_AREM,JT_ABITAND,JT_ABITOR,JT_ABITXOR,
   JT_ASHLEFT,JT_ASHRIGHT,JT_AUSHRIGHT,

#ifdef JSDEBUG
   JT_DEBUG=JTT_KEYWORD+63,
#endif
#ifdef JSADDRESS
   JT_ADDRESS=JTT_KEYWORD+64,
#endif
};

/*-----------------------------------------------------------------------*/

struct Element             /* A compiler element */
{  USHORT type;            /* Element type */
   long generation;
   short linenr;
   void *sub1,*sub2,*sub3,*sub4;
                           /* Child elements. */
};

struct Elementint          /* An integer type literal */
{  USHORT type;
   long generation;
   short linenr;
   long ivalue;
};

struct Elementfloat        /* A float type literal */
{  USHORT type;
   long generation;
   short linenr;
   double fvalue;
};

struct Elementstring       /* A string literal, or ID name */
{  USHORT type;
   long generation;
   short linenr;
   UBYTE *svalue;
};

struct Elementlist         /* A list type compiler element */
{  USHORT type;            /* Element type */
   long generation;
   short linenr;
   LIST(Elementnode) subs; /* List of child element references */
};

struct Elementfunc         /* A function [object] definition */
{  USHORT type;            /* ET_FUNCTION */
   long generation;
   short linenr;
   UBYTE *name;            /* Function name or NULL */
   LIST(Elementnode) subs; /* List of arguments */
   void *body;             /* Function body */
   struct Jobject *fscope; /* Scope for which function is defined, i.e. scope to look
                            * for global variables used in function body. */
};

struct Elementnode         /* A listnode reference to another element */
{  NODE(Elementnode);
   void *sub;
};

/* Element types serve as ID of polymorphic elements, and as index in element class table */
enum ELEMENT_TYPES
{  
   /* List type elements */
   ET_PROGRAM=1,           /* List of functions and/or statements */
   ET_CALL,                /* List, 1st sub is function, rest is arguments */
   ET_COMPOUND,            /* List of elements */
   ET_VARLIST,             /* List of ET_VAR which are variable declarations */
   
   /* Function type elements */
   ET_FUNCTION,

   /* Nullary elements */
   ET_BREAK,ET_CONTINUE,
   ET_THIS,
   ET_NULL,
   ET_EMPTY,
   
   /* Unary elements */
   ET_NEGATIVE,ET_NOT,ET_BITNEG,
   ET_PREINC,ET_PREDEC,ET_POSTINC,ET_POSTDEC,
   ET_NEW,ET_DELETE,ET_TYPEOF,ET_VOID,ET_RETURN,
   ET_INTERNAL,            /* Internal function */
   ET_FUNCEVAL,            /* Evaluate Function object body */
   
   /* Binary elements */
   ET_PLUS,ET_MINUS,ET_MULT,ET_DIV,ET_REM,ET_BITAND,ET_BITOR,ET_BITXOR,
   ET_SHLEFT,ET_SHRIGHT,ET_USHRIGHT,
   ET_EQ,ET_NE,ET_LT,ET_GT,ET_LE,ET_GE,
   ET_AND,ET_OR,
   ET_ASSIGN,
   ET_APLUS,ET_AMINUS,ET_AMULT,ET_ADIV,ET_AREM,ET_ABITAND,ET_ABITOR,ET_ABITXOR,
   ET_ASHLEFT,ET_ASHRIGHT,ET_AUSHRIGHT,
   ET_COMMA,
   ET_WHILE,ET_DO,ET_WITH,ET_DOT,ET_INDEX,
   ET_VAR,
   
   /* Ternary elements */
   ET_COND,ET_IF,ET_FORIN,
   
   /* Quarnery elements */
   ET_FOR,
   
   /* Literals and identifier */
   ET_INTEGER,ET_FLOAT,ET_BOOLEAN,ET_STRING,
   ET_IDENTIFIER,

#ifdef JSDEBUG
   ET_DEBUG,
#endif
#ifdef JSADDRESS
   ET_ADDRESS,
#endif
};

/*-----------------------------------------------------------------------*/

#endif
