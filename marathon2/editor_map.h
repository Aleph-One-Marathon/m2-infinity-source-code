/* New stuff for Marathon only is tagged at the end. */

typedef short map_icon;

/*
EDITOR.H
Monday, November 29, 1993 8:50:34 AM

Monday, November 29, 1993 9:45:03 AM
	the calling conventions of get_environment_icons and get_monster_icons have
	changed slightly.
*/

/* these are STR# resources which provide the editor with the legal environments
	and monsters (i.e., if there are 5 monster names, then indexes 0-4 are valid
	monster codes. */
#define strENVIRONMENT_NAMES 4000
#define strMONSTER_NAMES 4001

/* the following two functions fill an array, icon_codes, with the icon
	codes specified by the given environment or monster code.  They return
	the number of icon codes placed into the array.  If called with a NULL
	array, no copying is done but the icon count is still returned */
short get_environment_icons(short environment_code, map_icon *icon_codes);
short get_monster_icons(short environment_code, map_icon *icon_codes);

/* get_icon_type() returns one of the following types, given an icon_code */
enum /* icon types */
{
	_icon_is_blank,		/* Nothing- clear passageway */
	_icon_is_wall,
	_icon_is_door,
	_icon_is_monster,
	_icon_is_scenery,
	_icon_is_item
};
short get_icon_type(map_icon icon_code);


#define MAP_VERSION 0

#define MAP_HEIGHT 32
#define MAP_WIDTH 32
#define MONSTER_TYPES_PER_MAP 3

struct monster_specifier
{
	short monster_code;
	short appearing; /* i’ll give you documentation for this field later */
};

struct map_data
{
	short version; /* currently zero */
	short environment;
	struct monster_specifier monsters[MONSTER_TYPES_PER_MAP];
	
	/* pad out the header to 256 bytes (should be initialized to zero) */
	short unused[120];

	/* 32x32 icon codes */
	map_icon data[MAP_HEIGHT][MAP_WIDTH];
};



/*
	editor_map.h
	structure created by the header file for a map.

	Tuesday, November 30, 1993 8:41:55 PM- rdm begun.
*/



#define strSCENARIO_LEVEL_NAMES 128
#define LEVEL_RESOURCE_TYPE 'MAP '

/* ---------------- constants */
#define TYPE_SHIFT 12

#define WALL (_icon_is_wall<<TYPE_SHIFT)
#define DOOR (_icon_is_door<<TYPE_SHIFT)
#define MONSTER (_icon_is_monster<<TYPE_SHIFT)
#define SCENERY (_icon_is_scenery<<TYPE_SHIFT)
#define ITEM (_icon_is_item<<TYPE_SHIFT)

#if 0
#define GET_ICON_TYPE(x) ((x)>>TYPE_SHIFT)		/* REAL get_icon_type */
#endif
#define GET_ICON_TYPE(x) ((x))

/* Fill these in with the proper method */
enum icon_door_state {
	_icon_door_opened=0x0000,
	_icon_door_closed=0x0800
};
#define GET_ICON_DOOR_STATE(i) (_icon_door_closed) /* assume all doors start closed */

/* #define GET_ICON_DOOR_TYPE(i) ((i)&(word)0x0fff) */ /* This is the real one 256 door types */
#define GET_ICON_DOOR_TYPE(i) (0)

/* ------------ monster/scenery/item stuff */
#define ICON_COLLECTION(i) (FIRST_OBJECT_COLLECTION+((i)&(word)(0x0fff)))
#define ICON_SHAPE(i) (0)
#define MONSTER_TYPE(i) ((i)&&(word)0x0fff)