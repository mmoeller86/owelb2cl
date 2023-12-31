/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2023 The Open Watcom Contributors. All Rights Reserved.
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


extern  void    BuffStart( temp_buff *temp, uint def );
extern  void    BuffEnd( segment_id segid );
extern  uint    BuffLoc( void );
extern  void    BuffPatch( byte val, uint loc );
extern  void    BuffByte( byte b );
extern  void    BuffWord( uint w );
extern  void    BuffDWord( uint_32 w );
extern  void    BuffOffset( offset w );
extern  void    BuffValue( uint_32 val, uint class );
extern  void    BuffRelocatable( pointer ptr, fixup_kind type, offset off );
extern  void    BuffBack( pointer back, int off );
extern  void    BuffAddr( pointer sym );
extern  void    BuffForward( dbg_patch *dpatch );
extern  void    BuffWSLString( const char *str );
extern  void    BuffString( uint len, const char *str );
extern  void    BuffIndex( uint tipe );
