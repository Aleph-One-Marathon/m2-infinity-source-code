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
Wednesday, June 7, 1995 10:50:05 AM
	this is now MARATHON II.
Thursday, September 28, 1995 7:37:12 PM
	reintegrated interface.c/shell.c.  Tried to keep this clean, but I didn't succeed as
	well as I would have liked.
*/

#include "macintosh_cseries.h"
#include <appleevents.h>
#include <aliases.h>
#ifdef SUPPORT_INPUT_SPROCKET
// we understand and use the grouping byte
#define USE_OLD_ISPNEED_STRUCT 0
#include "InputSprocket.h"
#endif

#ifdef SUPPORT_DRAW_SPROCKET
#include "DrawSprocket.h"
#endif

#if defined(SUPPORT_INPUT_SPROCKET) || defined(SUPPORT_DRAW_SPROCKET)
#include <CodeFragments.h>
#endif

#include "my32bqd.h"

#include "map.h"
#include "monsters.h"
#include "player.h"
#include "render.h"
#include "shell.h"
#include "interface.h"
#include "game_sound.h"
#include "fades.h"
#include "screen.h"

#include "portable_files.h"
#include "music.h"
#include "images.h"
#include "vbl.h"

#include "preferences.h"
#include "tags.h" /* for scenario file type.. */

#include "network_sound.h"
#include "mouse.h"
#include "screen_drawing.h"
#include "overhead_map.h"
#include "computer_interface.h"
#include "game_wad.h" /* yuck. •• */
#include "game_window.h" /* for draw_interface() */
#include "wad.h"
#include "extensions.h"

#include "items.h"
#include "macintosh_network.h" /* For NetDDPOpen() */

#include "interface_menus.h"

#ifndef FINAL
#include "weapons.h" /* To remove process_new_item_for_reloading warning and debug_print_weapon_status warning */
#endif

#define kMINIMUM_NETWORK_HEAP (3*MEG)

#ifdef PERFORMANCE
#include <Perf.h>
#endif

#ifdef mpwc
#pragma segment shell
#endif

/* ---------- constants */
#define alrtQUIT_FOR_SURE 138

#ifdef COMPILE_TIME
#define DAYS_TO_EXPIRATION 18
#define SECONDS_TO_EXPIRATION (DAYS_TO_EXPIRATION * 60 * 60 * 24)
#define EXPIRATION_TIME (COMPILE_TIME + SECONDS_TO_EXPIRATION)
#endif

#define kMINIMUM_MUSIC_HEAP (4*MEG)
#define kMINIMUM_SOUND_HEAP (3*MEG)

struct system_information_data *system_information;

#ifndef __MWERKS__
QDGlobals qd;
#endif

#ifdef PERFORMANCE
TP2PerfGlobals perf_globals;
#endif

#ifdef SUPPORT_INPUT_SPROCKET
#include "macintosh_input.h"

ISpElementReference *input_sprocket_elements;
boolean use_input_sprocket= FALSE;

#include "input_sprocket_needs.h"
#endif

#ifdef SUPPORT_DRAW_SPROCKET
boolean use_draw_sprocket= FALSE;
#endif

extern long first_frame_tick, frame_count; /* for determining frame rate */

/* ---------- externs that I couldn't fit into the #include heirarchy nicely */
extern boolean load_and_start_game(FileDesc *file);
extern boolean handle_open_replay(FileDesc *replay_file);

/* ---------- private code */

static void verify_environment(void);
static void initialize_application_heap(void);

static void initialize_system_information(void);
static void initialize_core_events(void);
static void initialize_marathon_music_handler(void);
static pascal OSErr handle_open_document(AppleEvent *event, AppleEvent *reply, long myRefCon);
static pascal OSErr handle_quit_application(AppleEvent *event, AppleEvent *reply, long myRefCon);
static pascal OSErr handle_print_document(AppleEvent *event, AppleEvent *reply, long myRefCon);
static pascal OSErr handle_open_application(AppleEvent *event, AppleEvent *reply, long myRefCon);
static OSErr required_appleevent_check(AppleEvent *event);

static void marathon_dialog_header_proc(DialogPtr dialog, Rect *frame);

static void process_event(EventRecord *event);
static void process_screen_click(EventRecord *event);
static void process_key(EventRecord *event, short key);
static void wait_for_highlevel_event(void);
static void main_event_loop(void);
static void handle_high_level_event(EventRecord *event);

void update_any_window(WindowPtr window, EventRecord *event);
void activate_any_window(WindowPtr window, EventRecord *event, boolean active);

#ifndef FINAL
static short process_keyword_key(char key);
static void handle_keyword(short type_of_cheat);
#endif

boolean is_keypad(short keycode);

#ifdef SUPPORT_INPUT_SPROCKET
static void input_sprocket_handler(void);
static void enable_input_sprocket_device_class(OSType deviceClass, boolean enable);
#endif

