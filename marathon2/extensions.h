/*

	extensions.h
	Tuesday, October 31, 1995 11:42:19 AM- rdm created.

*/

#define BUNGIE_PHYSICS_DATA_VERSION 0
#define PHYSICS_DATA_VERSION 1

struct definition_data
{
	long tag;
	void *data;
	short count;
	short size;
};

#ifdef INCLUDE_STRUCTURES
static struct definition_data definitions[]=
{
	{MONSTER_PHYSICS_TAG, monster_definitions, NUMBER_OF_MONSTER_TYPES, sizeof(struct monster_definition)},
	{EFFECTS_PHYSICS_TAG, effect_definitions, NUMBER_OF_EFFECT_TYPES, sizeof(struct effect_definition)},
	{PROJECTILE_PHYSICS_TAG, projectile_definitions, NUMBER_OF_PROJECTILE_TYPES, sizeof(struct projectile_definition)},
	{PHYSICS_PHYSICS_TAG, physics_models, NUMBER_OF_PHYSICS_MODELS, sizeof(struct physics_constants)},
	{WEAPONS_PHYSICS_TAG, weapon_definitions, MAXIMUM_NUMBER_OF_WEAPONS, sizeof(struct weapon_definition)}
};
#define NUMBER_OF_DEFINITIONS (sizeof(definitions)/sizeof(definitions[0]))
#endif

/* ------------- prototypes */

/* Set the physics file to read from.. */
void set_physics_file(FileDesc *file);

void set_to_default_physics_file(void);

/* Proceses the entire physics file.. */
void import_definition_structures(void);

void *get_network_physics_buffer(long *physics_length);
void process_network_physics_model(void *data);
void import_physics_wad_data(struct wad_data *wad);

void *get_physics_array_and_size(long tag, long *size);
