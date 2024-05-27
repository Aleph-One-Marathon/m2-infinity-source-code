/*
GAME_DIALOGS.C
Wednesday, December 15, 1993 8:26:14 PM
Saturday, July 9, 1994 5:54:26 PM (alain)
   	ajr--brought back the preferences from the dead.
Thursday, August 11, 1994 3:47:55 PM (alain)
	added dialog for configuring the keys
Monday, September 19, 1994 11:16:09 PM  (alain)
	completely revamping the preferences dialog, now using System 6 popups instead
	of millions of radio buttons. (millions? ok, ok, maybe billions.)
Tuesday, September 20, 1994 7:41:56 PM  (alain)
	key config dialog also has popup for selecting which key setup to use.
Wednesday, June 14, 1995 8:47:39 AM
	gutted.  Keyboard stuff is now in keyboard_dialog.c.  Preferences related stuff is
		now in preferences.c.
*/

#include "macintosh_cseries.h"
#include <string.h>

#include "map.h"
#include "shell.h"
#include "interface.h"
#include "preferences.h"
#include "screen.h"
#include "portable_files.h"

#define DECODE_ONLY
#include "serial_numbers.c"

#ifdef mpwc
	#pragma segment dialogs
#endif

enum {
	dlogQUIT_WITHOUT_SAVING= 129
};

enum {
	dlogSERIAL_NUMBER= 133,
	iSERIAL_NAME_BOX= 3,
	iSERIAL_NUMBER_BOX
};

#define MAXIMUM_SERIAL_NUMBER_RETRIES 3

/* ----------- private prototypes */
static void serial_dialog_instantiate_proc(DialogPtr dialog);
static void delete_partial_preferences_file(void);

/* ----------- code */
boolean quit_without_saving(
	void)
{
	DialogPtr dialog;
	GrafPtr old_port;
	short item_hit;
	boolean quit= FALSE;
	Point origin= {78, 134};
	
	dialog= myGetNewDialog(dlogQUIT_WITHOUT_SAVING, NULL, (WindowPtr) -1, 0);
	assert(dialog);

	GetPort(&old_port);
	SetPort(screen_window);
	LocalToGlobal(&origin);
	SetPort(old_port);
	MoveWindow(dialog, origin.h, origin.v, FALSE);
	ShowWindow(dialog);
	
	ModalDialog(get_general_filter_upp(), &item_hit);
	DisposeDialog(dialog);
	
	return item_hit!=iOK ? FALSE : TRUE; /* note default button is the safe, don’t quit, one */
}

void ask_for_serial_number(
	void)
{
	DialogPtr dialog= myGetNewDialog(dlogSERIAL_NUMBER, NULL, (WindowPtr) -1, 0);
	boolean valid_serial_number= FALSE;
	short retries= 0;
	short item_hit;
	
	assert(dialog);
	assert(serial_preferences);
	
	/* setup and show dialog */
	serial_dialog_instantiate_proc(dialog);
	ShowWindow(dialog);

	do
	{
		boolean reinstantiate= FALSE;
		
		ModalDialog(get_general_filter_upp(), &item_hit);
		
		switch(item_hit)
		{
			case iSERIAL_NAME_BOX:
			case iSERIAL_NUMBER_BOX:
			case iOK:
				reinstantiate= TRUE;
				break;
		}
		
		if (reinstantiate) serial_dialog_instantiate_proc(dialog);

		if (item_hit==iOK)	
		{		
			byte short_serial_number[BYTES_PER_SHORT_SERIAL_NUMBER];
			byte inferred_pad[BYTES_PER_SHORT_SERIAL_NUMBER];
			
			long_serial_number_to_short_serial_number_and_pad(serial_preferences->long_serial_number, short_serial_number, inferred_pad);

			// also allow marathon2 network-only numbers
			if ((PADS_ARE_EQUAL(inferred_pad, actual_pad) || 
				(PADS_ARE_EQUAL(inferred_pad, actual_pad_m2) && ((char)short_serial_number[2])<0)) &&
				VALID_INVERSE_SEQUENCE(short_serial_number))
			{
				serial_preferences->network_only= ((char)short_serial_number[2])<0 ? TRUE : FALSE;
				valid_serial_number= TRUE;
			}
			else
			{
				/* If they are about to sped. */
				if(retries+1>=MAXIMUM_SERIAL_NUMBER_RETRIES) delete_partial_preferences_file();
				alert_user(++retries>=MAXIMUM_SERIAL_NUMBER_RETRIES ? fatalError : infoError, strERRORS, badSerialNumber, 0);
				SelIText(dialog, iSERIAL_NUMBER_BOX, 0, SHORT_MAX);
			}
		}
	}
	while ((item_hit!=iOK || !valid_serial_number) && item_hit!=iCANCEL);
	
	DisposeDialog(dialog);

	if (!valid_serial_number) 
	{
		delete_partial_preferences_file();
		exit(0);
	}

	return;
}

static void delete_partial_preferences_file(
	void)
{
	FSSpec preferences_file;
	
	getpstr((char *)preferences_file.name, strFILENAMES, filenamePREFERENCES);
	find_preferences_location((FileDesc *)&preferences_file);
	FSpDelete(&preferences_file);

	return;
}

/* ------------- private code */
static void serial_dialog_instantiate_proc(
	DialogPtr dialog)
{
	short item_type;
	Handle item_handle;
	Rect item_rectangle;
	
	GetDItem(dialog, iSERIAL_NAME_BOX, &item_type, &item_handle, &item_rectangle);
	GetIText(item_handle, serial_preferences->user_name);

	GetDItem(dialog, iSERIAL_NUMBER_BOX, &item_type, &item_handle, &item_rectangle);
	GetIText(item_handle, serial_preferences->tokenized_serial_number);

	modify_control(dialog, iOK, (*serial_preferences->user_name && *serial_preferences->tokenized_serial_number) ? CONTROL_ACTIVE : CONTROL_INACTIVE, 0);
	
	generate_long_serial_number_from_tokens((char *)serial_preferences->tokenized_serial_number+1, serial_preferences->long_serial_number);
	
	return;
}