/* ---------- code */

void main(
	void)
{	
	initialize_application_heap();
	main_event_loop();
	exit(0);
}

/* somebody wants to do something important; free as much temporary memory as possible and
	unlock anything we can */
void free_and_unlock_memory(
	void)
{
	stop_all_sounds();
	
	return;
}

void *level_transition_malloc(
	size_t size)
{
	void *ptr= malloc(size);
	if (!ptr)
	{
		unload_all_sounds();
		
		ptr= malloc(size);
		if (!ptr)
		{
			unload_all_collections();
			
			ptr= malloc(size);
		}
	}
	
	return ptr;
}

/* ---------- private code */
#ifdef SUPPORT_DRAW_SPROCKET
static void draw_sprocket_handler(
	void)
{
	OSStatus err= noErr;
	
	err= DSpShutdown();
	
	return;
}
#endif

#ifdef SUPPORT_INPUT_SPROCKET
static void input_sprocket_handler(
	void)
{
	OSStatus err= noErr;
	
	err= ISpStop();
	ISpElement_DisposeVirtual(NUMBER_OF_INPUT_SPROCKET_NEEDS, input_sprocket_elements);
	free(input_sprocket_elements);
	
	return;
}
#endif

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

#if defined(DEBUG) && !defined(GAMMA)
	initialize_debugger(TRUE);
#endif

	SetCursor(*GetCursor(watchCursor));

	qd.randSeed = TickCount();

#ifdef PERFORMANCE
	{
		Boolean succeeded;
		
		succeeded = InitPerf(&perf_globals, 4, 8, TRUE, TRUE, "\pCODE", 0, "", 0, 0, 0, 0);
		assert(succeeded);
	}
#endif

	/* Determine what type of system we are working on.. */
	initialize_system_information();
	initialize_core_events();

	initialize_preferences();
	GetDateTime(&player_preferences->last_time_ran);
	write_preferences();

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
		if (current_time < player_preferences->last_time_ran || current_time > EXPIRATION_TIME)
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

#ifdef SUPPORT_INPUT_SPROCKET
	if (system_information->has_input_sprocket)
	{
		OSStatus err= noErr;

		// turn on keyboard, mouse, speech and ticklish classes and ignore errors
		ISpDevices_ActivateClass(kISpDeviceClass_Mouse);
		ISpDevices_ActivateClass(kISpDeviceClass_Keyboard);
		ISpDevices_ActivateClass(kISpDeviceClass_SpeechRecognition);
		ISpDevices_ActivateClass(kISpDeviceClass_Tickle);
		
		input_sprocket_elements= (ISpElementReference *) malloc(sizeof(ISpElementReference) * NUMBER_OF_INPUT_SPROCKET_NEEDS);
		assert(input_sprocket_elements);
		err= ISpElement_NewVirtualFromNeeds(NUMBER_OF_INPUT_SPROCKET_NEEDS, input_sprocket_needs, input_sprocket_elements, 0);
		if (err) dprintf("ISpElementVirtuals err=%d",err);
		err= ISpInit(NUMBER_OF_INPUT_SPROCKET_NEEDS, input_sprocket_needs, input_sprocket_elements, '52.4', 'fooc', 0, 0, 0);
		if (err) dprintf("ISpInit err=%d",err); 
		atexit(input_sprocket_handler);
		err= ISpSuspend();	// start out suspended, resume when we begin the game
	
		if (input_preferences->input_device)
		{
			use_input_sprocket = true;
		}
	}
#endif

#ifdef SUPPORT_DRAW_SPROCKET
	if (system_information->has_draw_sprocket)
	{
		OSStatus err;
		
		err= DSpStartup();
		atexit(draw_sprocket_handler);
	}
#endif

	initialize_my_32bqd();
	verify_environment();

	if (FreeMem()>kMINIMUM_SOUND_HEAP) initialize_sound_manager(sound_preferences);

	initialize_marathon_music_handler();

	initialize_keyboard_controller();
	initialize_screen(&graphics_preferences->screen_mode);
	initialize_marathon();
	initialize_screen_drawing();
	initialize_overhead_map();
	initialize_terminal_manager();
	initialize_shape_handler();
	initialize_fades();

	kill_screen_saver();

	set_dialog_header_proc(marathon_dialog_header_proc);
	initialize_images_manager();

	/* Load the environment.. */
	load_environment_from_preferences();

	initialize_game_state();
	SetCursor(&qd.arrow);

	return;
}

void global_idle_proc(
	void)
{
	music_idle_proc();
	network_speaker_idle_proc();
	sound_manager_idle_proc();
	
	return;
}

