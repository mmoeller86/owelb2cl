/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2022 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Linux trap file startup code.
*
****************************************************************************/


#include <stdlib.h>
#include "trpld.h"
#include "trpcomm.h"
#include "lnxstrt.h"


char                        **dbg_environ;  /* pointer to parent's environment table */
static const trap_callbacks *Client;
static const trap_requests  ImpInterface = { TrapInit, TrapRequest, TrapFini } ;

const trap_requests *TrapLoad( const trap_callbacks *client )
{
    Client = client;
    if( Client->len <= offsetof(trap_callbacks,signal) )
        return( NULL );
    dbg_environ = *Client->environp;
    return( &ImpInterface );
}

#if !defined( BUILTIN_TRAP_FILE )

void *malloc( size_t size )
{
    return( Client->malloc( size ) );
}

void *realloc( void *ptr, size_t size )
{
    return( Client->realloc( ptr, size ) );
}

void free( void *ptr )
{
    Client->free( ptr );
}

char *getenv( const char *name )
{
    return( Client->getenv( name ) );
}

#if 0 /* redefining signal is not yet necessary */
void    (*signal( int __sig, void (*__func)(int) ))(int)
{
    return( Client->signal( __sig, __func ) );
}
#endif

#endif
