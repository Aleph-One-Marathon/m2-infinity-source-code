/*
DIALOGS.C
Friday, October 29, 1993 10:27:55 AM

Friday, October 29, 1993 10:28:11 AM
	copied lots of code from MACINTOSH_UTILITIES.C, added some DTS code for real command-.
	events and for cursor tracking.
Saturday, November 20, 1993 1:34:55 PM
	internationalization.
Friday, April 1, 1994 9:03:41 AM
	it ocurred to me just now that dialogs can eat suspend and resume events (for the
	application) just like they used to eat update events.  this is not cool.
*/

#include "macintosh_cseries.h"

#include <ctype.h>
#include <string.h>

#ifdef mpwc
#pragma segment modules
#endif

/* ---------- constants */

/* ---------- globals */

static ModalFilterUPP general_filter_upp= (ModalFilterUPP) NULL;

static dialog_header_proc_ptr dialog_header_proc= (dialog_header_proc_ptr) NULL;
static update_any_window_proc_ptr update_any_window_proc= (update_any_window_proc_ptr) NULL;
static suspend_resume_proc_ptr suspend_resume_proc= (suspend_resume_proc_ptr) NULL;

static boolean cursor_tracking= TRUE;

/* ---------- private prototypes */

static void handle_shift_tab(DialogPtr dialog);

static void draw_dialog_header(DialogPtr dialog);
static void draw_dialog_frames(DialogPtr dialog);
static void draw_broken_frame(Rect *outline_rect, Rect *stop_rect);

static short dialog_keyboard_equivilents(DialogPtr dialog, char key);
static boolean dialog_has_edit_items(DialogPtr dialog);
static void track_dialog_cursor(DialogPtr dialog);

static void position_window(Rect *frame, word code);

/* ---------- code */

void set_dialog_header_proc(
	dialog_header_proc_ptr proc)
{
	dialog_header_proc= proc;
	
	return;
}

void set_update_any_window_proc(
	update_any_window_proc_ptr proc)
{
	update_any_window_proc= proc;
	
	return;
}

void set_suspend_resume_proc(
	suspend_resume_proc_ptr proc)
{
	suspend_resume_proc= proc;
	
	return;
}

void set_dialog_cursor_tracking(
	boolean status)
{
	cursor_tracking= status;
	
	return;
}

ModalFilterUPP get_general_filter_upp(
	void)
{
	if (!general_filter_upp)
	{
		general_filter_upp= NewModalFilterProc(general_filter_proc);
	}
	
	return general_filter_upp;
}

