/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2023 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  NT trap dll internal interfaces
*
****************************************************************************/


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <tlhelp32.h>
#include "madconf.h"
#include "brkptcpu.h"
#include "trpimp.h"
#include "trpcomm.h"
#include "packet.h"
#include "exepe.h"
#include "exeos2.h"
#include "exedos.h"
#if ( MADARCH & MADARCH_X86 ) && defined( WOW )
#include "vdmdbg.h"
#endif


#undef GetThreadContext
#define GetThreadContext Dont_call_GetThreadContext_directly__Call_MyGetThreadContext_instead
#undef SetThreadContext
#define SetThreadContext Dont_call_SetThreadContext_directly__Call_MySetThreadContext_instead

#if MADARCH & MADARCH_X64
    #define MYCONTEXT           WOW64_CONTEXT
    // position in Windows CONTEXT,
    // it is offset in FXSAVE/FXRSTOR memory structure
    #define MYCONTEXT_MXCSR     24
    #define MYCONTEXT_XMM       (10 * 16)

    #define MYCONTEXT_CONTROL   WOW64_CONTEXT_CONTROL;
#else
    #define MYCONTEXT           CONTEXT
    // position in Windows CONTEXT,
    // it is offset in FXSAVE/FXRSTOR memory structure
    #define MYCONTEXT_MXCSR     24
    #define MYCONTEXT_XMM       (10 * 16)
#endif

    #define MYCONTEXT_CONTROL   CONTEXT_CONTROL;

#if MADARCH & MADARCH_X86
    #define MYCONTEXT_TO_USE    (CONTEXT_FULL | CONTEXT_FLOATING_POINT | \
                        CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS)
#elif MADARCH & MADARCH_X64
    #define MYCONTEXT_TO_USE    (CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)
#else
    #define MYCONTEXT_TO_USE    CONTEXT_FULL
#endif

#if MADARCH & MADARCH_X64
    #define WOW64CONTEXT           WOW64_CONTEXT
    // position in Windows CONTEXT,
    // it is offset in FXSAVE/FXRSTOR memory structure
    #define WOW64CONTEXT_MXCSR     24
    #define WOW64CONTEXT_XMM       (10 * 16)

    #define WOW64CONTEXT_CONTROL   WOW64_CONTEXT_CONTROL;
    #define WOW64CONTEXT_TO_USE    (WOW64_CONTEXT_FULL | WOW64_CONTEXT_FLOATING_POINT | \
                        WOW64_CONTEXT_DEBUG_REGISTERS | WOW64_CONTEXT_EXTENDED_REGISTERS)
#endif

#if MADARCH & MADARCH_X64
#define RDWORD  DWORD64
#else
#define RDWORD  DWORD
#endif

#if MADARCH & (MADARCH_X86 | MADARCH_X64)
#define GET_LDT_BASE(d) \
    ((ULONG_PTR)(d).BaseLow + ((ULONG_PTR)(d).HighWord.Bytes.BaseMid << 16L) + \
    ((ULONG_PTR)(d).HighWord.Bytes.BaseHi << 24L))

#define GET_LDT_LIMIT(d) \
    ((d).HighWord.Bits.Granularity ? \
    ((1 + (ULONG_PTR)(d).LimitLow + ((ULONG_PTR)(d).HighWord.Bits.LimitHi << 16L)) << 12L) : \
    (1 + (ULONG_PTR)(d).LimitLow + ((ULONG_PTR)(d).HighWord.Bits.LimitHi << 16L)))
#endif

#define COND_VDM_START                  0x80000000

#define STATE_NONE                      0x00000000
#define STATE_WATCH                     0x00000001
#define STATE_WATCH_386                 0x00000002
#define STATE_EXPECTING_FAULT           0x00000004
#define STATE_IGNORE_DEAD_THREAD        0x00000008
#define STATE_WAIT_FOR_VDM_START        0x00000010
#define STATE_IGNORE_DEBUG_OUT          0x00000020

typedef struct {
    WORD                signature;
    union {
        pe_exe_header   pehdr;
        os2_exe_header  nehdr;
    } u;
    char                modname[16];
} header_info;

#if ( MADARCH & MADARCH_X86 ) && defined( WOW )
typedef struct {
    addr48_ptr          addr;
    DWORD               tid;
    WORD                htask;
    DWORD               hmodule;
    char                modname[MAX_MODULE_NAME+1];
} wow_info;
#endif

