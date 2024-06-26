/*

	keyboard_dialog.c
	Tuesday, June 13, 1995 5:49:10 PM- rdm created.

*/

#include "macintosh_cseries.h"
#include "interface.h"
#include "shell.h"
#include "screen.h"

#ifdef SUPPORT_INPUT_SPROCKET
#include "InputSprocket.h"
#endif

#define strKEYCODES_TO_ASCII        133
#define alrtDUPLICATE_KEY           136
#define OPTION_KEYCODE             0x3a

enum {
	dlogCONFIGURE_KEYS= 1000,
	dlogCONFIGURE_KEYS12,
	iFORWARD=3,
	iBACKWARD,
	iTURN_LEFT,
	iTURN_RIGHT,
	iSLIDE_LEFT,
	iSLIDE_RIGHT,
	iLOOK_LEFT,
	iLOOK_RIGHT,
	iLOOK_UP,
	iLOOK_DOWN,
	iLOOK_STRAIGHT,
	iPREV_WEAPON,
	iNEXT_WEAPON,
	iTRIGGER,
	iALT_TRIGGER,
	iSIDESTEP,
	iRUN,
	iLOOK,
	iACTION_KEY,
	iTOGGLE_MAP,
	iUSE_MICROPHONE,
	iKEY_LAYOUT_POPUP_TITLE,
	iMOVED_OK= 55,
	iKEY_LAYOUT_POPUP= 62
};

#define FIRST_KEY_ITEM (iFORWARD)  // first item in dlogCONFIGURE_KEYS with a keycode
#define LAST_KEY_ITEM (iUSE_MICROPHONE)  // last item in dlogCONFIGURE_KEYS with a keycode

enum {
	_custom_keyboard_item= 4
};

/* Necessary globals, only for the key_setup_filter_proc */
struct keyboard_setup_struct {
	KeyMap old_key_map;
	short *keycodes;	
	short current_key_setup;
#ifdef SUPPORT_INPUT_SPROCKET
	ISpElementReference *isElements;
#endif
};

/* -------- globals. */
static struct keyboard_setup_struct keyboard_setup_globals;

/* -------- private prototypes */
boolean configure_key_setup(short *keycodes);
static pascal Boolean key_setup_filter_proc(DialogPtr dialog, EventRecord *event, short *item_hit);
static short setup_key_dialog(DialogPtr dialog, short *keycodes);
static short set_current_keyboard_layout(DialogPtr dialog, short *keycodes);
static void fill_in_key_name(DialogPtr dialog, short *keycodes, short which);
static short find_key_hit(byte *key_map, byte *old_key_map);
static short find_duplicate_keycode(short *keycodes);
static short keycode_to_charcode(short keycode);
static boolean is_pressed(short key_code);