pascal Boolean general_filter_proc(
	DialogPtr dialog,
	EventRecord *event,
	short *item_hit)
{
	char key;
	Rect item_rectangle;
	short item_type;
	Handle item_handle;
	GrafPtr old_port;
	short part;

	global_idle_proc();

	if (cursor_tracking) track_dialog_cursor(dialog);
	
	switch (event->what)
	{
		case keyDown:
		case autoKey:
			if (CmdPeriodEvent(event))
			{
				if (hit_dialog_button(dialog, iCANCEL))
				{
					*item_hit= iCANCEL;
					return TRUE;
				}
				else
				{
					GetDItem(dialog, iCANCEL, &item_type, &item_handle, &item_rectangle);
					if (item_type!=ctrlItem+btnCtrl&&hit_dialog_button(dialog, iOK))
					{
						*item_hit= iOK;
						return TRUE;
					}
				}
				
				/* eat it, this dialog can’t be cancelled */
			}
			else
			{
				key= event->message&charCodeMask;
				switch (key)
				{
					case 0:
						/* we might get zeros from the stay_awake() proc, and we should eat them */
						return TRUE;
					
					case 0x09:
						if (event->modifiers&shiftKey)
						{
							if (dialog_has_edit_items(dialog))
							{
								handle_shift_tab(dialog);
								
								*item_hit= ((DialogPeek)dialog)->editField+1;
								return TRUE;
							}
						}
						break;
					
					case 0x0D:
					case 0x03:
						if (hit_dialog_button(dialog, iOK))
						{
							*item_hit= iOK;
							return TRUE;
						}
						else
						{
							GetDItem(dialog, iOK, &item_type, &item_handle, &item_rectangle);
							if (item_type!=ctrlItem+btnCtrl&&hit_dialog_button(dialog, iCANCEL))
							{
								*item_hit= iCANCEL;
								return TRUE;
							}
							else
							{
								/* we want to eat RETURNs and ENTERs, right?  we don’t want an editText
									item getting a hold of them */
								event->what= nullEvent;
							}
						}
						break;

#if 0
					/* this seems like a sketchy bit of code to be running on a non-US system */						
					case 'C':
					case 'V':
					case 'X':
						if (event->modifiers&cmdKey)
						{
							if (dialog_has_edit_items(dialog))
							{
								switch(key)
								{
									case 'C':
										DlgCopy(dialog);
										break;
									case 'V':
										DlgPaste(dialog);
										break;
									case 'X':
										DlgCut(dialog);
										break;
								}
								*item_hit= ((DialogPeek)dialog)->editField+1;
								return TRUE;
							}
						}
						break;
#endif
				}
				
				if (!(event->modifiers&(optionKey|controlKey)))
				{
					if (!dialog_has_edit_items(dialog)||(event->modifiers&cmdKey))
					{
						part= dialog_keyboard_equivilents(dialog, key);
						if (part!=NONE)
						{
							*item_hit= part;
							return TRUE;
						}
					}
				}
			}
			break;

		case activateEvt:
			if ((Ptr)(event->message)!=(Ptr)dialog)
			{
				activate_any_window((WindowPtr)(event->message), event, event->modifiers&activeFlag);
			}
			break;

		case updateEvt:
			if ((Ptr)(event->message)!=(Ptr)dialog)
			{
				update_any_window((WindowPtr)event->message, event);
			}
			else
			{
				GetPort(&old_port);
				SetPort(dialog);
				
				/* outline default button */
				GetDItem(dialog, iOK, &item_type, (Handle *) &item_handle, &item_rectangle);
				if (item_type==ctrlItem+btnCtrl)
				{
					PenSize(3, 3);
					InsetRect(&item_rectangle, -4, -4);
					FrameRoundRect(&item_rectangle, 16, 16);
					PenSize(1, 1);
				}
				
				if (dialog_header_proc) draw_dialog_header(dialog);
				draw_dialog_frames(dialog);
				
				SetPort(old_port);
			}
			break;
	}
	
	return FALSE;
}

boolean hit_dialog_button(
	DialogPtr dialog,
	short item)
{
	short item_type;
	Handle item_handle;
	Rect item_rectangle;
	
	GetDItem(dialog, item, &item_type, &item_handle, &item_rectangle);
	if (item_type==ctrlItem+btnCtrl)
	{
		if ((*(ControlHandle)item_handle)->contrlHilite==CONTROL_ACTIVE)
		{
			HiliteControl((ControlHandle)item_handle, CONTROL_HIGHLIGHTED);
			hold_for_visible_delay();
			HiliteControl((ControlHandle)item_handle, CONTROL_ACTIVE);
			return TRUE;
		}
	}
	
	return FALSE;
}

void modify_radio_button_family(
	DialogPtr dialog,
	short start,
	short end,
	short current)
{
	Rect rectangle;
	short type;
	ControlHandle handle;
	short i, item_count;
	
	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	assert(end>start);
	assert(start>0&&start<=item_count);
	assert(end>0&&end<=item_count);
	assert(current>=start&&current<=end);
	
	for (i=start;i<=end;++i)
	{
		GetDItem(dialog, i, &type, (Handle *) &handle, &rectangle);
		assert(type==ctrlItem+radCtrl);
		SetCtlValue(handle, i==current);
	}
	
	return;
}

