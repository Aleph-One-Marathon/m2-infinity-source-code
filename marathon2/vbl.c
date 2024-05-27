/*
VBL.C
Friday, August 21, 1992 7:06:54 PM

Tuesday, November 17, 1992 3:53:29 PM
	the new task of the vbl controller is only to move the player.  this is necessary for
	good control of the game.  everything else (doors, monsters, projectiles, etc) will
	be moved immediately before the next frame is drawn, based on delta-time values.
	collisions (including the player with walls) will also be handled at this time.
Thursday, November 19, 1992 1:27:23 AM
	the enumeration 'turning_head' had to be changed to '_turn_not_rotate' to make this
	file compile.  go figure.
Wednesday, December 2, 1992 2:31:05 PM
	the world doesn’t change while the mouse button is pressed.
Friday, January 15, 1993 11:19:11 AM
	the world doesn’t change after 14 ticks have passed without a screen refresh.
Friday, January 22, 1993 3:06:32 PM
	world_ticks was never being initialized to zero.  hmmm.
Saturday, March 6, 1993 12:23:48 PM
	at exit, we remove our vbl task.
Sunday, May 16, 1993 4:07:47 PM
	finally recoding everything
Monday, August 16, 1993 10:22:17 AM
	#ifdef CHARLES added.
Saturday, August 21, 1993 12:35:29 PM
	from pathways VBL_CONTROLLER.C.
Sunday, May 22, 1994 8:51:15 PM
	all the world physics has been moved into PHYSICS.C; all we do now is maintain and
	distribute a circular queue of keyboard flags (we're the keyboard_controller, not the
	movement_controller).
Thursday, June 2, 1994 12:55:52 PM
	gee, now we don’t even maintain the queue we just send our actions to PLAYER.C.
Tuesday, July 5, 1994 9:27:49 PM
	nuked most of the shit in here. changed the vbl task to a time
	manager task. the only functions from the old vbl.c that remain are precalculate_key_information()
	and parse_keymap().
Thursday, July 7, 1994 11:59:32 AM
	Added recording/replaying
Wednesday, August 10, 1994 2:44:57 PM
	added caching system for FSRead.
Friday, January 13, 1995 11:38:51 AM  (Jason')
	fixed the 'a' key getting blacklisted.
*/

#include "macintosh_cseries.h"
#include "map.h"
#include "shell.h"
#include "preferences.h"
#include "interface.h"
#include "mytm.h"
#include "player.h"
#include "mouse.h"
#include "key_definitions.h"
#include "tags.h" // for filetypes.
#include "computer_interface.h"

#include <Folders.h>
#include <Script.h>  // for verUS
#include <string.h>

#ifdef mpwc
#pragma segment input
#endif

/* ---------- constants */

#define MAXIMUM_QUEUE_SIZE           512
#define RECORD_CHUNK_SIZE            (MAXIMUM_QUEUE_SIZE/2)
#define END_OF_RECORDING_INDICATOR  (RECORD_CHUNK_SIZE+1)
#define MAXIMUM_TIME_DIFFERENCE     15 // allowed between heartbeat_count and dynamic_world->tick_count
#define FILM_RESOURCE_TYPE          'film'
#define MAXIMUM_NET_QUEUE_SIZE       8
#define DISK_CACHE_SIZE             ((sizeof(short)+sizeof(long))*100)
#define MAXIMUM_FLAG_PERSISTENCE    15
#define DOUBLE_CLICK_PERSISTENCE    10
#define MAXIMUM_REPLAY_SPEED         5
#define MINIMUM_REPLAY_SPEED        -5

/* ---------- macros */

#define INCREMENT_QUEUE_COUNTER(c) { (c)++; if ((c)>=MAXIMUM_QUEUE_SIZE) (c) = 0; }

/* ---------- structures */

typedef struct action_queue /* 8 bytes */
{
	short read_index, write_index;
	long *buffer;
} ActionQueue;

struct recording_header
{
	long length;
	short num_players;
	short level_number;
	unsigned long map_checksum;
	short version;
	struct player_start_data starts[MAXIMUM_NUMBER_OF_PLAYERS];
	struct game_data game_information;
};

struct replay_private_data {
	boolean valid;
	struct recording_header header;
	short replay_speed;
	boolean game_is_being_replayed;
	boolean game_is_being_recorded;
	boolean have_read_last_chunk;
	ActionQueue *recording_queues;
	
	short vRefNum; // for free bytes check
	short recording_file_refnum;
	char *fsread_buffer;
	char *location_in_cache;
	long bytes_in_cache;
	
	long film_resource_offset;
	Handle film_resource;
};

/* ---------- globals */

static long         heartbeat_count;
static boolean      input_task_active;
static myTMTaskPtr  input_task;

static struct replay_private_data replay;

#ifdef OBSOLETE
// to be moved to map.c when this all works?
boolean game_is_being_recorded;
boolean game_is_being_replayed;
static long         saved_header_size;
static short        recording_vrefnum;
static short        recording_file_refnum;
static long         film_resource_offset;
static boolean      have_read_last_chunk;
static Handle       film_resource;
static ActionQueue  *recording_queues;
#endif

#define NUMBER_OF_SPECIAL_FLAGS (sizeof(special_flags)/sizeof(struct special_flag_data))
static struct special_flag_data special_flags[]=
{
	{_double_flag, _look_dont_turn, _looking_center},
	{_double_flag, _run_dont_walk, _swim},
	{_latched_flag, _action_trigger_state},
	{_latched_flag, _cycle_weapons_forward},
	{_latched_flag, _cycle_weapons_backward},
	{_latched_flag, _toggle_map}
};

#ifndef DEBUG
	#define get_player_recording_queue(x) (replay.recording_queues+(x))
#else
ActionQueue *get_player_recording_queue(
	short player_index)
{
	assert(replay.recording_queues);
	assert(player_index>=0 && player_index<MAXIMUM_NUMBER_OF_PLAYERS);
	
	return (replay.recording_queues+player_index);
}
#endif

/* ---------- private prototypes */
static void remove_input_controller(void);
static void precalculate_key_information(void);
static void save_recording_queue_chunk(short player_index);
static void read_recording_queue_chunks(void);
static boolean pull_flags_from_recording(short count);
static OSErr vblFSRead(short refnum, long *count, void *dest);
static boolean input_controller(void);
static void record_action_flags(short player_identifier, long *action_flags, short count);
static OSErr get_freespace_on_disk(short vRefNum, unsigned long *free_space);
static boolean get_recording_fsspec(FSSpec *file);
static OSErr copy_file(FSSpec *source, FSSpec *destination);
static short get_recording_queue_size(short which_queue);

// #define DEBUG_REPLAY

#ifdef DEBUG_REPLAY
static void open_stream_file(void);
static void debug_stream_of_flags(long action_flag, short player_index);
static void close_stream_file(void);
#endif

