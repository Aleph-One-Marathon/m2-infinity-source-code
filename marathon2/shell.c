/*
SHELL.C
Tuesday, June 19, 1990 7:20:13 PM

Tuesday, July 17, 1990 12:26:24 AM
	Minotaur modifications.
Tuesday, November 26, 1991 11:29:11 PM
	This shit needs to be totally rewritten.
Tuesday, December 17, 1991 11:19:09 AM
	(strongly favoring tuesdays) ... we now work under single finder, hopefully, and this shit
	is being totally rewritten.
Tuesday, November 17, 1992 3:32:11 PM
	changes for Pathways into Darkness.  this shell will compile into editor_shell.c.o and
	game_shell.c.o based on whether GAME or EDITOR is defined in the preprocessor.
Saturday, December 12, 1992 9:41:53 AM
	we now pass through some command keys, when not handled by the menus.
Friday, April 23, 1993 7:44:35 AM
	added ai timing voodoo for mouse-in-world and closing immediately after an explicit save.
Wednesday, June 2, 1993 9:17:08 AM
	lots of changes for new first-event dialogs.  happy birthday, yesterday.  talked to mara for
	half an hour in front of brek.
Friday, July 30, 1993 6:49:48 PM
	the day before the big build; final modifications to copy-protection stuff.
Saturday, August 21, 1993 4:31:50 PM
	hacks for OZONE.
Monday, August 30, 1993 3:18:37 PM
	menu bar?, what menu bar?
Wednesday, December 8, 1993 1:02:36 PM
	this is now MARATHON.
*/


#include "macintosh_cseries.h"
#include "my32bqd.h"
#include "preferences.h"

#include "map.h"
#include "monsters.h"
#include "player.h"
#include "render.h"
#include "shell.h"
#include "interface.h"
#include "sound.h"
#include "music.h"
#include "fades.h"

#include <appleevents.h>
//#include <gestaltequ.h>
#include "tags.h" /* for scenario file type.. */

#include "network_sound.h"
#include "mouse.h"
#include "screen_drawing.h"
#include "computer_interface.h"

#ifdef PERFORMANCE
#include <Perf.h>
#endif

#ifdef mpwc
#pragma segment shell
#endif

/* ---------- constants */
#define alrtQUIT_FOR_SURE 138

#define TICKS_BETWEEN_XNEXTEVENT_CALLS 10
#define MAXIMUM_CONSECUTIVE_GETOSEVENT_CALLS 12 // 2 seconds

#ifdef COMPILE_TIME
#define DAYS_TO_EXPIRATION 18
#define SECONDS_TO_EXPIRATION (DAYS_TO_EXPIRATION * 60 * 60 * 24)
#define EXPIRATION_TIME (COMPILE_TIME + SECONDS_TO_EXPIRATION)
#endif

#define kMINIMUM_MUSIC_HEAP (4*MEG)
#define kMINIMUM_SOUND_HEAP (3*MEG)

#define MAXIMUM_KEYWORD_LENGTH 20
#define NUMBER_OF_KEYWORDS (sizeof(keywords)/sizeof(struct keyword_data))

#define CLOSE_WITHOUT_WARNING_DELAY (5*TICKS_PER_SECOND)

#define menuGAME  128

enum // cheat tags
{
	_tag_health,
	_tag_oxygen,
	_tag_map,
	_tag_fusion,
	_tag_invincible,
	_tag_invisible,
	_tag_extravision,
	_tag_infravision,
	_tag_pistol,
	_tag_rifle,
	_tag_missle,
	_tag_toaster,  // flame-thrower
	_tag_pzbxay,
	_tag_ammo,
	_tag_pathways,
	_tag_view,
	_tag_jump,
	_tag_aslag,
	_tag_save
};

enum // command keys
{
	_command_key_pause,
	_command_key_save,
	_command_key_revert,
	_command_key_close,
	_command_key_quit,
	NUMBER_OF_COMMAND_KEYS
};

/* ---------- resources */

/* ---------- structures */
struct keyword_data
{
	short tag;
	char *keyword; /* in uppercase */
};

struct command_key_data
{
	short tag;
	short command_key; // gotten from resource
};

/* ---------- globals */
static struct keyword_data keywords[]=
{
	{_tag_health, "HEALTH"},
	{_tag_oxygen, "OXYGEN"},
	{_tag_map, "MAP"},
	{_tag_invincible, "HAVESOME"},
	{_tag_infravision, "ISEEYOU"},
	{_tag_extravision, "FISHEYE"},
	{_tag_invisible, "BYE"},
	{_tag_pistol, "PISTOL"},
	{_tag_rifle, "RIFLE"},
	{_tag_missle, "POW"},
	{_tag_toaster, "TOAST"},
	{_tag_fusion, "MELT"},
	{_tag_pzbxay, "PZBXAY"}, // the alien shotgon, in the phfor's language
	{_tag_ammo, "AMMO"},
	{_tag_jump, "ASD"},
	{_tag_aslag, "ASLAG"},
	{_tag_save, "SAVE"},
};
static char keyword_buffer[MAXIMUM_KEYWORD_LENGTH+1];

static struct command_key_data command_keys[NUMBER_OF_COMMAND_KEYS]=
{
	{_command_key_pause},
	{_command_key_save},
	{_command_key_revert},
	{_command_key_close},
	{_command_key_quit}
};

static short game_status;
struct preferences_data *preferences;
struct system_information_data *system_information;
static short initial_sound_volume;

static boolean suppress_background_tasks= TRUE;

#ifdef envppc
QDGlobals qd;
#endif

#ifdef PERFORMANCE
TP2PerfGlobals perf_globals;
#endif

extern long first_frame_tick, frame_count; /* for determining frame rate */

/* ---------- private code */

static void process_event(EventRecord *event);
static void process_key(EventRecord *event, short key);
static process_command_key(short key);
static short process_keyword_key(char key);

