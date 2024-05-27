#ifdef OBSOLETE

/*
DOORS.C
Friday, November 19, 1993 8:48:11 PM

Friday, November 19, 1993 8:48:15 PM
	based loosely pathways DOORS.C
Tuesday, December 7, 1993 9:14:00 PM
	added ryan’s new_door().
Sunday, January 2, 1994 10:58:08 AM
	use blocked door flag so we don't constantly play the can't close sound.
Monday, May 2, 1994 2:44:48 PM
	Doors now work with the non-orthogonal rendering engine.
Wednesday, May 11, 1994 4:00:10 PM
	This is a huge steaming pile of dung.  Armies of dung beetles must be hired
	to clean this up and make it livable.
Thursday, June 16, 1994 4:03:38 AM
	Following Napolean's scorched earth policy, this code has been completely razed.  It is now
	not a pile of shit.
Friday, September 23, 1994 2:53:08 AM (Jason)
	failed to fix hang in find_line_crossed_leaving_polygon().
*/

#include "cseries.h"
#include "map.h"
#include "interface.h"
#include "platforms.h"
#include "doors.h"

#include <string.h>

#ifdef mpwc
#pragma segment doors
#endif

/* ---------- constants */
/* ---------- structures */

struct door_definition /* 16 bytes */
{
	short type; // _simple_up, etc...
	boolean door_stays_put;
	short ticks_to_open;
	
	short ticks_until_close;
	
	short opening_sound, open_stop_sound;
	short closing_sound, close_stop_sound;
	short cant_close_sound;
};

/* ---------- globals */