/* ----------- entry point */
boolean configure_key_setup(
	short *keycodes)
{
	short item_hit, location_of_duplicate, menu_selection;
	DialogPtr dialog;
	boolean data_is_bad;
	ModalFilterUPP key_setup_filter_upp;
	short current_key_set;
	
	if (RECTANGLE_WIDTH(&(*world_device)->gdRect)<640 || RECTANGLE_HEIGHT(&(*world_device)->gdRect)<480)
	{
		dialog = myGetNewDialog(dlogCONFIGURE_KEYS12, NULL, (WindowPtr) -1, 0);
	}
	else
	{
		dialog = myGetNewDialog(dlogCONFIGURE_KEYS, NULL, (WindowPtr) -1, refCONFIGURE_KEYBOARD_DIALOG);
	}
	assert(dialog);
	key_setup_filter_upp= NewModalFilterProc(key_setup_filter_proc);
	
	/* Setup the keyboard dialog.. */
	current_key_set= setup_key_dialog(dialog, keycodes);
	
	/* Select the text.. */
	SelIText(dialog, iFORWARD, 0, SHORT_MAX);

	set_dialog_cursor_tracking(FALSE);
	ShowWindow(dialog);

	/* Setup the globals.. */
	GetKeys(keyboard_setup_globals.old_key_map);
	keyboard_setup_globals.keycodes= keycodes;
	keyboard_setup_globals.current_key_setup= current_key_set;

	do
	{
		do
		{
			ControlHandle control;
			short item_type;
			Rect bounds;

			ModalDialog(key_setup_filter_upp, &item_hit);
			
			switch(item_hit)
			{
				case iKEY_LAYOUT_POPUP:
					GetDItem(dialog, item_hit, &item_type, (Handle *) &control, &bounds);
					menu_selection= GetCtlValue(control) - 1;
					if (menu_selection != _custom_keyboard_item && keyboard_setup_globals.current_key_setup != menu_selection)
					{
						set_default_keys(keycodes, menu_selection);
						
						// looks slightly nicer to deselect text before changing and reselecting it.
						SelIText(dialog, ((DialogRecord *) dialog)->editField + 1, 0, 0);
						keyboard_setup_globals.current_key_setup= setup_key_dialog(dialog, keycodes);
						SelIText(dialog, ((DialogRecord *) dialog)->editField + 1, 0, SHORT_MAX);
					}
					break;
					
				default:
					break;
			}
		} while(item_hit != iMOVED_OK && item_hit != iCANCEL);

		if (!is_pressed(OPTION_KEYCODE))
		{
			location_of_duplicate= find_duplicate_keycode(keycodes);
			if (item_hit == iMOVED_OK && location_of_duplicate != NONE)
			{
				data_is_bad = TRUE;
				SelIText(dialog, location_of_duplicate + FIRST_KEY_ITEM, 0, SHORT_MAX);
				ParamText((StringPtr)getpstr(temporary, strKEYCODES_TO_ASCII, keycodes[location_of_duplicate]), (StringPtr)"", (StringPtr)"", (StringPtr)"");
				Alert(alrtDUPLICATE_KEY, NULL);
			}
			else
			{
				data_is_bad = FALSE;
			}
		}
		else
		{
			data_is_bad = FALSE;
		}
	} while (data_is_bad);

	DisposeRoutineDescriptor(key_setup_filter_upp);
	DisposeDialog(dialog);

	set_dialog_cursor_tracking(TRUE);
			
	return item_hit==iMOVED_OK;
}