static void verify_environment(void);
static void initialize_application_heap(void);

static void initialize_core_events(void);
void handle_high_level_event(EventRecord *event);
static pascal OSErr handle_open_document(AppleEvent *event, AppleEvent *reply, long myRefCon);
static pascal OSErr handle_quit_application(AppleEvent *event, AppleEvent *reply, long myRefCon);
static pascal OSErr handle_print_document(AppleEvent *event, AppleEvent *reply, long myRefCon);
static pascal OSErr handle_open_application(AppleEvent *event, AppleEvent *reply, long myRefCon);
static OSErr required_appleevent_check(AppleEvent *event);
static boolean has_system_seven(void);
static void get_machine_type(boolean *machine_is_68k, boolean *machine_is_68040, boolean *machine_is_ppc);

static void marathon_dialog_header_proc(DialogPtr dialog, Rect *frame);

static void adjust_chase_camera(void);
short can_get_to_point(short start_polygon, world_point3d *start, world_point3d *end);

void handle_keyword(short type_of_cheat);
boolean is_keypad(short keycode);
static void setup_command_keys(void);

static void check_for_map_file(void);

static void script_demo(void);

/* ---------- code */

void main(
	void)
{	

	set_game_status(no_game_in_progress);
	
	initialize_application_heap();

	initialize_core_events();

	do_top_level_interface();
	
	exit(0);
}

void game_main_event_loop(void)
{
	boolean  done = FALSE;
	EventRecord event;
	long last_xnextevent_call= 0;
	short consecutive_getosevent_calls= 0;

	while (!done)
	{
		short ticks_elapsed;
		
		/* get and handle events; GetOSEvent() is much less friendly to background processes than
			WaitNextEvent() */
		if (TickCount()-last_xnextevent_call>=TICKS_BETWEEN_XNEXTEVENT_CALLS)
		{
			stay_awake();
			global_idle_proc();
			if (suppress_background_tasks && EmptyRgn(((WindowPeek)screen_window)->updateRgn)) // && consecutive_getosevent_calls<MAXIMUM_CONSECUTIVE_GETOSEVENT_CALLS)
			{
				if (GetOSEvent(everyEvent, &event)) process_event(&event);
//				consecutive_getosevent_calls+= 1;
			}
			else
			{
				if (WaitNextEvent(everyEvent, &event, 0, (RgnHandle) NULL)) process_event(&event);
//				consecutive_getosevent_calls= 0;
			}
			FlushEvents(keyDownMask|keyUpMask|autoKeyMask, 0);
			
			last_xnextevent_call= TickCount();
		}

		network_speaker_idle_proc();
		sound_manager_idle_proc();

		switch(get_game_status())
		{
			case game_in_progress:
			case replay_in_progress:
			case demo_in_progress:
				break;
			case user_wants_quit_from_game:
				done = TRUE; // bring up dialog, etc...
				break;
			case user_wants_quit_from_demo:
			case user_wants_quit_from_replay:
				done = TRUE;
				break;
			case demo_wants_to_switch_demos:
				done = TRUE;
				break;
			case no_game_in_progress:
				done = TRUE;
				halt(); // shouldn't happen?
				break;
			case user_wants_to_revert_game:
				revert_game();
				game_status = game_in_progress;
				break;
			default:
				halt();
				break;
		}
		
		/* if we’re not paused and there’s something to draw (i.e., anything different from
			last time), render a frame */
		if (!done && get_keyboard_controller_status() && (ticks_elapsed= update_world()))
		{
			switch (preferences->input_device)
			{
				case _mouse_yaw_pitch:
				case _mouse_yaw_velocity:
// now called from parse_keymap() (so calling it here would eat data)
//					mouse_idle(preferences->input_device); 
					break;
			}

			render_screen(ticks_elapsed);
		}
	}
	
	return;
}

/* somebody wants to do something important; free as much temporary memory as possible and
	unlock anything we can */
void free_and_unlock_memory(
	void)
{
	stop_all_sounds();
	
	return;
}

/* ---------- private code */

static void initialize_application_heap(
	void)
{
	MaxApplZone();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	MoreMasters();

	InitGraf((Ptr)&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0); /* resume procedure ignored for multifinder and >=system 7.0 */
	InitCursor();

#ifdef DEBUG
	initialize_debugger(TRUE);
#endif

//	{
//		OSErr error;
//		
//		error= InitTSMAwareApplication();
//		vassert(error==noErr, csprintf(temporary, "InitTSMAwareApplication() == #%d", error));
//	}

	SetCursor(*GetCursor(watchCursor));

	qd.randSeed = TickCount();

#ifdef PERFORMANCE
	{
		Boolean succeeded;
		
		succeeded = InitPerf(&perf_globals, 4, 8, TRUE, TRUE, "\pCODE", 0, "", 0, 0, 0, 0);
		assert(succeeded);
	}
#endif

	set_to_default_map();

	read_preferences_file(&preferences, getpstr(temporary, strFILENAMES, filenamePREFERENCES),
		PREFERENCES_CREATOR, PREFERENCES_TYPE, PREFERENCES_VERSION, sizeof(struct preferences_data),
		initialize_preferences);
	verify_preferences(preferences);
//	if (!machine_supports_16bit() && preferences->screen_mode.bit_depth == 16)
//	{
//		preferences->screen_mode.bit_depth = 8;
//		write_preferences_file(preferences);
//	}

	GetDateTime(&preferences->last_time_ran);
	write_preferences_file(preferences);

#if defined(EXPIRATION_TIME) && !defined(AI_LIBRARY)
	{
		Handle         some_resource;
		unsigned long  current_time;
		
		// we’re assuming that beta copies are all fat versions
		some_resource = GetResource('SOCK', 128); // the socket listener
		if (!some_resource)
		{
			alert_user(fatalError, strERRORS, copyHasExpired, 0);
			ExitToShell();
		}

		GetDateTime(&current_time);
		if (current_time < preferences->last_time_ran || current_time > EXPIRATION_TIME)
		{
			some_resource= GetResource('CODE', 8); // the “render” segment
			if (some_resource) RmveResource(some_resource);
			some_resource= GetResource('SOCK', 128); // the socket listener
			if (some_resource) RmveResource(some_resource);
			UpdateResFile(CurResFile());
			alert_user(fatalError, strERRORS, copyHasExpired, 0);
			ExitToShell(); // fool the cracker. heh.
		}

	}
#endif

	initialize_my_32bqd();
	verify_environment();

	if (FreeMem()>kMINIMUM_SOUND_HEAP) initialize_sound_manager((struct sound_manager_parameters *)&preferences->sound_parameters);
	if (FreeMem()>kMINIMUM_MUSIC_HEAP) initialize_background_music();

	initialize_keyboard_controller();
	initialize_screen(&preferences->screen_mode);
	initialize_marathon();
	initialize_screen_drawing();
	initialize_shape_handler();
	initialize_fades();

	kill_screen_saver();

	set_dialog_header_proc(marathon_dialog_header_proc);

	setup_command_keys();
	
	SetCursor(&qd.arrow);

	return;
}

