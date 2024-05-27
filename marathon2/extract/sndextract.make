# {TOOLNAME}.MAKE
# Saturday, July 3, 1993 8:18:48 AM
# Friday, August 25, 1995 4:15:45 PM  (Jason)

ToolName= sndextract

COptions= -i "{CSeriesInterfaces}" -d DEBUG -opt full -b2 -r -mc68020 -k "{CSeriesLibraries}"

{ToolName}.c.o ƒ {ToolName}.make

OBJECTS= {ToolName}.c.o {CSeriesLibraries}"cseries.debug.lib"
{ToolName} ƒ {OBJECTS}
	Link -w -c 'MPS ' -t MPST {OBJECTS} -sn STDIO=Main -sn INTENV=Main -sn %A5Init=Main ∂
		"{Libraries}"Stubs.o "{Libraries}MacRuntime.o" "{Libraries}"Interface.o "{CLibraries}"StdCLib.o ∂
		"{CLibraries}"CSANELib.o "{Libraries}IntEnv.o" "{CLibraries}"Math.o "{Libraries}"ToolLibs.o ∂
		"{Libraries}MathLib.o" -o {ToolName}
