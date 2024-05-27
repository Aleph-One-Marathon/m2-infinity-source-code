### CSERIES.LIB
### this is not your father’s makefile
### Thursday, March 10, 1994 7:08:19 PM

MakeFile=
#makefile

# default dependency rule for .c.o files
.c.o ƒ .c
	C {Default}.c {COptions} -o "{Obj68k}"{Default}.c.o

# default dependency rule for .ppc.o files
.ppc.o ƒ .c
	MrC {Default}.c {PPCCOptions} -o "{ObjPPC}"{Default}.ppc.o 

Headers= "{CSeriesInterfaces}"cseries.h "{CSeriesInterfaces}"my32bqd.h "{CSeriesInterfaces}"rle.h "{CSeriesInterfaces}"checksum.h ∂
	"{CSeriesInterfaces}"preferences.h "{CSeriesInterfaces}"macintosh_cseries.h "{CSeriesInterfaces}"proximity_strcmp.h ∂
	"{CSeriesInterfaces}"byte_swapping.h "{CSeriesInterfaces}"cseries.a "{CSeriesInterfaces}"macintosh_interfaces.c ∂
	"{CSeriesInterfaces}"mytm.h

both ƒ cseries.lib cseries.xcoff
both.debug ƒ cseries.debug.lib cseries.debug.xcoff

# the object file directory depends on the current directory (where the source is)
{Obj68k} ƒ :
{ObjPPC} ƒ :

# --------------------------------------------------------------------------------------
# 68K LIBRARY
# --------------------------------------------------------------------------------------

Objects68k= {Obj68k}macintosh_utilities.c.o {Obj68k}my32bqd.c.o {Obj68k}checksum.c.o {Obj68k}rle.c.o {Obj68k}preferences.c.o {Obj68k}proximity_strcmp.c.o ∂
	{Obj68k}byte_swapping.c.o {Obj68k}devices.c.o {Obj68k}dialogs.c.o {Obj68k}mytm.c.o ∂
	{Obj68k}device_dialog.c.o
{Obj68k}proximity_strcmp.c.o ƒ proximity_strcmp.h cseries.h {MakeFile}
{Obj68k}rle.c.o ƒ rle.h cseries.h {MakeFile}
{Obj68k}checksum.c.o ƒ checksum.h cseries.h {MakeFile}
{Obj68k}macintosh_utilities.c.o ƒ macintosh_cseries.h cseries.h {MakeFile}
{Obj68k}my32bqd.c.o ƒ my32bqd.h macintosh_cseries.h cseries.h {MakeFile}
{Obj68k}devices.c.o ƒ RequestVideo.c RequestVideo.h macintosh_cseries.h cseries.h {MakeFile}
{Obj68k}device_dialog.c.o ƒ macintosh_cseries.h cseries.h {MakeFile}
{Obj68k}dialogs.c.o ƒ macintosh_cseries.h cseries.h {MakeFile}
{Obj68k}preferences.c.o ƒ preferences.h preferences.h macintosh_cseries.h cseries.h {MakeFile}
{Obj68k}mytm.c.o ƒ mytm.h mytm.h macintosh_cseries.h cseries.h {MakeFile}
{Obj68k}byte_swapping.c.o ƒ byte_swapping.h macintosh_cseries.h cseries.h {MakeFile}
{Targ68k} ƒƒ {Objects68k}
	Lib -o {Targ68k} {Objects68k}
	Move -y {Targ68k} "{CSeriesLibraries}"

# --------------------------------------------------------------------------------------
# PPC LIBRARY
# --------------------------------------------------------------------------------------

ObjectsPPC= {ObjPPC}macintosh_utilities.ppc.o {ObjPPC}my32bqd.ppc.o {ObjPPC}checksum.ppc.o {ObjPPC}rle.ppc.o {ObjPPC}preferences.ppc.o {ObjPPC}proximity_strcmp.ppc.o ∂
	{ObjPPC}byte_swapping.ppc.o {ObjPPC}devices.ppc.o {ObjPPC}dialogs.ppc.o {ObjPPC}mytm.ppc.o ∂
	{ObjPPC}device_dialog.ppc.o #{CSeriesLibraries}DisplayLib
{ObjPPC}proximity_strcmp.ppc.o ƒ proximity_strcmp.h cseries.h {MakeFile}
{ObjPPC}rle.ppc.o ƒ rle.h cseries.h {MakeFile}
{ObjPPC}checksum.ppc.o ƒ checksum.h cseries.h {MakeFile}
{ObjPPC}macintosh_utilities.ppc.o ƒ macintosh_cseries.h cseries.h {MakeFile}
{ObjPPC}my32bqd.ppc.o ƒ my32bqd.h macintosh_cseries.h cseries.h {MakeFile}
{ObjPPC}devices.ppc.o ƒ RequestVideo.c RequestVideo.h macintosh_cseries.h cseries.h {MakeFile}
{ObjPPC}device_dialog.ppc.o ƒ macintosh_cseries.h cseries.h {MakeFile}
{ObjPPC}dialogs.ppc.o ƒ macintosh_cseries.h cseries.h {MakeFile}
{ObjPPC}preferences.ppc.o ƒ preferences.h macintosh_cseries.h cseries.h {MakeFile}
{ObjPPC}mytm.ppc.o ƒ mytm.h macintosh_cseries.h cseries.h {MakeFile}
{ObjPPC}byte_swapping.ppc.o ƒ byte_swapping.h macintosh_cseries.h cseries.h {MakeFile}
{TargPPC} ƒƒ {ObjectsPPC}
	PPCLink -o {TargPPC} {SymbolsPPC} -xm library {ObjectsPPC}
	Move -y {TargPPC} "{CSeriesLibraries}"

# --------------------------------------------------------------------------------------
# LIBRARY HEADERS
# --------------------------------------------------------------------------------------

cseries ƒƒ cseries.lib cseries.xcoff
cseries.debug ƒƒ cseries.debug.lib cseries.debug.xcoff
cseries.sym ƒƒ cseries.sym.xcoff

{Targ68k} ƒƒ {Headers}
{TargPPC} ƒƒ {Headers}
"{CSeriesInterfaces}"cseries.h ƒ cseries.h
	Duplicate -y cseries.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"cseries.a ƒ cseries.a
	Duplicate -y cseries.a "{CSeriesInterfaces}"
"{CSeriesInterfaces}"my32bqd.h ƒ my32bqd.h
	Duplicate -y my32bqd.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"rle.h ƒ rle.h
	Duplicate -y rle.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"checksum.h ƒ checksum.h
	Duplicate -y checksum.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"preferences.h ƒ preferences.h
	Duplicate -y preferences.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"mytm.h ƒ mytm.h
	Duplicate -y mytm.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"macintosh_cseries.h ƒ macintosh_cseries.h
	Duplicate -y macintosh_cseries.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"proximity_strcmp.h ƒ proximity_strcmp.h
	Duplicate -y proximity_strcmp.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"byte_swapping.h ƒ byte_swapping.h
	Duplicate -y byte_swapping.h "{CSeriesInterfaces}"
"{CSeriesInterfaces}"macintosh_interfaces.c ƒ macintosh_interfaces.c
	Duplicate -y macintosh_interfaces.c "{CSeriesInterfaces}"macintosh_interfaces.c
