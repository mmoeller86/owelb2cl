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
* Description:  Utility to create setup.inf files for Watcom installer.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined( __WATCOMC__ ) || !defined( __UNIX__ )
#include <process.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bool.h"
#include "disksize.h"
#include "iopath.h"

#include "clibext.h"


#define RoundUp( size, limit )  ( ( ( size + limit - 1 ) / limit ) * limit )

#define IS_EMPTY(p)         ((p)[0] == '\0' || (p)[0] == '.' && (p)[1] == '\0')

#define IS_WS(c)            ((c) == ' ' || (c) == '\t')
#define SKIP_WS(p)          while(IS_WS(*(p))) (p)++

typedef struct path_info {
    struct path_info    *next;
    char                *path;
    int                 target;
    int                 parent;
} PATH_INFO;

typedef struct size_list {
    struct size_list    *next;
    long                size;
    time_t              stamp;
    char                redist;
    char                remove;
    char                *dst_var;
    char                name[1];
} size_list;

typedef struct file_info {
    struct file_info    *next;
    char                *file;
    char                *pack;
    char                *condition;
    int                 path;
    int                 old_path;
    int                 num_files;
    size_list           *sizes;
    long                cmp_size;
} FILE_INFO;


typedef struct list {
    struct list         *next;
    char                *item;
    int                 type;
} LIST;

enum {
    DELETE_DIALOG,
    DELETE_FILE,
    DELETE_DIR
};

static char                    *Product;
static long                    DiskSize;
static int                     BlockSize;
static char                    *RelRoot;
static char                    *PackDir;
static char                    *Setup;
static FILE_INFO               *FileList = NULL;
static PATH_INFO               *PathList = NULL;
static LIST                    *AppSection = NULL;
static LIST                    *IconList = NULL;
static LIST                    *SupplementList = NULL;
static LIST                    *IniList = NULL;
static LIST                    *AutoList = NULL;
static LIST                    *CfgList = NULL;
static LIST                    *EnvList = NULL;
static LIST                    *DialogList = NULL;
static LIST                    *BootTextList = NULL;
static LIST                    *ExeList = NULL;
static LIST                    *TargetList = NULL;
static LIST                    *LabelList = NULL;
static LIST                    *UpgradeList = NULL;
static LIST                    *AutoSetList = NULL;
static LIST                    *AfterList = NULL;
static LIST                    *BeforeList = NULL;
static LIST                    *EndList = NULL;
static LIST                    *DeleteList = NULL;
static LIST                    *ForceDLLInstallList = NULL;
static LIST                    *ErrMsgList = NULL;
static LIST                    *SetupErrMsgList = NULL;
static unsigned                DiskNum;
static unsigned                MaxDiskFiles;
static int                     FillFirst = 1;
static int                     Lang = 1;
static bool                    Upgrade = false;
static LIST                    *Include = NULL;
//static const char              MkdiskInf[] = "mkdisk.inf";
static const char               MkdiskInf[] = "mksetup.inf";


#define NUM_LINES       23
#define PRODUCT_LINE    3


static const char encode36_table[] = "0123456789abcdefghijklmnopqrstuvwxyz";

static char                     *BootText[NUM_LINES] =
{
"",
"",
"    Welcome to",
"        insert_product_name_here",
"",
"",
"",
"    from",
"        WATCOM International Corp.",
"        415 Phillip Street",
"        Waterloo, Ontario",
"        Canada, N2L 3X2",
"",
"        (519) 886-3700",
"        FAX: (519) 747-4971",
"",
"",
"",
"",
"    Insert a DOS diskette",
"    press any key to start DOS...",
"",
""
};


static void ConcatDirElem( char *dir, const char *elem )
/******************************************************/
{
    size_t      len;

    len = strlen( dir );
    if( len > 0 ) {
        char c = dir[len - 1];
        if( !IS_PATH_SEP( c ) ) {
            dir[len++] = DIR_SEP;
        }
    }
    strcpy( dir + len, elem );
}


static char *mygets( char *buf, int max_len, FILE *fp )
/*****************************************************/
{
    char        *p,*q,*start;
    int         lang;
    size_t      got;

    /* syntax is //nstring//mstring//0 */

    p = buf;
    for( ;; ) {
        if( fgets( p, max_len, fp ) == NULL )
            return( NULL );
        q = p;
        SKIP_WS( q );
        got = strlen( q );
        if( p != q )
            memmove( p, q, got + 1 );
        /* check '\n' char */
        if( got == 0 || p[got - 1] != '\n' )
            break;
        /* skip '\n' */
        got--;
        /* check continuation char '\\' */
        if( got == 0 || p[got - 1] != '\\' ) {
            /* terminate buffer */
            p[got] = '\0';
            break;
        }
        /* skip '\\' */
        got--;
        /* continuation, append next line to the buffer */
        p += got;
        len -= got;
    }
    p = buf;
    while( *p ) {
        if( p[0] == '/' && p[1] == '/' && isdigit( p[2] ) ) {
            start = p;
            lang = p[2] - '0';
            if( lang == 0 ) {
                strcpy( start, start + 3 );
                continue;
            }
            p += 3;
            while( p[0] != '/' || p[1] != '/' ) {
                ++p;
            }
            if( lang == Lang ) {
                strcpy( start, start+3 );
                p -= 3;
            } else {
                strcpy( start, p );
                p = start;
            }
        } else {
            ++p;
        }
    }
    return( buf );
}


