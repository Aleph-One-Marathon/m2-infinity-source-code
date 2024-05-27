/*
DEFINITIONS.H
Sunday, October 2, 1994 1:19:21 PM  (Jason')
*/

/* ---------- tags */

enum /* tags */
{
	MONSTER_TAG= 'mons',
	EFFECT_TAG= 'effe',
	PROJECTILE_TAG= 'proj',
	PHYSICS_TAG= 'phys',
	WEAPON_TAG= 'weap',
	DESCRIPTION_TAG= 'desc' // ascii, used to describe this physics model
};

/* ---------- structures */

struct definition_header /* 1k */
{
	short version;
	long size;
	
	char type[256]; /* editor software */
	char creator[256]; /* human */
	
	short unused[253];
};

struct definition_data /* 32 bytes */
{
	long tag; /* unknown tags are ignored */
	long offset, size;
	
	short unused[10];
};
