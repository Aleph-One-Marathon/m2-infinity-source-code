#ifndef __PREFERENCES_H
#define __PREFERENCES_H

/*
PREFERENCES.H
Tuesday, September 29, 1992 11:17:46 AM
*/

/* ---------- prototypes: PREFERENCES.C */

OSErr read_preferences_file(void **preferences, char *prefName, OSType prefCreator,
	OSType prefType, short expected_version, long expected_size,
	void (*initialize_preferences)(void *preferences));
OSErr write_preferences_file(void *preferences);

#endif