static void AddToList( LIST *new, LIST **list )
//=============================================

{
    while( *list != NULL ) {
        list = &((*list)->next);
    }
    *list = new;
}


long FileSize( const char *file )
//===============================

{
    struct stat         stat_buf;

    if( stat( file, &stat_buf ) != 0 ) {
        printf( "Can't find '%s'\n", file );
        return( 0 );
    } else {
        return( RoundUp( stat_buf.st_size, BlockSize ) );
    }
}


void CreateBootFile()
//===================

{
    int                 line;
    LIST                *list;
    FILE                *fp;

    line = PRODUCT_LINE;
    for( list = BootTextList; list != NULL; list = list->next ) {
        BootText[line] = list->item;
        ++line;
    }
    fp = fopen( "__boot__", "w" );
    if( fp == NULL ) {
        printf( "Cannot create boot text file '__boot__'\n" );
    } else {
        for( line = 0; line < NUM_LINES; ++line ) {
            fprintf( fp, "%s\n", BootText[line] );
        }
        fclose( fp );
    }
}


void NewDisk( FILE *fp )
//======================

{
    fprintf( fp, "\ncall %%DSDISK%% %2.2u %%1 %%2 %%3 %%4 %%5 %%6\n", ++DiskNum );
}


void EndDisk( FILE *fp )
//======================

{
    fprintf( fp, "call %%DEDISK%% %2.2u %%1 %%2 %%3 %%4 %%5 %%6\n", DiskNum );
}


void NextDisk( FILE *fp )
//=======================

{
    EndDisk( fp );
    NewDisk( fp );
}

void OneFile( FILE *fp, const char *dir, const char *name, const char *dst )
{
    fprintf( fp, "call %%DCOPY%% " );
    if( dir != NULL )
        fprintf( fp, "%s\\", dir );
    fprintf( fp, "%s", name );
    if( dst != NULL ) {
        fprintf( fp, "\t%s", dst );
    }
    fprintf( fp, "\n" );
}

void CreateBatch( FILE *fp )
//=========================================

{
    int                 i;
    long                curr_size, this_disk, extra;
    FILE_INFO           *curr;
    LIST                *list;
    char                buff[80];
    unsigned            nfiles;

    DiskNum = 0;
    fprintf( fp, "call initmk %%1 %%2 %%3 %%4 %%5 %%6\n" );
    NewDisk( fp );

    OneFile( fp, NULL, Setup, "setup.exe" );
    OneFile( fp, NULL, "setup.inf", NULL );
    for( list = ExeList; list != NULL; list = list->next ) {
        OneFile( fp, NULL, list->item, NULL );
    }
    this_disk = FileSize( Setup ) + FileSize( "setup.inf" );
    for( list = ExeList; list != NULL; list = list->next ) {
        this_disk += FileSize( list->item );
    }

    if( !FillFirst ) this_disk = DiskSize;
    nfiles = 0;
    for( curr = FileList; curr != NULL; curr = curr->next ) {
        if( ++nfiles > MaxDiskFiles ) {
            nfiles = 0;
            NextDisk( fp );
            this_disk = 0;
        }
        curr_size = RoundUp( curr->cmp_size, BlockSize );
        if( this_disk == DiskSize ) {
            NextDisk( fp );
            this_disk = 0;
        }
        if( this_disk + curr_size > DiskSize ) {
            // place what will fit onto the current disk
            extra = DiskSize - this_disk;
            fprintf( fp, "splitfil %s\\%s %ld %ld\n", PackDir, curr->pack,
                         extra, DiskSize );
            sprintf( buff, "%s.1", curr->pack );
            OneFile( fp, PackDir, buff, NULL );
            fprintf( fp, "del %s\\%s.1\n", PackDir, curr->pack );
            // place rest of current file on subsequent disks
            curr_size -= extra;
            for( i = 2; ; ++i ) {
                NextDisk( fp );
                sprintf( buff, "%s.%d", curr->pack, i );
                OneFile( fp, PackDir, buff, NULL );
                fprintf( fp, "del %s\\%s.%d\n", PackDir, curr->pack, i );
                if( curr_size <= DiskSize ) break;
                curr_size -= DiskSize;
            }
            this_disk = curr_size;
            nfiles = 1;
        } else {
            OneFile( fp, PackDir, curr->pack, NULL );
            this_disk += curr_size;
        }
    }
    EndDisk( fp );
    fprintf( fp, "\ncall finimk %%1 %%2 %%3 %%4 %%5 %%6\n" );
}


void CreateDisks()
//===========================

{
    FILE                *fp;

    if( BootTextList != NULL ) {
        CreateBootFile();
    }
    // create DODISK.BAT
    fp = fopen( "dodisk.bat", "w" );
    if( fp == NULL ) {
        printf( "Cannot create file 'dodisk.bat'\n" );
        fp = stdout;
    }
    CreateBatch( fp );
    if( fp != stdout ) {
        fclose( fp );
    }
}


bool CheckParms( int *pargc, char **pargv[] )
//===========================================