/* the assertion, below, wasn’t working for disabled items-- that is, it wasn’t working for
	items disabled from resedit, which is somehow different from items disabled at run-time. */
void modify_control(
	DialogPtr dialog,
	short control,
	short status,
	short value)
{
	Rect item_rectangle;
	short item_type, item_count;
	ControlHandle item_handle;
	
	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	assert(control>0&&control<=item_count);

	GetDItem(dialog, control, &item_type, (Handle *) &item_handle, &item_rectangle);
	assert(item_type>=ctrlItem&&item_type<=ctrlItem+resCtrl);
	if (status!=NONE) HiliteControl(item_handle, status);
	if (value!=NONE) SetCtlValue(item_handle, value);
	
	return;
}

short myAlert(
	short alertID,
	ModalFilterUPP filterProc)
{
	Handle tmplate;
	
	tmplate= GetResource('ALRT', alertID);
	assert(tmplate);
	
	if (tmplate && GetHandleSize(tmplate)==14)
	{
		position_window((Rect*)*tmplate, *((word*)*tmplate+6));
	}
	
	return Alert(alertID, filterProc);
}

DialogPtr myGetNewDialog(
	short dialogID,
	void *dStorage,
	WindowPtr behind,
	long refCon)
{
	DialogPtr dialog;
	Handle tmplate;
	
	tmplate= GetResource('DLOG', dialogID);
	assert(tmplate);
	
	if (tmplate && GetHandleSize(tmplate)==24)
	{
		position_window((Rect*)*tmplate, *((word*)*tmplate+11));
	}
	
	dialog= GetNewDialog(dialogID, dStorage, behind);
	if (dialog) SetWRefCon(dialog, refCon);
	
	return dialog;
}

#define DIALOG_HEADER_SPACING 4

void standard_dialog_header_proc(
	DialogPtr dialog,
	Rect *frame)
{
	Rect background, destination;
	PicHandle header;

	#pragma unused (dialog)

	header= (PicHandle) GetResource('PICT', 256);
	assert(header);

	if (header)
	{
		SetRect(&background, frame->left, frame->top, frame->right, frame->top+
			RECTANGLE_HEIGHT(&(*header)->picFrame)+2*DIALOG_HEADER_SPACING);
		PaintRect(&background);
		
		destination= (*header)->picFrame;
		OffsetRect(&destination, frame->left+2*DIALOG_HEADER_SPACING-destination.left,
			frame->top+DIALOG_HEADER_SPACING-destination.top);
		DrawPicture(header, &destination);
	}
	
	return;
}

/* Constants for checking on command-. */
#define kMaskModifiers 0xFE00     /* Mask for modifiers w/o command key */
#define kMaskASCII1    0x00FF0000 /* get the key out of the ASCII1 byte */
#define kMaskASCII2    0x000000FF /* get the key out of the ASCII2 byte */