/* look for obviously ridiculous values in the preferences */
void verify_preferences(
	struct preferences_data *preferences)
{
	short    i;
	boolean  need_to_reinitialize = FALSE;
	
	if (preferences->name[0] > PREFERENCES_NAME_LENGTH || preferences->name[0] < 0)
		need_to_reinitialize = TRUE;
	else if (preferences->screen_mode.size != _100_percent
		&&   preferences->screen_mode.size != _75_percent
		&&   preferences->screen_mode.size != _50_percent
		&&   preferences->screen_mode.size != _full_screen)
		need_to_reinitialize = TRUE;
	else if (preferences->input_device != _keyboard_or_game_pad 
		&&   preferences->input_device != _mouse_yaw_pitch
		&&   preferences->input_device != _mouse_yaw_velocity
		&&   preferences->input_device != _cybermaxx_input)
		need_to_reinitialize = TRUE;
	else if (preferences->screen_mode.bit_depth != 8 && preferences->screen_mode.bit_depth != 16 && preferences->screen_mode.bit_depth != 32)
		need_to_reinitialize = TRUE;
	else if (preferences->difficulty_level < _wuss_level || preferences->difficulty_level > _total_carnage_level)
		need_to_reinitialize = TRUE;
	else
	{
		for (i = 0; i < NUMBER_OF_KEYS; i++)
		{
			if (preferences->keycodes[i] > 0x7f || preferences->keycodes[i] < 0)
			{
				need_to_reinitialize = TRUE;
				break;
			}
		}
	}
	
	if (need_to_reinitialize)
	{
		initialize_preferences(preferences);
		write_preferences_file(preferences);
	}
}

/* passed to read_preferences() */
void initialize_preferences(
	struct preferences_data *preferences)
{
	short    i;
	boolean  machine_is_68k, machine_is_ppc, machine_is_68040;

	set_random_seed(TickCount());
	for (i= 0; i<sizeof(struct preferences_data)/sizeof(word); ++i) ((word*)preferences)[i]= random();

#if !defined(DEMO) && defined(FINAL)
	ask_for_serial_number();
#endif
	
	get_machine_type(&machine_is_68k, &machine_is_68040, &machine_is_ppc);

	preferences->device_spec.slot= NONE;
	preferences->device_spec.flags= deviceIsColor;
	preferences->device_spec.bit_depth= 8;
		
	preferences->screen_mode.gamma_level= DEFAULT_GAMMA_LEVEL;
	if (hardware_acceleration_code() == _valkyrie_acceleration)
	{
		preferences->screen_mode.size= _100_percent;
		preferences->screen_mode.bit_depth = 16;
		preferences->screen_mode.high_resolution = FALSE;
		preferences->screen_mode.acceleration = _valkyrie_acceleration;
		preferences->screen_mode.texture_floor= TRUE, preferences->screen_mode.texture_ceiling= TRUE;
	}
	else if (machine_is_68k)
	{
		preferences->screen_mode.size= _100_percent;
		preferences->screen_mode.high_resolution= FALSE;
		if (machine_is_68040)
			preferences->screen_mode.texture_floor= TRUE, preferences->screen_mode.texture_ceiling= TRUE;
		else
			preferences->screen_mode.texture_floor= FALSE, preferences->screen_mode.texture_ceiling= FALSE;
		preferences->screen_mode.acceleration = _no_acceleration;
		preferences->screen_mode.bit_depth = 8;
	}
	else // we got a good machine
	{
		preferences->screen_mode.size= _100_percent;
		preferences->screen_mode.high_resolution= TRUE;
		preferences->screen_mode.texture_floor= TRUE, preferences->screen_mode.texture_ceiling= TRUE;
		preferences->screen_mode.acceleration = _no_acceleration;
		preferences->screen_mode.bit_depth = 8;
	}
	
	preferences->screen_mode.draw_every_other_line= FALSE;
	
	/* new with version 2 of the preferences */
	preferences->name[0] = 0;
	preferences->team = 0;
	
	/* new with version 4 of the prefs */
	set_default_keys(preferences, _standard_keyboard_setup);
	for (i = 0; i < NUMBER_UNUSED_KEYS; i++)
	{
		preferences->unused_keycodes[i] = 0;
	}
	
	/* new with version 5 of the prefs */
	preferences->render_compass= TRUE;
	
	/* new with version 7 of the prefs */
	GetDateTime(&preferences->last_time_ran);
	
	/* version 8 eliminated the preferences version—now in preferences.c */
	
	/* new with version 9 */
	preferences->input_device = _keyboard_or_game_pad;
	
	/* new with version 10 */
	preferences->record_every_game = TRUE;