/* ----------- private code. */
static pascal Boolean key_setup_filter_proc(
	DialogPtr dialog,
	EventRecord *event,
	short *item_hit)
{
	short keycode, which_item, current_edit_field;
	KeyMap key_map;
	Point where;
	GrafPtr old_port;
	boolean handled= FALSE;
	
	GetPort(&old_port);
	SetPort(dialog);
	
	/* preprocess events */	
	switch(event->what)
	{
		case nullEvent:
#if 0
#ifdef SUPPORT_INPUT_SPROCKET
			{
				ISpElementListReference globalList = nil;
				OSStatus theStatus = noErr;
				ISpElementEvent theEvent;
				Boolean wasEvent;
				
				theStatus = ISpGetGlobalElementList(&globalList);
				assert(theStatus == noErr);
				
				// get event may return an error if there was a > 4 byte data, we ignore that data, so no error checking
				// kinda a hack
				ISpElementList_GetNextEvent(globalList, sizeof(ISpElementEvent), &theEvent, &wasEvent);
				
				if (wasEvent)
				{
					ISpElementInfo theInfo;
					ControlHandle control;
					Handle item_handle;
					Rect item_box;
					short item_type;
					Boolean setText = false;
					UInt32 enoughAxis = ((kISpAxisMaximum - kISpAxisMiddle) * 0.9) + kISpAxisMiddle;
					static ISpElementReference bogusAxis = nil;

					current_edit_field= ((DialogRecord *) dialog)->editField + 1;
					
					ISpElement_GetInfo(theEvent.element, &theInfo);
					
					if (bogusAxis != nil)
					{
						// do something sometime to clear the bogus axis again if it is
						// far enough back down...
					}
	
					if ((theInfo.theKind == kISpElementKind_Button) && (theEvent.data == kISpButtonDown))
					{
						// ok, this was a button stuff it into our structure
						// theEvent.element was what was hit
						setText = true;
					}
					else if ((theInfo.theKind == kISpElementKind_DPad) && (theEvent.data != kISpPadIdle))
					{
						// set this one to activate if it is also == theEvent.data
						setText = true;
					}
					else if ((theInfo.theAxis == kISpElementKind_Axis) && (theEvent.data > (enoughAxis)) && (bogusAxis != theEvent.element))
					{
						setText = true;
					}
					else
					{
						setText = false;
						
						// handle axis and other stuff later
						GetDItem(dialog, iKEY_LAYOUT_POPUP, &item_type, (Handle *) &control, &item_box);
						SetCtlValue(control, _custom_keyboard_item+1);
					}
					
					if (setText)
					{
						GetDItem(dialog, current_edit_field, &item_type, &item_handle, &item_box);
						SetIText(item_handle, theInfo.theString);
						
						GetDItem(dialog, iKEY_LAYOUT_POPUP, &item_type, (Handle *) &control, &item_box);
						SetCtlValue(control, _custom_keyboard_item+1);

						*item_hit = current_edit_field < LAST_KEY_ITEM ? current_edit_field + 1 : FIRST_KEY_ITEM;
						SelIText(dialog, *item_hit, 0, SHORT_MAX);
					}
				}
			}
			break;
#endif
#endif
		case keyDown:
		case autoKey:
			GetKeys(key_map);
			if (memcmp(key_map, keyboard_setup_globals.old_key_map, sizeof(KeyMap))) // the user has hit a new key
			{
				current_edit_field= ((DialogRecord *) dialog)->editField + 1;
				keycode= find_key_hit((byte *)key_map, (byte *)keyboard_setup_globals.old_key_map);

				// update the text field
				if (keycode != NONE && keycode != keyboard_setup_globals.keycodes[current_edit_field - FIRST_KEY_ITEM])
				{
					keyboard_setup_globals.keycodes[current_edit_field - FIRST_KEY_ITEM]= keycode;
					fill_in_key_name(dialog, keyboard_setup_globals.keycodes, current_edit_field - FIRST_KEY_ITEM);
				}
				
				if (keycode != NONE)
				{
					// select the next item.
					*item_hit = current_edit_field < LAST_KEY_ITEM ? current_edit_field + 1 : FIRST_KEY_ITEM;
					SelIText(dialog, *item_hit, 0, SHORT_MAX);
				}
				BlockMove(key_map, keyboard_setup_globals.old_key_map, sizeof(KeyMap));
			}
			
			/* Change the keysetup if necessary */
			keyboard_setup_globals.current_key_setup= 
				set_current_keyboard_layout(dialog, keyboard_setup_globals.keycodes);
			if (event->what != nullEvent)	handled = TRUE;
			break;
			
		case mouseDown:
			where = event->where;
			GlobalToLocal(&where);
			which_item= FindDItem(dialog, where) + 1;
			if (which_item >= FIRST_KEY_ITEM && which_item <= LAST_KEY_ITEM)
			{
				SelIText(dialog, which_item, 0, SHORT_MAX);
				*item_hit = which_item;
				handled = TRUE;
			}
			break;
			
		case updateEvt:
			break;
	}

	SetPort(old_port);

	return handled ? TRUE : general_filter_proc(dialog, event, item_hit);
}

static short setup_key_dialog(
	DialogPtr dialog, 
	short *keycodes)
{
	short key, current_key_setup;

	for (key= 0; key<NUMBER_OF_KEYS; key++)
	{
		fill_in_key_name(dialog, keycodes, key);
	}

	current_key_setup= set_current_keyboard_layout(dialog, keycodes);
	
	return current_key_setup;
}

static short set_current_keyboard_layout(
	DialogPtr dialog,
	short *keycodes)
{
	short current_key_setup;
	ControlHandle control;
	short item_type;
	Rect bounds;

	GetDItem(dialog, iKEY_LAYOUT_POPUP, &item_type, (Handle *) &control, &bounds);
	current_key_setup= find_key_setup(keycodes);
	if(current_key_setup==NONE)
	{
		current_key_setup= _custom_keyboard_item;
	}
	SetCtlValue(control, current_key_setup+1);

	return current_key_setup;
}

