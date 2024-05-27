/*
MARATHON.C
Friday, December 3, 1993 10:00:32 AM

Monday, September 5, 1994 2:42:28 PM (ajr)
	fixed kill_limit.
Saturday, September 17, 1994 6:04:59 PM   (alain)
	fixed autotriggering of platforms
Thursday, December 8, 1994 3:58:12 PM  (Jason)
	only players trigger platforms.
*/

#include "cseries.h"
#include "map.h"
#include "render.h"
#include "interface.h"
#include "flood_map.h"
#include "effects.h"
#include "monsters.h"
#include "projectiles.h"
#include "player.h"
#include "network.h"
#include "scenery.h"
#include "platforms.h"
#include "lightsource.h"
#include "music.h"

#ifdef mpwc
#pragma segment marathon
#endif

/* ---------- constants */

/* ---------- private prototypes */

static void game_timed_out(void);
static boolean game_is_over(void);

/* ---------- code */

void initialize_marathon(
	void)
{
#ifndef DEMO /* no external physics models for demo */
//	import_definition_structures();
#endif

	build_trig_tables();
	allocate_map_memory();
	allocate_render_memory();
	allocate_pathfinding_memory();
	allocate_flood_map_memory();
	allocate_texture_tables();
	initialize_weapons();
	initialize_game_window();

	return;
}

long NetGetNetTime(void);

short update_world(
	void)
{
	short lowest_time, highest_time;
	short i, time_elapsed;
	short player_index;

	/* find who has the most and the least queued action flags (we can only advance the world
		as far as we have action flags for every player).  the difference between the most and
		least queued action flags should be bounded in some way by the maximum number of action
		flags we can generate locally at interrupt time. */
	highest_time= SHORT_MIN, lowest_time= SHORT_MAX;
	for (player_index= 0;player_index<dynamic_world->player_count; ++player_index)
	{
		short queue_size;

		if (game_is_networked)
		{
			queue_size= MIN(get_action_queue_size(player_index), NetGetNetTime() - dynamic_world->tick_count);
		}
		else
		{
			queue_size= MIN(get_action_queue_size(player_index), get_heartbeat_count() - dynamic_world->tick_count);
		}
		if (queue_size<0) queue_size= 0; // thumb in dike to prevent update_interface from getting -1
		
		if (queue_size>highest_time) highest_time= queue_size;
		if (queue_size<lowest_time) lowest_time= queue_size;
	}

	time_elapsed= lowest_time;
	for (i=0;i<time_elapsed;++i)
	{
		update_lights();
		update_platforms();
		
		update_control_panels(); // don't put after update_players
		update_players();
		move_projectiles();
		move_monsters();
		update_effects();
		recreate_objects();
	
		dynamic_world->tick_count+= 1;
		dynamic_world->game_information.game_time_remaining-= 1;
	}

	/* Game is over. */
	if(game_is_over()) game_timed_out();

	if (time_elapsed)
	{
		update_interface(time_elapsed);
		update_fades();
	}
	
	check_recording_replaying(); // ajr

	return time_elapsed;
}

/* call this function before leaving the old level, but DO NOT call it when saving the player.
	it should be called when you're leaving the game (i.e., quitting or reverting, etc.) */
void leaving_map(
	void)
{
	stop_background_music();
	
	remove_all_projectiles();
	remove_all_nonpersistent_effects();
	
	/* mark our shape collections for unloading */
	mark_environment_collections(static_world->environment_code, FALSE);
	mark_all_monster_collections(FALSE);
	mark_player_collections(FALSE);

	/* all we do is mark them for unloading, we don't explicitly dispose of them; whenever the
		next level is loaded someone (probably entering_map, below) will call load_collections()
		and the stuff we marked as needed to be ditched will be */
	
	/* stop counting world ticks */
	set_keyboard_controller_status(FALSE);

	return;
}

/* call this function after the new level has been completely read into memory, after
	player->location and player->facing have been updated, and as close to the end of
	the loading process in general as possible. */
