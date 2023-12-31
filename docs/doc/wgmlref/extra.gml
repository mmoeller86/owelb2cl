:SET symbol='ibmpc' value='IBM PC/DOS'
:SET symbol='ibmvm' value='IBM VM/CMS'
:SET symbol='decvax' value='DEC VAX/VMS'
:SET symbol='isbn' value=''.
:SET symbol='wgml' value='WATCOM Script/GML'

.dm hp2 begin
:HP2.
.dm hp2 end

.dm ehp2 begin
:eHP2.
.dm ehp2 end

.dm choose begin
.in +3
.dm choose end

.dm echoose begin
.sk 2
.in -3
.dm echoose end

.dm mouse begin
.sk 2
.cp 6
.in -3
:HP2.Mouse::eHP2.
.sk
.in +3
.dm mouse end

.dm keyboard begin
.sk 2
.cp 6
.in -3
:HP2.Keyboard::eHP2.
.sk
.in +3
.dm keyboard end

.dm screen begin
:FIG frame=none.
.if &e'&dohelp ne 0 .do begin
:HBMP '&*1..bmp' i
.do end
.el .do begin
:GRAPHIC file='&*1..ps' depth='2.5i' scale=45.
.do end
:FIGCAP.&*2.
:eFIG.
.dm screen end

:cmt. ***************************************************************
:cmt. Highlighting
:cmt.   - various macros for highlighting text within a document
:cmt.   - each macro corresponds to a kind of text
:cmt.     1. keyboard keys              & key.a& ekey.
:cmt.     2. program keywords           & keyword.select& ekeyword.
:cmt.     3. terms                      & term.mouse& eterm.
:cmt.     4. program objects            & name.C1& ename.
:cmt.     5. values                     & value.25& evalue.
:cmt.     5. special symbols            & sym.a& esym.     (alpha)
:cmt. ***************************************************************

.dm sf1 begin
.ct :SF font=1.
.dm sf1 end

.dm sf2 begin
.ct :SF font=2.
.dm sf2 end

.dm sf7 begin
.ct :SF font=7.
.dm sf7 end

.dm sf8 begin
.ct :SF font=8.
.dm sf8 end

.dm sf20 begin
.ct :SF font=20.
.dm sf20 end

.dm esf begin
.ct :eSF.
.dm esf end

.dm evalue begin
.ct ":eSF.
.dm evalue end

:cmt. keyboard keys
:cmt.   eg. Enter, Esc, F1, a, etc.
:cmt.   - see also key macros later on in this file
    :set symbol='key'           value=';.sf2;.ct '.
    :set symbol='ekey'          value=';.esf;.ct '.

:cmt. program keywords
:cmt.   eg. combine, select, etc.
    :set symbol='keyword'       value=';.sf8;.ct '.
    :set symbol='ekeyword'      value=';.esf;.ct '.

:cmt. new terms
:cmt.   eg.
    :set symbol='term'          value=';.sf1;.ct '.
    :set symbol='eterm'         value=';.esf;.ct '.

:cmt. names of program objects
:cmt.  eg. spread sheet cell names such as A1, file names etc.
    :set symbol='name'          value=';.sf2;.ct '.
    :set symbol='ename'         value=';.esf;.ct '.

:cmt. values
:cmt.  eg. spread sheet cell values, watfile field values, numbers, etc.
    :set symbol='value'         value=';.sf8;.ct "'.
    :set symbol='evalue'        value=';.evalue;.ct '.

:cmt. special symbol font
:cmt.  eg. arrow keys, greek letters, etc.
    :set symbol='sym'           value=';.sf20;.ct '.
    :set symbol='esym'          value=';.esf;.ct '.

:SET symbol='hp2' value=';.hp2;.ct '
:SET symbol='ehp2' value=';.ehp2;.ct '

:cmt. menu item
    :SET symbol='menu' value=';.sf1;.ct '
    :SET symbol='emenu' value=';.esf;.ct '

:cmt. monospaced item
    :SET symbol='mono' value=';.sf8;.ct '
    :SET symbol='emono' value=';.esf;.ct '


:cmt. ***************************************************************
:cmt. Keyboard key macros
:cmt.   - various macros for keyboard keys
:cmt.   - includes arrow keys, etc
:cmt. ***************************************************************

:set symbol='crsup'     value=';.sf20;.ct ~U;.esf;.ct '.
:set symbol='crsdn'     value=';.sf20;.ct ~D;.esf;.ct '.
:set symbol='crslt'     value=';.sf20;.ct ~L;.esf;.ct '.
:set symbol='crsrt'     value=';.sf20;.ct ~R;.esf;.ct '.
:set symbol='bksp'      value=';.sf20;.ct ~L;.esf;.ct '.

:cmt. ***************************************************************
:cmt. Generic enter and escape symbols
:cmt. ***************************************************************

:set symbol='g_enter'     value=';.sf2;.ct Enter;.esf;.ct '.
:set symbol='g_escape'    value=';.sf2;.ct Escape;.esf;.ct '.