/* ---------- code */

#ifdef OBSOLETE
/*********************************************************************************************
 *
 * Function: initialize_keyboard_controller
 * Purpose:  get the input controller up and running
 *
 *********************************************************************************************/
void initialize_keyboard_controller(
	void)
{
	ActionQueue  *queue;
	
	vassert(NUMBER_OF_KEYS == NUMBER_OF_STANDARD_KEY_DEFINITIONS,
		csprintf(temporary, "NUMBER_OF_KEYS == %d, NUMBER_OF_KEY_DEFS = %d. Not Equal!", NUMBER_OF_KEYS, NUMBER_OF_STANDARD_KEY_DEFINITIONS));
	assert(NUMBER_OF_STANDARD_KEY_DEFINITIONS==NUMBER_OF_LEFT_HANDED_KEY_DEFINITIONS);
	assert(NUMBER_OF_LEFT_HANDED_KEY_DEFINITIONS==NUMBER_OF_POWERBOOK_KEY_DEFINITIONS);
	
	// get globals initialized
	heartbeat_count= 0;
	input_task_active= FALSE;
	memset(&replay, 0, sizeof(struct replay_private_data));
	replay.game_is_being_recorded= FALSE;
	replay.game_is_being_replayed= FALSE;
	saved_header_size= NONE;
	
	if (input_task == NULL)
	{
		input_task = myTMSetup(1000/TICKS_PER_SECOND, input_controller);
		assert(input_task);
	
		atexit(remove_input_controller);

		set_keys(input_preferences->keycodes);
	}
	
	if (recording_queues == NULL)
	{
		short player_index;
	
		recording_queues = (ActionQueue *) NewPtr(MAXIMUM_NUMBER_OF_PLAYERS * sizeof(ActionQueue));
		assert(recording_queues);
		for (player_index= 0; player_index<MAXIMUM_NUMBER_OF_PLAYERS; player_index++)
		{
			queue= get_player_recording_queue(player_index);
			queue->read_index= queue->write_index = 0;
			queue->buffer= (long *) NewPtr(MAXIMUM_QUEUE_SIZE*sizeof(long));
			assert(queue->buffer);
		}
	}

	enter_mouse(0);
	
	return;
}
#endif

/*********************************************************************************************
 *
 * Function: set_keyboard_controller_status
 * Purpose:  make the input controller active/inactive
 *
 *********************************************************************************************/
void set_keyboard_controller_status(
	boolean active)
{
	input_task_active = active;
//	if (active)	reset_recording_and_playback_queues();
}

/*********************************************************************************************
 *
 * Function: get_keyboard_controller_status
 * Purpose:  find out if the input controller is active or inactive.
 *
 *********************************************************************************************/
boolean get_keyboard_controller_status(
	void)
{
	return input_task_active;
}

/*********************************************************************************************
 *
 * Function: pause_keyboard_controller
 * Purpose:  like set_keyboard_controller_status, but it doesn't reset the queues.
 *
 *********************************************************************************************/
void pause_keyboard_controller(
	boolean active)
{
	input_task_active= active;
}

/*********************************************************************************************
 *
 * Function: get_heartbeat_count
 * Purpose:  find out how many heartbeats have passed us by. As sand flows through the
 *           hourglass, so do the heartbeats of our game...
 *
 *********************************************************************************************/
long get_heartbeat_count(
	void)
{
	return heartbeat_count;
}

/*********************************************************************************************
 *
 * Function: sync_heartbeat_count
 * Purpose:  called between games to reset the time.
 *
 *********************************************************************************************/
void sync_heartbeat_count(
	void)
{
	heartbeat_count= dynamic_world->tick_count;
}

#ifdef OBSOLETE
/*********************************************************************************************
 *
 * Function: start_recording
 * Purpose:  start the recording process
 *
 *********************************************************************************************/
void start_recording(
	long header_size, 
	byte *header)
{
	FSSpec recording_file;
	long   count;
	OSErr error;
	
	if(get_recording_fsspec(&recording_file))
	{
		FSpDelete(&recording_file);
	}
	
	error= FSpCreate(&recording_file, APPLICATION_CREATOR, FILM_FILE_TYPE, smSystemScript);
	if(!error)
	{
		/* I debate the validity of fsCurPerm here, but Alain had it, and it has been working */
		error= FSpOpenDF(&recording_file, fsCurPerm, &recording_file_refnum);
		if(!error && recording_file_refnum != NONE)
		{
			game_is_being_recorded= TRUE;
			recording_vrefnum= recording_file.vRefNum;
	
			// save a header containing information about the game.
			count = header_size; 
			FSWrite(recording_file_refnum, &count, header);
			saved_header_size = header_size;
		}
	}
}

/*********************************************************************************************
 *
 * Function: rewind_recording
 * Purpose:  essentially, erase the recording and start over again, with the same 
 *           header block. this is done when a game is being recorded and was reverted.
 *           This function is safe even if we aren't currently recording.
 *
 *********************************************************************************************/
void rewind_recording(
	void)
{
	if (game_is_being_recorded)
	{
		/* This is unnecessary, because it is called from reset_player_queues, */
		/* which is always called from revert_game */
//		reset_recording_and_playback_queues(); 
		SetEOF(recording_file_refnum, saved_header_size);
		SetFPos(recording_file_refnum, fsFromLEOF, 0);
	}
}

/*********************************************************************************************
 *
 * Function: stop_recording
 * Purpose:  stop the recording process, and save the few remaining key digests
 *
 *********************************************************************************************/
void stop_recording(
	void)
{
	if (game_is_being_recorded)
	{
		short player_index;
	
		for (player_index= 0; player_index<dynamic_world->player_count; player_index++)
		{
//dprintf("Final queue size: %d", get_recording_queue_size(player_index));
			save_recording_queue_chunk(player_index);
		}
		FSClose(recording_file_refnum);

		game_is_being_recorded = FALSE;

		/* Unecessary, because reset_player_queues calls this. */
//		reset_recording_and_playback_queues();
	}
	
	return;
}

/*********************************************************************************************
 *
 * Function: start_replay
 * Purpose:  get ready to do a replay
 *
 *********************************************************************************************/
boolean start_replay(
	long header_size, 
	byte *header, 
	boolean prompt_user_for_location)
{
	boolean successful = FALSE;
	
	if (prompt_user_for_location)
	{
		StandardFileReply reply;
		SFTypeList types;
		short type_count= 0;

		types[type_count++]= FILM_FILE_TYPE;
		
		StandardGetFile(NULL, type_count, types, &reply);
		if (reply.sfGood)
		{
			successful= setup_replay_from_file(&reply.sfFile, header, header_size);
		}
	}
	else
	{
		FSSpec recording_file_location;

		if (get_recording_fsspec(&recording_file_location))
		{
			successful= setup_replay_from_file(&recording_file_location, header, header_size);
		}
	}
	
	return successful;
}

