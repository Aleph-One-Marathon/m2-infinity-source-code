#	CSERIES.XCOFF
#	--------------------------------------------------------------
#	Thursday, July 11, 1991 11:30:44 AM
#	Thursday, January 13, 1994 7:35:38 AM

# {PPCCOptions} is used below in .xcoff dependency rule
#PPCCOptions= -i {CSeriesInterfaces} -appleext on -opt speed -dialect ansic -w conformance
PPCCOptions= -i {CSeriesInterfaces} -appleext on -sym on -dialect ansic -w conformance -d DEBUG

# Define our object file directory, and give a directory dependency rule (what does this
# do again?)
Obj= :Objects:
{Obj} ƒ :

# default dependency rule for .c files
.xcoff ƒ .c
	ppcc {PPCCOptions} {default}.c -o {Obj}{default}.xcoff

OBJECTS= {Obj}macintosh_utilities.xcoff {Obj}my32bqd.xcoff {Obj}rle.xcoff {Obj}preferences.xcoff {Obj}proximity_strcmp.xcoff ∂
	{Obj}temporary_files.xcoff {Obj}devices.xcoff {Obj}dialogs.xcoff

macintosh_cseries.h ƒ cseries.h

### Portable
{Obj}proximity_strcmp.xcoff ƒ proximity_strcmp.h cseries.h cseries.xcoff.make
{Obj}rle.xcoff ƒ rle.h cseries.h cseries.xcoff.make

### Macintosh
{Obj}macintosh_utilities.xcoff ƒ macintosh_cseries.h cseries.xcoff.make
{Obj}my32bqd.xcoff ƒ my32bqd.h macintosh_cseries.h cseries.xcoff.make
{Obj}devices.xcoff ƒ macintosh_cseries.h cseries.xcoff.make
{Obj}dialogs.xcoff ƒ macintosh_cseries.h cseries.xcoff.make
{Obj}preferences.xcoff ƒ preferences.h macintosh_cseries.h cseries.xcoff.make
{Obj}temporary_files.xcoff ƒ temporary_files.h macintosh_cseries.h cseries.xcoff.make

cseries.xcoff ƒƒ {OBJECTS}
	PPCLink -o cseries.xcoff -xm library {OBJECTS}
	Move -y cseries.xcoff {CSeriesLibraries}

cseries.xcoff ƒƒ {CSeriesInterfaces}cseries.h {CSeriesInterfaces}my32bqd.h {CSeriesInterfaces}rle.h ∂
	{CSeriesInterfaces}preferences.h {CSeriesInterfaces}macintosh_cseries.h {CSeriesInterfaces}proximity_strcmp.h ∂
	{CSeriesInterfaces}temporary_files.h {CSeriesInterfaces}cseries.a {CSeriesInterfaces}macintosh_interfaces.h

{CSeriesInterfaces}cseries.h ƒ cseries.h
	Duplicate -y cseries.h {CSeriesInterfaces}
{CSeriesInterfaces}cseries.a ƒ cseries.a
	Duplicate -y cseries.a {CSeriesInterfaces}
{CSeriesInterfaces}my32bqd.h ƒ my32bqd.h
	Duplicate -y my32bqd.h {CSeriesInterfaces}
{CSeriesInterfaces}rle.h ƒ rle.h
	Duplicate -y rle.h {CSeriesInterfaces}
{CSeriesInterfaces}preferences.h ƒ preferences.h
	Duplicate -y preferences.h {CSeriesInterfaces}
{CSeriesInterfaces}macintosh_cseries.h ƒ macintosh_cseries.h
	Duplicate -y macintosh_cseries.h {CSeriesInterfaces}
{CSeriesInterfaces}proximity_strcmp.h ƒ proximity_strcmp.h
	Duplicate -y proximity_strcmp.h {CSeriesInterfaces}
{CSeriesInterfaces}temporary_files.h ƒ temporary_files.h
	Duplicate -y temporary_files.h {CSeriesInterfaces}
{CSeriesInterfaces}macintosh_interfaces.h ƒ macintosh_interfaces.c
	Duplicate -y macintosh_interfaces.c {CSeriesInterfaces}macintosh_interfaces.h
