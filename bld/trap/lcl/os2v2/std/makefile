#pmake: build lcl os_os2 cpu_386 cpu_386 trap x86

tree_depth = 5

host_os  = os2
host_cpu = 386

target_cpu = x86

#
# Create the 32-bit OS/2 std.dll
#

srv   = lcl
which = TRAP

extra_objs =          &
        accmisc.obj   &
        os2v2acc.obj  &
        os22path.obj  &
        os2rfx.obj    &
        pgmexec.obj   &
        softmode.obj  &
        dbgthrd.obj   &
        os2conv.obj   &
        loadhelp.obj  &
        pmlock.obj    &
        x86cpu.obj    &
        supp.obj      &
        core.obj

extra_linker = &
        imp _TempStack wdsplice, DoReadWord_ wdsplice  &
        imp DoWriteWord_ wdsplice, DoClose_ wdsplice   &
        imp DoOpen_ wdsplice, DoWritePgmScrn_ wdsplice &
        imp DoReadXMMRegs_ wdsplice                    &
        imp DoWriteXMMRegs_ wdsplice                   &
        imp _XferBuff wdsplice, DoDupFile_ wdsplice    &
        lib $(os2api_dir)/os2286/lib/os2.lib

!include $(trap_dir)/lcl/os2v2/os2v2.mif
!include $(trap_dir)/lcl/lcl.mif
!include $(trap_dir)/master.mif
