# serial_numbers.MAKE
# Saturday, July 3, 1993 8:18:48 AM

# Sunday, October 2, 1994 1:02:49 PM  (Jason')
#	from NORESNAMES.MAKE

COptions= -i {CSeriesInterfaces} -b2 -r -mbg on -d DEBUG
Obj= ":objects:misc:"
Source= :

{Obj} ƒ {Source}

.c.o ƒ .c
	C {Default}.c {COptions} -o "{Obj}{Default}.c.o"

serial_numbers.c.o ƒ serial_numbers.make

OBJECTS= {Obj}serial_numbers.c.o {CSeriesLibraries}"cseries.debug.lib"
serial_numbers ƒ {OBJECTS}
	Link -w -c 'MPS ' -t MPST {OBJECTS} -sn STDIO=Main -sn INTENV=Main -sn %A5Init=Main ∂
		"{Libraries}"Stubs.o "{Libraries}MacRuntime.o" "{Libraries}"Interface.o "{CLibraries}"StdCLib.o ∂
		"{CLibraries}"CSANELib.o "{Libraries}IntEnv.o" "{CLibraries}"Math.o "{Libraries}"ToolLibs.o ∂
		"{Libraries}MathLib.o" -o serial_numbers