static struct door_definition door_definitions[]=
{
	{_simple_up, FALSE, TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
	{_simple_down, FALSE, TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
	{_simple_split, FALSE, TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, 1020, 1010, 1020, 1020},
	{_simple_up, TRUE, TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
	{_simple_down, TRUE, TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
	{_simple_split, TRUE, TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
//	{2*TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
//	{10*TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, 1020, 1010, 1020, 1020},
//	{2*TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
//	{2*TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020},
//	{2*TICKS_PER_SECOND, 4*TICKS_PER_SECOND, 1000, NONE, 1010, NONE, 1020}
};
#define NUMBER_OF_DOOR_DEFINITIONS (sizeof(door_definitions)/sizeof(struct door_definition))

/* ---------- private code */

/* Private prototypes (tons of them) */
static void attach_null_polygon(struct null_polygon_data *data);
static void detach_null_polygon(struct null_polygon_data *data);
static short find_line_index_in_polygon(short polygon_index, short line_index);
static void	set_door_ceiling_and_floor_heights(struct door_data *door);
static boolean door_unobstructed(short index);
static void	set_door_solidity(struct door_data *new_door, boolean solid);
short duplicate_map_polygon(short original_polygon_index);
static void	replace_line_in_polygon(short polygon_index,short original_line_index, short new_line_index);
static void replace_line_polygon_index(short line_index, short old_polygon, short new_polygon);
static void	add_shared_line_to_opposite_of_null(short null_polygon_index, short original_polygon_index,
	short shared_line_duplicate_index, short shared_line_index);
static void replace_all_point_references_in_polygon(short polygon_index, short old_endpoint_index,
	short new_endpoint_index);
static void replace_other_polygon_reference(short line_index, short polygon_index, 
	short new_polygon_index);
static short create_null_polygon(short polygon_index, short owner_index, short shared_line_index, 
	struct null_polygon_data *null_data);
static void new_vertical_door(struct door_data *door);
static short linear_interpolate(short start, short end, fixed fraction);
static void	set_door_textures(struct door_data *door, shape_descriptor texture);
static void	update_shadow(struct door_data *door, fixed fraction);
static short find_line_connects_endpoints(short polygon_index, short point_0, short point_1);
static void	insert_line_in_polygon(short polygon_index, short line_index, short where);
static void	fix_shared_line(short null_polygon_index, short other_polygon_index, boolean attaching);
static void	delete_line_in_polygon(short polygon_index, short line_index);
static void	replace_line_endpoint(short line_index, short old_point, short new_point);
static void fix_poly_changed_objects(short one_index, short two_index);
static fixed get_door_fraction(struct door_data *door);
static fixed get_shadow_fraction(struct door_data *door);
static void set_door_shadow_lightsources(struct door_data *door, short shadow_index, short shadow_lightsource);
static short door_texture_type(short type);
static void	fix_door_y_origins(struct door_data *door, short ceiling_delta);
static void fix_sliding_lightsources(struct door_data *door);

static void dprintf_poly(short which);

#ifdef DEBUG
static struct door_definition *get_door_definition(short index);
#else
#define get_door_definition(i) (door_definitions+(i))
#endif

/* ---------- code */
short new_door(
	short door_polygon_index,
	short shadow_polygon_index, /* may be NONE */
	short type,
	short shadow_lightsource)
{
	struct door_data *new_door;
	short door_index;
	struct polygon_data *polygon;
	struct door_definition *definition;

	/* Setup the door structure, and pass it on to the proper handler */
	assert(dynamic_world->door_count+1<MAXIMUM_DOORS_PER_MAP);
	assert(type>=0&&type<NUMBER_OF_DOOR_DEFINITIONS);
	
	door_index= dynamic_world->door_count++;
	new_door= doors+door_index;

	definition= get_door_definition(type);
	
	/* Setup the polygon */
	polygon= get_polygon_data(door_polygon_index);
	polygon->type= _polygon_is_door;
	polygon->permutation= door_index;
	
	new_door->type= type;
	new_door->owner_index= door_polygon_index;
	new_door->shadow_index= shadow_polygon_index;

	/* Initialize the door data.. */
	SET_DOOR_STATE(new_door, _door_closed);
	set_door_solidity(new_door, TRUE);
	new_door->ticks= definition->ticks_to_open;

	/* Set the heights */
	set_door_ceiling_and_floor_heights(new_door);

	/* Set the opened and closed shadow lightsources based on the door. */
	set_door_shadow_lightsources(new_door, shadow_polygon_index, shadow_lightsource);

//	switch(new_door->type)
	switch(definition->type)
	{
		case _simple_split:
		case _simple_up:
		case _simple_down:
			new_vertical_door(new_door);
			break;
		case _simple_left:
		case _simple_left_up:
		case _simple_left_down:
		case _simple_left_split:
		case _simple_right:
		case _simple_right_up:
		case _simple_right_down:
		case _simple_right_split:
//			new_horizontal_door(new_door);
			break;
		default:
			halt();
			break;
	}
	return door_index;
}

void fix_door_textures(
	void)
{
}

boolean player_has_access(
	struct player_data *player, 
	short door_index)
{
#pragma unused (player, door_index)
	return TRUE;
}

/* time_elapsed is always 1. */
void update_active_doors(
	void)
{
	register short i;
	short state;
	struct door_data *door;
	struct door_definition *definition;
	struct polygon_data *polygon;
	fixed fraction;
	fixed shadow_fraction;
	short new_floor_height, new_ceiling_height;
	short original_ceiling_height;
	
	door= doors;	
	for (i=0;i<dynamic_world->door_count;++i)
	{
		/* If we care about this door.. */
		if ((state= GET_DOOR_STATE(door))!=_door_closed)
		{
			polygon= get_polygon_data(door->owner_index);
			door->ticks -= 1;
			
			/* Used to update the y0 of the texture, if it is a split/up door. */
			original_ceiling_height= polygon->ceiling_height;
			definition= get_door_definition(door->type);
			
			/* IF we need to change states.. */
			if(door->ticks<0)
			{
				switch (state)
				{
					case _door_opening:
						if(change_polygon_height(door->owner_index, door->opened_floor_height, door->opened_ceiling_height))
						{
							SET_DOOR_STATE(door, _door_open);
							
							door->ticks= definition->ticks_until_close;
	
							/* Play the all the way open sound. */
							play_polygon_sound(door->owner_index, definition->open_stop_sound);
	
							/* Fix the door texture origins.. */
							fix_door_y_origins(door, polygon->ceiling_height-original_ceiling_height);
	
							/* Fix the door texture origins.. */
//							fix_door_y_origins(door, polygon->ceiling_height-original_ceiling_height);

							/* Change the polygon's endpoint data... */
							update_polygon_endpoint_data_for_height_change(
								door->owner_index, door->update_endpoint_data);
						} else {
							/* We were unable to change the height of the door... */
							door->ticks+= 1;
						}
						break;

					case _door_closing:
						if(change_polygon_height(door->owner_index, door->closed_floor_height, door->closed_ceiling_height))
						{
							SET_DOOR_STATE(door, _door_closed);
							set_door_solidity(door, TRUE);
		
							/* Set the shadow to max.. */
							play_polygon_sound(door->owner_index, definition->close_stop_sound);
	
							/* Fix the door texture origins.. */
							fix_door_y_origins(door, polygon->ceiling_height-original_ceiling_height);

							/* Change the polygon's endpoint data... */
							update_polygon_endpoint_data_for_height_change(
								door->owner_index, door->update_endpoint_data);
						} else {
							/* Unable to change the height.. */
							door->ticks+= 1;
						}
						break;
				
					case _door_open:
						/* this door is closing; call change_door_state so we get free sounds
							and obstacle checking */
						if (!definition->door_stays_put)
							change_door_state(i, _door_closing);
						break;
					
					default:
						halt();
				}
				
				/* Update the shadow.. */
				if(state==_door_closed || state==_door_open)
				{
					shadow_fraction= get_shadow_fraction(door);

					if(door->shadow_index != NONE)
					{
						update_shadow(door, shadow_fraction);
					}
				}
			} else {
				/* Manipulate things.. */
				switch(state)
				{
					case _door_opening:
					case _door_closing:
						/* Interpolate the polygon heights based on ticks */
						fraction= get_door_fraction(door);
						shadow_fraction= get_shadow_fraction(door);

						new_floor_height= door->closed_floor_height-
							linear_interpolate(door->opened_floor_height, door->closed_floor_height, fraction);

						new_ceiling_height= door->closed_ceiling_height-
							linear_interpolate(door->opened_ceiling_height, door->closed_ceiling_height, fraction);

						/* If we were able to change the height... */
						if(change_polygon_height(door->owner_index, new_floor_height, new_ceiling_height))
						{

							/* Update the shadow polygons */
							if(door->shadow_index != NONE)
							{
								update_shadow(door, shadow_fraction);
							}
	
							/* Fix the door texture origins.. */
							fix_door_y_origins(door, polygon->ceiling_height-original_ceiling_height);

							/* Change the polygon's endpoint data... */
							update_polygon_endpoint_data_for_height_change(
								door->owner_index, door->update_endpoint_data);
						} else {
							/* We were unable to change the height of the door... */
							door->ticks+= 1;
						}
						break;
						
					case _door_open:
						break;
						
					case _door_closed:
						halt();
						break;
				}
			}
		}
		door++;
	}
	
	return;
}

boolean player_can_open_door_without_aid(short door_index)
{
	boolean can_open= TRUE;
	struct door_data *door= doors+door_index;
	struct door_definition *definition;
	
	definition = get_door_definition(door->type);
	if (definition->door_stays_put)
	{
		play_polygon_sound(door->owner_index, definition->cant_close_sound);
		can_open= FALSE;
	}
	
	return can_open;
}

boolean monster_can_control_door(short polygon_index)
{
	boolean can_control= TRUE;
	struct door_data *door;
	short ii;
	
	for(ii=0, door= doors; ii<dynamic_world->door_count; ++ii, ++door)
	{
		if(door->owner_index==polygon_index)
		{
			struct door_definition *definition;
			
			/* Check for security clearance or something.. */
			definition= get_door_definition(door->type);
			if (definition->door_stays_put) can_control= FALSE;
		}
	}
	
	return can_control;
}

void change_door_state(
	short index,
	short new_status)
{
	short door_state;
	struct door_data *door;
	struct door_definition *definition;

	assert(new_status==_door_opening||new_status==_door_closing || new_status==_door_toggle);
	assert(index>=0 && index<dynamic_world->door_count);
	
	door= doors+index;
	door_state= GET_DOOR_STATE(door);

	definition= get_door_definition(door->type);

	if(new_status==_door_toggle)
	{
		/* Toggle the door state. */
		switch(door_state)
		{
			case _door_open:
			case _door_opening:
				new_status= _door_closing;
				break;
				
			case _door_closed:
			case _door_closing:
				new_status= _door_opening;
				break;
				
			default:
				halt();
				break;
		}
	}

	switch(new_status)
	{
		case _door_opening:
			switch(door_state)
			{
				case _door_open:
					/* Allow monsters or players to hold doors open */
					door->ticks= definition->ticks_until_close;
					break;
					
				case _door_closed:
					play_polygon_sound(door->owner_index, definition->opening_sound);
					SET_DOOR_STATE(door, new_status);
					SET_DOOR_BLOCKED_FLAG(door, FALSE);
					door->ticks= definition->ticks_to_open;
					set_door_solidity(door, FALSE);
					break;
					
				case _door_closing:
					play_polygon_sound(door->owner_index, definition->opening_sound);
					SET_DOOR_STATE(door, new_status);
					SET_DOOR_BLOCKED_FLAG(door, FALSE);
					/* Reverse the tick count for Lerp'ing */
					door->ticks= definition->ticks_to_open-door->ticks;
					break;
					
				case _door_opening:
					/* Don't do anything */
					break;
			}
			break;
			
		case _door_closing:
			switch(door_state)
			{
				case _door_open:
					if(door_unobstructed(index))
					{
						play_polygon_sound(door->owner_index, definition->closing_sound);
						SET_DOOR_STATE(door, new_status);
						SET_DOOR_BLOCKED_FLAG(door, FALSE);
						door->ticks= definition->ticks_to_open;
					} else {
						if (!DOOR_WAS_BLOCKED(door)) 
							play_polygon_sound(door->owner_index, definition->cant_close_sound);
						SET_DOOR_BLOCKED_FLAG(door, TRUE);
						door->ticks= definition->ticks_until_close;
					}
					break;

				case _door_opening:
					/* close the door */
					if(door_unobstructed(index))
					{
						play_polygon_sound(door->owner_index, definition->closing_sound);
						SET_DOOR_STATE(door, new_status);
						SET_DOOR_BLOCKED_FLAG(door, FALSE);
						door->ticks= definition->ticks_to_open-door->ticks;
					}
					break;
					
				case _door_closing:
				case _door_closed:
					/* Don't do anything */
					break;
			}
				
		default:
			break;
	}
	
	return;
}

boolean door_is_open(
	short door_index)
{
	struct door_data *door;
	
	assert(door_index>=0 && door_index<dynamic_world->door_count);
	door= doors+door_index;
	
	return (GET_DOOR_STATE(door)==_door_open);
}

short find_undetached_polygons_twin(
	short polygon_index)
{
	short undetached_polygon_index= NONE;
	struct door_data *door;
	short door_index;
	
	for (door_index=0,door=doors;door_index<dynamic_world->door_count;++door_index,++door)
	{
		/* Currently only normal doors can have shadows */
		if(polygon_index==door->shadow_index)
		{
			undetached_polygon_index= door->shadow_null_data.polygon_index;
			break;
		}
	}
	
	return undetached_polygon_index;
}

short find_detached_polygons_twin(
	short polygon_index)
{
	struct polygon_data *polygon= get_polygon_data(polygon_index);
	short loop;
	struct door_data *door;
	
	assert(POLYGON_IS_DETACHED(polygon));
	
	door= doors;
	for(loop=0; loop<dynamic_world->door_count; ++loop)
	{
		if(door->shadow_null_data.polygon_index==polygon_index) 
		{
			/* This is kinda tricky, because I don't store this anywhere */
			short shared_line_index= find_shared_line(door->shadow_null_data.polygon_index, 
				door->shadow_null_data.other_polygon_index);
			short index, opposite_polygon;
			struct polygon_data *polygon;
			short other_shared;
	
			opposite_polygon= find_adjacent_polygon(door->shadow_null_data.other_polygon_index, 
				shared_line_index);

			index= find_line_index_in_polygon(door->shadow_null_data.polygon_index, shared_line_index);
			polygon= get_polygon_data(door->shadow_null_data.polygon_index);
			index= (index+2)%polygon->vertex_count;

			other_shared= polygon->line_indexes[index];
			return find_adjacent_polygon(door->shadow_null_data.polygon_index, other_shared);
		}
		++door;
	}
	halt();
	
	return NONE;
}

/* ------------------------- Private functions */
static short linear_interpolate(
	short start, 
	short end, 
	fixed fraction)
{
	return FIXED_INTEGERAL_PART((end-start)*fraction);
}

static void new_vertical_door(
	struct door_data *door)
{
	struct polygon_data *door_polygon;
	struct polygon_data *shadow_polygon;
	struct polygon_data *new_shadow_polygon_data;
	short shared_line_index, line_location_in_shadow_polygon;
	short new_shadow_polygon;
	struct door_definition *definition = get_door_definition(door->type);
	
	door_polygon= get_polygon_data(door->owner_index);

	/* Since the heights of the door polygon have already been set, all we need to do here is */
	/* create the shadow if one was specified */
	
	/* If this polygon has a shadow, deal with it. Yuck */
	if(door->shadow_index!=NONE)
	{
		assert(door_polygon->vertex_count==4);

		shadow_polygon= get_polygon_data(door->shadow_index);
		assert(shadow_polygon->vertex_count==4);
		
		shared_line_index= find_shared_line(door->owner_index, door->shadow_index);
		line_location_in_shadow_polygon= find_line_index_in_polygon(
			door->shadow_index, shared_line_index);

		/* Shadow does funky things */
		new_shadow_polygon= create_null_polygon(door->shadow_index,	door->owner_index, shared_line_index, 
			&door->shadow_null_data);

		/* Set the lightsources.. */
		new_shadow_polygon_data= get_polygon_data(new_shadow_polygon);
//		switch(door->type)
		switch(definition->type)
		{
			case _simple_up:
				new_shadow_polygon_data->ceiling_lightsource_index= door->shadow_closed_lightsource;
				new_shadow_polygon_data->floor_lightsource_index= door->shadow_opened_lightsource;
				shadow_polygon->ceiling_lightsource_index= door->shadow_closed_lightsource;
				shadow_polygon->floor_lightsource_index= door->shadow_closed_lightsource;
				break;
				
			case _simple_down:
				new_shadow_polygon_data->ceiling_lightsource_index= door->shadow_opened_lightsource;
				new_shadow_polygon_data->floor_lightsource_index= door->shadow_closed_lightsource;
				shadow_polygon->ceiling_lightsource_index= door->shadow_closed_lightsource;
				shadow_polygon->floor_lightsource_index= door->shadow_closed_lightsource;
				break;
				
			case _simple_split:
				new_shadow_polygon_data->ceiling_lightsource_index= door->shadow_opened_lightsource;
				new_shadow_polygon_data->floor_lightsource_index= door->shadow_opened_lightsource;
				shadow_polygon->ceiling_lightsource_index= door->shadow_opened_lightsource;
				shadow_polygon->floor_lightsource_index= door->shadow_opened_lightsource;
				break;
				
			default: 
				halt(); 
				break;
		}
	}

	/* Finally, save off the endpoint data.. */
	save_polygon_endpoint_data(door->owner_index, door->update_endpoint_data);
}

/* Major workhorse function */
static short create_null_polygon(
	short polygon_index,
	short owner_index,
	short shared_line_index,
	struct null_polygon_data *null_data)
{
	struct polygon_data *polygon;
	struct polygon_data *null_polygon;
	struct line_data *shared_line;
	struct line_data *new_line, *temp_line;
	short null_polygon_index;
	short new_endpoint_index_0, new_endpoint_index_1;
	short original_index_0, original_index_1;
	short shared_line_duplicate_index;
	short other_point_0, other_point_1;
	short top_line, bottom_line, shared_line_location;
	short ii;

	polygon= get_polygon_data(polygon_index);
	shared_line= get_line_data(shared_line_index);

	/* Duplicate the original polygon */
	null_polygon_index= duplicate_map_polygon(polygon_index);

	/* Duplicate the shared line */
	shared_line_duplicate_index= duplicate_map_line(shared_line_index);
	new_line= get_line_data(shared_line_duplicate_index);
	SET_LINE_TRANSPARENCY(new_line, TRUE);
	SET_LINE_SOLIDITY(new_line, FALSE);
	new_line->clockwise_polygon_side_index= new_line->counterclockwise_polygon_side_index= NONE;

	/* It has no objects */
	null_polygon= get_polygon_data(null_polygon_index);
	null_polygon->first_object= NONE;

	/* Create the new endpoint_indexes */
	new_endpoint_index_0= duplicate_map_endpoint(shared_line->endpoint_indexes[0]);
	new_endpoint_index_1= duplicate_map_endpoint(shared_line->endpoint_indexes[1]);

	/* Change the endpoints of the new shared line */
	original_index_0= new_line->endpoint_indexes[0];
	original_index_1= new_line->endpoint_indexes[1];
	new_line->endpoint_indexes[0]= new_endpoint_index_0;
	new_line->endpoint_indexes[1]= new_endpoint_index_1;

	/* Change the shared line index in the old polygon */
	replace_line_in_polygon(polygon_index, shared_line_index, shared_line_duplicate_index);

	/* Change the line opposite the original shared line in the new polygon to the shared line */
	add_shared_line_to_opposite_of_null(null_polygon_index, polygon_index, shared_line_duplicate_index, 
		shared_line_index);

	/* At this point, the lines are all setup, but they are not connected at their endpoints. */
	/* remedy this problem */
	replace_all_point_references_in_polygon(polygon_index, original_index_0, new_endpoint_index_0);
	replace_all_point_references_in_polygon(polygon_index, original_index_1, new_endpoint_index_1);

	/* Fix the polygon reference of the little guy */
	replace_other_polygon_reference(shared_line_duplicate_index, polygon_index, null_polygon_index);

	/* Now fix the null polygon's lines */
	shared_line_location= find_line_index_in_polygon(polygon_index, shared_line_duplicate_index);
	top_line= (shared_line_location+1)%polygon->vertex_count;
	bottom_line= (shared_line_location+3)%polygon->vertex_count;

	other_point_0= clockwise_endpoint_in_line(polygon_index, polygon->line_indexes[top_line], 1);
	other_point_1= clockwise_endpoint_in_line(polygon_index, polygon->line_indexes[bottom_line], 0);

	if(clockwise_endpoint_in_line(polygon_index, polygon->line_indexes[top_line], 0)!=new_endpoint_index_0)
	{
		short temp;
		
		temp= other_point_0;
		other_point_0= other_point_1;
		other_point_1= temp;
	}

	replace_all_point_references_in_polygon(null_polygon_index, other_point_0, new_endpoint_index_0);
	replace_all_point_references_in_polygon(null_polygon_index, other_point_1, new_endpoint_index_1);

	/* Now set the duplicates to transparent */
	temp_line= get_line_data(polygon->line_indexes[top_line]);
	SET_LINE_TRANSPARENCY(temp_line, TRUE);
	SET_LINE_SOLIDITY(temp_line, FALSE);
	temp_line->clockwise_polygon_side_index= temp_line->counterclockwise_polygon_side_index= NONE;

	temp_line= get_line_data(polygon->line_indexes[bottom_line]);
	SET_LINE_TRANSPARENCY(temp_line, TRUE);
	SET_LINE_SOLIDITY(temp_line, FALSE);
	temp_line->clockwise_polygon_side_index= temp_line->counterclockwise_polygon_side_index= NONE;
	
	/* Now set all of the null_data */
	null_data->polygon_index= null_polygon_index;
	null_data->other_polygon_index= owner_index;
	null_data->insertion_points[0].point_indexes[0]= original_index_0;
	null_data->insertion_points[0].point_indexes[1]= other_point_0;
	null_data->insertion_points[0].new_point= new_endpoint_index_0;
	null_data->insertion_points[0].other_polygon= find_adjacent_polygon(polygon_index, polygon->line_indexes[top_line]);
	null_data->insertion_points[0].other_polygon= find_adjacent_polygon(polygon_index, 
		find_line_connects_endpoints(polygon_index, other_point_0, new_endpoint_index_0));

	null_data->insertion_points[1].point_indexes[0]= original_index_1;
	null_data->insertion_points[1].point_indexes[1]= other_point_1;
	null_data->insertion_points[1].new_point= new_endpoint_index_1;
	null_data->insertion_points[1].other_polygon= find_adjacent_polygon(polygon_index, polygon->line_indexes[bottom_line]);

	null_data->insertion_points[1].other_polygon= find_adjacent_polygon(polygon_index, 
		find_line_connects_endpoints(polygon_index, other_point_1, new_endpoint_index_1));

#if 0
dprintf("Polygon: %d Other: %d", null_data->polygon_index, null_data->other_polygon_index);
dprintf("0) Oi0: %d Oi1: %d np: %d op: %d", 
	null_data->insertion_points[0].point_indexes[0],
	null_data->insertion_points[0].point_indexes[1],
	null_data->insertion_points[0].new_point,
	null_data->insertion_points[0].other_polygon);
dprintf("1) Oi0: %d Oi1: %d np: %d op: %d", 
	null_data->insertion_points[1].point_indexes[0],
	null_data->insertion_points[1].point_indexes[1],
	null_data->insertion_points[1].new_point,
	null_data->insertion_points[1].other_polygon);

dprintf_poly(owner_index);
dprintf_poly(null_data->insertion_points[0].other_polygon);
dprintf_poly(null_data->insertion_points[1].other_polygon);
dprintf_poly(null_polygon_index);
#endif

	/* The null polygons start detached. */
	for(ii=0; ii<2; ++ii)
	{
		short line_index;
		
		line_index= find_line_connects_endpoints(null_data->insertion_points[ii].other_polygon,
			null_data->insertion_points[ii].point_indexes[1], null_data->insertion_points[ii].new_point);
		
		/* Replace the endpoint_index.. */
		replace_line_endpoint(line_index, null_data->insertion_points[ii].new_point,	
			null_data->insertion_points[ii].point_indexes[0]);		
	}
	SET_POLYGON_DETACHED_STATE(null_polygon, TRUE);
	
	fix_shared_line(null_polygon_index, owner_index, FALSE);

	recalculate_redundant_polygon_data(null_data->polygon_index);
	recalculate_redundant_polygon_data(null_data->other_polygon_index);
	recalculate_redundant_polygon_data(null_data->insertion_points[0].other_polygon);
	recalculate_redundant_polygon_data(null_data->insertion_points[1].other_polygon);

	return null_polygon_index;
}

static void replace_other_polygon_reference(
	short line_index,
	short polygon_index,
	short new_polygon_index)
{
	struct line_data *line;
	
	line= get_line_data(line_index);
	if(line->clockwise_polygon_owner==polygon_index)
	{
		line->counterclockwise_polygon_owner= new_polygon_index;
	} else {
		assert(line->counterclockwise_polygon_owner==polygon_index);
		line->clockwise_polygon_owner= new_polygon_index;
	}
}

static void replace_all_point_references_in_polygon(
	short polygon_index, 
	short old_endpoint_index,
	short new_endpoint_index)
{
	struct polygon_data *polygon;
	short ii;
	struct line_data *line;
	
	polygon= get_polygon_data(polygon_index);
	for(ii=0; ii<polygon->vertex_count; ++ii)
	{
		line= get_line_data(polygon->line_indexes[ii]);
		if(line->endpoint_indexes[0]==old_endpoint_index)
		{
			line->endpoint_indexes[0]= new_endpoint_index;
		}

		if(line->endpoint_indexes[1]==old_endpoint_index)
		{
			line->endpoint_indexes[1]= new_endpoint_index;
		}
	}
}

static void	add_shared_line_to_opposite_of_null(
	short null_polygon_index, 
	short original_polygon_index,
	short shared_line_duplicate_index, 
	short shared_line_index)
{
	struct polygon_data *polygon= get_polygon_data(null_polygon_index);
	short shared_line_location_in_polygon, opposite_line_index;
	short line_one, line_two;
	short new_line_one_index, new_line_two_index;

	/* Find the index of the line opposite the shared line */		
	shared_line_location_in_polygon= find_line_index_in_polygon(null_polygon_index, 
		shared_line_index);
	
	/* Change the opposite side to the proper line index */	
	opposite_line_index= (shared_line_location_in_polygon+2)%polygon->vertex_count;
	polygon->line_indexes[opposite_line_index]= shared_line_duplicate_index;

	/* Find the helper lines indexes */
	line_one= (shared_line_location_in_polygon+1)%polygon->vertex_count;
	line_two= (shared_line_location_in_polygon+3)%polygon->vertex_count;
	
	/* Create them */
	new_line_one_index= duplicate_map_line(polygon->line_indexes[line_one]);
	new_line_two_index= duplicate_map_line(polygon->line_indexes[line_two]);

	/* Stick them in the polygon */
	polygon->line_indexes[line_one]= new_line_one_index;	
	polygon->line_indexes[line_two]= new_line_two_index;
	
	/* Fix their polygon indexes.. */
	replace_line_polygon_index(new_line_one_index, original_polygon_index, null_polygon_index);
	replace_line_polygon_index(new_line_two_index, original_polygon_index, null_polygon_index);
	replace_line_polygon_index(shared_line_index, original_polygon_index, null_polygon_index);
}

static void replace_line_polygon_index(
	short line_index, 
	short old_polygon, 
	short new_polygon)
{
	struct line_data *line;
	
	line= get_line_data(line_index);
	if(line->clockwise_polygon_owner==old_polygon)
	{
		line->clockwise_polygon_owner= new_polygon;
	} else {
		assert(line->counterclockwise_polygon_owner==old_polygon);
		line->counterclockwise_polygon_owner= new_polygon;
	}
}

static void	replace_line_in_polygon(
	short polygon_index,
	short original_line_index,
	short new_line_index)
{
	short ii;
	struct polygon_data *polygon;
	
	polygon= get_polygon_data(polygon_index);
	for(ii=0; ii<polygon->vertex_count; ++ii)
	{
		if(polygon->line_indexes[ii]==original_line_index) 
		{
			polygon->line_indexes[ii]= new_line_index;
			break;
		}
	}
if(ii==polygon->vertex_count)
{
	dprintf("polygon_index: %d line_index: %d", polygon_index, original_line_index);
}
	assert(ii!=polygon->vertex_count);
}

short duplicate_map_polygon(
	short original_polygon_index)
{
	short polygon_index;
	struct polygon_data *polygon;
	struct polygon_data *original;

	assert(dynamic_world->polygon_count+1<MAXIMUM_POLYGONS_PER_MAP);
	polygon_index= dynamic_world->polygon_count++;
	polygon= get_polygon_data(polygon_index);
	original= get_polygon_data(original_polygon_index);
	
	*polygon= *original;
	
	return polygon_index;
}

static void	set_door_solidity(
	struct door_data *new_door, 
	boolean solid)
{
	short ii;
	struct polygon_data *polygon;
	struct line_data *line;
	
	polygon= get_polygon_data(new_door->owner_index);
	
	for(ii=0; ii<polygon->vertex_count; ++ii)
	{
		line= get_line_data(polygon->line_indexes[ii]);

		if(find_adjacent_polygon(new_door->owner_index, polygon->line_indexes[ii])!=NONE)
		{
			SET_LINE_TRANSPARENCY(line,!solid);
			SET_LINE_SOLIDITY(line, solid);
		} else {
			/* Lines that don't have polygons on the other side of them need to be set solid and */
			/* !transparent. */
			SET_LINE_TRANSPARENCY(line, FALSE);
			SET_LINE_SOLIDITY(line, TRUE);
		}
	}
}

static boolean door_unobstructed(
	short index)
{
	struct door_data *door;
	boolean unobstructed= FALSE;
	struct polygon_data *polygon;

	door= doors+index;
	polygon= get_polygon_data(door->owner_index);
	if (polygon->first_object==NONE)
	{
		unobstructed= TRUE;
	}
	
	return unobstructed;
}

#ifdef DEBUG
static struct door_definition *get_door_definition(short index)
{
	assert(index>=0 && index<NUMBER_OF_DOOR_DEFINITIONS);
	return door_definitions+index;
}
#endif

static void	set_door_ceiling_and_floor_heights(
	struct door_data *door)
{
	struct polygon_data *door_polygon;
	short split_point;
	struct door_definition *definition = get_door_definition(door->type);
	
	door_polygon= get_polygon_data(door->owner_index);
	assert(door_polygon->vertex_count==4);
	
	/* Set the heights of the door.. */
//	switch(door->type)
	switch(definition->type)
	{
		case _simple_up:
		case _simple_left_up:
		case _simple_right_up:
			door->opened_floor_height= 
				door->closed_floor_height= door_polygon->floor_height;
			door->opened_ceiling_height= door_polygon->ceiling_height;
			door->closed_ceiling_height= door_polygon->floor_height;
			break;

		case _simple_down:
		case _simple_left_down:
		case _simple_right_down:
			door->opened_ceiling_height= door->closed_ceiling_height=
				door_polygon->ceiling_height;
			door->opened_floor_height= door_polygon->floor_height;
			door->closed_floor_height= door_polygon->ceiling_height;
			break;

		case _simple_split:
		case _simple_left_split:
		case _simple_right_split:
			split_point= (door_polygon->ceiling_height-door_polygon->floor_height)/2;
			door->opened_ceiling_height= door_polygon->ceiling_height;
			door->closed_ceiling_height= door_polygon->ceiling_height-split_point;
			door->opened_floor_height= door_polygon->floor_height;
			door->closed_floor_height= door_polygon->floor_height+split_point;
			break;
			
		case _simple_left:
		case _simple_right:
			door->opened_ceiling_height= door->closed_ceiling_height= door_polygon->ceiling_height;
			door->opened_floor_height= door->closed_floor_height= door_polygon->floor_height;
			break;
	}
	
	door_polygon->floor_height= door->closed_floor_height;
	door_polygon->ceiling_height= door->closed_ceiling_height;
}

static short find_line_index_in_polygon(
	short polygon_index, 
	short line_index)
{
	struct polygon_data *polygon;
	short ii;
	
	polygon= get_polygon_data(polygon_index);
	for(ii=0; ii<polygon->vertex_count; ++ii)
	{
		if(polygon->line_indexes[ii]==line_index) break;
	}
	assert(ii!=polygon->vertex_count);

	return ii;
}

static void detach_null_polygon(
	struct null_polygon_data *data)
{
	struct polygon_data *polygon;
	struct polygon_data *other_polygon;
	short ii, line_index_in_polygon, line_index_in_null_polygon;
	
	polygon= get_polygon_data(data->polygon_index);
	other_polygon= get_polygon_data(data->other_polygon_index);
	
	if(!POLYGON_IS_DETACHED(polygon))
	{
		/* Insert the points in the other polygons */
		for(ii=0; ii<2; ++ii)
		{
			line_index_in_polygon= find_line_connects_endpoints(data->insertion_points[ii].other_polygon,
				data->insertion_points[ii].new_point, data->insertion_points[ii].point_indexes[1]);
			line_index_in_null_polygon= find_line_connects_endpoints(data->polygon_index,
				data->insertion_points[ii].point_indexes[0], data->insertion_points[ii].new_point);
	
			/* Replace the endpoint_index.. */
			replace_line_endpoint(line_index_in_polygon, data->insertion_points[ii].new_point,	
				data->insertion_points[ii].point_indexes[0]);
	
			/* Now delete the line */
			delete_line_in_polygon(data->insertion_points[ii].other_polygon, line_index_in_null_polygon);
		}
		SET_POLYGON_DETACHED_STATE(polygon, TRUE);
	
		fix_shared_line(data->polygon_index, data->other_polygon_index, FALSE);

		recalculate_redundant_polygon_data(data->polygon_index);
		recalculate_redundant_polygon_data(data->other_polygon_index);
		recalculate_redundant_polygon_data(data->insertion_points[0].other_polygon);
		recalculate_redundant_polygon_data(data->insertion_points[1].other_polygon);
	}
}

static void	delete_line_in_polygon(
	short polygon_index, 
	short line_index)
{
	struct polygon_data *polygon= get_polygon_data(polygon_index);
	short which= find_line_index_in_polygon(polygon_index, line_index);
	
	assert(polygon->vertex_count>=1);
	memmove(&polygon->line_indexes[which], &polygon->line_indexes[which+1], (polygon->vertex_count-which-1)*sizeof(short));
	polygon->vertex_count--;
}

static void	replace_line_endpoint(
	short line_index, 
	short old_point, 
	short new_point)
{
	struct line_data *line= get_line_data(line_index);
	if(line->endpoint_indexes[0]==old_point)
	{
		line->endpoint_indexes[0]= new_point;
	} else {
		assert(line->endpoint_indexes[1]==old_point);
		line->endpoint_indexes[1]= new_point;
	}
}

static void detach_connector_lines(
	short null_polygon_index, 
	short original_line_index, 
	short other_line)
{
	short index, opposite_polygon;
	struct polygon_data *polygon;
	
	index= find_line_index_in_polygon(null_polygon_index, original_line_index);
	polygon= get_polygon_data(null_polygon_index);
	index= (index+2)%polygon->vertex_count;
	
	opposite_polygon= find_adjacent_polygon(null_polygon_index, 
		polygon->line_indexes[index]);
	
	replace_line_polygon_index(other_line, null_polygon_index, opposite_polygon);
}

static void	fix_shared_line(
	short null_polygon_index, 
	short other_polygon_index, 
	boolean attaching)
{
	short shared_line_index= find_shared_line(null_polygon_index, other_polygon_index);
	short index, opposite_polygon;
	struct polygon_data *polygon, *other_polygon;
	short other_shared;
	
	if(attaching)
	{
		opposite_polygon= find_adjacent_polygon(other_polygon_index, shared_line_index);

		index= find_line_index_in_polygon(null_polygon_index, shared_line_index);
		polygon= get_polygon_data(null_polygon_index);
		index= (index+2)%polygon->vertex_count;

		other_shared= polygon->line_indexes[index];
		index= find_line_index_in_polygon(opposite_polygon, shared_line_index);
		
		other_polygon= get_polygon_data(opposite_polygon);
		other_polygon->line_indexes[index]= other_shared;

		replace_other_polygon_reference(shared_line_index, other_polygon_index, null_polygon_index);
	} else {
		index= find_line_index_in_polygon(null_polygon_index, shared_line_index);
		polygon= get_polygon_data(null_polygon_index);
		index= (index+2)%polygon->vertex_count;

		opposite_polygon= find_adjacent_polygon(null_polygon_index, polygon->line_indexes[index]);

		replace_line_polygon_index(shared_line_index, null_polygon_index, opposite_polygon);

		/* And finally, fix the line opposite the shared line. */
		other_polygon= get_polygon_data(opposite_polygon);
		other_shared= find_shared_line(opposite_polygon, null_polygon_index);
		index= find_line_index_in_polygon(opposite_polygon, other_shared);
		other_polygon->line_indexes[index]= shared_line_index;
	}
	recalculate_redundant_polygon_data(opposite_polygon);
}

static void attach_null_polygon(
	struct null_polygon_data *data)
{
	struct polygon_data *polygon, *other_polygon;
	short ii, line_index_in_polygon, line_index_in_null_polygon;
	short index_at_line;
	
	polygon= get_polygon_data(data->polygon_index);
	
	if(POLYGON_IS_DETACHED(polygon))
	{
		/* Insert the points in the other polygons */
		for(ii=0; ii<2; ++ii)
		{
			other_polygon= get_polygon_data(data->insertion_points[ii].other_polygon);
			line_index_in_polygon= find_line_connects_endpoints(data->insertion_points[ii].other_polygon,
				data->insertion_points[ii].point_indexes[0], data->insertion_points[ii].point_indexes[1]);
	
			line_index_in_null_polygon= find_line_connects_endpoints(data->polygon_index,
				data->insertion_points[ii].point_indexes[0], data->insertion_points[ii].new_point);
			
			/* Change the endpoint_index */
			replace_line_endpoint(line_index_in_polygon, data->insertion_points[ii].point_indexes[0],
				data->insertion_points[ii].new_point);
	
			/* Now insert the line */
			index_at_line= find_line_index_in_polygon(data->insertion_points[ii].other_polygon, line_index_in_polygon);

			/* If the zero vertex of the line equals the new point, insert before it, otherwise insert after */
			if(clockwise_endpoint_in_line(data->insertion_points[ii].other_polygon, line_index_in_polygon, 0)==data->insertion_points[ii].new_point)
			{
			} else {
				index_at_line= (index_at_line+1)%other_polygon->vertex_count;
			}

			insert_line_in_polygon(data->insertion_points[ii].other_polygon, line_index_in_null_polygon, 
				index_at_line);
		}
		SET_POLYGON_DETACHED_STATE(polygon, FALSE);

		/* Finally, change the polygon reference across the shared line */
		fix_shared_line(data->polygon_index, data->other_polygon_index, TRUE);

		recalculate_redundant_polygon_data(data->polygon_index);
		recalculate_redundant_polygon_data(data->other_polygon_index);
		recalculate_redundant_polygon_data(data->insertion_points[0].other_polygon);
		recalculate_redundant_polygon_data(data->insertion_points[1].other_polygon);
	}
}


static void	update_shadow(
	struct door_data *door, 
	fixed fraction)
{
	struct endpoint_data *slide, *start, *end;
	struct polygon_data *original_shadow_polygon;
	short ii;
	struct door_definition *definition = get_door_definition(door->type);
	
	/* Lerp */
	for(ii=0; ii<2; ++ii)
	{
		slide= get_endpoint_data(door->shadow_null_data.insertion_points[ii].new_point);
		start= get_endpoint_data(door->shadow_null_data.insertion_points[ii].point_indexes[0]);
		end= get_endpoint_data(door->shadow_null_data.insertion_points[ii].point_indexes[1]);
		slide->vertex.x= start->vertex.x + linear_interpolate(start->vertex.x, end->vertex.x, fraction);
		slide->vertex.y= start->vertex.y + linear_interpolate(start->vertex.y, end->vertex.y, fraction);
	}

	/* Max opened. */
	if(slide->vertex.x==end->vertex.x && slide->vertex.y==end->vertex.y)
	{
		original_shadow_polygon= get_polygon_data(door->shadow_index);
//		switch(door->type)
		switch(definition->type)
		{
			case _simple_split:
				original_shadow_polygon->ceiling_lightsource_index= door->shadow_opened_lightsource;
				original_shadow_polygon->floor_lightsource_index= door->shadow_opened_lightsource;
				break;
				
			case _simple_up:
				original_shadow_polygon->ceiling_lightsource_index= door->shadow_closed_lightsource;
				original_shadow_polygon->floor_lightsource_index= door->shadow_opened_lightsource;
				break;
				
			case _simple_down:
				original_shadow_polygon->ceiling_lightsource_index= door->shadow_opened_lightsource;
				original_shadow_polygon->floor_lightsource_index= door->shadow_closed_lightsource;
				break;
		}

		detach_null_polygon(&door->shadow_null_data);
	} 
	else if(slide->vertex.x==start->vertex.x && slide->vertex.x==start->vertex.x)
	{
		/* max closed */
		/* Detach the old shadow polygon */
		detach_null_polygon(&door->shadow_null_data);
	} 
	else 
	{
		/* Sliding */
		fix_sliding_lightsources(door);
		
		/* Make sure it is attached. */
		attach_null_polygon(&door->shadow_null_data);		
	}

	/* Update the positions */
	fix_poly_changed_objects(door->shadow_null_data.polygon_index, door->shadow_index);
}


static void fix_sliding_lightsources(struct door_data *door)
{
	struct polygon_data *original_shadow_polygon= get_polygon_data(door->shadow_index);
	struct door_definition *definition = get_door_definition(door->type);

//	switch(door->type)
	switch(definition->type)
	{
		case _simple_split:
		case _simple_down:
		case _simple_up:
			original_shadow_polygon->ceiling_lightsource_index= door->shadow_closed_lightsource;
			original_shadow_polygon->floor_lightsource_index= door->shadow_closed_lightsource;
			break;
			
		default:
			halt();
			break;
	}
}


static short find_line_connects_endpoints(
	short polygon_index, 
	short point_0, 
	short point_1)
{
	short ii, line_index;
	struct polygon_data *polygon;
	struct line_data *line;
	
	polygon= get_polygon_data(polygon_index);
	for(ii=0; ii<polygon->vertex_count; ++ii)
	{
		line_index= polygon->line_indexes[ii];
		line= get_line_data(line_index);

		if((line->endpoint_indexes[0]==point_0 && line->endpoint_indexes[1]==point_1) ||
		   (line->endpoint_indexes[1]==point_0 && line->endpoint_indexes[0]==point_1))
		{
			break;
		}
	}
	assert(ii!=polygon->vertex_count);
	
	return line_index;
}

static void	insert_line_in_polygon(
	short polygon_index, 
	short line_index, 
	short where)
{
	struct polygon_data *polygon= get_polygon_data(polygon_index);
	short last_line, ii, temp;
	
	assert(polygon->vertex_count+1<MAXIMUM_VERTICES_PER_POLYGON);
	last_line= line_index;
	for(ii= where; ii<polygon->vertex_count; ++ii)
	{
		temp= polygon->line_indexes[ii];
		polygon->line_indexes[ii]= last_line;
		last_line= temp;
	}
	polygon->line_indexes[ii]= last_line;
	
	polygon->vertex_count++;
}

static void dprintf_poly(
	short which)
{
	short loop;
	struct polygon_data *polygon;
	struct line_data *temp;
	
	polygon= get_polygon_data(which);
	dprintf("---- Poly %d ----", which);
	for(loop=0; loop<polygon->vertex_count; ++loop)
	{
		temp= get_line_data(polygon->line_indexes[loop]);
		dprintf("Line: %d (%d) 0: %d 1: %d rel to: %d other: %d", 
			polygon->line_indexes[loop], loop, temp->endpoint_indexes[0], temp->endpoint_indexes[1], 
			temp->clockwise_polygon_owner, temp->counterclockwise_polygon_owner);
	}
	dprintf("---- Done Poly %d ----", which);
}

static void fix_poly_changed_objects(
	short one_index, 
	short two_index)
{
	struct object_data *object;
	short next_object;
	struct polygon_data *one= get_polygon_data(one_index);
	struct polygon_data *two= get_polygon_data(two_index);
	short new_one_list, new_two_list, temp_first;		

	if(POLYGON_IS_DETACHED(one) || POLYGON_IS_DETACHED(two))
	{
		if(POLYGON_IS_DETACHED(one))
		{
			/* Connect everything to Two.. */
			next_object= one->first_object;
			one->first_object= NONE;
			
			while(next_object != NONE)
			{
				object= get_object_data(next_object);
				
				/* Change to polygon two. */
				object->polygon= two_index;
				temp_first= object->next_object;
				object->next_object= two->first_object;
				two->first_object= next_object;

				next_object= temp_first;
			}
		} else {
			/* Connect everything to One.. */
			next_object= two->first_object;
			two->first_object= NONE;

			while(next_object != NONE)
			{
				object= get_object_data(next_object);

				/* Change to polygon 1 */
				object->polygon= one_index;				
				temp_first= object->next_object;
				object->next_object= one->first_object;
				one->first_object= next_object;

				next_object= temp_first;
			}
		}
	} else {
		new_one_list= new_two_list= NONE;

		while(one->first_object != NONE)
		{
			object= get_object_data(one->first_object);
			if(point_in_polygon(one_index, (world_point2d *) &object->location))
			{
				temp_first= object->next_object;
				object->next_object= new_one_list;
				object->polygon= one_index;
				new_one_list= one->first_object;
			} else {
// Point in polygon is not accurate enough to assert on.
//				assert(point_in_polygon(two_index, (world_point2d *) &object->location));
				temp_first= object->next_object;
				object->next_object= new_two_list;
				object->polygon= two_index;
				new_two_list= one->first_object;
			}
			one->first_object= temp_first;
		}
		
		/* Now search the other polygon's objects */
		while(two->first_object != NONE)
		{
			object= get_object_data(two->first_object);
			if(point_in_polygon(one_index, (world_point2d *) &object->location))
			{
				temp_first= object->next_object;
				object->next_object= new_one_list;
				object->polygon= one_index;
				new_one_list= two->first_object;
			} else {
// Point in polygon is not accurate enough to assert on.
//				assert(point_in_polygon(two_index, (world_point2d *) &object->location));
				temp_first= object->next_object;
				object->next_object= new_two_list;
				object->polygon= two_index;
				new_two_list= two->first_object;
			}
			two->first_object= temp_first;
		}
		
		/* Now add their lists. */
		one->first_object= new_one_list;
		two->first_object= new_two_list;
	}
}

#ifdef OBSOLETE
#define FIX_POLYGON_COUNT 4
#define MAXIMUM_FIX_OBJECT_COUNT 128

static void fix_poly_changed_objects(
	short one_index, 
	short two_index)
{
	struct polygon_data *polygon= get_polygon_data(one_index);
	short polygon_indexes[FIX_POLYGON_COUNT];
	short object_indexes[MAXIMUM_FIX_OBJECT_COUNT];
	struct object_data *object;
	short object_count;
	short i, j;
	
	/* get the four polygons we may have to move objects between */
	for (i= 0; i<polygon->vertex_count; ++i)
	{
		if (polygon->adjacent_polygon_indexes[i]==two_index) break;
	}
	assert(i!=polygon->vertex_count);
	polygon_indexes[0]= one_index;
	polygon_indexes[1]= two_index;
	polygon_indexes[2]= polygon->adjacent_polygon_indexes[i ? i-1 : polygon->vertex_count-1];
	polygon_indexes[3]= polygon->adjacent_polygon_indexes[i==polygon->vertex_count-1 ? 0 : i+1];

	/* accumulate a list of objects we might have to move */
	object_count= 0;
	for (i= 0; i<FIX_POLYGON_COUNT; ++i)
	{
		if (polygon_indexes[i]!=NONE)
		{
			short object_index;
			
			object= (struct object_data *) NULL;
			polygon= get_polygon_data(polygon_indexes[i]);
			for (object_index= polygon->first_object; object_index!=NONE; object_index= object->next_object)
			{
				assert(object_index<MAXIMUM_FIX_OBJECT_COUNT);
				object_indexes[object_count++]= object_index;
				object= get_object_data(object_index);
			}
			
			polygon->first_object= NONE;
		}
	}

	/* on the movement pass, ignore detached polygons */
	for (i= 0; i<FIX_POLYGON_COUNT; ++i)
	{
		if (POLYGON_IS_DETACHED(get_polygon_data(polygon_indexes[i]))) polygon_indexes[i]= NONE;
	}
//	dprintf("polygon list;dm %x %x", polygon_indexes, sizeof(short)*4);
	
	/* check every object against all polygons to see where we end up; theoretically we should
		call monster_moved for all monsters which switch polygons, but actually that is only (?)
		useful for pathfinding and stuff which shouldn’t really need to know about small changes
		like this anyway */
	for (i= 0; i<object_count; ++i)
	{
		object= get_object_data(object_indexes[i]);
		
		for (j= 0; j<FIX_POLYGON_COUNT; ++j)
		{
			if (polygon_indexes[j]!=NONE)
			{
				if (point_in_polygon(polygon_indexes[j], (world_point2d *)&object->location))
				{
					if (object->polygon!=polygon_indexes[j]) dprintf("moved #%d from #%d to #%d;g;", object_indexes[i], object->polygon, polygon_indexes[j]);
					/* found where we belong */
					object->polygon= polygon_indexes[j];
					polygon= get_polygon_data(polygon_indexes[j]);
					object->next_object= polygon->first_object;
					polygon->first_object= object_indexes[i];
					break;
				}
			}
		}
		if (j==FIX_POLYGON_COUNT) dprintf("could not place #%d (was in #%d)", object_indexes[i], object->polygon);
		assert(j!=FIX_POLYGON_COUNT);
	}
	
	return;
}
#endif

static fixed get_door_fraction(
	struct door_data *door)
{
	fixed fraction;
	struct door_definition *definition= get_door_definition(door->type);
	
	switch(GET_DOOR_STATE(door))
	{
		case _door_open:
			fraction= FIXED_ONE;
			break;
			
		case _door_closed:
			fraction= INTEGER_TO_FIXED(0);
			break;
			
		case _door_opening:
			fraction= INTEGER_TO_FIXED(definition->ticks_to_open-door->ticks)/
				definition->ticks_to_open;
			break;
			
		case _door_closing:
			fraction= FIXED_ONE- (INTEGER_TO_FIXED(definition->ticks_to_open-door->ticks)/
				definition->ticks_to_open);
			break;
			
		default:
			halt();
			break;
	}
	
	return fraction;
}

static fixed get_shadow_fraction(
	struct door_data *door)
{
	fixed fraction;
	struct door_definition *definition= get_door_definition(door->type);
	
//	switch(door->type)
	switch(definition->type)
	{
		case _simple_up:
		case _simple_down:
			switch(GET_DOOR_STATE(door))
			{
				case _door_open:
					fraction= FIXED_ONE;
					break;
					
				case _door_closed:
					fraction= INTEGER_TO_FIXED(0);
					break;
					
				case _door_opening:
					fraction= INTEGER_TO_FIXED(definition->ticks_to_open-door->ticks)/
						definition->ticks_to_open;
					break;
					
				case _door_closing:
					fraction= FIXED_ONE- (INTEGER_TO_FIXED(definition->ticks_to_open-door->ticks)/
						definition->ticks_to_open);
					break;
					
				default:
					halt();
					break;
			}
			break;

		case _simple_split:
			switch(GET_DOOR_STATE(door))
			{
				case _door_open:
					fraction= INTEGER_TO_FIXED(0);
					break;
					
				case _door_closed:
					fraction= FIXED_ONE;
					break;
					
				case _door_opening:
					fraction= FIXED_ONE- (INTEGER_TO_FIXED(definition->ticks_to_open-door->ticks)/
						definition->ticks_to_open);
					break;
					
				case _door_closing:
					fraction= INTEGER_TO_FIXED(definition->ticks_to_open-door->ticks)/
						definition->ticks_to_open;
					break;
					
				default:
					halt();
					break;
			}
			break;

		default:
			halt();
			break;
	}
	
	return fraction;
}

static void set_door_shadow_lightsources(
	struct door_data *door, 
	short shadow_index, 
	short shadow_lightsource)
{
	struct polygon_data *original_shadow;
	struct door_definition *definition = get_door_definition(door->type);
	
	if(shadow_index != NONE)
	{
		original_shadow= get_polygon_data(shadow_index);
//		switch(door->type)
		switch(definition->type)
		{
			case _simple_split:
				door->shadow_closed_lightsource= shadow_lightsource;
				door->shadow_opened_lightsource= original_shadow->floor_lightsource_index;
				break;
				
			case _simple_up:
			case _simple_down:
				door->shadow_closed_lightsource= original_shadow->floor_lightsource_index;
				door->shadow_opened_lightsource= shadow_lightsource;
				break;
				
			default:
				halt();
				break;
		}
	}
}

static short door_texture_type(
	short type)
{
	short i;
	short texture_type;
	struct door_definition *definition;
	
	for (i = 0; i < NUMBER_OF_DOOR_DEFINITIONS; i++)
	{
		definition = get_door_definition(i);
		if (definition->type == type)
			break;
	}
	assert (i != NUMBER_OF_DOOR_DEFINITIONS);

	
	switch(definition->type)
	{
		case _simple_up:
			texture_type= _high_side;
			break;
			
		case _simple_down:
			texture_type= _low_side;
			break;
			
		case _simple_split:
			texture_type= _split_side;
			break;
			
		default:
			halt();
			break;
	}
	
	return texture_type;
}

static void	fix_door_y_origins(
	struct door_data *door, 
	short ceiling_delta)
{
	short ii;
	struct polygon_data *polygon= get_polygon_data(door->owner_index);
	struct line_data *line;
	struct side_data *side;
	struct door_definition *definition= get_door_definition(door->type);

	if(definition->type==_simple_up || definition->type==_simple_split)
	{
		for(ii=0; ii<polygon->vertex_count; ++ii)
		{
			if(find_adjacent_polygon(door->owner_index, polygon->line_indexes[ii])!=NONE)
			{
				line= get_line_data(polygon->line_indexes[ii]);

				if(line->clockwise_polygon_owner==door->owner_index)
				{
					side= get_side_data(line->counterclockwise_polygon_side_index);
					side->primary_texture.y0 += ceiling_delta;
				} else {
					assert(line->counterclockwise_polygon_owner==door->owner_index);
					side= get_side_data(line->clockwise_polygon_side_index);
					side->primary_texture.y0 += ceiling_delta;
				}
			}
		}
	}
}

#ifdef OBSOLETE
/* time_elapsed is always 1. */
void update_active_doors(
	void)
{
	register short i;
	short state;
	struct door_data *door;
	struct door_definition *definition;
	struct polygon_data *polygon;
	fixed fraction;
	fixed shadow_fraction;
	short original_ceiling_height;
	
	door= doors;	
	for (i=0;i<dynamic_world->door_count;++i)
	{
		if ((state= GET_DOOR_STATE(door))!=_door_closed)
		{
			polygon= get_polygon_data(door->owner_index);
			door->ticks -= 1;
			/* Used to update the y0 of the texture, if it is a split/up door. */
			original_ceiling_height= polygon->ceiling_height;
			definition= get_door_definition(door->type);
			switch (state)
			{
				case _door_opening:
					if(door->ticks<0) /* Are we all the way open? */
					{
						SET_DOOR_STATE(door, _door_open);
						
						polygon->floor_height= door->opened_floor_height;
						polygon->ceiling_height= door->opened_ceiling_height;
	
						door->ticks= definition->ticks_until_close;

						shadow_fraction= get_shadow_fraction(door);

						/* Play the all the way open sound. */
						play_polygon_sound(door->owner_index, definition->open_stop_sound);
					}
					else
					{
						/* Interpolate the polygon heights based on ticks */
						fraction= get_door_fraction(door);
						shadow_fraction= get_shadow_fraction(door);

						polygon->floor_height= door->closed_floor_height-
							linear_interpolate(door->opened_floor_height, door->closed_floor_height, fraction);

						polygon->ceiling_height= door->closed_ceiling_height-
							linear_interpolate(door->opened_ceiling_height, door->closed_ceiling_height, fraction);
					}
					
					if(door->shadow_index != NONE)
					{
						update_shadow(door, shadow_fraction);
					}
					break;

				case _door_closing:
					if (door->ticks<0) /* are we all the way closed? */
					{
						SET_DOOR_STATE(door, _door_closed);
						set_door_solidity(door, TRUE);
						
						polygon->floor_height= door->closed_floor_height;
						polygon->ceiling_height= door->closed_ceiling_height;
	
						/* Set the shadow to max.. */
						shadow_fraction= get_shadow_fraction(door);
						play_polygon_sound(door->owner_index, definition->close_stop_sound);
					}
					else
					{
						fraction= get_door_fraction(door);
						shadow_fraction= get_shadow_fraction(door);

						polygon->floor_height= door->closed_floor_height-
							linear_interpolate(door->opened_floor_height, door->closed_floor_height, fraction);

						polygon->ceiling_height= door->closed_ceiling_height-
							linear_interpolate(door->opened_ceiling_height, door->closed_ceiling_height, fraction);
					}

					if(door->shadow_index != NONE)
					{
						update_shadow(door, shadow_fraction);
					}
					break;
				
				case _door_open:
					if(door->ticks<0)
					{
						/* this door is closing; call change_door_state so we get free sounds
							and obstacle checking */
						change_door_state(i, _door_closing);
					}
					break;
					
				default:
					halt();
			}
			
			fix_door_y_origins(door, polygon->ceiling_height-original_ceiling_height);
		}
		door++;
	}
	
	return;
}
#endif
#endif