	/* new with version 12 */
	preferences->difficulty_level = _normal_level;

	/* new with version 13 */
	if (machine_is_ppc && can_play_background_music())
		preferences->background_music_on = TRUE;
	else
		preferences->background_music_on = FALSE;

	/* new with version 14. network game options */
	preferences->network_type = NONE;
	preferences->allow_microphone = TRUE;
	preferences->network_difficulty_level = 0;
	preferences->network_game_options =	_multiplayer_game | _ammo_replenishes | _weapons_replenish
		| _specials_replenish |	_monsters_replenish | _burn_items_on_death | _suicide_is_penalized 
		| _force_unique_teams;
#ifdef DEMO
	preferences->network_time_limit = 5 * TICKS_PER_SECOND * 60;
	preferences->network_game_options&= ~_suicide_is_penalized;
#else
	preferences->network_time_limit = 10 * TICKS_PER_SECOND * 60;
#endif
	preferences->network_kill_limit = 10;
	preferences->network_entry_point= 0;
	
	/* new with version 15 */
	preferences->network_game_is_untimed = FALSE;

	/* set sound parameters */
	default_sound_manager_parameters((struct sound_manager_parameters *)&preferences->sound_parameters);

	return;
}

static void process_event(
	EventRecord *event)
{
	WindowPtr window;
	short part_code;

	switch (event->what)
	{
		case mouseDown:
			part_code= FindWindow(event->where, &window);
			switch (part_code)
			{
				case inSysWindow: /* DAs and the menu bar can blow me */
				case inMenuBar:
					halt();
					
				case inContent:
					// process_screen_click(event); // no longer does anything.
					if (get_game_status() == demo_in_progress)
					{
						set_game_status(user_wants_quit_from_demo);
					}
					break;
			}
			break;
		
		case keyDown:
		case autoKey:
			process_key(event, toupper(event->message&charCodeMask));
			break;
			
		case updateEvt:
			update_any_window((WindowPtr)(event->message), event);
			break;
			
		case activateEvt:
			activate_any_window((WindowPtr)(event->message), event, event->modifiers&activeFlag);
			break;

		case OSEvt:
			switch (event->message>>24)
			{
				case SuspendResumeMessage:
					if (event->message&ResumeMask)
					{
						/* resume */
						SetCursor(&qd.arrow);
					}
					else
					{
						/* suspend */
					}
					break;
			}
			break;
	}
		
	return;
}

void global_idle_proc(
	void)
{
	background_music_idle_proc();
	
	return;
}

static void process_key(
	EventRecord *event,
	short key)
{
	short    virtual, type_of_cheat;
	boolean  changed_screen_mode = FALSE;
	boolean  changed_prefs = FALSE;
	boolean  changed_volume = FALSE;

	if (key == 0) // this is what stay_awake posts
		return;

	virtual = (event->message >> 8) & charCodeMask;
	
	if (!game_is_networked && get_game_status() == game_in_progress)
	{
		type_of_cheat = process_keyword_key(key);
		if (type_of_cheat != NONE) handle_keyword(type_of_cheat);
	}
	
	if (event->modifiers&cmdKey)
	{
		process_command_key(key);
	}
	else if (!is_keypad(virtual))
	{
		switch(key)
		{
			case '.': case '>': // sound volume up
				changed_prefs= adjust_sound_volume_up((struct sound_manager_parameters *)&preferences->sound_parameters, _snd_adjust_volume);
				break;
			case ',': case '<': // sound volume down.
				changed_prefs= adjust_sound_volume_down((struct sound_manager_parameters *)&preferences->sound_parameters, _snd_adjust_volume);
				break;
			case kDELETE: // switch player view
				walk_player_list();
				render_screen(0);
				break;
			case '+': case '=':
				zoom_overhead_map_in();
				break;
			case '-': case '_':
				zoom_overhead_map_out();
				break;
			case '[': case '{':
				if (game_status == game_in_progress)
				{
					scroll_inventory(-1);
				}
				else
				{
					decrement_replay_speed();
				}
				break;
			case ']': case '}':
				if (game_status == game_in_progress)
				{
					scroll_inventory(1);
				}
				else
				{
					increment_replay_speed();
				}
				break;

#ifndef FINAL
			case '!':
				script_demo();
				break;
#endif

			case '%':
				suppress_background_tasks= !suppress_background_tasks;
				break;

			case '?':
				{
					extern boolean displaying_fps;
					
					displaying_fps= !displaying_fps;
				}
				break;
			
			default: // well, let's check the function keys then, using the keycodes.
				switch(virtual)
				{
					case kcF1:
						preferences->screen_mode.size = _full_screen;
						changed_screen_mode = changed_prefs = TRUE;
						break;
					case kcF2:
						preferences->screen_mode.size = _100_percent;
						changed_screen_mode = changed_prefs = TRUE;
						break;
					case kcF3:
						preferences->screen_mode.size = _75_percent;
						changed_screen_mode = changed_prefs = TRUE;
						break;
					case kcF4:
						preferences->screen_mode.size = _50_percent;
						changed_screen_mode = changed_prefs = TRUE;
						break;
					case kcF5:
						if (preferences->screen_mode.acceleration != _valkyrie_acceleration)
						{
							preferences->screen_mode.high_resolution = !preferences->screen_mode.high_resolution;
							if (preferences->screen_mode.high_resolution) preferences->screen_mode.draw_every_other_line= FALSE;
							changed_screen_mode = changed_prefs = TRUE;
						}
						break;
#ifdef env68k
					case kcF6:
						if (!preferences->screen_mode.high_resolution && preferences->screen_mode.acceleration != _valkyrie_acceleration)
						{
							preferences->screen_mode.draw_every_other_line = !preferences->screen_mode.draw_every_other_line;
							changed_screen_mode = changed_prefs = TRUE;
						}
						break;
					case kcF7:
						preferences->screen_mode.texture_floor = !preferences->screen_mode.texture_floor;
						changed_screen_mode = changed_prefs = TRUE;
						break;
					case kcF8:
						preferences->screen_mode.texture_ceiling = !preferences->screen_mode.texture_ceiling;
						changed_screen_mode = changed_prefs = TRUE;
						break;
#endif
#if 0 /* can’t start music during network game; quicktime is too slow */
					case kcF9:
						if (can_play_background_music())
						{
							preferences->background_music_on = !preferences->background_music_on;
							changed_prefs= TRUE;
							if (preferences->background_music_on)
							{
								start_background_music(static_world->song_index);
							}
							else
							{
								stop_background_music();
							}
						}
						break;
#endif
					case kcF11:
						if (preferences->screen_mode.gamma_level)
						{
							preferences->screen_mode.gamma_level-= 1;
							change_gamma_level(preferences->screen_mode.gamma_level);
							changed_prefs= TRUE;
						}
						break;
					case kcF12:
						if (preferences->screen_mode.gamma_level<NUMBER_OF_GAMMA_LEVELS-1)
						{
							preferences->screen_mode.gamma_level+= 1;
							change_gamma_level(preferences->screen_mode.gamma_level);
							changed_prefs= TRUE;
						}
						break;
					default:
						if (get_game_status() == demo_in_progress)
							set_game_status(user_wants_quit_from_demo);
						break;
				}
				break;
		}
	}

	if (changed_screen_mode)
	{
		change_screen_mode(&preferences->screen_mode, TRUE);
		draw_interface();
		render_screen(0);
	}
	if (changed_prefs) write_preferences_file(preferences);
	
	return;
}