/* from DTS */
Boolean CmdPeriodEvent(
	EventRecord *anEvent) /* Event being tested */
{
	long    lowChar;      /* Low character of keyInfo */
	long    highChar;     /* High character of keyInfo */
	Handle  hKCHR;        /* Handle to the currently-used KCHR */
	long    keyInfo;      /* Key information returned from KeyTrans */
	long    keyScript;    /* Script of the current keyboard */
	unsigned long    state;        /* State used for KeyTrans */
	short   virtualKey;   /* Virtual keycode of the character-generating key */
	short   keyCode;      /* Keycode of the character-generating key */
	Boolean gotCmdPeriod; /* True if detected command-. */

	gotCmdPeriod = false;
	if (anEvent->what == keyDown || anEvent->what == autoKey)
	{
		if (anEvent->modifiers & cmdKey)
		{
			/* Get the virtual keycode of the code-generating key */
			virtualKey = (anEvent->message & keyCodeMask) >> 8;

			/* Get a copy of the current KCHR */
			keyScript = GetScript( GetEnvirons( smKeyScript ), smScriptKeys);
			hKCHR = GetResource('KCHR', keyScript);
			if (hKCHR != nil)
			{
				/* AND out the command key and OR in the virtualKey */
				keyCode = (anEvent->modifiers & kMaskModifiers) | virtualKey;

				/* Get key information */
				state = 0;
				keyInfo = KeyTrans( *hKCHR, keyCode, &state);
			}
			else
				keyInfo = anEvent->message;

			/* Check both low and high bytes of keyInfo for period */
			lowChar = keyInfo & kMaskASCII2;
			highChar = (keyInfo & kMaskASCII1) >> 16;
			if (lowChar == '.' || highChar == '.')
				gotCmdPeriod = true;
		}
		else
		{
			/* my addition */
			if ((anEvent->message&charCodeMask)==kESCAPE) gotCmdPeriod= TRUE;
		}
	}

	return gotCmdPeriod;
}

long extract_number_from_text_item(
	DialogPtr dialog,
	short item_number)
{
	long       num;
	Rect       item_box;
	short      item_type;
	Handle     item_handle;

	GetDItem(dialog, item_number, &item_type, &item_handle, &item_box);
	GetIText(item_handle, (unsigned char *)temporary);
	StringToNum((const unsigned char *)temporary, &num);
	
	return num;
}

float extract_float_from_text_item(
	DialogPtr dialog, 
	short item_num)
{
	Rect       item_box;
	short      item_type;
	float      num;
	Handle     item_handle;
	
	GetDItem(dialog, item_num, &item_type, &item_handle, &item_box);
	GetIText(item_handle, (unsigned char *)temporary);
	p2cstr((unsigned char *)temporary);
	sscanf(temporary, "%f", &num);
	
	return num;
}

void insert_number_into_text_item(
	DialogPtr dialog,
	short item_number,
	long new_value)
{
	char       number_text[12]; // big enough to handle 4294967295 for sure.
	Rect       item_box;
	short      item_type;
	Handle     item_handle;
	
	GetDItem(dialog, item_number, &item_type, &item_handle, &item_box);
	NumToString(new_value, (unsigned char *)number_text);
	SetIText(item_handle, (StringPtr)number_text);
}

void insert_float_into_text_item(
	DialogPtr dialog, 
	short item_number,
	float new_value)
{
	Rect       item_box;
	short      item_type;
	Handle     item_handle;
	
	sprintf(temporary, "%.3f", new_value);
	c2pstr(temporary);

	GetDItem(dialog, item_number, &item_type, &item_handle, &item_box);
	SetIText(item_handle, (StringPtr)temporary);
}

MenuHandle get_popup_menu_handle(
	DialogPtr dialog,
	short item)
{
	struct PopupPrivateData **privateHndl;
	MenuHandle menu;
	short item_type;
	ControlHandle control;
	Rect bounds;

	/* Add the maps.. */
	GetDItem(dialog, item, &item_type, (Handle *) &control, &bounds);

	/* I don't know how to assert that it is a popup control... <sigh> */
	privateHndl= (PopupPrivateData **) ((*control)->contrlData);
	assert(privateHndl);

	menu= (*privateHndl)->mHandle;
	assert(menu);

	return menu;
}

/* ---------- private code */

/* all this positioning stuff didn’t seem to work terribly well under a system that was already
	trying to position the windows: for some reason it was trashing (as far as i could tell) the
	first four bytes (the rectangle) of the ALRT or DLOG template after bringing up the alert
	or dialog for the first time.  so we only do this under pre-system seven */
