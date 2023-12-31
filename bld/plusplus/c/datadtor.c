/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "plusplus.h"
#include "cgfront.h"
#include "datadtor.h"
#include "fnbody.h"
#include "cdopt.h"
#ifdef DEVBUILD
    #include "dbg.h"
    #include "togglesd.h"
    #include "pragdefn.h"
#endif


static PTREE addPtIc(           // DECORATE TREE WITH PT_IC NODE
    PTREE tree,                 // - tree
    PTREE node )                // - IC node
{
    PTREE *a_expr;              // - expression for side-effect

    if( tree == NULL ) {
        CgFrontCodePtr( node->u.ic.opcode, node->u.ic.value.pvalue );
        CgFrontCode( IC_EXPR_TRASH );
        PTreeFree( node );
        tree = NULL;
    } else {
        PTREE dup = tree;
        if( NodeIsUnaryOp( tree, CO_EXPR_DONE ) ) {
            a_expr = &dup->u.subtree[0];
        } else {
            a_expr = &dup;
        }
        *a_expr = NodeAddSideEffect( node, *a_expr );
        tree = dup;
    }
    return( tree );
}


static PTREE addPtIcUnsigned(   // DECORATE TREE WITH PT_IC NODE (unsigned)
    PTREE tree,                 // - tree
    CGINTEROP opcode,           // - opcode
    unsigned value )            // - value
{
    return( addPtIc( tree, NodeIcUnsigned( opcode, value ) ) );
}


PTREE DataDtorObjPush(          // START OF DTORABLE OBJECT
    PTREE expr,                 // - expression to be decorated
    TYPE type,                  // - type of object
    SYMBOL init_sym,            // - symbol being initialized
    target_offset_t offset )    // - offset of object being initialized
{
    TYPE dtor_type;             // - type for dtor

#ifdef DEVBUILD
    if( TOGGLEDBG( dump_data_dtor ) ) {
        VBUF vbuf;
        printf( "DataDtorObjPush -- symbol %s\n"
                "                -- offset %x\n"
                "                -- "
              , DbgSymNameFull( init_sym, &vbuf )
              , offset );
        DumpFullType( type );
        VbufFree( &vbuf );
    }
#endif
    dtor_type = type;
    if( NULL != ArrayType( dtor_type ) ) {
        dtor_type = ArrayBaseType( dtor_type );
    }
    CDoptDtorBuild( dtor_type );
    FunctionHasRegistration();
    return( PtdObjPush( expr, type, init_sym, offset ) );
}


PTREE DataDtorObjPop(           // COMPLETE DTORABLE OBJECT
    PTREE expr )                // - expression to be decorated
{
#ifdef DEVBUILD
    if( TOGGLEDBG( dump_data_dtor ) ) {
        printf( "DataDtorObjPop\n" );
    }
#endif
    return( PtdObjPop( expr ) );
}


PTREE DataDtorCompClass(        // MARK CLASS OBJECT AS DTORABLE COMPONENT
    PTREE expr,                 // - expression to be decorated
    target_offset_t offset,     // - offset of component
    DTC_KIND kind )             // - kind of component
{
#ifdef DEVBUILD
    if( TOGGLEDBG( dump_data_dtor ) ) {
        printf( "DataDtorCompClass -- offset %x\n", offset );
    }
#endif
    FunctionHasCtorTest();
    return( PtdCompCtored( expr, offset, kind ) );
}


PTREE DataDtorCompArrEl(        // MARK ARRAY ELEMENT AS DTORABLE COMPONENT
    PTREE expr,                 // - expression to be decorated
    target_offset_t index )     // - array index
{
#ifdef DEVBUILD
    if( TOGGLEDBG( dump_data_dtor ) ) {
        printf( "DataDtorCompArrEl -- index(%x)\n", index );
    }
#endif
    return( addPtIcUnsigned( expr, IC_DTARRAY_INDEX, index ) );
}