void handle_game_key(
	EventRecord *event,
	short key)
{
	short virtual;
	boolean changed_screen_mode= FALSE;
	boolean changed_prefs= FALSE;
	boolean changed_volume= FALSE;
	boolean update_interface= FALSE;

	virtual = (event->message >> 8) & charCodeMask;
	
#ifndef FINAL
	if (!game_is_networked && (event->modifiers & controlKey))
	{
		short type_of_cheat;
		
		type_of_cheat = process_keyword_key(key);
		if (type_of_cheat != NONE) handle_keyword(type_of_cheat);
	}
#endif

	if (!is_keypad(virtual))
	{
		switch(key)
		{
			case '.': case '>': // sound volume up
				changed_prefs= adjust_sound_volume_up(sound_preferences, _snd_adjust_volume);
				break;
			case ',': case '<': // sound volume down.
				changed_prefs= adjust_sound_volume_down(sound_preferences, _snd_adjust_volume);
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
				if(player_controlling_game())
				{
					scroll_inventory(-1);
				}
				else
				{
					decrement_replay_speed();
				}
				break;
			case ']': case '}':
				if(player_controlling_game())
				{
					scroll_inventory(1);
				}
				else
				{
					increment_replay_speed();
				}
				break;

			case '%':
				toggle_suppression_of_background_tasks();
				break;

#ifndef FINAL
#ifdef DEBUG
			case '!':
				debug_print_weapon_status();
				break;
#endif
#endif

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
						graphics_preferences->screen_mode.size = _full_screen;
						changed_screen_mode = changed_prefs = TRUE;
						break;

					case kcF2:
						if(graphics_preferences->screen_mode.size==_full_screen) update_interface= TRUE;
						graphics_preferences->screen_mode.size = _100_percent;
						changed_screen_mode = changed_prefs = TRUE;
						break;

					case kcF3:
						if(graphics_preferences->screen_mode.size==_full_screen) update_interface= TRUE;
						graphics_preferences->screen_mode.size = _75_percent;
						changed_screen_mode = changed_prefs = TRUE;
						break;

					case kcF4:
						if(graphics_preferences->screen_mode.size==_full_screen) update_interface= TRUE;
						graphics_preferences->screen_mode.size = _50_percent;
						changed_screen_mode = changed_prefs = TRUE;
						break;

					case kcF5:
						if (graphics_preferences->screen_mode.acceleration != _valkyrie_acceleration)
						{
							graphics_preferences->screen_mode.high_resolution = !graphics_preferences->screen_mode.high_resolution;
							if (graphics_preferences->screen_mode.high_resolution) graphics_preferences->screen_mode.draw_every_other_line= FALSE;
							changed_screen_mode = changed_prefs = TRUE;
						}
						break;
#ifdef env68k
					case kcF6:
						if (!graphics_preferences->screen_mode.high_resolution && graphics_preferences->screen_mode.acceleration != _valkyrie_acceleration)
						{
							graphics_preferences->screen_mode.draw_every_other_line = !graphics_preferences->screen_mode.draw_every_other_line;
							changed_screen_mode = changed_prefs = TRUE;
						}
						break;

					case kcF7:
						graphics_preferences->screen_mode.texture_floor = !graphics_preferences->screen_mode.texture_floor;
						changed_screen_mode = changed_prefs = TRUE;
						break;

					case kcF8:
						graphics_preferences->screen_mode.texture_ceiling = !graphics_preferences->screen_mode.texture_ceiling;
						changed_screen_mode = changed_prefs = TRUE;
						break;
#else
					// PowerPC-only	secret no frame rate limit
					case kcF6:
						no_frame_rate_limit= !no_frame_rate_limit;
						break;
#endif
					case kcF9:
						if(event->modifiers & shiftKey)
						{
							short keys[NUMBER_OF_KEYS];
				
							set_default_keys((short *) &keys, 0);
							set_keys((short *) &keys);
						}
						break;
						
					case kcF10:
#ifdef envppc
						// secret ludicrous speed mode
						if (event->modifiers & shiftKey)
						{
							if (game_is_networked)
							{
								toggle_ludicrous_speed(FALSE);
								toggle_sound_pitch_modifier_override(FALSE);
							}
							else
							{
								toggle_ludicrous_speed(TRUE);
								toggle_sound_pitch_modifier_override(TRUE);
							}
						}
#endif
						break;
						
					case kcF11:
						if (graphics_preferences->screen_mode.gamma_level)
						{
							graphics_preferences->screen_mode.gamma_level-= 1;
							change_gamma_level(graphics_preferences->screen_mode.gamma_level);
							changed_prefs= TRUE;
						}
						break;
						
					case kcF12:
						if (graphics_preferences->screen_mode.gamma_level<NUMBER_OF_GAMMA_LEVELS-1)
						{
							graphics_preferences->screen_mode.gamma_level+= 1;
							change_gamma_level(graphics_preferences->screen_mode.gamma_level);
							changed_prefs= TRUE;
						}
						break;
						
					default:
						if(get_game_controller()==_demo)
						{
							set_game_state(_close_game);
						}
						break;
				}
				break;
		}
	}

	if (changed_screen_mode)
	{
		change_screen_mode(&graphics_preferences->screen_mode, TRUE);
		render_screen(0);
		if(update_interface) draw_interface();
	}
	if (changed_prefs) write_preferences();
	
	return;
}