{
    char                *size;
    struct stat         stat_buf;
    char                **argv;
    int                 argc;
    LIST                *new;

    FillFirst = 1;
    if( *pargc > 1 ) {
        while( ((*pargv)[1] != NULL) && ((*pargv)[1][0] == '-') ) {
            if( (*pargv)[1][1] == '0' ) {
                FillFirst = 0;
            } else if( tolower( (*pargv)[1][1] ) == 'l' ) {
                Lang = (*pargv)[1][2] - '0';
            } else if( tolower( (*pargv)[1][1] ) == 'd' ) {
                new = malloc( sizeof( LIST ) );
                if( new == NULL ) {
                    printf( "\nOut of memory\n" );
                    exit( 1 );
                }
                new->next = NULL;
                new->item = strdup( &(*pargv)[1][2] );
                AddToList( new, &AppSection );
            } else if( tolower( (*pargv)[1][1] ) == 'i' ) {
                new = malloc( sizeof( LIST ) );
                if( new == NULL ) {
                    printf( "\nOut of memory\n" );
                    exit( 1 );
                }
                new->next = NULL;
                new->item = strdup( &(*pargv)[1][2] );
                AddToList( new, &Include );
            } else if( tolower( (*pargv)[1][1] ) == 'u' ) {
                Upgrade = true;
            } else {
                printf( "Unrecognized option %s\n", (*pargv)[1] );
            }
            ++*pargv;
            --*pargc;
        }
    }
    argc = *pargc;
    argv = *pargv;
    if( argc != 6 ) {
        printf( "Usage: mkdisk [-x] <product> <size> <file_list> <pack_dir> <rel_root>\n" );
        return( false );
    }
    Product = argv[1];
    size = argv[2];
    if( strcmp( size, "360" ) == 0 ) {
        DiskSize = DISK_360;
        MaxDiskFiles = DISK_360_FN;
        BlockSize = 1024;
    } else if( strcmp( size, "720" ) == 0 ) {
        DiskSize = DISK_720;
        MaxDiskFiles = DISK_720_FN;
        BlockSize = 1024;
    } else if( strcmp( size, "1.2" ) == 0 ) {
        DiskSize = DISK_1p2;
        MaxDiskFiles = DISK_1p2_FN;
        BlockSize = 1024;
    } else if ( strcmp( size, "1.4" ) == 0 ) {
        DiskSize = DISK_1p4;
        MaxDiskFiles = DISK_1p4_FN;
        BlockSize = 512;
    } else {
        printf( "SIZE must be one of 360, 720, 1.2, 1.4\n" );
        return( false );
    }
    PackDir  = argv[4];
    if( stat( PackDir, &stat_buf ) != 0 ) {  // exists
        printf( "\nDirectory '%s' does not exist\n", PackDir );
        return( false );
    }
    RelRoot  = argv[5];
    if( stat( RelRoot, &stat_buf ) != 0 ) {  // exists
        printf( "\nDirectory '%s' does not exist\n", RelRoot );
        return( false );
    }
    return( true );
}


int AddTarget( const char *target )
//=================================
{
    int                 count;
    LIST                *new, *curr, **owner;

    count = 1;
    for( owner = &TargetList; (curr = *owner) != NULL; owner = &(curr->next) ) {
        if( stricmp( target, curr->item ) == 0 ) {
            return( count );
        }
        ++count;
    }

    new = malloc( sizeof( LIST ) );
    if( new == NULL ) {
        printf( "Out of memory\n" );
        return( 0 );
    }
    new->item = strdup( target );
    if( new->item == NULL ) {
        printf( "Out of memory\n" );
        return( 0 );
    }
    new->next = NULL;
    *owner = new;
    return( count );
}


int AddPath( const char *path, int target, int parent )
//=====================================================
{
    int                 count;
    PATH_INFO           *new, *curr, **owner;

    count = 1;
    for( owner = &PathList; (curr = *owner) != NULL; owner = &(curr->next) ) {
        if( stricmp( path, curr->path ) == 0 && target == curr->target ) {
            return( count );
        }
        ++count;
    }

    new = malloc( sizeof( PATH_INFO ) );
    if( new == NULL ) {
        printf( "Out of memory\n" );
        return( 0 );
    }
    new->path = strdup( path );
    if( new->path == NULL ) {
        printf( "Out of memory\n" );
        return( 0 );
    }
    new->target = target;
    new->parent = parent;
    new->next = NULL;
    *owner = new;
    return( count );
}


int AddPathTree( char *path, int target )
/***************************************/
{
    int         parent;
    char        *p;

    if( path == NULL )
        return( -1 );
    parent = AddPath( ".", target, -1 );
    p = strchr( path, '\\' );
    while( p != NULL ) {
        *p = '\0';
        parent = AddPath( path, target, parent );
        if( parent == 0 )
            return( 0 );
        *p = '\\';
        p = strchr( p + 1, '\\' );
    }
    return( AddPath( path, target, parent ) );
}