boolean setup_replay_from_file(
	FSSpec *file,
	void *header,
	long header_size)
{
	OSErr error;
	boolean successful= FALSE;

	error= FSpOpenDF(file, fsCurPerm, &recording_file_refnum);
	if(!error && recording_file_refnum > 0)
	{
		long count;
	
		have_read_last_chunk = FALSE;
		game_is_being_replayed = TRUE;
		film_resource = NULL;
		film_resource_offset = -1;
		
		count= header_size; 
		FSRead(recording_file_refnum, &count, header);

		fsread_buffer= NewPtr(DISK_CACHE_SIZE); 
		assert(fsread_buffer);
		if(!fsread_buffer) alert_user(fatalError, strERRORS, outOfMemory, MemError());
		
		location_in_cache= NULL;
		bytes_in_cache= 0;
		replay_speed= 1;
		
#ifdef DEBUG_REPLAY
		open_stream_file();
#endif
		successful= TRUE;
	}
	
	return successful;
}

/*********************************************************************************************
 *
 * Function: start_replay_from_random_resource
 * Purpose:  starts a replay from any one of the 'film' resources.
 * Returns:  TRUE if replay got started succesfully, FALSE otherwise
 *
 *********************************************************************************************/
boolean start_replay_from_random_resource(
	long header_size, 
	byte *header)
{
	short number_of_films;
	short which_film_to_play;
	static short index_of_last_film_played= 0;
	boolean success= FALSE;
	
	number_of_films = CountResources(FILM_RESOURCE_TYPE);
	if(number_of_films > 0)
	{
		if (number_of_films == 1)
		{
			which_film_to_play= 0;
		}
		else
		{
			for (which_film_to_play = index_of_last_film_played; 
					which_film_to_play == index_of_last_film_played;
					which_film_to_play = (abs(Random()) % number_of_films))
				;
			index_of_last_film_played= which_film_to_play;
		}
	
		film_resource = GetIndResource(FILM_RESOURCE_TYPE, which_film_to_play+1);
		vassert(film_resource, csprintf(temporary, "film_to_play = %d", which_film_to_play+1));
		film_resource_offset= 0;
		have_read_last_chunk= FALSE;
		game_is_being_replayed= TRUE;
		success= TRUE;
		
		heartbeat_count= 0;
		
		BlockMove(*film_resource, header, header_size);
		film_resource_offset += header_size;
		
		replay_speed= 1;
	}
	
	return success;
}
#endif

/*********************************************************************************************
 *
 * Function: stop_replay
 * Purpose:  stop the replay that is currently happening
 *
 *********************************************************************************************/
void stop_replay(
	void)
{
	assert(replay.valid);
	if (replay.game_is_being_replayed)
	{
		replay.game_is_being_replayed= FALSE;
		if (replay.film_resource)
		{
			ReleaseResource(replay.film_resource);
		}
		else
		{
			FSClose(replay.recording_file_refnum);
			assert(replay.fsread_buffer);
			DisposePtr(replay.fsread_buffer);
		}
#ifdef DEBUG_REPLAY
		close_stream_file();
#endif
	}
	/* Unecessary, because reset_player_queues calls this. */
//	reset_recording_and_playback_queues();
	replay.valid= FALSE;
	
	return;
}

void move_replay(
	void)
{
	Str255 suggested_name;
	StandardFileReply reply;
	
	getpstr(temporary, strPROMPTS, _save_replay_prompt);
	getpstr(suggested_name, strFILENAMES, filenameMARATHON_RECORDING);
	StandardPutFile(temporary, suggested_name, &reply);
	if(reply.sfGood)
	{
		FSSpec source_spec;
				
		if(get_recording_fsspec(&source_spec))
		{
			OSErr err;
		
			/* If we are replacing.. */
			if(reply.sfReplacing)
			{
				FSpDelete(&reply.sfFile);
			}

			err= copy_file(&source_spec, &reply.sfFile);

			/* Alert them on problems */			
			if (err) alert_user(infoError, strERRORS, fileError, err);
		}
	}
	
	return;
}

void increment_replay_speed(
	void)
{
	if (replay.replay_speed < MAXIMUM_REPLAY_SPEED) replay.replay_speed++;
}

void decrement_replay_speed(
	void)
{
	if (replay.replay_speed > MINIMUM_REPLAY_SPEED) replay.replay_speed--;
}

#ifdef OBSOLETE
/*********************************************************************************************
 *
 * Function: check_recording_replaying
 * Purpose:  save or fill the recording queues, if it's time.
 *
 *********************************************************************************************/
void check_recording_replaying(
	void)
{
	short player_index, queue_size;

	if (game_is_being_recorded)
	{
		boolean enough_data_to_save= TRUE;
	
		// it's time to save the queues if all of them have >= RECORD_CHUNK_SIZE flags in them.
		for (player_index= 0; enough_data_to_save && player_index<dynamic_world->player_count; player_index++)
		{
			queue_size= get_recording_queue_size(player_index);
			if (queue_size < RECORD_CHUNK_SIZE)	enough_data_to_save= FALSE;
		}
		
		if(enough_data_to_save)
		{
			OSErr error;
			unsigned long freespace;

			error= get_freespace_on_disk(&freespace);
			if (!error && freespace>(RECORD_CHUNK_SIZE*sizeof(short)*sizeof(long)*dynamic_world->player_count))
			{
				for (player_index= 0; player_index<dynamic_world->player_count; player_index++)
				{
					save_recording_queue_chunk(player_index);
				}
			}
		}
	}
	else if (game_is_being_replayed)
	{
		boolean load_new_data= TRUE;
	
		// it's time to refill the requeues if they all have < RECORD_CHUNK_SIZE flags in them.
		for (player_index= 0; load_new_data && player_index<dynamic_world->player_count; player_index++)
		{
			queue_size= get_recording_queue_size(player_index);
			if(queue_size>= RECORD_CHUNK_SIZE) load_new_data= FALSE;
		}
		
		if(load_new_data)
		{
			// at this point, we’ve determined that the queues are sufficently empty, so
			// we’ll fill ’em up.
			read_recording_queue_chunks();
		}
	}

	return;
}
#endif

/* Returns NONE if it is custom.. */
short find_key_setup(
	short *keycodes)
{
	short index, jj;
	short key_setup= NONE;
	
	for (index= 0; key_setup==NONE && index<NUMBER_OF_KEY_SETUPS; index++)
	{
		struct key_definition *definition = all_key_definitions[index];

		for (jj= 0; jj<NUMBER_OF_STANDARD_KEY_DEFINITIONS; jj++)
		{
			if (definition[jj].offset != keycodes[jj]) break;
		}

		if (jj==NUMBER_OF_STANDARD_KEY_DEFINITIONS)
		{
			key_setup= index;
		}
	}
	
	return key_setup;
}