static void verify_environment(
	void)
{
	SysEnvRec environment;
	OSErr result;

	result= SysEnvirons(curSysEnvVers, &environment);
	if (result!=noErr||environment.systemVersion<0x0700)
	{
		alert_user(fatalError, strERRORS, badSystem, environment.systemVersion);
	}
	
	if (!environment.hasColorQD)
	{
		alert_user(fatalError, strERRORS, badQuickDraw, 0);
	}
	
	if (!environment.processor&&environment.processor<env68040)
	{
		alert_user(fatalError, strERRORS, badProcessor, environment.processor);
	}
	
	if (FreeMem()<2900000)
	{
		alert_user(fatalError, strERRORS, badMemory, FreeMem());
	}
	
	return;
}

static void handle_high_level_event(
	EventRecord *event)
{
	if(system_information->has_apple_events)
	{
		AEProcessAppleEvent(event);
	}
}

static void initialize_core_events(
	void)
{
	if(system_information->has_seven && system_information->has_apple_events)
	{
		AEEventHandlerUPP open_document_proc;
		AEEventHandlerUPP quit_application_proc;
		AEEventHandlerUPP print_document_proc;
		AEEventHandlerUPP open_application_proc;
		OSErr err;

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
	}
}

static pascal OSErr handle_open_document(
	AppleEvent *event, 
	AppleEvent *reply, 
	long myRefCon)
{
	OSErr err;
	AEDescList docList;

#pragma unused (reply, myRefCon)	
	err= AEGetParamDesc(event, keyDirectObject, typeAEList, &docList);
	if(!err)
	{
		err= required_appleevent_check(event);
		if(!err)
		{
			long itemsInList;
		
			err= AECountItems(&docList, &itemsInList);

			if(!err)
			{
				boolean done= FALSE;
				long index;
				FSSpec myFSS;
				Size actualSize;

				for(index= 1; !done && index<= itemsInList; index++) 
				{
					DescType typeCode;
					AEKeyword theKeyword;

					err=AEGetNthPtr(&docList, index, typeFSS, &theKeyword, &typeCode, 
						(Ptr) &myFSS, sizeof(FSSpec), &actualSize);
					if(!err)
					{
						FInfo theFInfo;

						FSpGetFInfo(&myFSS, &theFInfo);
						switch(theFInfo.fdType)
						{
							case SCENARIO_FILE_TYPE:
								set_map_file((FileDesc *) &myFSS);
								break;
								
							case SAVE_GAME_TYPE:
#ifndef TRILOGY
								if (serial_preferences->network_only)
								{
									alert_user(infoError, strERRORS, networkOnlySerialNumber, 0);
								}
								else
#endif
								{
									if(load_and_start_game((FileDesc *) &myFSS))
									{
										done= TRUE;
									}
								}
								break;
								
							case FILM_FILE_TYPE:
								if(handle_open_replay((FileDesc *) &myFSS))
								{
									done= TRUE;
								}
								break;
								
							case PHYSICS_FILE_TYPE:
								set_physics_file((FileDesc *) &myFSS);
								break;
								
							case SHAPES_FILE_TYPE:
								open_shapes_file(&myFSS);
								break;
								
							case SOUNDS_FILE_TYPE:
						 		open_sound_file(&myFSS);
								break;
								
							default:
								break;
						}
					}
				}
			}
		}
	}
	
	return err;
}

static pascal OSErr handle_quit_application(
	AppleEvent *event, 
	AppleEvent *reply, 
	long myRefCon)
{
#pragma unused(reply, myRefCon)
	OSErr err;
	
	err= required_appleevent_check(event);
	if(err) return err;
	
	set_game_state(_quit_game);

	return noErr;
}

static pascal OSErr handle_print_document(
	AppleEvent *event, 
	AppleEvent *reply, 
	long myRefCon)
{
	#pragma unused(event, reply, myRefCon)

	return (errAEEventNotHandled);
}

static pascal OSErr handle_open_application(
	AppleEvent *event, 
	AppleEvent *reply, 
	long myRefCon)
{
#pragma unused(reply, myRefCon)
	OSErr	err;

	err=required_appleevent_check(event);
	if(err) return err;

	return err;
}

static OSErr required_appleevent_check(
	AppleEvent *event)
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

