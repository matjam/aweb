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

/* awebssl.h - AWeb common definitions for amissl and miamissl function libraries */

#ifndef AWEBSSL_H
#define AWEBSSL_H

/* Initialize ssl and AwebSslBase. */
struct Assl *Assl_initamissl(struct Library *socketbase);
struct Assl *Assl_initmiamissl(void);


/* Return values for Assl_connect(): */
#define ASSLCONNECT_OK     0  /* connection ok */
#define ASSLCONNECT_FAIL   1  /* connection failed */
#define ASSLCONNECT_DENIED 2  /* connection denied by user */


/* Hook called from SSL callback hook; defined in http.c */
extern BOOL Httpcertaccept(char *hostname,char *certname);


#endif