/*********************************************************************************************
 *
 * Function: set_default_keys
 * Purpose:  push a default key setup into the preferences.
 *
 *********************************************************************************************/
void set_default_keys(
	short *keycodes, 
	short which_default)
{
	short i;
	struct key_definition *definitions;
	
	assert(which_default >= 0 && which_default < NUMBER_OF_KEY_SETUPS);
	definitions= all_key_definitions[which_default];
	for (i= 0; i < NUMBER_OF_STANDARD_KEY_DEFINITIONS; i++)
	{
		keycodes[i] = definitions[i].offset;
	}
}

/*********************************************************************************************
 *
 * Function: set_keys
 * Purpose:  tell the input function what keys the user is using.
 *
 *********************************************************************************************/
void set_keys(
	short *keycodes)
{
	short index;
	struct key_definition *definitions;
	
	/* all of them have the same ordering, so which one we pick is moot. */
	definitions = all_key_definitions[_standard_keyboard_setup]; 
	
	for (index = 0; index < NUMBER_OF_STANDARD_KEY_DEFINITIONS; index++)
	{
		current_key_definitions[index].offset= keycodes[index];
		current_key_definitions[index].action_flag= definitions[index].action_flag;
		assert(current_key_definitions[index].offset <= 0x7f);
	}
	precalculate_key_information();
	
	return;
}

#ifdef OBSOLETE
/* called from reset_player_queues which is called from complete_restoring_level and */
/* new_game */
void reset_recording_and_playback_queues(
	void)
{
	short index;
	
	for(index= 0; index<MAXIMUM_NUMBER_OF_PLAYERS; ++index)
	{
		recording_queues[index].read_index= recording_queues[index].write_index= 0;
	}
}
#endif

/*********************************************************************************************
 *
 * Function: parse_keymap
 * Purpose:  take the current keymap and converts it to a keyboard digest.
 *
 *********************************************************************************************/
long parse_keymap(
	void)
{
	short i;
	long flags= 0;
	KeyMap key_map;
	struct key_definition *key= current_key_definitions;
	struct special_flag_data *special= special_flags;
	
	GetKeys(&key_map);

	/* parse the keymap */	
	for (i=0;i<NUMBER_OF_STANDARD_KEY_DEFINITIONS;++i,++key)
	{
		if (*((byte*)key_map + key->offset) & key->mask) flags|= key->action_flag;
	}

	/* post-process the keymap */
	for (i=0;i<NUMBER_OF_SPECIAL_FLAGS;++i,++special)
	{
		if (flags&special->flag)
		{
			switch (special->type)
			{
				case _double_flag:
					/* if this flag has a double-click flag and has been hit within
						DOUBLE_CLICK_PERSISTENCE (but not at MAXIMUM_FLAG_PERSISTENCE),
						mask on the double-click flag */
					if (special->persistence<MAXIMUM_FLAG_PERSISTENCE &&
						special->persistence>MAXIMUM_FLAG_PERSISTENCE-DOUBLE_CLICK_PERSISTENCE)
					{
						flags|= special->alternate_flag;
					}
					break;
				
				case _latched_flag:
					/* if this flag is latched and still being held down, mask it out */
					if (special->persistence==MAXIMUM_FLAG_PERSISTENCE) flags&= ~special->flag;
					break;
				
				default:
					halt();
			}
			
			special->persistence= MAXIMUM_FLAG_PERSISTENCE;
		}
		else
		{
			special->persistence= FLOOR(special->persistence-1, 0);
		}
	}

	/* handle the selected input controller */
	if (input_preferences->input_device!=_keyboard_or_game_pad)
	{
		fixed delta_yaw, delta_pitch, delta_velocity;
		
		mouse_idle(input_preferences->input_device);
		test_mouse(input_preferences->input_device, &flags, &delta_yaw, &delta_pitch, &delta_velocity);
		flags= mask_in_absolute_positioning_information(flags, delta_yaw, delta_pitch, delta_velocity);
	}

	if(player_in_terminal_mode(local_player_index))
	{
		flags= build_terminal_action_flags((char *) key_map);
	}

	return flags;
}

boolean has_recording_file(
	void)
{
	FSSpec spec;

	return get_recording_fsspec(&spec);
}

/* ---------- private code */
/*********************************************************************************************
 *
 * Function: remove_input_controller
 * Purpose:  called when the program exits, to remove the time manager task.
 *
 *********************************************************************************************/
static void remove_input_controller(
	void)
{
	myTMRemove(input_task);
	if (replay.game_is_being_recorded)
	{
		stop_recording();
	}
	else if (replay.game_is_being_replayed)
	{
		if (replay.film_resource)
		{
			ReleaseResource(replay.film_resource);
			replay.film_resource= NULL;
		}
		else
		{
			assert(replay.recording_file_refnum>0);
			FSClose(replay.recording_file_refnum);
		}
	}

	replay.valid= FALSE;
}

/*********************************************************************************************
 *
 * Function: input_controller
 * Purpose:  gets called every heartbeat. controls updating of keyboard digest queues
 *
 *********************************************************************************************/
static boolean input_controller(
	void)
{
	if (input_task_active)
	{
		if((heartbeat_count-dynamic_world->tick_count) < MAXIMUM_TIME_DIFFERENCE)
		{
			if (game_is_networked) // input from network
			{
				; // all handled elsewhere now. (in network.c)
			}
			else if (replay.game_is_being_replayed) // input from recorded game file
			{
				static short phase= 0; /* When this gets to 0, update the world */

				/* Minimum replay speed is a pause. */
				if(replay.replay_speed != MINIMUM_REPLAY_SPEED)
				{
					if (replay.replay_speed > 0 || (--phase<=0))
					{
						short flag_count= MAX(replay.replay_speed, 1);
					
						if (!pull_flags_from_recording(flag_count)) // oops. silly me.
						{
							if (replay.have_read_last_chunk)
							{
								assert(get_game_state()==_game_in_progress || get_game_state()==_switch_demo);
								set_game_state(_switch_demo);
							}
						}
						else
						{	
							/* Increment the heartbeat.. */
							heartbeat_count+= flag_count;
						}
	
						/* Reset the phase-> doesn't matter if the replay speed is positive */					
						/* +1 so that replay_speed 0 is different from replay_speed 1 */
						phase= -(replay.replay_speed) + 1;
					}
				}
			}
			else // then getting input from the keyboard/mouse
			{
				long action_flags= parse_keymap();
				
				process_action_flags(local_player_index, &action_flags, 1);
				heartbeat_count++; // ba-doom
			}
		} else {
// dprintf("Out of phase.. (%d);g", heartbeat_count - dynamic_world->tick_count);
		}
	}
	
	return TRUE; // tells the time manager library to reschedule this task
}