static void initialize_system_information(
	void)
{
	long system_version, apple_events_present, processor_type;
	OSErr err;
	
	/* Allocate the system information structure.. */	
	system_information= (struct system_information_data *) 
		NewPtr(sizeof(struct system_information_data));
	assert(system_information);

	/* System Version */
	system_information->has_seven= FALSE;
	err= Gestalt(gestaltSystemVersion, &system_version);
	if (!err)
	{
		if(system_version>=0x0700) system_information->has_seven= TRUE;;
	}

	system_information->machine_has_network_memory= (FreeMem()>5000000) ? TRUE : FALSE;

	/* Appleevents? */
	err= Gestalt(gestaltAppleEventsAttr, &apple_events_present);
	if(!err && (apple_events_present & 1<<gestaltAppleEventsPresent))
	{
		system_information->has_apple_events= TRUE;
	} else {
		system_information->has_apple_events= FALSE;
	}

	/* Is appletalk available? */
	if(system_information->has_seven && NetDDPOpen()==noErr && FreeMem()>kMINIMUM_NETWORK_HEAP)
	{
		system_information->appletalk_is_available= TRUE;
	} else {
		system_information->appletalk_is_available= FALSE;
	}

	/* Type of machine? */
	err= Gestalt(gestaltNativeCPUtype, &processor_type);
	if(!err)
	{
		switch (processor_type)
		{
			case gestaltCPU68000:
			case gestaltCPU68010: // highly unlikely, wouldn't you say?
			case gestaltCPU68020:
			case gestaltCPU68030:
				system_information->machine_is_68k= TRUE; 
				system_information->machine_is_68040= FALSE; 
				system_information->machine_is_ppc= FALSE;
				break;
				
			case gestaltCPU68040:
				system_information->machine_is_68k= TRUE; 
				system_information->machine_is_68040= TRUE; 
				system_information->machine_is_ppc= FALSE;
				break;
				
			case gestaltCPU601:
			case gestaltCPU603:
			case gestaltCPU604:
			default: // assume the best
				system_information->machine_is_68k= FALSE; 
				system_information->machine_is_68040= FALSE; 
				system_information->machine_is_ppc= TRUE;
				break;
		}
	}
	else // handle sys 6 machines, which are certainly not ppcs. (can they even be '040s?)
	{
		system_information->machine_is_68k= TRUE; 
		system_information->machine_is_ppc= FALSE;

		err= Gestalt(gestaltProcessorType, &processor_type);
		if(!err)
		{
			if(processor_type==gestalt68040)
			{
				system_information->machine_is_68040= TRUE; 
			} else {
				system_information->machine_is_68040= FALSE; 
			}
		}
	}
	
#ifdef SUPPORT_INPUT_SPROCKET
	{
		// assume false, if we linked and have version 1.2 mark as true
		// we require 1.1 because we use some 1.1 specific calls
		// and we use 1.2's delta features (which unfortunately crash 1.1)
		system_information->has_input_sprocket= FALSE;
		
		if (ISpGetVersion != kUnresolvedCFragSymbolAddress)
		{
			NumVersion inputSprocketVersionStruct = ISpGetVersion();
			UInt32 inputSprocketVersion = * (UInt32 *) &(inputSprocketVersionStruct);
		
			if (inputSprocketVersion > 0x01200000)	// require 1.2 (binary coded decimal)
			{
				system_information->has_input_sprocket= TRUE;
			}
		}
	}
#endif
#ifdef SUPPORT_DRAW_SPROCKET
	if ((Ptr)DSpStartup != (Ptr)kUnresolvedCFragSymbolAddress)
	{
		system_information->has_draw_sprocket= TRUE;
	}
	else
	{
		system_information->has_draw_sprocket= FALSE;
	}
#endif

	return;
}

boolean is_keypad(
	short keycode)
{
	return keycode >= 0x41 && keycode <= 0x5c;
}

boolean networking_available(
	void)
{
	return system_information->appletalk_is_available;
}

/* ---------- dialog headers */

#define DIALOG_HEADER_RESOURCE_OFFSET 8000
#define DIALOG_HEADER_VERTICAL_INSET 10
#define DIALOG_HEADER_HORIZONTAL_INSET 13

/* if this dialog has a known refCon, draw the appropriate header graphic */
static void marathon_dialog_header_proc(
	DialogPtr dialog,
	Rect *frame)
{
	long refCon= GetWRefCon(dialog);
	
	if (refCon>=FIRST_DIALOG_REFCON && refCon<=LAST_DIALOG_REFCON)
	{
		PicHandle picture= GetPicture(DIALOG_HEADER_RESOURCE_OFFSET - FIRST_DIALOG_REFCON + refCon);
		
		if (picture)
		{
			Rect destination= (*picture)->picFrame;
			
			OffsetRect(&destination, frame->left-destination.left+DIALOG_HEADER_HORIZONTAL_INSET,
				frame->top-destination.top+DIALOG_HEADER_VERTICAL_INSET);
			DrawPicture(picture, &destination);
		}
	}
		
	return;
}

static void initialize_marathon_music_handler(
	void)
{
	FSSpec music_file_spec;
	OSErr error;
	boolean initialized= FALSE;
	
	error= get_file_spec(&music_file_spec, strFILENAMES, filenameMUSIC, strPATHS);
	if (!error)
	{
		boolean is_folder, was_aliased;
	
		ResolveAliasFile(&music_file_spec, TRUE, &is_folder, &was_aliased);
		initialized= initialize_music_handler((FileDesc *) &music_file_spec);
	}
	
	return;
}