static void fill_in_key_name(
	DialogPtr dialog, 
	short *keycodes, 
	short which)
{
	Rect item_box;
	short item_type;
	Handle item_handle;
	
	vassert(keycodes[which] >= 0 && keycodes[which] <= 0x7f,
		csprintf(temporary, "which = %d, keycodes[which] = %d", which, keycodes[which]));
	getpstr(temporary, strKEYCODES_TO_ASCII, keycodes[which]);
	GetDItem(dialog, which + FIRST_KEY_ITEM, &item_type, &item_handle, &item_box);
	SetIText(item_handle, (StringPtr)temporary);
}

static short find_key_hit(
	byte *key_map, 
	byte *old_key_map)
{
	byte mask;
	byte bit_count = 0;
	short i;
	short keycode = NONE;
	
	for (i = 0; i < 16; i++)
	{
		if (key_map[i] > old_key_map[i]) // an extra bit is set
		{
			mask = key_map[i] ^ old_key_map[i];
			while (!(mask & 1))
			{
				mask >>= 1;
				bit_count++;
			}
			keycode = (i * 8) + bit_count; 
			break;
		}
	}

	if(keycode != NONE)
	{
		short error_message= NONE;
		short charcode;

		charcode = keycode_to_charcode(keycode);
		switch(charcode)
		{
			case ',':
			case '.':
				if (keycode != 0x41) // IS_KEYPAD() is worthless here.
				{
					error_message= keyIsUsedForSound;
				}
				break;
				
			case '-':
			case '_':
			case '+':
			case '=':
				if (keycode != 0x4e && keycode != 0x45) // IS_KEYPAD() is worthless here
				{
					error_message=  keyIsUsedForMapZooming;
				}
				break;
				
			case '[':
			case ']':
				error_message= keyIsUsedForScrolling;
				break;
				
			default:
				switch(keycode)
				{
					case kcF1:
					case kcF2:
					case kcF3:
					case kcF4:
					case kcF5:
					case kcF6:
					case kcF7:
					case kcF8:
					case kcF9:
					case kcF10:
					case kcF11:
					case kcF12:
						error_message= keyIsUsedAlready;
						break;
						
					case 0x7f: /* This is the power key, which is not reliable... */
						keycode = NONE;
						break;
				}
				break;
		}
	
		if (error_message != NONE)
		{
			alert_user(infoError, strERRORS, error_message, 0);
			keycode= NONE;
		}
	}
	
	return keycode;
}

static short find_duplicate_keycode(
	short *keycodes)
{
	short i, j;
	
	for (i= 0; i<NUMBER_OF_KEYS; i++)
	{
		for (j= i+1; j<NUMBER_OF_KEYS; j++)
		{
			if (keycodes[i] == keycodes[j])	return j;
		}
	}
	
	return NONE;
}

/* DANGER! DANGER! DANGER!! This is Alain's code, untouched.... */
static short keycode_to_charcode(
	short keycode)
{
	byte    locked;
	long    kchr_resource_id;
	long    bullshit_from_system;
	short   charcode;
	Handle  kchr_resource;

	static unsigned long state = 0;

	// get the resource and lock it down
	kchr_resource_id = GetScript(GetEnvirons(smKeyScript), smScriptKeys);
	kchr_resource = GetResource('KCHR', (short) kchr_resource_id); // probably DON’t want to release it.
	assert(kchr_resource);
	locked = HGetState(kchr_resource) & 0x80;
	HLock(kchr_resource);

	// translate the key
	// note the 2 methods for getting the character code. the first one is what inside mac
	// tells me to do. the 2nd one is what i have to do, because of what keytrans returns
	// to me. oy vay. liars all around me.
	bullshit_from_system = KeyTrans(*kchr_resource, keycode, &state);
//	charcode = (bullshit_from_system >> 16) & 0x00ff;
	charcode = bullshit_from_system & 0x000000ff;

	// unlock it, if that's the way the system had it.
	if (!locked) HUnlock(kchr_resource);
	
	return charcode;
}

static boolean is_pressed(
	short key_code)
{
	KeyMap key_map;
	
	GetKeys(key_map);
	return ((((byte*)key_map)[key_code>>3] >> (key_code & 7)) & 1);
}
