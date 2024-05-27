#	CSERIES.LIB
#	--------------------------------------------------------------
#	Thursday, July 11, 1991 11:30:44 AM
#	Saturday, January 15, 1994 2:04:56 PM

# Define our object file directory, and give a directory dependency rule
Obj= :Objects:
{Obj} ƒ :

COptions= -opt full -b2 -r -mbg on -d DEBUG -k {CSeriesLibraries}
#COptions= -opt full -b2 -r -mbg off -k {CSeriesLibraries}
OBJECTS= {Obj}macintosh_utilities.c.o {Obj}my32bqd.c.o {Obj}rle.c.o {Obj}preferences.c.o {Obj}proximity_strcmp.c.o ∂
	{Obj}temporary_files.c.o {Obj}devices.c.o {Obj}dialogs.c.o

macintosh_cseries.h ƒ cseries.h

### Portable
{Obj}proximity_strcmp.c.o ƒ proximity_strcmp.h cseries.h cseries.lib.make
{Obj}rle.c.o ƒ rle.h cseries.h cseries.lib.make

### Macintosh
{Obj}macintosh_utilities.c.o ƒ macintosh_cseries.h cseries.lib.make
{Obj}my32bqd.c.o ƒ my32bqd.h macintosh_cseries.h cseries.lib.make
{Obj}devices.c.o ƒ macintosh_cseries.h cseries.lib.make
{Obj}dialogs.c.o ƒ macintosh_cseries.h cseries.lib.make
{Obj}preferences.c.o ƒ preferences.h macintosh_cseries.h cseries.lib.make
{Obj}temporary_files.c.o ƒ temporary_files.h macintosh_cseries.h cseries.lib.make

cseries.lib ƒƒ {OBJECTS}
	Lib -o cseries.lib {OBJECTS}
	Move -y cseries.lib {CSeriesLibraries}

cseries.lib ƒƒ {CSeriesInterfaces}cseries.h {CSeriesInterfaces}my32bqd.h {CSeriesInterfaces}rle.h ∂
	{CSeriesInterfaces}preferences.h {CSeriesInterfaces}macintosh_cseries.h {CSeriesInterfaces}proximity_strcmp.h ∂
	{CSeriesInterfaces}temporary_files.h {CSeriesInterfaces}cseries.a {CSeriesInterfaces}macintosh_interfaces.c

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
{CSeriesInterfaces}macintosh_interfaces.c ƒ macintosh_interfaces.c
	Duplicate -y macintosh_interfaces.c {CSeriesInterfaces}macintosh_interfaces.c