/* ------------------------- cheating (not around if FINAL is defined) */
#ifndef FINAL

#define MAXIMUM_KEYWORD_LENGTH 20
#define NUMBER_OF_KEYWORDS (sizeof(keywords)/sizeof(struct keyword_data))

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
	_tag_save,
	_tag_level
};

/* ---------- resources */

/* ---------- structures */
struct keyword_data
{
	short tag;
	char *keyword; /* in uppercase */
};

/* ---------- globals */
static struct keyword_data keywords[]=
{
	{_tag_health, "NRG"},
	{_tag_oxygen, "OTWO"},
	{_tag_map, "MAP"},
	{_tag_invisible, "BYE"},
	{_tag_invincible, "NUKE"},
	{_tag_infravision, "SEE"},
	{_tag_extravision, "WOW"},
	{_tag_invisible, "BYE"},
	{_tag_pistol, "MAG"},
	{_tag_rifle, "RIF"},
	{_tag_missle, "POW"},
	{_tag_toaster, "TOAST"},
	{_tag_fusion, "MELT"},
	{_tag_pzbxay, "PZBXAY"}, // the alien shotgon, in the phfor's language
	{_tag_ammo, "AMMO"},
	{_tag_jump, "QWE"},
	{_tag_aslag, "SHIT"},
	{_tag_save, "YOURMOM"},
	{_tag_level, "LEVEL"}
};
static char keyword_buffer[MAXIMUM_KEYWORD_LENGTH+1];


static short process_keyword_key(
	char key)
{
	short i;
	short tag = NONE;

	/* copy the buffer down and insert the new character */
	for (i=0;i<MAXIMUM_KEYWORD_LENGTH-1;++i)
	{
		keyword_buffer[i]= keyword_buffer[i+1];
	}
	keyword_buffer[MAXIMUM_KEYWORD_LENGTH-1]= key+'A'-1;
	keyword_buffer[MAXIMUM_KEYWORD_LENGTH]= 0;
	
	/* any matches? */
	for (i=0; i<NUMBER_OF_KEYWORDS; ++i)
	{
		if (!strcmp(keywords[i].keyword, keyword_buffer+MAXIMUM_KEYWORD_LENGTH-strlen(keywords[i].keyword)))
		{
			/* wipe the buffer if we have a match */
			memset(keyword_buffer, 0, MAXIMUM_KEYWORD_LENGTH);
			tag= keywords[i].tag;
			break;
		}
	}
	
	return tag;
}

static void handle_keyword(
	short tag)
{
	boolean cheated= TRUE;

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
		case _tag_jump:
			accelerate_monster(local_player->monster_index, WORLD_ONE/10, 0, 0);
			break;
		case _tag_pistol:
			local_player->items[_i_magnum_magazine]= 10;
			break;
		case _tag_rifle:
			local_player->items[_i_assault_rifle]= 1;
			local_player->items[_i_assault_rifle_magazine]= 10;
			local_player->items[_i_assault_grenade_magazine]= 8;
			break;
		case _tag_fusion:
			local_player->items[_i_plasma_pistol]= 1;
			local_player->items[_i_plasma_magazine]= 10;
			break;
		case _tag_save:
			save_game();
			break;
		case _tag_aslag:
			{
				short items[]= { _i_assault_rifle, _i_magnum, _i_missile_launcher, _i_flamethrower,
					_i_plasma_pistol, _i_alien_shotgun, _i_shotgun,
					_i_assault_rifle_magazine, _i_assault_grenade_magazine, 
					_i_magnum_magazine, _i_missile_launcher_magazine, _i_flamethrower_canister,
					_i_plasma_magazine, _i_shotgun_magazine, _i_shotgun, _i_smg, _i_smg_ammo };
				short index;
				
				for(index= 0; index<sizeof(items)/sizeof(short); ++index)
				{
					switch(get_item_kind(items[index]))
					{
						case _weapon:
							if(items[index]==_i_shotgun || items[index]==_i_magnum)
							{
								assert(items[index]>=0 && items[index]<NUMBER_OF_ITEMS);
								if(local_player->items[items[index]]==NONE)
								{
									local_player->items[items[index]]= 1;
								} else {
									local_player->items[items[index]]++;
								}
							} else {	
								local_player->items[items[index]]= 1;
							}
							break;
							
						case _ammunition:
							local_player->items[items[index]]= 10;
							break;
							
						case _powerup:
						case _weapon_powerup:
							break;
							
						default:
							halt();
							break;
					} 
					process_new_item_for_reloading(local_player_index, items[index]);
				}
			}
			local_player->suit_energy = 3*PLAYER_MAXIMUM_SUIT_ENERGY;
			update_interface(NONE);
			break;
		
		case _tag_level:
			{
				struct player_data *player= get_player_data(0);
				int level_number= dynamic_world->current_level_number+1;
				
				player->teleporting_destination= -level_number;
				player->delay_before_teleport= TICKS_PER_SECOND/2;
			}
			break;
			
		default:
			cheated= FALSE;
			break;
	}