static void position_window(
	Rect *frame,
	word code)
{
	Rect bounds, window_frame;
	SysEnvRec environment;
	GDHandle device;
	WindowPtr window;	

	if (SysEnvirons(curSysEnvVers, &environment)==noErr)
	{
		if (environment.systemVersion<0x0700)
		{
			if ((code&0xff)==0x0a)
			{
				/* default to main screen */
				bounds= (*GetMainDevice())->gdRect;
				bounds.top+= GetMBarHeight();
				window= FrontWindow();
				
				switch (code&0xc000)
				{
					/* main screen */
					case 0x0000:
						break;
					
					/* parent window, defaults to main screen if no window */
					case 0x8000:
						if (window)
						{
							get_window_frame(window, &bounds);
						}
						break;
					
					/* parent window’s screen, defaults to main screen if no window */
					case 0x4000:
						if (window)
						{
							get_window_frame(window, &window_frame);
							device= MostDevice(&window_frame);
							if (device&&device!=GetMainDevice())
							{
								bounds= (*device)->gdRect;
							}
						}
						break;
					
					default:
						halt();
				}
		
				switch (code&0x3800)
				{
					/* center */
					case 0x2800:
						AdjustRect(&bounds, frame, frame, centerRect);
						break;
					
					/* alert */
					case 0x3000:
						AdjustRect(&bounds, frame, frame, alertRect);
						break;
					
					/* stagger (not supported) */
					case 0x3800:
					default:
						halt();
				}
			}
		}
	}
	
	return;
}

static void draw_dialog_frames(
	DialogPtr dialog)
{
	short item, item_count;
	Rect frame, text;
	Handle item_handle;
	short item_type;
	boolean color;
	
	color= ((dialog->portBits.rowBytes&0xc000)==0xc000) ? TRUE : FALSE;
	if (color)
	{
		RGBForeColor(system_colors+gray33Percent);
	}
	else
	{
		PenPat(&qd.gray);
	}
	
	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	for (item=1;item<=item_count;++item)
	{
		boolean drew_frame = FALSE;
		
		GetDItem(dialog, item, &item_type, &item_handle, &frame);
		if ((item_type&~itemDisable)==userItem)
		{
			if (item != item_count)
				GetDItem(dialog, item+1, &item_type, &item_handle, &text);
			if (frame.top == frame.bottom-1)
			{
				MoveTo(frame.left, frame.top);
				LineTo(frame.right, frame.top);
				drew_frame= TRUE;
			}
			else if (frame.left == frame.right-1)
			{
				MoveTo(frame.left, frame.top);
				LineTo(frame.left, frame.bottom);
				drew_frame = TRUE;
			}
			if (item!=item_count&&((item_type&~itemDisable)==statText || (item_type&~itemDisable)==resCtrl+ctrlItem))
			{
				if (text.top<frame.top&&text.bottom>frame.top&&text.left>frame.left)
				{
					draw_broken_frame(&frame, &text);
					item+= 1;
					drew_frame = TRUE;
				}
			}			
			if (!drew_frame && item > 1)
			{
				GetDItem(dialog, item-1, &item_type, &item_handle, &text);
				if ((item_type&~itemDisable)==statText || (item_type&~itemDisable)==resCtrl+ctrlItem)
				{
					if (text.top<frame.top&&text.bottom>frame.top&&text.left>frame.left)
					{
						draw_broken_frame(&frame, &text);
					}
				}			
			}
		}
	}

	if (color)
	{
		RGBForeColor(&rgb_black);
	}
	else
	{
		PenPat(&qd.black);
	}
	
	return;
}

static void draw_broken_frame(Rect *outline_rect, Rect *stop_rect)
{
	MoveTo(stop_rect->left-2, outline_rect->top);
	LineTo(outline_rect->left, outline_rect->top);
	LineTo(outline_rect->left, outline_rect->bottom);
	LineTo(outline_rect->right, outline_rect->bottom);
	LineTo(outline_rect->right, outline_rect->top);
	LineTo(stop_rect->right+2, outline_rect->top);
	
	return;
}

