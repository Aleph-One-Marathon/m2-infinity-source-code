/*
MACINTOSH_INTERFACES.C
Tuesday, July 17, 1990 6:01:28 PM

Monday, December 24, 1990 8:40:57 PM
	Removed STDIO.H.
Wednesday, December 26, 1990 11:17:09 PM
	We are now part of the macintosh core (ooOooOoh).
Tuesday, June 25, 1991 2:17:39 AM
	The macintosh core is now all but defunct and we are in the Libraries
	directory like we should have been all along; unfortunately there doesn’t
	seem to be a nice way to put it there so we have hardcoded the full path
	of the libraries folder, {Libraries}.
Thursday, July 11, 1991 11:12:55 AM
	Ah ha!  We finally figured out how to do that (I think).  We will bring in
	the environment variables via the -d command-line option.
Friday, July 12, 1991 7:37:36 PM
	Not.  The output file is copied by the make file.  :(
Wednesday, August 28, 1991 7:39:29 PM
	Preprocessor conditional MC68881 used for #pragma dump directive.
Sunday, September 1, 1991 6:00:19 PM
	MPW already defines mc68881, so we use that now.
Friday, October 30, 1992 1:07:25 AM
	We now compile stuff with SystemSixOrLater defined, to avoid linking some
	glue code (hopefully).
Sunday, November 15, 1992 9:04:27 PM
	removed Limits.h, TextEdit.h, Traps.h (!), SysEqu.h (this will certainly break
	something, but we don’t need *all* those low-memory constants), Scrap.h, SANE.h, Picker.h,
	AppleTalk.h, Serial.h, and String.h (the ANSI header).  Hopefully this will speed compile
	time considerably.
Thursday, December 29, 1994 12:23:59 PM  (Jason')
	added GestaltEqu.h, string.h and ctype.h
*/

#ifdef __MWERKS__
	#ifdef powerc
		#pragma precompile_target "macintosh_interfaces.ppc"
	#else
		#pragma precompile_target "macintosh_interfaces.68k"
	#endif
#endif

#define SystemSevenOrLater 1

#include <OSEvents.h>
#include <Errors.h>
#include <Palettes.h>
#include <Packages.h> /* kept for IUCompString */
#include <Types.h>
#include <Resources.h>
#include <QDOffscreen.h>
#include <Events.h>
#include <Windows.h>
#include <Menus.h>
#include <Dialogs.h>
#include <Desk.h>
#include <ToolUtils.h>
#include <Memory.h>
#include <SegLoad.h>
#include <Files.h>
#include <OSUtils.h>
#include <Sound.h>
#include <Lists.h>
#include <OSUtils.h>
#include <Devices.h>
#include <Strings.h> /* added for c2pstr and p2cstr */
#include <Fonts.h>
#include <StandardFile.h>
#include <LowMem.h>
#include <GestaltEqu.h>
#include <Displays.h>
#include <Video.h>

#include <string.h>
#include <ctype.h>

#ifdef applec
#pragma dump DUMP_FILE
#endif