//	/* can’t use computer terminals or save in the final version if we’ve cheated */
//	if (cheated) SET_PLAYER_HAS_CHEATED(local_player);
#if 0
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
	
	return;
}
#endif

#ifdef SUPPORT_INPUT_SPROCKET

static Boolean check_input_sprocket_button(	ISpElementReference theElement)
{
			OSStatus	err;
			ISpElementEvent input_sprocket_event;
			Boolean was_input_sprocket_event;
			
	err= ISpElement_GetNextEvent(	theElement,
									sizeof(ISpElementEvent),
									&input_sprocket_event,
									&was_input_sprocket_event);
												
	if (!err && was_input_sprocket_event && (input_sprocket_event.data == kISpButtonDown))
	{
		return true;
	}
	else
	{
		return false;
	}
}

#endif
  
static void main_event_loop(
	void)
{
	wait_for_highlevel_event();
	
	while(get_game_state()!=_quit_game)
	{
		boolean use_waitnext;
		
		if(try_for_event(&use_waitnext))
		{
			EventRecord event;
			boolean got_event= FALSE;

#ifdef SUPPORT_INPUT_SPROCKET
			unsigned int key_state= 0;
			boolean changed_screen_mode = FALSE;
			boolean changed_prefs = FALSE;
			boolean update_interface = FALSE;
			
			if (use_input_sprocket)
			{
				ISpTickle();	// give input sprocket time
				
				// check slow stuff here
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_quit]))
				{
					do_menu_item_command(mGame, iQuitGame, FALSE);
				}
				
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_volume_up]))
				{
					changed_prefs= adjust_sound_volume_up(sound_preferences, _snd_adjust_volume);
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_volume_down]))
				{
					changed_prefs= adjust_sound_volume_down(sound_preferences, _snd_adjust_volume);
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_change_view]))
				{
					walk_player_list();
					render_screen(0);
				}
				
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_zoom_map_in]))
				{
					zoom_overhead_map_in();
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_zoom_map_out]))
				{
					zoom_overhead_map_out();
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_scroll_back_decrement_replay]))
				{
					if(player_controlling_game())
					{
						scroll_inventory(-1);
					}
					else
					{
						decrement_replay_speed();
					}

				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_scroll_forward_increment_replay]))
				{
					if(player_controlling_game())
					{
						scroll_inventory(1);
					}
					else
					{
						increment_replay_speed();
					}
				}

#ifdef ALEX_DISABLED
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_toggle_background_tasks]))
				{
					toggle_suppression_of_background_tasks();
				}
#endif
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_show_fps]))
				{
					extern boolean displaying_fps;
					
					displaying_fps= !displaying_fps;
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_full_screen]))
				{
					graphics_preferences->screen_mode.size = _full_screen;
					changed_screen_mode = changed_prefs = TRUE;
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_100_percent]))
				{
					if(graphics_preferences->screen_mode.size==_full_screen) update_interface= TRUE;
					graphics_preferences->screen_mode.size = _100_percent;
					changed_screen_mode = changed_prefs = TRUE;
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_75_percent]))
				{
					if(graphics_preferences->screen_mode.size==_full_screen) update_interface= TRUE;
					graphics_preferences->screen_mode.size = _75_percent;
					changed_screen_mode = changed_prefs = TRUE;
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_50_percent]))
				{
					if(graphics_preferences->screen_mode.size==_full_screen) update_interface= TRUE;
					graphics_preferences->screen_mode.size = _50_percent;
					changed_screen_mode = changed_prefs = TRUE;
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_low_res]))
				{
					if (graphics_preferences->screen_mode.acceleration != _valkyrie_acceleration)
					{
						graphics_preferences->screen_mode.high_resolution = !graphics_preferences->screen_mode.high_resolution;
						if (graphics_preferences->screen_mode.high_resolution) graphics_preferences->screen_mode.draw_every_other_line= FALSE;
						changed_screen_mode = changed_prefs = TRUE;
					}
				}
#ifdef ALEX_DISABLED
#ifdef env68k
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_high_res]))
				{
					if (!graphics_preferences->screen_mode.high_resolution && graphics_preferences->screen_mode.acceleration != _valkyrie_acceleration)
					{
						graphics_preferences->screen_mode.draw_every_other_line = !graphics_preferences->screen_mode.draw_every_other_line;
						changed_screen_mode = changed_prefs = TRUE;
					}
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_texture_floor_toggle]))
				{
					graphics_preferences->screen_mode.texture_floor = !graphics_preferences->screen_mode.texture_floor;
					changed_screen_mode = changed_prefs = TRUE;
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_texture_ceiling_toggle]))
				{
					graphics_preferences->screen_mode.texture_ceiling = !graphics_preferences->screen_mode.texture_ceiling;
					changed_screen_mode = changed_prefs = TRUE;
				}