static process_command_key(short key)
{
	short i;
	short command_key_tag = NONE;
	
	for (i = 0; i < NUMBER_OF_COMMAND_KEYS; i++)
	{
		if (key == command_keys[i].command_key)
		{
			command_key_tag = command_keys[i].tag;
			break;
		}
	}
	
	switch (command_key_tag)
	{
		boolean really_wants_to_quit = FALSE;
		
		case _command_key_close: /* CMD-Q; quit */
		case _command_key_quit: /* CMD-W; close current game */
			if (get_game_status() != demo_in_progress && get_game_status() != replay_in_progress)
			{
				if (game_is_networked || PLAYER_IS_DEAD(local_player) ||
					dynamic_world->tick_count-local_player->ticks_at_last_successful_save<CLOSE_WITHOUT_WARNING_DELAY)
				{
					really_wants_to_quit= TRUE;
				}
				else
				{
					pause_game();
					ShowCursor();
					really_wants_to_quit= quit_without_saving();
					HideCursor();
					resume_game();
				}
			}
			else
			{
				really_wants_to_quit = TRUE;
			}
			if (really_wants_to_quit == TRUE)
			{
				render_screen(0); // get rid of hole made by alert
				switch(get_game_status())
				{
					case game_in_progress:
						set_game_status(user_wants_quit_from_game);
						break;
					case replay_in_progress:
						set_game_status(user_wants_quit_from_replay);
						break;
					case demo_in_progress:
						set_game_status(user_wants_quit_from_demo);
						break;					
				}
			}
			break;
#if 0
		case _command_key_save: /* save game */
			if (get_game_status() != demo_in_progress)
			{
				if (get_game_status() == game_in_progress && !game_is_networked)
				{
					save_game();
					validate_world_window();
				}
			}
			else
			{
				set_game_status(user_wants_quit_from_demo);
			}
			break;
#endif
		case _command_key_pause:
			if (!game_is_networked)
			{
				if (game_status != demo_in_progress)
				{
					if (get_keyboard_controller_status() == TRUE)
					{
						pause_game();
					}
					else
					{
						resume_game();
					}
				}
				else
				{
					set_game_status(user_wants_quit_from_demo);
				}
			}
			break;

		case _command_key_revert: // not yet implemented
		default:
			if (get_game_status() == demo_in_progress)
				set_game_status(user_wants_quit_from_demo);
			break;
	}
}

static short process_keyword_key(
	char key)
{
	short i;
	short tag = NONE;

	if (iscntrl(key))
	{
		/* copy the buffer down and insert the new character */
		for (i=0;i<MAXIMUM_KEYWORD_LENGTH-1;++i)
		{
			keyword_buffer[i]= keyword_buffer[i+1];
		}
		keyword_buffer[MAXIMUM_KEYWORD_LENGTH-1]= key+'A'-1;
		keyword_buffer[MAXIMUM_KEYWORD_LENGTH]= 0;
		
		/* any matches? */
		for (i=0;i<NUMBER_OF_KEYWORDS;++i)
		{
			if (!strcmp(keywords[i].keyword, keyword_buffer+MAXIMUM_KEYWORD_LENGTH-strlen(keywords[i].keyword)))
			{
				/* wipe the buffer if we have a match */
				memset(keyword_buffer, 0, MAXIMUM_KEYWORD_LENGTH);
				
				/* and return the tag */
				tag = keywords[i].tag;
			}
		}
	}
	
	return tag;
}


static void verify_environment(
	void)
{
	SysEnvRec environment;
	OSErr result;

	result= SysEnvirons(curSysEnvVers, &environment);
	if (result!=noErr||environment.systemVersion<0x0605)
	{
		alert_user(fatalError, strERRORS, badSystem, environment.systemVersion);
	}
	
	if (!environment.hasColorQD)
	{
		alert_user(fatalError, strERRORS, badQuickDraw, 0);
	}
	
	if (!environment.processor&&environment.processor<env68020)
	{
		alert_user(fatalError, strERRORS, badProcessor, environment.processor);
	}
	
	if (FreeMem()<1900000)
	{
		alert_user(fatalError, strERRORS, badMemory, FreeMem());
	}
	
	return;
}

void set_game_status(short status)
{
	game_status = status;
}

short get_game_status(void)
{
	return game_status;
}

