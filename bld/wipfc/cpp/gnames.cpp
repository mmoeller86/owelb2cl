/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2009-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Global Names data
* Obtained from the "id" or "name" attribute of an :hn tag iff the "global"
* attribute flag is set
* STD1::uint16_t dictIndex[ IpfHeader.panelCount ]; //in ascending order
* STD1::uint16_t TOCIndex[ IpfHeader.panelCount ];
*
****************************************************************************/


#include "wipfc.hpp"
#include <cstdio>
#include "gnames.hpp"
#include "errors.hpp"

void GNames::insert( GlobalDictionaryWord* wordent, word toc )
{
    NameIter itr( _names.find( wordent ) );   //look up word in names
    if( itr != _names.end() )
        throw Class3Error( ERR3_DUPID );
    _names.insert( std::map< GlobalDictionaryWord*, word, ptrLess< GlobalDictionaryWord* > >::value_type( wordent, toc ) );
}
/***************************************************************************/
STD1::uint32_t GNames::write( std::FILE *out ) const
{
    dword start( 0 );
    if( _names.size() ) {
        start = std::ftell( out );
        for( ConstNameIter itr = _names.begin(); itr != _names.end(); ++itr ) {
            word index = (itr->first)->index();
            if( std::fwrite( &index, sizeof( word ), 1, out ) != 1 ) {
                throw FatalError( ERR_WRITE );
            }
        }
        for( ConstNameIter itr = _names.begin(); itr != _names.end(); ++itr ) {
            word toc = itr->second;
            if( std::fwrite( &toc, sizeof( word ), 1, out ) != 1 ) {
                throw FatalError( ERR_WRITE );
            }
        }
    }
    return start;
}