bool AddFile( char *path, char *old_path, char redist, const char *file, const char *rel_file, const char *patch, const char *dst_var, const char *cond )
/*************************************************************************************************************************/
{
    int                 path_dir, old_path_dir, target;
    FILE_INFO           *new, *curr, **owner;
    long                act_size, cmp_size;
    time_t              time;
    struct stat         stat_buf;
    char                *p;
    char                *root_file;
    char                src[_MAX_PATH], dst[_MAX_PATH];
    size_list           *ns,*sl;

    if( !IS_EMPTY( rel_file ) ) {
        if( HAS_PATH( rel_file ) ) {
            strcpy( src, rel_file );
        } else {
            strcpy( src, RelRoot );
            ConcatDirElem( src, rel_file );
        }
    } else if( HAS_PATH( path ) ) {
        // path is absolute. don't use RelRoot
        strcpy( src, path );
        ConcatDirElem( src, file );
    } else {
        strcpy( src, RelRoot );
        if( !IS_EMPTY( path ) ) {
            ConcatDirElem( src, path );
        }
        ConcatDirElem( src, file );
    }
    if( stat( src, &stat_buf ) != 0 ) {
        printf( "\n'%s' does not exist\n", src );
        return( false );
    } else {
        act_size = stat_buf.st_size;
        time = stat_buf.st_mtime;
    }
    strcpy( dst, PackDir );
    ConcatDirElem( dst, patch );
    if( stat( dst, &stat_buf ) != 0 ) {
        printf( "\n'%s' does not exist\n", dst );
        return( false );
    } else {
        cmp_size = stat_buf.st_size;
    }
#if 0
    printf( "\r%s                              \r", file );
    fflush( stdout );
#endif
    act_size = RoundUp( act_size, 512 );
    cmp_size = RoundUp( cmp_size, BlockSize );

    // strip target off front of path
    if( *path == '%' ) {
        p = strchr( ++path, '%' );
        if( p == NULL ) {
            printf( "Invalid path(%s)\n", path );
            return( false );
        }
        *p = '\0';
        target = AddTarget( path );
        path = p + 1;
        if( *path == '\0' ) {
            path = ".";
        } else if( *path == '\\' ) {
            ++path;
        } else {
            printf( "Invalid path (%s)\n", path );
            return( false );
        }
    } else {
        target = AddTarget( "DstDir" );
    }
    if( target == 0 ) {
        return( false );
    }

    // handle sub-directories in path before full path
    path_dir = AddPathTree( path, target );
    old_path_dir = AddPathTree( old_path, target );
    if( path_dir == 0 ) {
        return( false );
    }
    root_file = p = file;
    while( *p != '\0' ) {
        switch( *p ) {
        case '/':
        case ':':
        case '\\':
            root_file = p + 1;
            break;
        }
        ++p;
    }

    // see if the pack_file has been seen already
    for( owner = &FileList; (curr = *owner) != NULL; owner = &(curr->next) ) {
        if( stricmp( curr->pack, patch ) != 0 )
            continue;
        // this file is already in the current pack file
        curr->num_files++;
        ns = malloc( sizeof( size_list ) + strlen( root_file ) );
        if( ns == NULL ) {
            printf( "Out of memory\n" );
            return( false );
        }
        strcpy( ns->name, root_file );
        for( sl = curr->sizes; sl != NULL; sl = sl->next ) {
            if( stricmp( sl->name, ns->name ) == 0 ) {
                printf( "file '%s' included in archive '%s' more than once\n", sl->name, curr->pack );
                return( false );
            }
        }
        ns->size = act_size;
        ns->stamp = time;
        ns->redist = redist;
        ns->dst_var = dst_var;
        ns->next = curr->sizes;
        curr->sizes = ns;
        if( curr->path != path_dir ) {
            printf( "\nPath for archive '%s' changed to '%s'\n", curr->pack, path );
            return( false );
        }
        if( strcmp( curr->condition, cond ) != 0 ) {
            printf( "\nCondition for archive '%s' changed:\n", curr->pack );
            printf( "Old: <%s>\n", curr->condition );
            printf( "New: <%s>\n", cond );
            return( false );
        }
        return( true );
    }

    // add to list
    new = malloc( sizeof( FILE_INFO ) );
    if( new == NULL ) {
        printf( "Out of memory\n" );
        return( false );
    }
    new->file = strdup( file );
    new->pack = strdup( patch );
    new->condition = strdup( cond );
    if( new->file == NULL || new->pack == NULL || new->condition == NULL ) {
        printf( "Out of memory\n" );
        return( false );
    }
    new->path = path_dir;
    new->old_path = old_path_dir;
    new->num_files = 1;
    ns = malloc( sizeof( size_list ) + strlen( root_file ) );
    if( ns == NULL ) {
        printf( "Out of memory\n" );
        return( false );
    }
    strcpy( ns->name, root_file );
    ns->size = act_size;
    ns->stamp = time;
    ns->next = NULL;
    ns->redist = redist;
    ns->dst_var = dst_var;
    new->sizes = ns;
    new->cmp_size = cmp_size;
    new->next = NULL;
    *owner = new;
    return( true );
}


bool ReadList( FILE *fp )
//=======================

{
    char        *path;
    char        *old_path;
    char        *file;
    char        *rel_fil;
    char        *condition;
    char        *patch;
    char        *dst_var;
    char        buf[512];
    char        redist;
    bool        no_error;

    printf( "Checking files...\n" );
    no_error = true;
    while( fgets( buf, sizeof( buf ), fp ) != NULL ) {
        buf[strlen( buf ) - 1] = '\0';
        redist = buf[0];
        path = strtok( buf + 1, " \t" );
        if( path == NULL ) {
            printf( "Invalid list file format - 'path' not found\n" );
            exit( 2 );
        }
        old_path = strtok( NULL, " \t" );
        if( old_path == NULL ) {
            printf( "Invalid list file format - 'old path' not found\n" );
            exit( 2 );
        }
        if( stricmp( path, old_path ) == 0 ) old_path = NULL;
        file = strtok( NULL, " \t" );
        if( file == NULL ) {
            printf( "Invalid list file format - 'file' not found\n" );
            exit( 2 );
        }
        rel_fil = strtok( NULL, " \t" );
        if( rel_fil == NULL ) {
            printf( "Invalid list file format - 'rel file' not found\n" );
            exit( 2 );
        }
        patch = strtok( NULL, " \t" );
        if( patch == NULL ) {
            printf( "Invalid list file format - 'patch' not found\n" );
            exit( 2 );
        }
        dst_var = strtok( NULL, " \t" );
        if( dst_var == NULL ) {
            printf( "Invalid list file format - 'destination' not found\n" );
            exit( 2 );
        }
        condition = strtok( NULL, "\0" ); // rest of line
        if( condition == NULL ) { // no packfile
            condition = dst_var;
            dst_var = ".";
        }
        while( *condition == ' ' ) ++condition;
        while( *dst_var == ' ' ) ++dst_var;
        if( IS_EMPTY( dst_var ) ) {
            dst_var = NULL;
        } else {
            dst_var = strdup( dst_var );
        }
        if( !AddFile( path, old_path, redist, file, rel_fil, patch, dst_var, condition ) ) {
            no_error = false;
        }
    }
    printf( ".\n" );
    return( no_error );
}