void handle_high_level_event(EventRecord *event)
{
	if(system_information->has_apple_events)
	{
		AEProcessAppleEvent(event);
	}
}

static void initialize_core_events(void)
{
	/* Allocate the system information structure.. */	
	system_information= (struct system_information_data *) 
		NewPtr(sizeof(struct system_information_data));
	assert(system_information);

	if(has_system_seven())
	{
		AEEventHandlerUPP open_document_proc;
		AEEventHandlerUPP quit_application_proc;
		AEEventHandlerUPP print_document_proc;
		AEEventHandlerUPP open_application_proc;
		OSErr err;
		long apple_events_present;

		err= Gestalt(gestaltAppleEventsAttr, &apple_events_present);
		if(!err && (apple_events_present & 1<<gestaltAppleEventsPresent))
		{
			open_document_proc= NewAEEventHandlerProc(handle_open_document);
			quit_application_proc= NewAEEventHandlerProc(handle_quit_application);
			print_document_proc= NewAEEventHandlerProc(handle_print_document);
			open_application_proc= NewAEEventHandlerProc(handle_open_application);
			assert(open_document_proc && quit_application_proc 
				&& print_document_proc && open_application_proc);
		
			err= AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, open_document_proc, 0,
				FALSE);
			assert(!err);
			
			err= AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, quit_application_proc, 0,
				FALSE);
			assert(!err);
		
			err= AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, print_document_proc, 0,
				FALSE);
			assert(!err);
		
			err= AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, open_application_proc, 0,
				FALSE);
			assert(!err);

			system_information->has_apple_events= TRUE;
		} else {
			system_information->has_apple_events= FALSE;
		}
		system_information->has_seven= TRUE;
		set_to_default_map(); // in case start of game comes before high-level event
	} else {
		/* Sorry, you can only play with the default map. You lose */
		set_to_default_map();
		system_information->has_seven= FALSE;
		system_information->has_apple_events= FALSE;
	}
}

static pascal OSErr handle_open_document(AppleEvent *event, AppleEvent *reply, long myRefCon)
{
	OSErr err;
	AEDescList docList;
	FSSpec myFSS;
	long itemsInList;
	AEKeyword	theKeyword;
	DescType typeCode;
	Size actualSize;
	long i;
	FInfo	theFInfo;
	char name[65];
	boolean map_set= FALSE;

#pragma unused (reply, myRefCon)	
	err=AEGetParamDesc(event, keyDirectObject, typeAEList, &docList);
	if(err) return err;
	
	err=required_appleevent_check(event);
	if(err) return err;

#ifndef FINAL	
	err=AECountItems(&docList, &itemsInList);
	if(err) return err;
	
	for(i=1; i<=itemsInList; i++) 
	{
		err=AEGetNthPtr(&docList, i, typeFSS, &theKeyword, &typeCode, 
			(Ptr) &myFSS, sizeof(FSSpec), &actualSize);
		if(err) return err;
		
		FSpGetFInfo(&myFSS, &theFInfo);
		if(theFInfo.fdType==SCENARIO_FILE_TYPE)
		{
			BlockMove(myFSS.name, name, myFSS.name[0]+1);
			p2cstr(name);
			set_map_file(myFSS.vRefNum, myFSS.parID, name);
			map_set= TRUE;
		}
	}

	/* Default to the proper map. */	
	if(!map_set) set_to_default_map();
#endif
	
	return noErr;
}

static pascal OSErr handle_quit_application(AppleEvent *event, AppleEvent *reply, long myRefCon)
{
#pragma unused(reply, myRefCon)
	OSErr err;
	
	err= required_appleevent_check(event);
	if(err) return err;
	
	exit(0);
}

static pascal OSErr handle_print_document(AppleEvent *event, AppleEvent *reply, long myRefCon)
{
	#pragma unused(event, reply, myRefCon)

	/* Default to the proper map. */	
	set_to_default_map();
	
	return (errAEEventNotHandled);
}

static pascal OSErr handle_open_application(AppleEvent *event, AppleEvent *reply, long myRefCon)
{
#pragma unused(reply, myRefCon)
	OSErr	err;

	err=required_appleevent_check(event);
	if(err) return err;

	/* Default map anyone? */
	set_to_default_map();
	return err;
}

static OSErr required_appleevent_check(AppleEvent *event)
{
	OSErr err;
	DescType typeCode;
	Size actualSize;
	
	err=AEGetAttributePtr(event, keyMissedKeywordAttr, typeWildCard, &typeCode,
		0l, 0, &actualSize);
		
	if(err==errAEDescNotFound) return noErr;
	if(err==noErr) return (errAEEventNotHandled);

	return err;
}

static boolean has_system_seven(
	void)
{
	long sysVersion;
	OSErr err;

	err= Gestalt(gestaltSystemVersion, &sysVersion);
	if (!err)
	{
		if(sysVersion>=0x0700) return TRUE;
	}
	
	return FALSE;
}

static void get_machine_type(
	boolean *machine_is_68k,
	boolean *machine_is_68040,
	boolean *machine_is_ppc)
{
	long processor_type;
	OSErr error;
	
	error = Gestalt(gestaltNativeCPUtype, &processor_type);

	if (error == noErr)
	{
		switch (processor_type)
		{
			case gestaltCPU68000:
			case gestaltCPU68010: // highly unlikely, wouldn't you say?
			case gestaltCPU68020:
			case gestaltCPU68030:
				*machine_is_68k = TRUE; *machine_is_68040 = FALSE; *machine_is_ppc = FALSE;
				break;
			case gestalt68040: // ick.
			case gestaltCPU68040:
				*machine_is_68k = TRUE; *machine_is_68040 = TRUE; *machine_is_ppc = FALSE;
				break;
			case gestaltCPU601:
			case gestaltCPU603:
			case gestaltCPU604:
				*machine_is_68k = FALSE; *machine_is_68040 = FALSE; *machine_is_ppc = TRUE;
				break;
			default: // assume the best, since no other low-end processors are likely to come out for macs
				*machine_is_68k = FALSE; *machine_is_68040 = FALSE; *machine_is_ppc = TRUE;
				break;
		}
	}
	else // handle sys 6 machines, which are certainly not ppcs. (can they even be '040s?)
	{
		*machine_is_68k = TRUE;
		*machine_is_ppc = FALSE;

		error = Gestalt(gestaltProcessorType, &processor_type);
		if (error == noErr && processor_type == gestalt68040)
			*machine_is_68040 = TRUE;
		else
			*machine_is_68040 = FALSE;
	}
	
	return;

}