/*********************************************************************************************
 *
 * Function: process_action_flags
 * Purpose:  queues the action flags, and records them if necessary
 *
 *********************************************************************************************/
void process_action_flags(
	short player_identifier, 
	long *action_flags, 
	short count)
{
	if (replay.game_is_being_recorded)
	{
		record_action_flags(player_identifier, action_flags, count);
	}

	queue_action_flags(player_identifier, action_flags, count);
}

/*********************************************************************************************
 *
 * Function: record_action_flags
 * Purpose:  saves the action flags in a buffer that will be saved to a file later
 *
 *********************************************************************************************/
static void record_action_flags(
	short player_identifier, 
	long *action_flags, 
	short count)
{
	short index;
	ActionQueue  *queue;
	
	queue= get_player_recording_queue(player_identifier);
	assert(queue && queue->write_index >= 0 && queue->write_index < MAXIMUM_QUEUE_SIZE);
	for (index= 0; index<count; index++)
	{
		*(queue->buffer + queue->write_index) = *action_flags++;
		INCREMENT_QUEUE_COUNTER(queue->write_index);
		if (queue->write_index == queue->read_index)
		{
			dprintf("blew recording queue for player %d", player_identifier);
		}
	}
}

/*********************************************************************************************
 *
 * Function: save_recording_queue_chunk
 * Purpose:  saves one chunk of the queue to the recording file, using run-length encoding.
 *
 *********************************************************************************************/
static void save_recording_queue_chunk(
	short player_index)
{
	long *location;
	long last_flag, count, flag = 0;
	short i, run_count, num_flags_saved, max_flags;
	static long *buffer= NULL;
	ActionQueue *queue;

	if (buffer == NULL)
	{
		buffer = (long *)NewPtr((RECORD_CHUNK_SIZE * sizeof(long)) + RECORD_CHUNK_SIZE * sizeof(short));
	}
	
	location= buffer;
	count= 0; // keeps track of how many bytes we'll save.
	last_flag= NONE;

	queue= get_player_recording_queue(player_index);
	
	// don't want to save too much stuff
	max_flags= MIN(RECORD_CHUNK_SIZE, get_recording_queue_size(player_index)); 

	// save what's in the queue
	run_count= num_flags_saved= 0;
	for (i = 0; i<max_flags; i++)
	{
		flag = *(queue->buffer + queue->read_index);
		INCREMENT_QUEUE_COUNTER(queue->read_index);
		
		if (i && flag != last_flag)
		{
			*(short*)location = run_count;
			((short*)location)++;
			*location++ = last_flag;
			count += sizeof(short) + sizeof(long);
			num_flags_saved += run_count;
			run_count = 1;
		}
		else
		{
			run_count++;
		}
		last_flag = flag;
	}
	
	// now save the final run
	*(short*)location = run_count;
	((short*)location)++;
	*location++ = last_flag;
	count += sizeof(short) + sizeof(long);
	num_flags_saved += run_count;
	
	if (max_flags<RECORD_CHUNK_SIZE)
	{
		*(short*)location = END_OF_RECORDING_INDICATOR;
		((short*)location)++;
		*location++ = 0;
		count += sizeof(short) + sizeof(long);
		num_flags_saved += RECORD_CHUNK_SIZE-max_flags;
	}
	
	FSWrite(replay.recording_file_refnum, &count, buffer);
	replay.header.length+= count;
		
	vwarn(num_flags_saved == RECORD_CHUNK_SIZE,
		csprintf(temporary, "bad recording: %d flags, max=%d, count = %d;dm #%d #%d", num_flags_saved, max_flags, 
			count, buffer, count));
}

/*********************************************************************************************
 *
 * Function: read_recording_queue_chunks
 * Purpose:  reads chunks into all of the player queues
 *
 *********************************************************************************************/
static void read_recording_queue_chunks(
	void)
{
	long         i;
	long         sizeof_read;
	long         action_flags;
	Size         film_size;
	short        count, player_index, num_flags;
	OSErr        error;
	ActionQueue  *queue;
	
	if (replay.film_resource) film_size = GetHandleSize(replay.film_resource);
	
	for (player_index = 0; player_index < dynamic_world->player_count; player_index++)
	{
		queue= get_player_recording_queue(player_index);
		for (count = 0; count < RECORD_CHUNK_SIZE; )
		{
			if (replay.film_resource)
			{
				boolean hit_end= FALSE;
				
				if (replay.film_resource_offset >= film_size)
				{
					hit_end = TRUE;
				}
				else
				{
					HLock(replay.film_resource);
					num_flags = * (short *) (*replay.film_resource + replay.film_resource_offset);
					replay.film_resource_offset += sizeof(num_flags);
					action_flags = * (long *) (*replay.film_resource + replay.film_resource_offset);
					replay.film_resource_offset += sizeof(action_flags);
				}
				
				if (hit_end || num_flags == END_OF_RECORDING_INDICATOR)
				{
					replay.have_read_last_chunk= TRUE;
					break;
				}
			}
			else
			{
				sizeof_read = sizeof(num_flags);
				error = vblFSRead(replay.recording_file_refnum, &sizeof_read, &num_flags);
				if (error == noErr)
				{
					sizeof_read = sizeof(action_flags);
					error = vblFSRead(replay.recording_file_refnum, &sizeof_read, &action_flags);
					assert(error == noErr || (error == eofErr && sizeof_read == sizeof(action_flags)));
				}
				
				if ((error == eofErr && sizeof_read != sizeof(long)) || num_flags == END_OF_RECORDING_INDICATOR)
				{
					replay.have_read_last_chunk = TRUE;
					break;
				}
			}
			assert(replay.have_read_last_chunk || num_flags);
			count += num_flags;
			vassert((num_flags != 0 && count <= RECORD_CHUNK_SIZE) || replay.have_read_last_chunk, 
				csprintf(temporary, "num_flags = %d, count = %d", num_flags, count));
			for (i = 0; i < num_flags; i++)
			{
				*(queue->buffer + queue->write_index) = action_flags;
				INCREMENT_QUEUE_COUNTER(queue->write_index);
				assert(queue->read_index != queue->write_index);
			}
		}
		assert(replay.have_read_last_chunk || count == RECORD_CHUNK_SIZE);
	}
}

/*********************************************************************************************
 *
 * Function: pull_flags_from_recording
 * Purpose:  remove one flag from each queue from the recording buffer.
 * Returns:  TRUE if it pulled the flags, FALSE if it didn't
 *
 *********************************************************************************************/