#define STRING_include          "include="
#define STRING_icon             "icon="
#define STRING_supplemental     "supplemental="
#define STRING_ini              "ini="
#define STRING_auto             "auto="
#define STRING_cfg              "cfg="
#define STRING_autoset          "autoset="
#define STRING_spawnafter       "spawnafter="
#define STRING_spawnbefore      "spawnbefore="
#define STRING_spawnend         "spawnend="
#define STRING_env              "env="
#define STRING_dialog           "dialog="
#define STRING_boottext         "boottext="
#define STRING_exe              "exe="
#define STRING_label            "label="
#define STRING_deletedialog     "deletedialog="
#define STRING_deletefile       "deletefile="
#define STRING_deletedir        "deletedir="
#define STRING_language         "language="
#define STRING_upgrade          "upgrade="
#define STRING_forcedll         "forcedll="
#define STRING_errmsg           "errmsg="
#define STRING_setuperrmsg      "setuperrmsg="

#define STRING_IS( buf, new, string ) \
        ( strnicmp( buf, string, sizeof( string ) - 1 ) == 0 && \
          ( new->item = strdup( buf + sizeof( string ) - 1 ) ) != NULL )


static FILE *PathOpen( const char *name )
/***************************************/
{
    FILE        *fp;
    LIST        *p;
    char        buf[_MAX_PATH];

    fp = fopen( name, "r" );
    for( p = Include; p != NULL && fp == NULL; p = p->next ) {
        _makepath( buf, NULL, p->item, name, NULL );
        fp = fopen( buf, "r" );
    }
    if( fp == NULL ) {
        printf( "\nCannot open include file '%s'\n", name );
        exit( 1 );
    }
    return( fp );
}

static char *ReplaceEnv( char *file_name )
/****************************************/
// if file_name is of the form $env_var$\name, replace with
// value of the environment variable
{
    char                *p, *q, *e, *var;
    char                buff[_MAX_PATH];

    // copy and make changes into 'buff'
    q = buff;
    p = file_name;
    for( ;; ) {
        if( *p == '$' ) {
            e = strchr( p + 1, '$' );
            if( e == NULL ) {
                strcpy( q, p );
                break;
            } else {
                *e = '\0';
                var = getenv( p + 1 );
                if( var == NULL ) {
                    printf( "Error: environment variable '%s' not found\n", p + 1 );
                } else {
                    strcpy( q, var );
                    q += strlen( var );
                }
                p = e + 1;
            }
        } else {
            *q = *p;
            if( *p == '\0' ) break;
            ++p;
            ++q;
        }
    }
    if( strcmp( buff, file_name ) == 0 ) {
        // no environment variables found
        return( file_name );
    } else {
        return( strdup( buff ) );
    }
}

static bool processLine( const char *line, LIST **list )
{
    LIST            *new;

    new = malloc( sizeof( LIST ) );
    if( new == NULL ) {
        return( true );
    }
    new->next = NULL;
    if( STRING_IS( line, new, STRING_icon ) ) {
        AddToList( new, &IconList );
    } else if( STRING_IS( line, new, STRING_supplemental ) ) {
        AddToList( new, &SupplementList );
    } else if( STRING_IS( line, new, STRING_ini ) ) {
        AddToList( new, &IniList );
    } else if( STRING_IS( line, new, STRING_auto ) ) {
        AddToList( new, &AutoList );
    } else if( STRING_IS( line, new, STRING_cfg ) ) {
        AddToList( new, &CfgList );
    } else if( STRING_IS( line, new, STRING_autoset ) ) {
        AddToList( new, &AutoSetList );
    } else if( STRING_IS( line, new, STRING_spawnafter ) ) {
        AddToList( new, &AfterList );
    } else if( STRING_IS( line, new, STRING_spawnbefore ) ) {
        AddToList( new, &BeforeList );
    } else if( STRING_IS( line, new, STRING_spawnend ) ) {
        AddToList( new, &EndList );
    } else if( STRING_IS( line, new, STRING_env ) ) {
        AddToList( new, &EnvList );
    } else if( STRING_IS( line, new, STRING_dialog ) ) {
        AddToList( new, &DialogList );
    } else if( STRING_IS( line, new, STRING_boottext ) ) {
        AddToList( new, &BootTextList );
    } else if( STRING_IS( line, new, STRING_exe ) ) {
        new->item = ReplaceEnv( new->item );
        AddToList( new, &ExeList );
    } else if( STRING_IS( line, new, STRING_label ) ) {
        AddToList( new, &LabelList );
    } else if( STRING_IS( line, new, STRING_upgrade ) ) {
        AddToList( new, &UpgradeList );
    } else if( STRING_IS( line, new, STRING_deletedialog ) ) {
        new->type = DELETE_DIALOG;
        AddToList( new, &DeleteList );
    } else if( STRING_IS( line, new, STRING_deletefile ) ) {
        new->type = DELETE_FILE;
        AddToList( new, &DeleteList );
    } else if( STRING_IS( line, new, STRING_deletedir ) ) {
        new->type = DELETE_DIR;
        AddToList( new, &DeleteList );
    } else if( STRING_IS( line, new, STRING_language ) ) {
        Lang = new->item[0] - '0';
        free( new->item );
        free( new );
    } else if( STRING_IS( line, new, STRING_forcedll ) ) {
        AddToList( new, &ForceDLLInstallList );
    } else if( STRING_IS( line, new, STRING_assoc ) ) {
        AddToList( new, &AssociationList );
    } else if( STRING_IS( line, new, STRING_errmsg ) ) {
        AddToList( new, &ErrMsgList );
    } else if( STRING_IS( line, new, STRING_setuperrmsg ) ) {
        AddToList( new, &SetupErrMsgList );
    } else {
        new->item = strdup( line );
        AddToList( new, list );
    }
    return( false );
}

