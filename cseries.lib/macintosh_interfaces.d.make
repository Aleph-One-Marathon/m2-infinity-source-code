#	MACINTOSH_INTERFACES.D
#	--------------------------------------------------------------
#	Thursday, July 11, 1991 11:30:55 AM
#	Saturday, January 15, 1994 2:03:18 PM

CFLAGS= -i "{CSeriesInterfaces}" -b2 -r -mbg on -k "{CseriesLibraries}"
CFLAGS881= -i "{CSeriesInterfaces}" -b2 -r -mbg on -mc68881 -elems881 -k "{CSeriesLibraries}"

macintosh_interfaces.d ƒ macintosh_interfaces.c macintosh_interfaces.d.make
	C -c -d DUMP_FILE='"macintosh_interfaces.d"' {CFLAGS} macintosh_interfaces.c
	C -c -d DUMP_FILE='"macintosh_interfaces881.d"' {CFLAGS881} macintosh_interfaces.c
