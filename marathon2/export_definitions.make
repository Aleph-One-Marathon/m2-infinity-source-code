#   File:       patchtest.make
#   Target:     patchtest
#   Sources:    patch.c
#               test.c
#   Created:    Tuesday, October 25, 1994 10:34:03 PM


COptions= -i {CSeriesInterfaces} -opt full -b2 -r -mbg on -d DEBUG -d PREPROCESSING_CODE -d TERMINAL_EDITOR -mc68020 -k {CSeriesLibraries}

OBJECTS = ∂
		export_definitions.c.o
		
export_definitions ƒƒ export_definitions.make  {OBJECTS}
	Link ∂
		-t 'MPST' ∂
		-c 'MPS ' ∂
		{OBJECTS} ∂
		"{CLibraries}"StdClib.o ∂
 		"{Libraries}"Runtime.o ∂
 		"{Libraries}"Interface.o ∂
		":Objects:Game:68k:Beta:wad.lib" ∂
		{CSeriesLibraries}cseries.debug.lib ∂
		-o export_definitions
	delete export_definitions.c.o
		
export_definitions.c.o ƒ export_definitions.make extensions.h ∂
	weapon_definitions.h projectile_definitions.h monster_definitions.h ∂
	effect_definitions.h physics_models.h ":Objects:Game:68k:Beta:wad.lib"