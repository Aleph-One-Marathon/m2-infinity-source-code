/*

	network_games.c
	Wednesday, July 19, 1995 10:16:51 AM- rdm created.

*/

#include "cseries.h"
#include "map.h"
#include "items.h"
#include "player.h"
#include "monsters.h"
#include "network_games.h"
#include "game_window.h" // for mark_player_network_stats_as_dirty
#include "game_sound.h"

/* ----------- #defines */
#define SINGLE_BALL_COLOR (1)

/* ----------- enums */

/* Net game parameters */
enum { // for king of the hill
	_king_of_hill_time= 0 
};

enum { // for kill the man with the ball
	_ball_carrier_time= 0
};

enum { // for offense/defense
	_offender_time_in_base= 0, // for player->netgame_parameters
	_defending_team= 0, // for game_information->paramters
	_maximum_offender_time_in_base= 1 // for game_information->parameters
};

enum { // for rugby
	_points_scored= 0
};

enum { // for tag
	_time_spent_it= 0
};

enum { // for capture the flag.
	_flag_pulls= 0, // for player->netgame_parameters
	_winning_team= 0 // for game_information->parameters[]
};


/* ----------------- private prototypes */
static boolean player_has_ball(short player_index, short color);
static void destroy_players_ball(short player_index);

/* ------------------ code */
long get_player_net_ranking(
	short player_index,
	short *kills,
	short *deaths,
	boolean game_is_over)
{
	short index;
	long total_monster_damage, monster_damage;
	struct player_data *player= get_player_data(player_index);
	long ranking;

	*kills= 0;
	*deaths = player->monster_damage_taken.kills;
	monster_damage= player->monster_damage_given.damage;
	
	total_monster_damage= monster_damage;
	for (index= 0; index<dynamic_world->player_count; ++index)
	{
		if (index!=player_index)
		{
			struct player_data *other_player= get_player_data(index);

			(*kills)+= other_player->damage_taken[player_index].kills;
			total_monster_damage+= other_player->monster_damage_given.damage;
		}
		
		(*deaths)+= player->damage_taken[index].kills;
	}

	switch(GET_GAME_TYPE())
	{
		case _game_of_kill_monsters:
			ranking= (*kills)-(*deaths);
			break;
				
		case _game_of_cooperative_play:
			ranking= total_monster_damage ? (100*monster_damage)/total_monster_damage : 0;
			break;

		case _game_of_capture_the_flag:
			ranking= player->netgame_parameters[_flag_pulls];
			break;
			
		case _game_of_king_of_the_hill:
			ranking= player->netgame_parameters[_king_of_hill_time];
			break;
			
		case _game_of_kill_man_with_ball:
			ranking= player->netgame_parameters[_ball_carrier_time];
			break;
			
		case _game_of_tag:
			ranking= -player->netgame_parameters[_time_spent_it];
			break;

		case _game_of_defense:
			ranking= (*kills)-(*deaths);

			/* Bogus for now.. */
			if(game_is_over && GET_GAME_PARAMETER(_winning_team)==player->team)
			{
				ranking += 50;
			}
			break;
			
		case _game_of_rugby:
			ranking= (*kills)-(*deaths);
			break;
			
		default:
			vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
			break;
	}
	
	return ranking;
}

void initialize_net_game(
	void)
{
	switch (GET_GAME_TYPE())
	{
		case _game_of_king_of_the_hill:
			// calculate the center of the hill
			{
				long x, y;
				short count;
				struct polygon_data *polygon;
				short polygon_index;
				
				count= x= y= 0;
				for (polygon_index= 0, polygon= map_polygons; polygon_index<dynamic_world->polygon_count; ++polygon_index, ++polygon)
				{
					if (polygon->type==_polygon_is_hill)
					{
						count+= 1;
						x+= polygon->center.x, y+= polygon->center.y;
					}
				}
				
				dynamic_world->game_beacon.x= x/count;
				dynamic_world->game_beacon.y= y/count;
			}
			break;

		case _game_of_kill_man_with_ball:
			dynamic_world->game_player_index= NONE;
//			play_local_sound(_snd_got_ball);
			break;
			
		case _game_of_tag:
			dynamic_world->game_player_index= NONE; // nobody is it, yet
			break;
	}

	return;
}

#define NETWORK_COMPASS_SLOP SIXTEENTH_CIRCLE