#define SECTION_BUF_SIZE 8192   // allow long text strings

static char             SectionBuf[SECTION_BUF_SIZE];

void ReadSection( FILE *fp, const char *section, LIST **list )
/************************************************************/
{
    int             file_curr = 0;
    FILE            *file_stack[20];
    bool            found;

    found = false;
    for( ;; ) {
        if( mygets( SectionBuf, sizeof( SectionBuf ), fp ) == NULL ) {
            if( file_curr-- > 0 ) {
                fclose( fp );
                fp = file_stack[file_curr];
                continue;
            }
            break;
        }
        if( SectionBuf[0]== '#' || SectionBuf[0]== '\0' )
            continue;
        if( SectionBuf[0] == '[' && SectionBuf[strlen( SectionBuf ) - 1] == ']' ) {
            if( stricmp( SectionBuf, section ) == 0 ) {
                found = true;
            } else if( found ) {
                break;
            }
        } else if( found ) {
            if( strnicmp( SectionBuf, STRING_setup, sizeof( STRING_setup ) - 1 ) == 0 ) {
                free( Setup );
                Setup = ReplaceEnv( strdup( SectionBuf + sizeof( STRING_setup ) - 1 ) );
            } else if( strnicmp( SectionBuf, STRING_include, sizeof( STRING_include ) - 1 ) == 0 ) {
                file_stack[file_curr++] = fp;
                fp = PathOpen( SectionBuf + sizeof( STRING_include ) - 1 );
            } else if( processLine( SectionBuf, list ) ) {
                while( file_curr-- > 0 ) {
                    fclose( fp );
                    fp = file_stack[file_curr];
                }
                fclose( fp );
                printf( "\nOut of memory\n" );
                exit( 1 );
            }
        }
    }
    if( !found ) {
        printf( "%s section not found in '%s'\n", section, MksetupInf );
    }
}


void ReadInfFile( void )
/**********************/
{
    FILE                *fp;
    char                ver_buf[80];

    fp = PathOpen( MkdiskInf );
    if( fp == NULL ) {
        printf( "Cannot open '%s'\n", MkdiskInf );
        return;
    }
    sprintf( ver_buf, "[%s]", Product );
    ReadSection( fp, ver_buf, &AppSection );
    fclose( fp );
}


static char *encode36( char *buffer, unsigned long value )
/********************************************************/
{
    char        *p = buffer;
    char        *q;
    unsigned    rem;
    char        buf[34];        // only holds ASCII so 'char' is OK

    buf[0] = '\0';
    q = &buf[1];
    do {
        rem = value % 36;
        value = value / 36;
        *q = encode36_table[rem];
        ++q;
    } while( value != 0 );
    while( (*p++ = (char)*--q) != '\0' )
        ;
    return( buffer );
}

static void fput36( FILE *fp, long value )
/****************************************/
{
    char        buff[30];

    if( value < 0 ) {
        fprintf( fp, "-" );
        value = -value;
    }
    fprintf( fp, "%s", encode36( buff, value ) );
}

void DumpSizes( FILE *fp, FILE_INFO *curr )
/*****************************************/
{
    size_list   *csize;

    fput36( fp, curr->num_files );
    fprintf( fp, "," );
    if( curr->num_files > 1 ) {
        fprintf( fp, "\\\n" );
    }
    for( csize = curr->sizes; csize != NULL; csize = csize->next ) {
        fprintf( fp, "%s!", csize->name );
        fput36( fp, csize->size/512 );
        fprintf( fp, "!" );
        if( csize->redist != ' ' ) {
            fput36( fp, (long)( csize->stamp ) );
        }
        fprintf( fp, "!" );
        if( csize->dst_var ) {
            fprintf( fp, "%s", csize->dst_var );
        }
        if( csize->redist != ' ' ) {
            // 's' for supplemental file (not deleted)
            fprintf( fp, "!%c", tolower( csize->redist ) );
        }
        if( csize->dst_var ) {
            fprintf( fp, "!%s", csize->dst_var );
        }
        fprintf( fp, "," );
        if( curr->num_files > 1 ) {
            fprintf( fp, "\\\n" );
        }
    }
}