boolean entering_map(
	void)
{
	boolean success= TRUE;

	/* if any active monsters think they have paths, we'll make them reconsider */
	initialize_monsters_for_new_level();

	/* and since no monsters have paths, we should make sure no paths think they have monsters */
	reset_paths();
	
	/* mark our shape collections for loading and load them */
	mark_environment_collections(static_world->environment_code, TRUE);
	mark_all_monster_collections(TRUE);
	mark_player_collections(TRUE);
	load_collections();
	
	if (success)
	{
		if (!game_is_networked && get_game_status()==game_in_progress) try_and_display_chapter_screen(dynamic_world->current_level_number);
	}

	start_background_music(static_world->song_index);

	/* tell the keyboard controller to start recording keyboard flags */
	if (game_is_networked) success= NetSync(); /* make sure everybody is ready */
	reset_player_queues();
	sync_heartbeat_count();
	set_keyboard_controller_status(TRUE);
	
	/* make sure nobody’s holding a weapon illegal in the new environment */
	check_player_weapons_for_environment_change();
	
	randomize_scenery_shapes();

	if (!success) leaving_map();

	return success;
}

/* This is called when an object of some mass enters a polygon from another */
/* polygon.  It handles triggering lightsources, platforms, and whatever */
/* else it is that we can think of. */
void changed_polygon(
	short original_polygon_index,
	short new_polygon_index,
	boolean object_is_player)
{
	struct polygon_data *new_polygon= get_polygon_data(new_polygon_index);
	
	#pragma unused (original_polygon_index)
	
	/* Entering this polygon.. */
	switch (new_polygon->type)
	{
		case _polygon_is_light_on_trigger:
		case _polygon_is_light_off_trigger:
			set_light_status(new_polygon->permutation,
				new_polygon->type==_polygon_is_light_off_trigger ? FALSE : TRUE);
			break;
			
		case _polygon_is_platform:
			platform_was_entered(new_polygon->permutation, object_is_player);
			break;
		case _polygon_is_platform_on_trigger:
		case _polygon_is_platform_off_trigger:
			if (object_is_player)
			{
				try_and_change_platform_state(get_polygon_data(new_polygon->permutation)->permutation,
					new_polygon->type==_polygon_is_platform_off_trigger ? FALSE : TRUE);
			}
			break;
	}

	return;
}

/* _level_failed is the same as _level_finished but indicates a non-fatal failure condition (e.g.,
	too many civilians died during _mission_rescue) */
short calculate_level_completion_state(
	void)
{
	short completion_state= _level_finished;
	
	/* if there are any monsters left on an extermination map, we haven’t finished yet */
	if (static_world->mission_flags&_mission_extermination)
	{
		if (live_aliens_on_map()) completion_state= _level_unfinished;
	}
	
	/* if there are any polygons which must be explored and are not on the automap, we’re not done */
	if (static_world->mission_flags&_mission_exploration)
	{
		short polygon_index;
		struct polygon_data *polygon;
		
		for (polygon_index= 0, polygon= map_polygons; polygon_index<dynamic_world->polygon_count; ++polygon_index, ++polygon)
		{
			if (polygon->type==_polygon_must_be_explored && !POLYGON_IS_IN_AUTOMAP(polygon_index))
			{
				completion_state= _level_unfinished;
				break;
			}
		}
	}
	
	/* if there are any items left on this map, we’re not done */
	if (static_world->mission_flags&_mission_retrieval)
	{
		if (unretrieved_items_on_map()) completion_state= _level_unfinished;
	}
	
	/* if there are any untoggled repair switches on this level then we’re not there */
	if (static_world->mission_flags&_mission_repair)
	{
		if (untoggled_repair_switches_on_level()) completion_state= _level_unfinished;
	}

	/* if we’ve finished the level, check failure conditions */
	if (completion_state==_level_finished)
	{
		/* if this is a rescue mission and more than half of the civilians died, the mission failed */
		if (static_world->mission_flags&_mission_rescue &&
			dynamic_world->current_civilian_causalties>dynamic_world->current_civilian_count/2)
		{
			completion_state= _level_failed;
		}
	}
	
	return completion_state;
}