short get_network_compass_state(
	short player_index)
{
	short state= _network_compass_all_off;
	world_point2d *beacon= (world_point2d *) NULL;
	
	switch (GET_GAME_TYPE())
	{
		case _game_of_king_of_the_hill: // where’s the hill
			if (get_polygon_data(get_player_data(player_index)->supporting_polygon_index)->type==_polygon_is_hill)
			{
				state= _network_compass_all_on;
			}
			else
			{
				beacon= &dynamic_world->game_beacon;
			}
			break;
			
		case _game_of_tag: // where’s it
			if (dynamic_world->game_player_index==player_index)
			{
				state= _network_compass_all_on;
			}
			else
			{
				if (dynamic_world->game_player_index!=NONE)
				{
					beacon= (world_point2d *) &get_player_data(dynamic_world->game_player_index)->location;
				}
			}
			break;
		
		case _game_of_kill_man_with_ball: // where’s the ball
			if (player_has_ball(player_index, SINGLE_BALL_COLOR))
			{
				state= _network_compass_all_on;
			}
			else
			{
				if (dynamic_world->game_player_index!=NONE)
				{
					beacon= (world_point2d *) &get_player_data(dynamic_world->game_player_index)->location;
				}
			}
			break;
	}

	if (beacon)
	{
		struct player_data *player= get_player_data(player_index);
		struct world_point2d *origin= (world_point2d *) &player->location;
		angle theta= NORMALIZE_ANGLE(get_object_data(player->object_index)->facing-arctangent(origin->x-beacon->x, origin->y-beacon->y));
		
		if (theta>FULL_CIRCLE-NETWORK_COMPASS_SLOP || theta<QUARTER_CIRCLE+NETWORK_COMPASS_SLOP) state|= _network_compass_se;
		if (theta>QUARTER_CIRCLE-NETWORK_COMPASS_SLOP && theta<HALF_CIRCLE+NETWORK_COMPASS_SLOP) state|= _network_compass_ne;
		if (theta>HALF_CIRCLE-NETWORK_COMPASS_SLOP && theta<HALF_CIRCLE+QUARTER_CIRCLE+NETWORK_COMPASS_SLOP) state|= _network_compass_nw;
		if (theta>HALF_CIRCLE+QUARTER_CIRCLE-NETWORK_COMPASS_SLOP || theta<NETWORK_COMPASS_SLOP) state|= _network_compass_sw;
	}

	return state;
}

// if FALSE is returned, don’t attribute kill
boolean player_killed_player(
	short dead_player_index,
	short aggressor_player_index)
{
	boolean attribute_kill= TRUE;
	
	if (dynamic_world->player_count>1)
	{
		switch (GET_GAME_TYPE())
		{
			case _game_of_tag:
				if (aggressor_player_index==dynamic_world->game_player_index || // killed by it
					dead_player_index==aggressor_player_index || // killed themselves
					dynamic_world->game_player_index==NONE) // died without an it
				{
					if (dynamic_world->game_player_index!=dead_player_index)
					{
						// change of ‘it’
#if 1
						struct player_data *player= get_player_data(dead_player_index);
						
						play_object_sound(player->object_index, _snd_you_are_it);
#endif
						dynamic_world->game_player_index= dead_player_index;
					}
				}
				break;
		}
		
		if (attribute_kill)
		{
			switch (GET_GAME_TYPE())
			{
				case _game_of_kill_monsters:
					mark_player_network_stats_as_dirty(current_player_index);
					break;
			}
		}
	}

	return attribute_kill;
}