bool CheckForDuplicateFiles( void )
/********************************/
{
    FILE_INFO           *file1,*file2;
    size_list           *name1,*name2;
    bool                ok = true;

    if( FileList != NULL ) {
        for( file1 = FileList; file1->next != NULL; file1 = file1->next ) {
            for( file2 = file1->next; file2 != NULL; file2 = file2->next ) {
                if( file1->path != file2->path || file1->condition != file2->condition )
                    continue;
                for( name1 = file1->sizes; name1 != NULL; name1 = name1->next ) {
                    for( name2 = file2->sizes; name2 != NULL; name2 = name2->next ) {
                        if( stricmp( name1->name, name2->name ) == 0 ) {
                            printf( "'%s' is in 2 pack files (%s) (%s)\n",
                                    name1->name,
                                    file1->pack,
                                    file2->pack );
                            ok = false;
                        }
                    }
                }
            }
        }
    }
    return( ok );
}


static void DumpFile( FILE *out, const char *fname )
/**************************************************/
{
    FILE                *in;
    char                *buf;

    in = PathOpen( fname );
    if( in == NULL ) {
        printf( "Cannot open '%s'\n", fname );
        return;
    }
    buf = malloc( SECTION_BUF_SIZE );
    if( buf == NULL ) {
        printf( "Out of memory\n" );
        return;
    }
    for( ;; ) {
        if( mygets( buf, SECTION_BUF_SIZE, in ) == NULL ) {
            break;
        }
        if( strnicmp( buf, STRING_include, sizeof( STRING_include ) - 1 ) == 0 ) {
            DumpFile( out, buf + sizeof( STRING_include ) - 1 );
        } else {
            fputs( buf, out );
        }
    }
    free( buf );
    fclose( in );
}