static boolean pull_flags_from_recording(
	short count)
{
	short player_index;
	boolean success= TRUE;
	
	// first check that we can pull something from each player’s queue
	// (at the beginning of the game, we won’t be able to)
	// i'm not sure that i really need to do this check. oh well.
	for (player_index = 0; success && player_index<dynamic_world->player_count; player_index++)
	{
		if(get_recording_queue_size(player_index)==0) success= FALSE;
	}

	if(success)
	{
		for (player_index = 0; player_index < dynamic_world->player_count; player_index++)
		{
			short index;
			ActionQueue  *queue;
		
			queue= get_player_recording_queue(player_index);
			for (index= 0; index<count; index++)
			{
				if (queue->read_index != queue->write_index)
				{
#ifdef DEBUG_REPLAY
					debug_stream_of_flags(*(queue->buffer+queue->read_index), player_index);
#endif
					queue_action_flags(player_index, queue->buffer+queue->read_index, 1);
					INCREMENT_QUEUE_COUNTER(queue->read_index);
				} else {
dprintf("Dropping flag?");
				}
			}
		}
	}
	
	return success;
}

/*********************************************************************************************
 *
 * Function: vblFSRead
 * Purpose:  a replacement for FSRead, only used in this file (from read_recording_queue_chunks)
 *           It is called exactly like FSRead. It reads in a bigger chunk than necessary, so 
 *           that we don't call FSRead so often. This is particularly important since FSRead is
 *           currently not native on the powermacs.
 * Note:     This limits read calls so that you can only read less than DISK_CACHE_SIZE bytes.
 *
 *********************************************************************************************/
static OSErr vblFSRead(short 
	refnum, 
	long *count, 
	void *dest)
{
	long   fsread_count;
	OSErr  error = noErr;
	
	assert(replay.fsread_buffer);

	if (replay.bytes_in_cache < *count)
	{
		assert(replay.bytes_in_cache + *count < DISK_CACHE_SIZE);
		if (replay.bytes_in_cache)
		{
			BlockMove(replay.location_in_cache, replay.fsread_buffer, replay.bytes_in_cache);
		}
		replay.location_in_cache = replay.fsread_buffer;
		fsread_count = DISK_CACHE_SIZE - replay.bytes_in_cache;
		assert(fsread_count > 0);
		error = FSRead(refnum, &fsread_count, replay.fsread_buffer + replay.bytes_in_cache);
		replay.bytes_in_cache += fsread_count;
	}

	if (error == eofErr && replay.bytes_in_cache < *count)
	{
		*count = replay.bytes_in_cache;
	}
	else
	{
		error = noErr;
	}
	
	BlockMove(replay.location_in_cache, dest, *count);
	replay.bytes_in_cache -= *count;
	replay.location_in_cache += *count;
	
	return error;
}

static OSErr get_freespace_on_disk(
	short vRefNum,
	unsigned long *free_space)
{
	OSErr           error;
	HParamBlockRec  parms;

	memset(&parms, 0, sizeof(HParamBlockRec));	
	parms.volumeParam.ioCompletion = NULL;
	parms.volumeParam.ioVolIndex = 0;
	parms.volumeParam.ioNamePtr = temporary;
	parms.volumeParam.ioVRefNum = vRefNum;
	
	error = PBHGetVInfo(&parms, FALSE);
	if (error == noErr)
		*free_space = (unsigned long) parms.volumeParam.ioVAlBlkSiz * (unsigned long) parms.volumeParam.ioVFrBlk;
	return error;
}

static short get_recording_queue_size(
	short which_queue)
{
	short size;
	ActionQueue *queue= get_player_recording_queue(which_queue);

	/* Note that this is a circular queue */
	size= queue->write_index-queue->read_index;
	if(size<0) size+= MAXIMUM_QUEUE_SIZE;
	
	return size;
}

/*********************************************************************************************
 *
 * Function: precalculate_key_information
 * Purpose:  fix up the keys structure for parse_keymaps to work properly.
 *
 *********************************************************************************************/
static void precalculate_key_information(
	void)
{
	short i;
	
	/* convert raw key codes to offets and masks */
	for (i = 0; i < NUMBER_OF_STANDARD_KEY_DEFINITIONS; ++i)
	{
		current_key_definitions[i].mask = 1 << (current_key_definitions[i].offset&7);
		current_key_definitions[i].offset >>= 3;
	}
	
	return;
}

/* true if it found it, false otherwise. always fills in vrefnum and dirid*/
static boolean get_recording_fsspec(
	FSSpec *file)
{
	short vRef;
	long parID;
	OSErr error;

	error= FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, &vRef, &parID);
	if(!error)
	{
		getpstr(temporary, strFILENAMES, filenameMARATHON_RECORDING);
		error= FSMakeFSSpec(vRef, parID, temporary, file);
	}
	
	return (error==noErr);
}

#define COPY_BUFFER_SIZE (3*1024)

static OSErr copy_file(
	FSSpec *source,
	FSSpec *destination)
{
	OSErr err;
	FInfo info;
	
	err= FSpGetFInfo(source, &info);
	if(!err)
	{
		err= FSpCreate(destination, info.fdCreator, info.fdType, smSystemScript);
		if(!err)
		{
			short dest_refnum, source_refnum;
		
			err= FSpOpenDF(destination, fsWrPerm, &dest_refnum);
			if(!err)
			{
				err= FSpOpenDF(source, fsRdPerm, &source_refnum);
				if(!err)
				{
					/* Everything is opened. Do the deed.. */
					Ptr data;
					long total_length;
					
					SetFPos(source_refnum, fsFromLEOF, 0l);
					GetFPos(source_refnum, &total_length);
					SetFPos(source_refnum, fsFromStart, 0l);
					
					data= NewPtr(COPY_BUFFER_SIZE);
					if(data)
					{
						long running_length= total_length;
						
						while(running_length && !err)
						{
							long count= MIN(COPY_BUFFER_SIZE, running_length);
						
							err= FSRead(source_refnum, &count, data);
							if(!err)
							{
								err= FSWrite(dest_refnum, &count, data);
							}
							running_length -= count;
						}
					
						DisposePtr(data);
					} else {
						err= MemError();
					}
					
					FSClose(source_refnum);
				}

				FSClose(dest_refnum);
			}

			/* Delete it on an error */
			if(err) FSpDelete(destination);
		}
	}
	
	return err;
}

#ifdef DEBUG_REPLAY
#define MAXIMUM_STREAM_FLAGS (8192) // 48k

struct recorded_flag {
	long flag;
	short player_index;
};

static short stream_refnum= NONE;
static struct recorded_flag *action_flag_buffer= NULL;
static long action_flag_index= 0;

static void open_stream_file(
	void)
{
	FSSpec file;
	char name[]= "\pReplay Stream";
	OSErr error;

	get_my_fsspec(&file);
	memcpy(file.name, name, name[0]+1);
	
	FSpDelete(&file);
	error= FSpCreate(&file, 'ttxt', 'TEXT', smSystemScript);
	if(error) dprintf("Err:%d", error);
	error= FSpOpenDF(&file, fsWrPerm, &stream_refnum);
	if(error || stream_refnum==NONE) dprintf("Open Err:%d", error);
	
	action_flag_buffer= (struct recorded_flag *) malloc(MAXIMUM_STREAM_FLAGS*sizeof(struct recorded_flag));
	assert(action_flag_buffer);
	action_flag_index= 0;
}