boolean update_net_game(
	void)
{
	boolean net_game_over= FALSE;
	short player_index;

	if (dynamic_world->player_count>1)
	{
		switch(GET_GAME_TYPE())
		{
			case _game_of_kill_monsters:
			case _game_of_cooperative_play:
				/* These games have no housekeeping associated with them. */
				break;
				
#if 0
			case _game_of_capture_the_flag:
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					struct player_data *player= get_player_data(player_index);
					struct polygon_data *polygon= get_polygon_data(player->camera_polygon_index);
					
					if(polygon->type==_polygon_is_base && polygon->permutation==player->team)
					{
						short ball_color= find_player_ball_color(player_index);
						
						if(ball_color != NONE && ball_color != player->team)
						{
							player->netgame_parameters[_flag_pulls]++;
							destroy_players_ball(player_index);
						}
					}
				}
				break;
#endif

			case _game_of_king_of_the_hill:
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					struct player_data *player= get_player_data(player_index);
					
					if(!PLAYER_IS_DEAD(player))
					{
						struct polygon_data *polygon= get_polygon_data(player->supporting_polygon_index);
					
						if(polygon->type==_polygon_is_hill)
						{
							player->netgame_parameters[_king_of_hill_time]++;
						}
					}
				}
				break;
				
			case _game_of_kill_man_with_ball:
				dynamic_world->game_player_index= NONE;
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					if (player_has_ball(player_index, SINGLE_BALL_COLOR))
					{
						struct player_data *player= get_player_data(player_index);
						
						player->netgame_parameters[_ball_carrier_time]++;
						dynamic_world->game_player_index= player_index;
						
						break;
					}
				}
				break;
				
			case _game_of_tag:
				if (dynamic_world->game_player_index!=NONE)
				{
					struct player_data *player= get_player_data(dynamic_world->game_player_index);
					
					if (!PLAYER_IS_DEAD(player) || PLAYER_IS_TOTALLY_DEAD(player))
					{
						player->netgame_parameters[_time_spent_it]+= 1;
					}
				}
				break;

			case _game_of_defense:
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					struct player_data *player= get_player_data(player_index);
					short defending_team= GET_GAME_PARAMETER(_defending_team);
					
					if(player->team != defending_team)
					{
						struct polygon_data *polygon= get_polygon_data(player->supporting_polygon_index);

						/* They are in our base! */
						if(polygon->type==_polygon_is_base && polygon->permutation==defending_team)
						{
							player->netgame_parameters[_offender_time_in_base]++;
							if(player->netgame_parameters[_offender_time_in_base]>GET_GAME_PARAMETER(_maximum_offender_time_in_base))
							{
dprintf("Game is over. Offender won.");
//••
								dynamic_world->game_information.parameters[_winning_team]= player->team;
								net_game_over= TRUE;
							}
						}
						break; /* there is only one ball */
					}
				}
				break;
				
			case _game_of_rugby:
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					struct player_data *player= get_player_data(player_index);
					struct polygon_data *polygon= get_polygon_data(player->supporting_polygon_index);

					if(polygon->type==_polygon_is_base && polygon->permutation != player->team)
					{
						if(player_has_ball(player_index, SINGLE_BALL_COLOR))
						{
							/* Goal! */
							player->netgame_parameters[_points_scored]++;
							
							/* Ditch the ball.. (it will be recreated by the timer..) */
							destroy_players_ball(player_index);
						}
					}
				}
				break;
				
			default:
				vhalt(csprintf(temporary, "What is game type: %d?", GET_GAME_TYPE()));
				break;
		}

		if (--current_player->interface_decay<0 && GET_GAME_TYPE()!=_game_of_kill_monsters)
		{
			mark_player_network_stats_as_dirty(current_player_index);
		}
	}
	
	return net_game_over;
}

void calculate_player_rankings(
	struct player_ranking_data *rankings)
{
	struct player_ranking_data temporary_copy[MAXIMUM_NUMBER_OF_PLAYERS];
	short player_index, count;
	
	/* First get the stats. */
	for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
	{
		short kills, deaths;

		temporary_copy[player_index].player_index= player_index;
		temporary_copy[player_index].ranking= get_player_net_ranking(player_index, &kills, &deaths,
			FALSE);
	}

	/* Now sort them.. */
	count= 0;
	while(count!=dynamic_world->player_count)
	{
		long highest_ranking= LONG_MIN;
		short highest_index= NONE;

		for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
		{
			if(temporary_copy[player_index].ranking>highest_ranking)
			{
				highest_index= player_index;
				highest_ranking= temporary_copy[player_index].ranking;
			}
		}
	
		assert(highest_index != NONE);
		rankings[count++]= temporary_copy[highest_index];
		temporary_copy[highest_index].ranking= LONG_MIN;
	}
}

/* These aren't in resources for speed.... */
void calculate_ranking_text(
	char *buffer, 
	long ranking)
{
	long seconds;
	
	switch(GET_GAME_TYPE())
	{
		case _game_of_kill_monsters:
		case _game_of_capture_the_flag:
		case _game_of_rugby:
			sprintf(buffer, "%d", ranking);
			break;
			
		case _game_of_cooperative_play:
			sprintf(buffer, "%d%%", ranking);
			break;

		case _game_of_king_of_the_hill:
		case _game_of_kill_man_with_ball:
		case _game_of_tag:
			seconds= ABS(ranking)/TICKS_PER_SECOND;
			sprintf(buffer, "%d:%02d", seconds/60, seconds%60);
			break;
			
		case _game_of_defense:
			break;
			
		default:
			vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
			break;
	}
}