int CreateScript( long init_size, unsigned padding )
/**************************************************/
{
    int                 disk;
    long                curr_size, this_disk, extra;
    FILE                *fp;
    FILE_INFO           *curr;
    PATH_INFO           *path;
    LIST                *list;
    LIST                *list2;
    unsigned            nfiles;
    int                 i;

    fp = fopen( "setup.inf", "w" );
    if( fp == NULL ) {
        printf( "Cannot create file 'setup.inf'\n" );
        fp = stdout;
    }
    fprintf( fp, "[Application]\n" );
    for( list = AppSection; list != NULL; list = list->next ) {
        fprintf( fp, "%s\n", list->item );
    }
    fprintf( fp, "DisketteSize=%ld\n", DiskSize );
    if( Upgrade ) {
        fprintf( fp, "IsUpgrade=1\n" );
    }
    fprintf( fp, "\n[Targets]\n" );
    for( list = TargetList; list != NULL; list = list->next ) {
        fprintf( fp, "%s", list->item );
        for( list2 = SupplementList; list2 != NULL; list2 = list2->next ) {
            if( stricmp( list->item, list2->item ) == 0 ) {
                fprintf( fp, ",supplemental" );
            }
        }
        fprintf( fp, "\n" );
    }

    fprintf( fp, "\n[Dirs]\n" );
    for( path = PathList; path != NULL; path = path->next ) {
        fprintf( fp, "%s,%d,%d\n", path->path, path->target, path->parent );
    }

    disk = 1;
    this_disk = init_size;
    nfiles = 0;
    if( !FillFirst ) this_disk = DiskSize;
    fprintf( fp, "\n[Files]\n" );
    for( curr = FileList; curr != NULL; curr = curr->next ) {
        if( ++nfiles > MaxDiskFiles ) {
            nfiles = 0;
            ++disk;
            this_disk = 0;
        }
        curr_size = RoundUp( curr->cmp_size, BlockSize );
        if( this_disk == DiskSize ) {
            this_disk = 0;
            ++disk;
        }
        if( this_disk + curr_size > DiskSize ) {
            // place what will fit onto the current disk
            extra = DiskSize - this_disk;
            fprintf( fp, "%s.1,0,", curr->pack );
            fput36( fp, curr->path );
            fprintf( fp, "," );
            fput36( fp, curr->old_path );
            fprintf( fp, "," );
            fput36( fp, disk );
            fprintf( fp, ",1,%s\n\n", curr->condition );
            // place rest of current file on subsequent disks
            curr_size -= extra;
            for( i = 2; ; ++i ) {
                ++disk;
                if( curr_size <= DiskSize ) {
                    fprintf( fp, "%s.%d,", curr->pack, i );
                    DumpSizes( fp, curr );
                    fput36( fp, curr->path );
                    fprintf( fp, "," );
                    fput36( fp, curr->old_path );
                    fprintf( fp, "," );
                    fput36( fp, disk );
                    fprintf( fp, ",$,%s\n", curr->condition );
                    break;
                } else {
                    fprintf( fp, "%s.%d,0,", curr->pack, i );
                    fput36( fp, curr->path );
                    fprintf( fp, "," );
                    fput36( fp, curr->old_path );
                    fprintf( fp, "," );
                    fput36( fp, disk );
                    fprintf( fp, ",M,%s\n\n", curr->condition );
                }
                curr_size -= DiskSize;
            }
            this_disk = curr_size;
            nfiles = 1;
        } else {
            fprintf( fp, "%s,", curr->pack );
            DumpSizes( fp, curr );
            fput36( fp, curr->path );
            fprintf( fp, "," );
            fput36( fp, curr->old_path );
            fprintf( fp, "," );
            fput36( fp, disk );
            fprintf( fp, ",.,%s\n", curr->condition );
            this_disk += curr_size;
        }
    }

    if( DeleteList != NULL ) {
        fprintf( fp, "\n[DeleteFiles]\n" );
        for( list = DeleteList; list != NULL; list = list->next ) {
            switch( list->type ) {
            case DELETE_DIALOG:
                fprintf( fp, "dialog=" );
                break;
            case DELETE_FILE:
                fprintf( fp, "file=" );
                break;
            case DELETE_DIR:
                fprintf( fp, "directory=" );
                break;
            }
            fprintf( fp, "%s\n", list->item );
        }
    }

    fprintf( fp, "\n[Disks]\n" );          // do this after # of disks determined
    for( i = 1; i <= disk; ++i ) {
        fprintf( fp, "Disk %d\n", i );
    }

    if( IconList != NULL ) {
        fprintf( fp, "\n[PM Info]\n" );
        for( list = IconList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( IniList != NULL ) {
        fprintf( fp, "\n[Profile]\n" );
        for( list = IniList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( AutoList != NULL ) {
        fprintf( fp, "\n[Autoexec]\n" );
        for( list = AutoList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( CfgList != NULL ) {
        fprintf( fp, "\n[Config]\n" );
        for( list = CfgList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( AutoSetList != NULL ) {
        fprintf( fp, "\n[AutoSet]\n" );
        for( list = AutoSetList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( BeforeList != NULL || AfterList != NULL || EndList != NULL ) {
        fprintf( fp, "\n[Spawn]\n" );
        for( list = BeforeList; list != NULL; list = list->next ) {
            fprintf( fp, "before=%s\n", list->item );
        }
        for( list = AfterList; list != NULL; list = list->next ) {
            fprintf( fp, "after=%s\n", list->item );
        }
        for( list = EndList; list != NULL; list = list->next ) {
            fprintf( fp, "end=%s\n", list->item );
        }
    }

    if( EnvList != NULL ) {
        fprintf( fp, "\n[Environment]\n" );
        for( list = EnvList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( DialogList != NULL ) {
        for( list = DialogList; list != NULL; list = list->next ) {
            fprintf( fp, "\n[Dialog]\n" );
            DumpFile( fp, list->item );
        }
    }

    if( LabelList != NULL ) {
        fprintf( fp, "\n[Labels]\n" );
        for( list = LabelList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( UpgradeList != NULL ) {
        fprintf( fp, "\n[Upgrade]\n" );
        for( list = UpgradeList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( ForceDLLInstallList != NULL ) {
        fprintf( fp, "\n[ForceDLLInstall]\n" );
        for( list = ForceDLLInstallList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( ErrMsgList != NULL ) {
        fprintf( fp, "\n[ErrorMessage]\n" );
        for( list = ErrMsgList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    if( SetupErrMsgList != NULL ) {
        fprintf( fp, "\n[SetupErrorMessage]\n" );
        for( list = SetupErrMsgList; list != NULL; list = list->next ) {
            fprintf( fp, "%s\n", list->item );
        }
    }

    fprintf( fp, "\n[End]\n" );

    while( padding != 0 ) {
        /* add some padding to bring the size up to old size */
        fputc( ' ', fp );
        --padding;
    }

    fclose( fp );
    return( disk );
}


int MakeScript( void )
/********************/
{
    int                 disks;
    FILE_INFO           *curr;
    size_list           *csize;
    long                act_size;
    long                cmp_size;
    long                size, old_size, inf_size;
    LIST                *list;

    act_size = 0;
    cmp_size = 0;
    for( curr = FileList; curr != NULL; curr = curr->next ) {
        for( csize = curr->sizes; csize != NULL; csize = csize->next ) {
            act_size += csize->size;
        }
        cmp_size += curr->cmp_size;
    }
    printf( "Installed size = %ld, Disk space = %ld\n", act_size, cmp_size );

//  place SETUP.EXE, *.EXE on the 1st disk
    size = FileSize( Setup );
    for( list = ExeList; list != NULL; list = list->next ) {
        size += FileSize( list->item );
    }
    inf_size = 0;
    old_size = 0;
    if( !FillFirst ) size = DiskSize;
    for( ;; ) {
        /* keep creating script until size stabilizes */
        disks = CreateScript( size+inf_size, 0 );
        inf_size = FileSize( "setup.inf" );
        if( old_size > inf_size ) {
            /*
                If the new size of the install script is less than the
                old size, we can create the script with some padding
                to bring it up to the old size. This prevents us from
                going into an oscillation between two sizes.
            */
            disks = CreateScript( size+old_size, old_size - inf_size );
            inf_size = old_size;
        }
        if( old_size == inf_size ) break;
        old_size = inf_size;
    }
    printf( "Installation will require %d disks\n", disks );
    return( disks );
}


int main( int argc, char *argv[] )
//================================

{
    bool                ok;
    FILE                *fp;

    if( !CheckParms( &argc, &argv ) ) {
        return( 1 );
    }
    fp = fopen( argv[3], "r" );
    if( fp == NULL ) {
        printf( "Cannot open '%s'\n", argv[3] );
        return( 1 );
    }
    printf( "Reading Info File...\n" );
    ReadInfFile();
    ok = ReadList( fp );
    fclose( fp );
    if( !ok )
        return( 1 );
    printf( "Checking for duplicate files...\n" );
    ok = CheckForDuplicateFiles();
    if( !ok )
        return( 1 );
    printf( "Making script...\n" );
    MakeScript();
    CreateDisks();
    return( 0 );
}