typedef struct thread_info {
    struct thread_info  *next;
    DWORD               tid;
    HANDLE              thread_handle;
    LPVOID              start_addr;
    addr_off            brk_addr;
    opcode_type         old_opcode;
    char                alive:1;
    char                suspended:1;
    char                is_wow:1;
    char                is_dos:1;
    char                is_foreign:1;
} thread_info;

typedef struct process_info {
    thread_info         *thread_list;
    DWORD               pid;
    HANDLE              __unused_field;
    HANDLE              process_handle;
    LPVOID              base_addr;
} process_info;

typedef enum {
    SS_UNKNOWN          = 0x0000,
    SS_NATIVE           = 0x0001,
    SS_WINDOWS_GUI      = 0x0002,
    SS_WINDOWS_CHAR     = 0x0003,
    SS_OS2_CHAR         = 0x0005,
    SS_POSIX_CHAR       = 0x0007
} subsystems;

typedef struct msg_list {
    struct msg_list     *next;
    char                msg[1]; /* variable size */
} msg_list;

typedef unsigned        myconditions;

#if !defined( WOW ) || ( MADARCH & MADARCH_X64 )
typedef void            IMAGE_NOTE;
#endif

/*
 * global variables prototypes
 */

#include "globals.h"

/*
 * function prototypes
 */

/* accmap.c */
extern bool             FindExceptInfo( LPVOID off, LPVOID *base, DWORD *size );
extern void             FixUpDLLNames( void );
extern void             RemoveModuleFromLibList( char *module, char *filename );
extern bool             IsMagicalFileHandle( HANDLE h );
extern HANDLE           GetMagicalFileHandle( char *name );
extern void             AddProcess( header_info * );
extern void             DelProcess( bool );
extern void             VoidProcess( void );
extern void             AddLib( bool, IMAGE_NOTE *im );
extern void             DelLib( void );
extern void             FreeLibList( void );
extern bool             DoListLibs( char *buff, int is_first, int want_16, int want_32, int verbose, int sel );

/* accmem.c */
extern DWORD            WriteMemory( addr48_ptr *addr, LPVOID buff, DWORD size );
extern DWORD            ReadMemory( addr48_ptr *addr, LPVOID buff, DWORD size );

/* accmisc.c */
extern bool             GetSelectorLDTEntry( WORD sel, LDT_ENTRY *ldt );
extern bool             IsBigSel( WORD sel );
extern void             AddMessagePrefix( char *buff, size_t len );

/* accrun.c */
extern opcode_type      place_breakpoint_lin( HANDLE process, LPVOID base );
extern int              remove_breakpoint_lin( HANDLE process, LPVOID base, opcode_type old_opcode );
extern myconditions     DebugExecute( DWORD state, bool *tsc, bool );
extern void             InterruptProgram( void );
extern bool             Terminate( void );

/* misc.c */
extern bool             MyGetThreadContext( thread_info *ti, MYCONTEXT *pc );
extern bool             MySetThreadContext( thread_info *ti, MYCONTEXT *pc );

/* peread.c */
extern bool             SeekRead( HANDLE handle, DWORD newpos, void *buff, WORD size );
extern bool             GetEXEHeader( HANDLE handle, header_info *hi, WORD *stack );
extern bool             GetModuleName( HANDLE fhdl, char *name );

/* thread.c */
extern void             AddThread( DWORD tid, HANDLE th, LPVOID sa );
extern void             DeadThread( DWORD tid );
extern thread_info      *FindThread( DWORD tid );
extern void             RemoveThread( DWORD tid );
extern void             RemoveAllThreads( void );

/* accbrwat.c */
extern bool             CheckWatchPoints( void );
extern bool             SetDebugRegs( void );
extern void             ClearDebugRegs( void );
extern DWORD            GetDR6( void );
extern void             SetDR7( DWORD tmp );
extern bool             FindBreak( WORD segment, DWORD offset, opcode_type *old_opcode );
extern bool             IsWatch( void );

/* dbgthrd.c */
extern DWORD            StartControlThread( char *name, DWORD *pid, DWORD cr_flags );
extern bool             MyWaitForDebugEvent( void );
extern void             MyContinueDebugEvent( int );
extern void             StopControlThread( void );
extern void             ProcessQueuedRepaints( void );
extern void             ParseServiceStuff( char *name,
                            char **pdll_name, char **pservice_name,
                            char **pdll_destination, char **pservice_parm );

/* accregs.c */
extern LPVOID           AdjustIP( MYCONTEXT *, int );
extern void             SetIP( MYCONTEXT *, LPVOID );

extern void             say( char *fmt, ... );
