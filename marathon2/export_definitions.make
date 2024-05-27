# EXPORT_DEFINITIONS.MAKE
# Saturday, July 3, 1993 8:18:48 AM

# Sunday, October 2, 1994 1:02:49 PM  (Jason')
#	from NORESNAMES.MAKE

COptions= -i {CSeriesInterfaces} -b2 -r -mbg on -d DEBUG
Obj= ":objects:misc:"
Source= :
{Obj} ƒ {Source}

.c.o ƒ .c
	C {Default}.c {COptions} -o "{Obj}{Default}.c.o"

export_definitions.c.o ƒ export_definitions.make

OBJECTS= {Obj}export_definitions.c.o {CSeriesLibraries}"cseries.debug.lib"
export_definitions ƒ {OBJECTS}
	Link -w -c 'MPS ' -t MPST {OBJECTS} -sn STDIO=Main -sn INTENV=Main -sn %A5Init=Main ∂
		"{Libraries}"Stubs.o "{Libraries}"Runtime.o "{Libraries}"Interface.o "{CLibraries}"StdCLib.o ∂
		"{CLibraries}"CSANELib.o "{CLibraries}"Math.o "{Libraries}"ToolLibs.o ∂
		-o export_definitions