short calculate_damage(
	struct damage_definition *damage)
{
	short total_damage= damage->base + (damage->random ? random()%damage->random : 0);
	
	total_damage= FIXED_INTEGERAL_PART(total_damage*damage->scale);
	
	/* if this damage was caused by an alien modify it for the current difficulty level */
	if (damage->flags&_alien_damage)
	{
		switch (dynamic_world->game_information.difficulty_level)
		{
			case _wuss_level: total_damage-= total_damage>>1; break;
			case _easy_level: total_damage-= total_damage>>2; break;
			/* harder levels do not cause more damage */
		}
	}
	
	return total_damage;
}

#define MINOR_OUCH_FREQUENCY 0xf
#define MAJOR_OUCH_FREQUENCY 0x7
#define MINOR_OUCH_DAMAGE 15
#define MAJOR_OUCH_DAMAGE 7

void cause_polygon_damage(
	short polygon_index,
	short monster_index)
{
	struct polygon_data *polygon= get_polygon_data(polygon_index);
	struct monster_data *monster= get_monster_data(monster_index);
	struct object_data *object= get_object_data(monster->object_index);

	if ((polygon->type==_polygon_is_minor_ouch && !(dynamic_world->tick_count&MINOR_OUCH_FREQUENCY) && object->location.z==polygon->floor_height) ||
		(polygon->type==_polygon_is_major_ouch && !(dynamic_world->tick_count&MAJOR_OUCH_FREQUENCY)))
	{
		struct damage_definition damage;
		
		damage.flags= _alien_damage;
		damage.type= polygon->type==_polygon_is_minor_ouch ? _damage_polygon : _damage_major_polygon;
		damage.base= polygon->type==_polygon_is_minor_ouch ? MINOR_OUCH_DAMAGE : MAJOR_OUCH_DAMAGE;
		damage.random= 0;
		damage.scale= FIXED_ONE;
		
		damage_monster(monster_index, NONE, NONE, (world_point3d *) NULL, &damage);
	}
	
	return;
}

/* ---------- private code */
	
/* They ran out of time.  This means different things depending on the */
/* type of game.. */
static void game_timed_out(
	void)
{
	/* Currently they all do the same thing.. */
	switch(GET_GAME_TYPE())
	{
		case _game_of_kill_monsters:
		case _game_of_tag:
		case _game_of_carnage:
		case _game_of_kill_the_man_with_the_ball:
		case _game_of_capture_the_flag:
			switch(get_game_status())
			{
				case game_in_progress:
				case user_wants_to_revert_game:
					set_game_status(user_wants_quit_from_game);
					break;
				case replay_in_progress:
				case user_wants_quit_from_replay:
					set_game_status(user_wants_quit_from_replay);
					break;
				case demo_in_progress:
				case demo_wants_to_switch_demos:
					set_game_status(demo_wants_to_switch_demos);
					break;
					
				default:
					vhalt(csprintf(temporary, "bad game status: #%d", get_game_status()));
					break;
			}
			break;
	}
}

static boolean game_is_over(
	void)
{
	boolean game_over= FALSE;

	if (GET_GAME_OPTIONS() & _game_has_kill_limit)
	{
		struct player_data *player;
		short player_index, max_players;

		/* Find out if the kill limit has been reached */
		max_players = dynamic_world->player_count;
		for(player_index= 0; player_index<max_players; ++player_index)
		{
			player= get_player_data(player_index);
			// make sure we subtract our suicides.
			if (player->total_damage_given.kills-player->damage_taken[player_index].kills >= dynamic_world->game_information.kill_limit)
			{
				// we don't actually want the game to end right away, but give a second or
				// two to see the player die.
				dynamic_world->game_information.game_options &= ~_game_has_kill_limit;
				dynamic_world->game_information.game_time_remaining = 2 * TICKS_PER_SECOND;
//				game_over= TRUE;
				break;
			}
		}
	}
	else if (dynamic_world->game_information.game_time_remaining<=0)
	{
		game_over= TRUE;
	}
	
	return game_over;
}