#endif

#ifdef envppc
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_high_res]))
				{
						no_frame_rate_limit= !no_frame_rate_limit;
				}
#endif
#endif
				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_gamma_plus]))
				{
					if (graphics_preferences->screen_mode.gamma_level<NUMBER_OF_GAMMA_LEVELS-1)
					{
						graphics_preferences->screen_mode.gamma_level+= 1;
						change_gamma_level(graphics_preferences->screen_mode.gamma_level);
						changed_prefs= TRUE;
					}
				}

				if (check_input_sprocket_button(input_sprocket_elements[_input_sprocket_gamma_minus]))
				{
					if (graphics_preferences->screen_mode.gamma_level > 0)
					{
						graphics_preferences->screen_mode.gamma_level-= 1;
						change_gamma_level(graphics_preferences->screen_mode.gamma_level);
						changed_prefs= TRUE;
					}
				}
			}
			
			if (changed_screen_mode)
			{
				change_screen_mode(&graphics_preferences->screen_mode, TRUE);
				render_screen(0);
				if(update_interface) draw_interface();
			}
			if (changed_prefs) write_preferences();

#endif
			if(use_waitnext)
			{
				got_event= WaitNextEvent(everyEvent, &event, 2, (RgnHandle) NULL);
			}
			else
			{
				got_event= GetOSEvent(everyEvent, &event);
			}
			
			if(got_event) process_event(&event);
			
			if(get_game_state()==_game_in_progress) 
			{
				FlushEvents(keyDownMask|keyUpMask|autoKeyMask, 0);
			}
		}

		idle_game_state();
	}

	/* Flush all events on quit.. */
	FlushEvents(everyEvent, 0);
}

/* ------------- private code */
static void wait_for_highlevel_event(
	void)
{
	EventRecord event;
	boolean done= FALSE;
	
	// CAF -- this was blocking forever	
#if !SUPPORT_DRAW_SPROCKET
	while(!done)
	{
#endif
		if(WaitNextEvent(highLevelEventMask, &event, 0, NULL))
		{
			process_event(&event);
			done= TRUE;
		}
#if !SUPPORT_DRAW_SPROCKET
	}
#endif
	return;
}

/* Can't be static because the general_filter_proc calls this */
void update_any_window(
	WindowPtr window,
	EventRecord *event)
{
	GrafPtr old_port;

	GetPort(&old_port);
	SetPort(window);
	BeginUpdate(window);

	if(window==screen_window)
	{
		update_game_window(window, event);
	}
	EndUpdate(window);
	SetPort(old_port);
	
	return;
}

/* Can't be static because the general_filter_proc calls this */
void activate_any_window(
	WindowPtr window,
	EventRecord *event,
	boolean active)
{
	if(window==screen_window)
	{
		activate_screen_window(window, event, active);
	}
	
	return;
}

static void process_event(
	EventRecord *event)
{
	WindowPtr window;
	short part_code;

	// CAF -- give DSp a chance to handle the event
#if SUPPORT_DRAW_SPROCKET
	Boolean theEventWasProcessed = false;
	
	DSpProcessEvent( event, &theEventWasProcessed );
	if( theEventWasProcessed )
		return;
#endif

	switch (event->what)
	{
		case mouseDown:
			part_code= FindWindow(event->where, &window);
			switch (part_code)
			{
				case inSysWindow: /* DAs and the menu bar can blow me */
				case inMenuBar:
					halt();
					break;
					
				case inContent:
					process_screen_click(event);
					break;
					
				default:
					vhalt(csprintf(temporary, "What the hell is part code: %d?", part_code));
					break;
			}
			break;
		
		case keyDown:
		case autoKey:
			process_key(event, toupper(event->message&charCodeMask));
			break;
			
		case updateEvt:
			update_any_window((WindowPtr)event->message, event);
			break;
			
		case activateEvt:
			activate_any_window((WindowPtr)(event->message), event, event->modifiers&activeFlag);
			break;

		case kHighLevelEvent:
			handle_high_level_event(event);
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
			
		default:
			break;
	}
		
	return;
}

static void process_screen_click(
	EventRecord *event)
{
	GrafPtr old_port;
	Point where= event->where;
	boolean cheatkeys_down;
	
	GetPort(&old_port);
	SetPort(screen_window);
	
	GlobalToLocal(&where);
	cheatkeys_down= has_cheat_modifiers(event);
	portable_process_screen_click(where.h, where.v, cheatkeys_down);

	SetPort(old_port);	
	
	return;
}

static void process_key(
	EventRecord *event,
	short key)
{
	process_game_key(event, key);
	return;
}
