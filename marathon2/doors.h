#ifdef OBSOLETE
/*
	DOORS.H
	Friday, July 15, 1994 4:15:22 PM
*/

/* --------- macros */
#define MAXIMUM_DOORS_PER_MAP 40

#define DOOR_IS_SOLID(d) ((d)->flags&(word)0x8000)
#define SET_DOOR_SOLIDITY(d,v) ((v)?((d)->flags|=(word)0x8000):((d)->flags&=(word)~0x8000))

#define SET_DOOR_BLOCKED_FLAG(d,v) ((v)?((d)->flags|=(word)0x2000):((d)->flags&=(word)~0x2000))
#define DOOR_WAS_BLOCKED(d) ((d)->flags&(word)0x2000)

#define GET_DOOR_STATE(d) ((d)->flags&(word)7)
#define SET_DOOR_STATE(d,s) { assert((s)>=0&&(s)<=7); (d)->flags&= (word)~7; (d)->flags|= (s); }

/* ---------- constants */
enum /* door states (8) */
{
	_door_open, /* and waiting to close */
	_door_closed,
	_door_closing,
	_door_opening,
	_door_toggle
	/* 3 states unused */
};

enum /* door types */
{
	_simple_up,
	_simple_down,
	_simple_split,
	_simple_left,
	_simple_left_up,
	_simple_left_down,
	_simple_left_split,
	_simple_right,
	_simple_right_up,
	_simple_right_down,
	_simple_right_split,
	MAX_DOOR_TYPES
};

/* ---------- structures */
struct insert_side_data
{
	short point_indexes[2];
	short new_point;
	short other_polygon;
};

struct null_polygon_data
{
	short polygon_index;	/* My index */
	short other_polygon_index;	/* Guy across from me that I have to fiddle with on add/detach */
	struct insert_side_data insertion_points[2];
};

struct door_data
{
	short type;
	word flags;
	
	short owner_index;
	short shadow_index;
	
	short shadow_closed_lightsource;
	short shadow_opened_lightsource;
	
	short complex_door_index; /* May be NONE */
	
	struct null_polygon_data shadow_null_data;
	struct null_polygon_data door_null_data;

	short opened_floor_height;
	short closed_floor_height;
	
	short opened_ceiling_height;
	short closed_ceiling_height;

	short ticks;
	short access_level;

	/* 48 bytes of wonderful carnage */
	struct redundant_endpoint_data update_endpoint_data[MAXIMUM_VERTICES_PER_POLYGON];

	short unused[7];
};

/* This structure is stored with the map.. */
struct door_extra_data
{
	short polygon_index;
	short access_level;
};

/* ---------- globals */
extern struct door_data *doors;

/* ---------- prototypes */
short new_door(
	short door_polygon_index,
	short shadow_polygon_index, /* may be NONE */
	short type,
	short shadow_lightsource);

/* Update the door textures after the shapes are loaded (in entering_map) */
void fix_door_textures(void);

void update_active_doors(void);

shape_descriptor get_door_texture(struct door_data *door);
void change_door_state(short index, short new_status);

boolean door_is_open(short door_index);

short find_detached_polygons_twin(short polygon_index);
short find_undetached_polygons_twin(short polygon_index);

boolean player_can_open_door_without_aid(short door_index);
boolean monster_can_control_door(short polygon_index);
#endif