enum {
	strNETWORK_GAME_STRINGS= 140,
	flagPullsFormatString= 0,
	minutesPossessedFormatString,
	pointsFormatString,
	teamString,
	timeWithBallString,
	flagsCapturedString,
	timeItString,
	goalsString,
	reignString
};

void calculate_ranking_text_for_post_game(
	char *buffer,
	long ranking)
{
	long seconds;
	char format[40];

	switch(GET_GAME_TYPE())
	{
		case _game_of_kill_monsters:
		case _game_of_cooperative_play:
			break;
			
		case _game_of_capture_the_flag:
			getcstr(format, strNETWORK_GAME_STRINGS, flagPullsFormatString);
			sprintf(buffer, format, ranking);
			break;

		case _game_of_rugby:
			getcstr(format, strNETWORK_GAME_STRINGS, pointsFormatString);
			sprintf(buffer, format, ranking);
			break;
			
		case _game_of_king_of_the_hill:
		case _game_of_kill_man_with_ball:
		case _game_of_tag:
			seconds= ABS(ranking)/TICKS_PER_SECOND;
			getcstr(format, strNETWORK_GAME_STRINGS, minutesPossessedFormatString);
			sprintf(buffer, format, seconds/60, seconds%60);
			break;
			
		case _game_of_defense:
			break;
			
		default:
			vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
			break;
	}
}

boolean get_network_score_text_for_postgame(
	char *buffer, 
	boolean team_mode)
{
	short string_id= NONE;

	switch(GET_GAME_TYPE())
	{
		case _game_of_kill_monsters:
		case _game_of_cooperative_play:
			string_id= NONE;
			break;
			
		case _game_of_capture_the_flag:
			string_id= flagsCapturedString;
			break;
			
		case _game_of_king_of_the_hill:
			string_id= reignString;
			break;
			
		case _game_of_kill_man_with_ball:
			string_id= timeWithBallString;
			break;
			
		case _game_of_defense:
dprintf("Not supported!");
			string_id= timeWithBallString;
			break;

		case _game_of_tag:
			string_id= timeItString;
			break;
			
		default:
			vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
			break;
	}

	if(string_id != NONE)
	{
		char text[40];
		char team[20];

		if(team_mode)
		{
			getcstr(team, strNETWORK_GAME_STRINGS, teamString);
			getcstr(text, strNETWORK_GAME_STRINGS, string_id);
			sprintf(buffer, "%s %s", team, text);
		} else {
			getcstr(text, strNETWORK_GAME_STRINGS, string_id);
			sprintf(buffer, "%s", text);
		}
	}			

	return (string_id!=NONE);
}

boolean current_net_game_has_scores(
	void)
{
	boolean has_scores;
	
	switch(GET_GAME_TYPE())
	{
		case _game_of_kill_monsters:
		case _game_of_cooperative_play:
			has_scores= FALSE;
			break;
			
		case _game_of_capture_the_flag:
		case _game_of_rugby:
		case _game_of_king_of_the_hill:
		case _game_of_kill_man_with_ball:
		case _game_of_defense:
		case _game_of_tag:
			has_scores= TRUE;
			break;
			
		default:
			has_scores= FALSE;
			vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
			break;
	}

	return has_scores;
}

boolean current_game_has_balls(
	void)
{
	boolean has_ball;
	
	switch(GET_GAME_TYPE())
	{
		case _game_of_kill_monsters:
		case _game_of_cooperative_play:
		case _game_of_king_of_the_hill:
		case _game_of_defense:
		case _game_of_tag:
			has_ball= FALSE;
			break;
			
		case _game_of_capture_the_flag:
		case _game_of_rugby:
		case _game_of_kill_man_with_ball:
			has_ball= TRUE;
			break;
			
		default:
			has_ball= FALSE;
			vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
			break;
	}

	return has_ball;
}

