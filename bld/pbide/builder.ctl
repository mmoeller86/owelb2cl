# TODO: Restructure directories, redo makefiles. This project is a mess.
#       Parts of it seem obsolete (wfscopy).
#
# PBIDE Builder Control file
# ==========================

set PROJNAME=pbide

set PROJDIR=<CWD>

[ INCLUDE "<OWROOT>/build/prolog.ctl" ]

[ INCLUDE "<OWROOT>/build/defrule.ctl" ]

[ BLOCK <BLDRULE> rel ]
#======================
    cdsay "<PROJDIR>"

[ BLOCK <BLDRULE> rel cprel ]
#============================
    <CCCMD> fscopy/wfscopy.exe                  "<OWRELROOT>/binw/wfscopy.exe"
    <CCCMD> run/wini86/run.exe                  "<OWRELROOT>/binw/watrun.exe"
    <CCCMD> dlldbg/wini86/dlldbg.exe            "<OWRELROOT>/binw/dlldbg.exe"
    <CCCMD> wig/wini86.dll/pbide.dll            "<OWRELROOT>/binw/pbide.dll"
    <CCCMD> cfg/wini86/*.cfg                    "<OWRELROOT>/binw/"
    <CCCMD> run/nt386/run.exe                   "<OWRELROOT>/binnt/watrun.exe"
    <CCCMD> dlldbg/nt386/dlldbg.exe             "<OWRELROOT>/binnt/dlldbg.exe"
    <CCCMD> wig/nt386.dll/pbide.dll             "<OWRELROOT>/binnt/pbide.dll"
    <CCCMD> dlldbg/pbwdnt.dbg                   "<OWRELROOT>/binnt/pbwdnt.dbg"
    <CCCMD> cfg/nt386/*.cfg                     "<OWRELROOT>/binnt/"

    <CPCMD> dlldbg/pbend.dbg                    "<OWRELROOT>/binw/pbend.dbg"
    <CPCMD> dlldbg/pbstart.dbg                  "<OWRELROOT>/binw/pbstart.dbg"
    <CPCMD> dlldbg/pbwd.dbg                     "<OWRELROOT>/binw/pbwd.dbg"
    <CPCMD> pbdll.h                             "<OWRELROOT>/h/pbdll.h"

[ BLOCK . . ]

[ INCLUDE "<OWROOT>/build/epilog.ctl" ]