/* the default visRgn (on the front window) and the default clipRgn both clip to the content
	region.  it might be more robust, here, to set these to the structure region (but that’s
	in global coordinates, so we’d have to do some region voodoo to get it like we want.  why
	aren’t there LocalRgn() and GlobalRgn() calls? */
static void draw_dialog_header(
	DialogPtr dialog)
{
	RgnHandle old_visRgn, old_clipRgn, new_region;
	Rect frame;
	
	old_visRgn= dialog->visRgn;
	old_clipRgn= dialog->clipRgn;
	
	new_region= NewRgn();
	frame= dialog->portRect;
	InsetRect(&frame, -DIALOG_INSET, -DIALOG_INSET);
	RectRgn(new_region, &frame);
	
	dialog->visRgn= new_region;
	dialog->clipRgn= new_region;
	dialog_header_proc(dialog, &frame);
	dialog->visRgn= old_visRgn;
	dialog->clipRgn= old_clipRgn;
	
	DisposeRgn(new_region);

	return;
}

static short dialog_keyboard_equivilents(
	DialogPtr dialog,
	char key)
{
	short item_count, item;
	short item_type;
	Handle item_handle;
	Rect item_rectangle;

	key= tolower(key);
	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	
	for (item=1;item<=item_count;++item)
	{
		GetDItem(dialog, item, &item_type, &item_handle, &item_rectangle);
		if (item_type==ctrlItem+btnCtrl)
		{
			if ((*(ControlHandle)item_handle)->contrlHilite==CONTROL_ACTIVE)
			{
				GetCTitle((ControlHandle)item_handle, (unsigned char *)temporary);
				if (((byte)*temporary)<128) /* ignore high ascii for kanji buttons */
				{
					if (key==tolower(*(temporary+1)))
					{
						HiliteControl((ControlHandle)item_handle, CONTROL_HIGHLIGHTED);
						hold_for_visible_delay();
						HiliteControl((ControlHandle)item_handle, CONTROL_ACTIVE);
						
						return item;
					}
				}
			}
		}
	}
	
	return NONE;
}

static boolean dialog_has_edit_items(
	DialogPtr dialog)
{
	short item_count, item;
	short item_type;
	Handle item_handle;
	Rect item_rectangle;
	
	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	for (item=1;item<=item_count;++item)
	{
		GetDItem(dialog, item, &item_type, &item_handle, &item_rectangle);
		if (item_type==editText)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

static void handle_shift_tab(
	DialogPtr dialog)
{
	short item, item_count, old_field;
	short item_type;
	Handle item_handle;
	Rect item_rectangle;
	
	item_count= GET_DIALOG_ITEM_COUNT(dialog);
	old_field= ((DialogPeek)dialog)->editField+1;
	item= old_field-1;
	if (item<1) item= item_count;

	while (item!=old_field)
	{
		GetDItem(dialog, item, &item_type, &item_handle, &item_rectangle);
		if (item_type==editText)
		{
			SelIText(dialog, item, 0, 32767);
			return;
		}
		
		item-= 1;
		if (item<1) item= item_count;
	}
	
	return;
}

static void track_dialog_cursor(
	DialogPtr dialog)
{
	GrafPtr old_port;
	Point mouse;
	short item_type;
	Handle item_handle;
	Rect item_rectangle;
	short edit_item;
	
	if (dialog_has_edit_items(dialog))
	{
		edit_item= GET_DIALOG_EDIT_ITEM(dialog);
		GetDItem(dialog, edit_item, &item_type, &item_handle, &item_rectangle);
		
		GetPort(&old_port);
		SetPort(dialog);
		GetMouse(&mouse);
		SetPort(old_port);

		if (PtInRect(mouse, &item_rectangle))
		{
			SetCursor(*(GetCursor(iBeamCursor)));
		}
		else
		{
			InitCursor(); /* excessive, perhaps */
		}
	}
	
	return;
}