boolean game_window_renders_compass(void)
{
	return preferences->render_compass;
}

#include "items.h"

void handle_keyword(
	short tag)
{
	boolean cheated= TRUE;

#if 0
#ifndef FINAL	
	switch (tag)
	{
		case _tag_health:
			if (local_player->suit_energy<PLAYER_MAXIMUM_SUIT_ENERGY)
			{
				local_player->suit_energy= PLAYER_MAXIMUM_SUIT_ENERGY;
			}
			else
			{
				if (local_player->suit_energy<2*PLAYER_MAXIMUM_SUIT_ENERGY)
				{
					local_player->suit_energy= 2*PLAYER_MAXIMUM_SUIT_ENERGY;
				}
				else
				{
					local_player->suit_energy= 3*PLAYER_MAXIMUM_SUIT_ENERGY;
				}
			}
			mark_shield_display_as_dirty();
			break;
		case _tag_oxygen:
			local_player->suit_oxygen= PLAYER_MAXIMUM_SUIT_OXYGEN;
			mark_oxygen_display_as_dirty();
			break;
		case _tag_map:
			dynamic_world->game_information.game_options^= (_overhead_map_shows_items|_overhead_map_shows_monsters|_overhead_map_shows_projectiles);
			break;
		case _tag_invincible:
			process_player_powerup(local_player_index, _i_invincibility_powerup);
			break;
		case _tag_invisible:
			process_player_powerup(local_player_index, _i_invisibility_powerup);
			break;
		case _tag_infravision:
			process_player_powerup(local_player_index, _i_infravision_powerup);
			break;
		case _tag_extravision:
			process_player_powerup(local_player_index, _i_extravision_powerup);
			break;
		case _tag_pistol:
			if (local_player->items[_i_magnum]<=1) new_item((world_point2d *) &local_player->location, _i_magnum, local_player->supporting_polygon_index);
			break;
		case _tag_rifle:
			if (local_player->items[_i_assault_rifle]<1) new_item((world_point2d *) &local_player->location, _i_assault_rifle, local_player->supporting_polygon_index);
			break;
		case _tag_missle:
			if (local_player->items[_i_missile_launcher]<1) new_item((world_point2d *) &local_player->location, _i_missile_launcher, local_player->supporting_polygon_index);
			break;
		case _tag_toaster:
			if (local_player->items[_i_flamethrower]<1) new_item((world_point2d *) &local_player->location, _i_flamethrower, local_player->supporting_polygon_index);
			break;
		case _tag_pzbxay:
			if (local_player->items[_i_alien_shotgun]<1) new_item((world_point2d *) &local_player->location, _i_alien_shotgun, local_player->supporting_polygon_index);
			break;
		case _tag_fusion:
			if (local_player->items[_i_plasma_pistol]<1) new_item((world_point2d *) &local_player->location, _i_plasma_pistol, local_player->supporting_polygon_index);
			break;
		case _tag_ammo:
			local_player->items[_i_assault_rifle_magazine]= 10;
			local_player->items[_i_assault_grenade_magazine]= 10;
			local_player->items[_i_magnum_magazine]= 10;
			local_player->items[_i_missile_launcher_magazine]= 10;
			local_player->items[_i_flamethrower_canister]= 10;
			local_player->items[_i_plasma_magazine]= 10;
#if 0
			local_player->items[_i_assult_rifle_magazine]= 10;
			local_player->items[_i_assult_rifle_magazine]= 10;
			if (local_player->items[_i_magnum] > 0 && local_player->items[_i_magnum_magazine] < 10)
				new_item((world_point2d *) &local_player->location, _i_magnum_magazine, local_player->supporting_polygon_index);
			if (local_player->items[_i_assault_rifle] > 0 && local_player->items[_i_assault_rifle_magazine] < 10)
				new_item((world_point2d *) &local_player->location, _i_assault_rifle_magazine, local_player->supporting_polygon_index);
			if (local_player->items[_i_assault_rifle] > 0 && local_player->items[_i_assault_grenade_magazine] < 10)
				new_item((world_point2d *) &local_player->location, _i_assault_grenade_magazine, local_player->supporting_polygon_index);
			if (local_player->items[_i_missile_launcher] > 0 && local_player->items[_i_missile_launcher_magazine] < 10)
				new_item((world_point2d *) &local_player->location, _i_missile_launcher_magazine, local_player->supporting_polygon_index);
			if (local_player->items[_i_flamethrower] > 0 && local_player->items[_i_flamethrower_canister] < 10)
				new_item((world_point2d *) &local_player->location, _i_flamethrower_canister, local_player->supporting_polygon_index);
			if (local_player->items[_i_alien_shotgun] > 0&& local_player->items[_i_alien_shotgun_magazine] < 10)
				new_item((world_point2d *) &local_player->location, _i_alien_shotgun_magazine, local_player->supporting_polygon_index);
			if (local_player->items[_i_plasma_pistol] > 0&& local_player->items[_i_plasma_magazine] < 10)
				new_item((world_point2d *) &local_player->location, _i_plasma_magazine, local_player->supporting_polygon_index);
#endif
			break;
		case _tag_pathways:
			static_world->physics_model= _editor_model;
			break;
		case _tag_jump:
			accelerate_monster(local_player->monster_index, WORLD_ONE/10, 0, 0);
			break;
		case _tag_save:
			save_game();
		case _tag_aslag:
			if (local_player->items[_i_assault_rifle] < 1)
				new_item((world_point2d *) &local_player->location, _i_assault_rifle, local_player->supporting_polygon_index);
			if (local_player->items[_i_missile_launcher] < 1)
				new_item((world_point2d *) &local_player->location, _i_missile_launcher, local_player->supporting_polygon_index);
			new_item((world_point2d *) &local_player->location, _i_missile_launcher_magazine, local_player->supporting_polygon_index);
			if (local_player->items[_i_assault_rifle_magazine] < 8)
			{
				new_item((world_point2d *) &local_player->location, _i_assault_rifle_magazine, local_player->supporting_polygon_index);
				new_item((world_point2d *) &local_player->location, _i_assault_rifle_magazine, local_player->supporting_polygon_index);
				new_item((world_point2d *) &local_player->location, _i_assault_rifle_magazine, local_player->supporting_polygon_index);
			}
			if (local_player->items[_i_assault_grenade_magazine] < 8)
			{
				new_item((world_point2d *) &local_player->location, _i_assault_grenade_magazine, local_player->supporting_polygon_index);
				new_item((world_point2d *) &local_player->location, _i_assault_grenade_magazine, local_player->supporting_polygon_index);
				new_item((world_point2d *) &local_player->location, _i_assault_grenade_magazine, local_player->supporting_polygon_index);
			}
			local_player->suit_energy = 3*PLAYER_MAXIMUM_SUIT_ENERGY;
			update_interface(NONE);
			break;
		default:
			cheated= FALSE;
	}

//	/* can’t use computer terminals or save in the final version if we’ve cheated */
//	if (cheated) SET_PLAYER_HAS_CHEATED(local_player);
#ifndef env68k
	if (cheated)
	{
		long final_ticks;
		
		SetSoundVol(7);
		play_local_sound(20110);
		Delay(45, &final_ticks);
		play_local_sound(20110);
		Delay(45, &final_ticks);
		play_local_sound(20110);
	}
#endif
#endif
#endif
	
	return;
}