static void write_flags(
	struct recorded_flag *buffer, 
	long count)
{
	long index, size;
	boolean sorted= TRUE;
	OSErr error;

	sprintf(temporary, "%d Total Flags\n", count);
	size= strlen(temporary);
	error= FSWrite(stream_refnum, &size, temporary);
	if(error) dprintf("Error: %d", error);

	if(sorted)
	{
		short player_index;
		
		for(player_index= 0; player_index<dynamic_world->player_count; ++player_index)
		{
			short player_action_flag_count= 0;
		
			for(index= 0; index<count; ++index)
			{
				if(buffer[index].player_index==player_index)
				{
					if(!(player_action_flag_count%TICKS_PER_SECOND))
					{
						sprintf(temporary, "%d 0x%08x (%d secs)\n", buffer[index].player_index, 
							buffer[index].flag, player_action_flag_count/TICKS_PER_SECOND);
					} else {
						sprintf(temporary, "%d 0x%08x\n", buffer[index].player_index, buffer[index].flag);
					}
					size= strlen(temporary);
					error= FSWrite(stream_refnum, &size, temporary);
					if(error) dprintf("Error: %d", error);
					player_action_flag_count++;
				}
			}
		}
	} else {
		for(index= 0; index<count; ++index)
		{
			sprintf(temporary, "%d 0x%08x\n", buffer[index].player_index, buffer[index].flag);
			size= strlen(temporary);
			error= FSWrite(stream_refnum, &size, temporary);
			if(error) dprintf("Error: %d", error);
		}
	}
}

static void debug_stream_of_flags(
	long action_flag,
	short player_index)
{
	if(stream_refnum != NONE)
	{
		assert(action_flag_buffer);
		if(action_flag_index<MAXIMUM_STREAM_FLAGS)
		{
			action_flag_buffer[action_flag_index].player_index= player_index;
			action_flag_buffer[action_flag_index++].flag= action_flag;
		}
	}
}

static void close_stream_file(
	void)
{
	if(stream_refnum != NONE)
	{
		assert(action_flag_buffer);

		write_flags(action_flag_buffer, action_flag_index-1);
		FSClose(stream_refnum);
		
		free(action_flag_buffer);
		action_flag_buffer= NULL;
	}
}
#endif

void set_recording_header_data(
	short number_of_players, 
	short level_number, 
	unsigned long map_checksum,
	short version, 
	struct player_start_data *starts, 
	struct game_data *game_information)
{
	assert(!replay.valid);
	memset(&replay.header, 0, sizeof(struct recording_header));
	replay.header.num_players= number_of_players;
	replay.header.level_number= level_number;
	replay.header.map_checksum= map_checksum;
	replay.header.version= version;
	memcpy(replay.header.starts, starts, MAXIMUM_NUMBER_OF_PLAYERS*sizeof(struct player_start_data));
	memcpy(&replay.header.game_information, game_information, sizeof(struct game_data));
	replay.header.length= sizeof(struct recording_header);

	return;
}

void get_recording_header_data(
	short *number_of_players, 
	short *level_number, 
	unsigned long *map_checksum,
	short *version, 
	struct player_start_data *starts, 
	struct game_data *game_information)
{
	assert(replay.valid);
	*number_of_players= replay.header.num_players;
	*level_number= replay.header.level_number;
	*map_checksum= replay.header.map_checksum;
	*version= replay.header.version;
	memcpy(starts, replay.header.starts, MAXIMUM_NUMBER_OF_PLAYERS*sizeof(struct player_start_data));
	memcpy(game_information, &replay.header.game_information, sizeof(struct game_data));
	
	return;
}

boolean setup_for_replay_from_file(
	FSSpec *file,
	unsigned long map_checksum)
{
	OSErr error;
	boolean successful= FALSE;

	error= FSpOpenDF(file, fsCurPerm, &replay.recording_file_refnum);
	if(!error && replay.recording_file_refnum > 0)
	{
		long count;
	
		replay.valid= TRUE;
		replay.have_read_last_chunk = FALSE;
		replay.game_is_being_replayed = TRUE;
		replay.film_resource= NULL;
		replay.film_resource_offset= NONE;
		
		count= sizeof(struct recording_header); 
		FSRead(replay.recording_file_refnum, &count, &replay.header);

		if(replay.header.map_checksum==map_checksum)
		{
			replay.fsread_buffer= NewPtr(DISK_CACHE_SIZE); 
			assert(replay.fsread_buffer);
			if(!replay.fsread_buffer) alert_user(fatalError, strERRORS, outOfMemory, MemError());
			
			replay.vRefNum= file->vRefNum;
			replay.location_in_cache= NULL;
			replay.bytes_in_cache= 0;
			replay.replay_speed= 1;
			
#ifdef DEBUG_REPLAY
			open_stream_file();
#endif
			successful= TRUE;
		}
	}
	
	return successful;
}

boolean setup_replay_from_random_resource(
	unsigned long map_checksum)
{
	short number_of_films;
	static short index_of_last_film_played= 0;
	boolean success= FALSE;
	
	number_of_films= Count1Resources(FILM_RESOURCE_TYPE);
	if(number_of_films > 0)
	{
		short which_film_to_play;

		if (number_of_films == 1)
		{
			which_film_to_play= 0;
		}
		else
		{
			for (which_film_to_play = index_of_last_film_played; 
					which_film_to_play == index_of_last_film_played;
					which_film_to_play = (abs(Random()) % number_of_films))
				;
			index_of_last_film_played= which_film_to_play;
		}
	
		replay.film_resource= GetIndResource(FILM_RESOURCE_TYPE, which_film_to_play+1);
		vassert(replay.film_resource, csprintf(temporary, "film_to_play = %d", which_film_to_play+1));
		
//		heartbeat_count= 0;
		
		HLock(replay.film_resource);
		BlockMove(*replay.film_resource, &replay.header, sizeof(struct recording_header));
		HUnlock(replay.film_resource);

		if(replay.header.map_checksum==map_checksum)
		{
			replay.film_resource_offset= 0;
			replay.have_read_last_chunk= FALSE;
			replay.game_is_being_replayed= TRUE;
			replay.film_resource_offset += sizeof(struct recording_header);

			success= TRUE;
		} else {
			ReleaseResource(replay.film_resource);
			replay.film_resource= NULL;
		}
		
		replay.replay_speed= 1;
		replay.valid= TRUE;
	}
	
	return success;
}