/* Note that kill limit means different things.. */
/* if capture the flag- the number of flag pulls */
boolean game_is_over(
	void)
{
	boolean game_over= FALSE;

	if (dynamic_world->game_information.game_time_remaining<=0)
	{
		game_over= TRUE;
	} 
	else if(GET_GAME_OPTIONS() & _game_has_kill_limit) 
	{
		short player_index;
		short count;

		switch(GET_GAME_TYPE())
		{
			case _game_of_kill_monsters:
			case _game_of_cooperative_play:
			case _game_of_king_of_the_hill:
			case _game_of_kill_man_with_ball:
			case _game_of_tag:
				/* Find out if the kill limit has been reached */
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					struct player_data *player= get_player_data(player_index);
					
					// make sure we subtract our suicides.
					if (player->total_damage_given.kills-player->damage_taken[player_index].kills >= dynamic_world->game_information.kill_limit)
					{
						// we don't actually want the game to end right away, but give a second or
						// two to see the player die.
						dynamic_world->game_information.game_options &= ~_game_has_kill_limit;
						dynamic_world->game_information.game_time_remaining= 2*TICKS_PER_SECOND;
						break;
					}
				}
				break;
				
#if 0
			case _game_of_capture_the_flag:
				/* Kill limit is the number of flag pulls */
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					struct player_data *player= get_player_data(player_index);
					
					// make sure we subtract our suicides.
					count+= player->netgame_parameters[_flag_pulls];
				}
				
				if(count>=dynamic_world->game_information.kill_limit)
				{
					game_over= TRUE;
				}
				break;
				
			case _game_of_rugby:
				/* Kill limit is the number of flag pulls */
				for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
				{
					struct player_data *player= get_player_data(player_index);
					
					// make sure we subtract our suicides.
					count+= player->netgame_parameters[_points_scored];
				}
				
				if(count>=dynamic_world->game_information.kill_limit)
				{
					game_over= TRUE;
				}
				break;
#endif

			case _game_of_defense:
//••
				break;
				
			default:
				vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
				break;
		}
	}
	
	return game_over;
}

enum {
	joinNetworkStrings= 142,
	_standard_format= 0,
	_carnage_word,
	_cooperative_string,
	_capture_the_flag,
	_king_of_the_hill,
	_kill_the_man_with_the_ball,
	_defender_offender,
	_rugby,
	_tag
};

void get_network_joined_message(
	char *buffer,
	short game_type)
{
	short format_word= NONE; /* means cooperative */
	
	switch(game_type)
	{
		case _game_of_kill_monsters: format_word= _carnage_word; break;
		case _game_of_cooperative_play:	format_word= NONE; break;
		case _game_of_capture_the_flag: format_word= _capture_the_flag; break;
		case _game_of_rugby: format_word= _rugby; break;
		case _game_of_king_of_the_hill: format_word= _king_of_the_hill; break;
		case _game_of_kill_man_with_ball: format_word= _kill_the_man_with_the_ball; break;
		case _game_of_defense: format_word= _defender_offender; break;
		case _game_of_tag: format_word= _tag; break;
		default:
			vhalt(csprintf(temporary, "What is game type %d?", GET_GAME_TYPE()));
			break;
	}

	if(format_word != NONE)
	{
		char format_string[128];
		char game_type_word[50];

		getcstr(format_string, joinNetworkStrings, _standard_format);
		getcstr(game_type_word, joinNetworkStrings, format_word);
		sprintf(buffer, format_string, game_type_word);
	} else {
		/* Cooperative */
		getcstr(buffer, joinNetworkStrings, _cooperative_string);
	}
}

/* This function is used only at network.. */
long get_entry_point_flags_for_game_type(
	short game_type)
{
	long entry_flags;
	
	switch(game_type)
	{
		case _game_of_kill_man_with_ball:
			entry_flags= _capture_the_flag_entry_point;
			break;
			
		case _game_of_kill_monsters:
		case _game_of_tag:
			entry_flags= _multiplayer_carnage_entry_point;
			break;
			
		case _game_of_cooperative_play:
			entry_flags= _multiplayer_cooperative_entry_point;
			break;

		case _game_of_king_of_the_hill:
			entry_flags= _king_of_hill_entry_point;
			break;

		default:
			vhalt(csprintf(temporary, "What is game type %d?", game_type));
			break;
	}
		
	return entry_flags;
}

/* ------------------ local code */
static boolean player_has_ball(
	short player_index,
	short color)
{
	struct player_data *player= get_player_data(player_index);
	boolean has_ball= FALSE;
	
	if(player->items[BALL_ITEM_BASE+color]>0)
	{
		has_ball= TRUE;
	}
	
	return has_ball;
}

static void destroy_players_ball(
	short player_index)
{
	short color, item_type;
	struct player_data *player= get_player_data(player_index);
	
	color= find_player_ball_color(player_index);
	assert(color != NONE);

	/* Get rid of it. */
	item_type= BALL_ITEM_BASE+color;
	player->items[item_type]= NONE;
	
	/* Destroy the object (placement will recreate it..) */
	object_was_just_destroyed(_object_is_item, item_type);
	
	/* Mark the player inventory as dirty.. */
	mark_player_inventory_as_dirty(player_index, _i_knife);
}
