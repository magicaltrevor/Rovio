/****************************************************************************
 *                                                                          *
 * Copyright (c) 2000 - 2002 Winbond Electronics Corp. All rights reserved. *
 *                                                                          *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     wbtypes.h
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     This file contains PreDefined data types for W90N740 software development
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *     03/11/02		 Ver 1.0 Created by PC30 YCHuang
 *
 * REMARK
 *     None
 **************************************************************************/

#ifndef _WBTYPES_H
#define _WBTYPES_H

/* Winbond W90N740 coding standard draft 2.0 */
/* wbtypes.h Release 1.0 */

#define CONST             const

#define FALSE             0
#define TRUE              1

typedef void              VOID;
typedef void *            PVOID;

#if 0	//xhchen: Don't define BOOL to char!
typedef char              BOOL;
typedef char *            PBOOL;
#else
typedef int              BOOL;
typedef int *            PBOOL;
#endif

typedef char              INT8;
typedef char              CHAR;
typedef char *            PINT8;
typedef char *            PCHAR;
typedef unsigned char     UINT8;
typedef unsigned char     UCHAR;
typedef unsigned char *   PUINT8;
typedef unsigned char *   PUCHAR;
typedef char *            PSTR;
typedef const char *      PCSTR;

typedef short             SHORT;
typedef short *           PSHORT;
typedef unsigned short    USHORT;
typedef unsigned short *  PUSHORT;

typedef short             INT16;
typedef short *           PINT16;
typedef unsigned short    UINT16;
typedef unsigned short *  PUINT16;

typedef int               INT;
typedef int *             PINT;
typedef unsigned int      UINT;
typedef unsigned int *    PUINT;

typedef int               INT32;
typedef int *             PINT32;
typedef unsigned int      UINT32;
typedef unsigned int *    PUINT32;

typedef __int64           INT64;
typedef unsigned __int64  UINT64;

typedef float             FLOAT;
typedef float *           PFLOAT;

typedef double            DOUBLE;
typedef double *          PDOUBLE;

typedef int               SIZE_T;

typedef unsigned char     REG8;
typedef unsigned short    REG16;
typedef unsigned int      REG32;

#endif /* _WBTYPES_H */