boolean is_keypad(
	short keycode)
{
	return keycode >= 0x41 && keycode <= 0x5c;
}

static void setup_command_keys(
	void)
{
	short       i;
	short       command_key;
	MenuHandle  game_menu;
	
	game_menu = GetMenu(menuGAME);
	assert(game_menu);

	assert(CountMItems(game_menu) >= NUMBER_OF_COMMAND_KEYS);
	for (i = 0; i < NUMBER_OF_COMMAND_KEYS; i++)
	{
		GetItemCmd(game_menu, i+1, &command_key);
		command_keys[i].command_key = toupper(command_key);
	}
	
	DeleteMenu(menuGAME);
	ReleaseResource((Handle) game_menu);
	
	return;
}

/* stolen from game_wad.c */
void pause_game(
	void)
{
	stop_fade();
	darken_world_window();
	pause_keyboard_controller(FALSE);
	
	return;
}

/* stolen from game_wad.c */
void resume_game(
	void)
{
//	GrafPtr old_port;
	
//	GetPort(&old_port);
//	SetPort(screen_window);
//	ValidRect(&screen_window->portRect);
//	SetPort(old_port);

	validate_world_window();
	pause_keyboard_controller(TRUE);
	
	return;
}

static void script_demo(
	void)
{
	DialogPtr dialog= myGetNewDialog(132, NULL, (WindowPtr) -1, 0);
	short item_hit, number= NONE;
	short state= 0;
	
	assert(dialog);
	modify_control(dialog, 4+state, CONTROL_ACTIVE, TRUE);
	do
	{
		ModalDialog(get_general_filter_upp(), &item_hit);
		switch (item_hit)
		{
			case 4: case 5: case 6:
				modify_radio_button_family(dialog, 4, 6, 4+state);
				state= item_hit-4;
				break;
		}
	}
	while (item_hit!=iOK && item_hit!=iCANCEL);
	
	if (item_hit==iOK)
	{
		number= extract_number_from_text_item(dialog, 3);
		if (!GetResource('levl', number+1000)) number= NONE;
	}
	DisposeDialog(dialog);
	present_computer_interface(number, state);
	
	return;
}

/* ---------- dialog headers */

#define FIRST_HEADER_SHAPE 13
#define DIALOG_HEADER_VERTICAL_INSET 10
#define DIALOG_HEADER_HORIZONTAL_INSET 13

/* if this dialog has a known refCon, draw the appropriate header graphic */
static void marathon_dialog_header_proc(
	DialogPtr dialog,
	Rect *frame)
{
	long refCon= GetWRefCon(dialog);
	
	if (refCon>=FIRST_DIALOG_REFCON&&refCon<=LAST_DIALOG_REFCON)
	{
		PixMapHandle pixmap= get_shape_pixmap(BUILD_DESCRIPTOR(_collection_splash_graphics, FIRST_HEADER_SHAPE+refCon-FIRST_DIALOG_REFCON), FALSE);
		Rect destination= (*pixmap)->bounds;
		RGBColor old_forecolor, old_backcolor;
		
		OffsetRect(&destination, frame->left-destination.left+DIALOG_HEADER_HORIZONTAL_INSET,
			frame->top-destination.top+DIALOG_HEADER_VERTICAL_INSET);
		GetForeColor(&old_forecolor);
		GetBackColor(&old_backcolor);
		RGBForeColor(&rgb_black);
		RGBBackColor(&rgb_white);
		CopyBits((const BitMap *) *pixmap, (const BitMap *) &dialog->portBits,
			&(*pixmap)->bounds, &destination, srcCopy, (RgnHandle) NULL);
		RGBForeColor(&old_forecolor);
		RGBBackColor(&old_backcolor);
	}
		
	return;
}