/* Note that we _must_ set the header information before we start recording!! */
void start_recording(
	void)
{
	FSSpec recording_file;
	long count;
	OSErr error;
	
	assert(!replay.valid);
	replay.valid= TRUE;
	
	if(get_recording_fsspec(&recording_file))
	{
		FSpDelete(&recording_file);
	}
	
	error= FSpCreate(&recording_file, APPLICATION_CREATOR, FILM_FILE_TYPE, smSystemScript);
	if(!error)
	{
		/* I debate the validity of fsCurPerm here, but Alain had it, and it has been working */
		error= FSpOpenDF(&recording_file, fsCurPerm, &replay.recording_file_refnum);
		if(!error && replay.recording_file_refnum != NONE)
		{
			replay.game_is_being_recorded= TRUE;
			replay.vRefNum= recording_file.vRefNum;
	
			// save a header containing information about the game.
			count= sizeof(struct recording_header); 
			FSWrite(replay.recording_file_refnum, &count, &replay.header);
		}
	}
}

void stop_recording(
	void)
{
	if (replay.game_is_being_recorded)
	{
		short player_index;
		long count;
		long total_length;

		assert(replay.valid);
		for (player_index= 0; player_index<dynamic_world->player_count; player_index++)
		{
			save_recording_queue_chunk(player_index);
		}

		replay.game_is_being_recorded = FALSE;
		
		/* Rewrite the header, since it has the new length */
		SetFPos(replay.recording_file_refnum, fsFromStart, 0l); 
		count= sizeof(struct recording_header);
		FSWrite(replay.recording_file_refnum, &count, &replay.header);
		assert(count==sizeof(struct recording_header));

		SetFPos(replay.recording_file_refnum, fsFromLEOF, 0l);
		GetFPos(replay.recording_file_refnum, &total_length);
		assert(total_length==replay.header.length);
		
		FSClose(replay.recording_file_refnum);

	}

	replay.valid= FALSE;
	
	return;
}

void rewind_recording(
	void)
{
	if(replay.game_is_being_recorded)
	{
		/* This is unnecessary, because it is called from reset_player_queues, */
		/* which is always called from revert_game */
		SetEOF(replay.recording_file_refnum, sizeof(struct recording_header));
		SetFPos(replay.recording_file_refnum, fsFromLEOF, 0);
		replay.header.length= sizeof(struct recording_header);
	}
	
	return;
}

void initialize_keyboard_controller(
	void)
{
	ActionQueue *queue;
	short player_index;
	
	vassert(NUMBER_OF_KEYS == NUMBER_OF_STANDARD_KEY_DEFINITIONS,
		csprintf(temporary, "NUMBER_OF_KEYS == %d, NUMBER_OF_KEY_DEFS = %d. Not Equal!", NUMBER_OF_KEYS, NUMBER_OF_STANDARD_KEY_DEFINITIONS));
	assert(NUMBER_OF_STANDARD_KEY_DEFINITIONS==NUMBER_OF_LEFT_HANDED_KEY_DEFINITIONS);
	assert(NUMBER_OF_LEFT_HANDED_KEY_DEFINITIONS==NUMBER_OF_POWERBOOK_KEY_DEFINITIONS);
	
	// get globals initialized
	heartbeat_count= 0;
	input_task_active= FALSE;
	memset(&replay, 0, sizeof(struct replay_private_data));
	replay.game_is_being_recorded= FALSE;
	replay.game_is_being_replayed= FALSE;
	replay.valid= FALSE;
	
	input_task = myTMSetup(1000/TICKS_PER_SECOND, input_controller);
	assert(input_task);
	
	atexit(remove_input_controller);
	set_keys(input_preferences->keycodes);
	
	/* Allocate the recording queues */	
	replay.recording_queues = (ActionQueue *) NewPtr(MAXIMUM_NUMBER_OF_PLAYERS * sizeof(ActionQueue));
	assert(replay.recording_queues);
	if(!replay.recording_queues) alert_user(fatalError, strERRORS, outOfMemory, MemError());
	
	/* Allocate the individual ones */
	for (player_index= 0; player_index<MAXIMUM_NUMBER_OF_PLAYERS; player_index++)
	{
		queue= get_player_recording_queue(player_index);
		queue->read_index= queue->write_index = 0;
		queue->buffer= (long *) NewPtr(MAXIMUM_QUEUE_SIZE*sizeof(long));
		assert(queue->buffer);
		if(!queue->buffer) alert_user(fatalError, strERRORS, outOfMemory, MemError());
	}
	enter_mouse(0);
	
	return;
}

void check_recording_replaying(
	void)
{
	short player_index, queue_size;

	if (replay.game_is_being_recorded)
	{
		boolean enough_data_to_save= TRUE;
	
		// it's time to save the queues if all of them have >= RECORD_CHUNK_SIZE flags in them.
		for (player_index= 0; enough_data_to_save && player_index<dynamic_world->player_count; player_index++)
		{
			queue_size= get_recording_queue_size(player_index);
			if (queue_size < RECORD_CHUNK_SIZE)	enough_data_to_save= FALSE;
		}
		
		if(enough_data_to_save)
		{
			OSErr error;
			unsigned long freespace;

			error= get_freespace_on_disk(replay.vRefNum, &freespace);
			if (!error && freespace>(RECORD_CHUNK_SIZE*sizeof(short)*sizeof(long)*dynamic_world->player_count))
			{
				for (player_index= 0; player_index<dynamic_world->player_count; player_index++)
				{
					save_recording_queue_chunk(player_index);
				}
			}
		}
	}
	else if (replay.game_is_being_replayed)
	{
		boolean load_new_data= TRUE;
	
		// it's time to refill the requeues if they all have < RECORD_CHUNK_SIZE flags in them.
		for (player_index= 0; load_new_data && player_index<dynamic_world->player_count; player_index++)
		{
			queue_size= get_recording_queue_size(player_index);
			if(queue_size>= RECORD_CHUNK_SIZE) load_new_data= FALSE;
		}
		
		if(load_new_data)
		{
			// at this point, we’ve determined that the queues are sufficently empty, so
			// we’ll fill ’em up.
			read_recording_queue_chunks();
		}
	}

	return;
}

void reset_recording_and_playback_queues(
	void)
{
	short index;
	
	for(index= 0; index<MAXIMUM_NUMBER_OF_PLAYERS; ++index)
	{
		replay.recording_queues[index].read_index= replay.recording_queues[index].write_index= 0;
	}
}

boolean find_replay_to_use(
	boolean ask_user, 
	FSSpec *file)
{
	boolean successful = FALSE;
	
	if(ask_user)
	{
		StandardFileReply reply;
		SFTypeList types;
		short type_count= 0;

		types[type_count++]= FILM_FILE_TYPE;
		
		StandardGetFile(NULL, type_count, types, &reply);
		if (reply.sfGood)
		{
			memcpy(file, &reply.sfFile, sizeof(FSSpec));
			successful= TRUE;
		}
	}
	else
	{
		successful= get_recording_fsspec(file);
	}

	return successful;
}