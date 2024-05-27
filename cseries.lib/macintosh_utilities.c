/*
SETTINGS.C
	Original: Saturday, April 21, 1990 12:00:53 PM (Prism)

Saturday, August 25, 1990 10:57:23 AM
	Tee-hee. 70% code loss, we now use resource manager.
Wednesday, December 19, 1990 9:15:38 AM
	Notice that it is pretty much a bad thing if we don't find the
	resource, because we don't create it (and somebody will be trying
	to use a NULL handle!).  This is now fixed.
	
ERRORS.C (minotaur)
Friday, April 6, 1990 9:06:12 PM

4/6/90    jgj  Added Fail() and OpenDebugFile() functions
4/7/90    jgj  Added FatalError() and NonFatalError() functions, Fail() is now
	AssertionFailure(), added SetFatalErrorProc() and CloseWithoutSaving().
4/19/90   jgj  Adapted for use with Concept/32 Editor
6/19/90   jgj  Adapted for use with Modem Surround
Tuesday, July 17, 1990 12:22:09 AM
	removed CloseWithoutSaving(), changed AssertionFailure().
Wednesday, July 18, 1990 8:16:37 AM
	about box is now a window with a PICT instead of a dialog resource.
Monday, December 24, 1990 8:42:51 PM
	the fatal-error procedure was a good idea, but we never use it (axe), all
	code reformatted to the 'new' style.

MACINTOSH_UTILITIES.C
	Original: Thursday, July 19, 1990 2:52:01 AM (Minotaur)

Monday, December 24, 1990 8:34:30 PM
	some time ago, general_filter_proc and modify_control were added.  my_draw_string,
	and my_draw_number were just removed.  read_settings was moved from settings.c and
	settings.c was deleted.  change history of settings.c is provided above.
Monday, December 24, 1990 8:50:12 PM
	added grab_window_corner and the open/close about_box functions.
Wednesday, December 26, 1990 11:17:33 PM
	we are now part of the Macintosh Core.
Friday, December 28, 1990 8:38:30 PM
	fixed bug in open_about_window which didn't work with PICTs based on a non-zero origin;
	the about_box also opens itself on initialization.
Tuesday, June 25, 1991 1:59:55 AM
	major overhauls to reflect changes to CSERIES.H and MACINTOSH_CSERIES.H and in an
	effort to make this module more self-sufficient.  _user_assertion_failure is not
	implemented yet.
Saturday, August 17, 1991 7:54:57 AM
	added new modify_radio_button_family() function.
Sunday, August 18, 1991 9:30:06 AM
	added choose_and_open_file().  remember that dump_pict_into_gworld isn’t totally
	operational yet (bit_depth is hardcoded at 32bpp because CollectColors blows big chunks).
Thursday, August 22, 1991 11:08:58 PM
	fixes to general_filter_proc; now calls update_any_window to respond to updates
	for non-dialog windows, break added, etc.
Saturday, September 7, 1991 3:14:52 PM
	dump_pict_into_gworld() now takes bit depth as parameter.  Added wait_for_mouse_to_move().
Sunday, September 8, 1991 1:53:45 PM
	general_filter_proc now supports DlgCut, DlgCopy and DlgPaste and respects dimmed OK
	and cancel buttons (11Dec91).
Wednesday, December 11, 1991 4:27:21 PM
	major changes; we didn't need all that shit from MicroMAP!
Thursday, December 19, 1991 1:03:33 PM
	long standing ‘implementation failure’ in modify_menu_item wiped hierarchical menu IDs;
	reduced code size of get_window_frame.
Thursday, August 20, 1992 5:25:57 PM
	added get_string_address for parsing STR# resources; fixed get_window_frame, it didn’t
	previously work (minotaur made this obvious, but we ignored it).
Saturday, August 22, 1992 12:55:50 PM
	removed ‘do_shutdown’ shit, we’ll now use atexit() functions; revived ‘alert_user’.
Tuesday, September 22, 1992 12:06:40 AM
	finally fixed assert handler to call MacsBug only if installed (checking MacJmp, 0x200).
Thursday, November 12, 1992 3:16:43 PM
	added ‘get_system_colors’, the two rgb globals for grays, and expanded our standard
	dialog filter to support picts at the top of dialogs and gray-framed user items.  it’s
	about time for this file to be split up.
Sunday, November 15, 1992 9:16:37 PM
	removed c2pstr and p2cstr in favor of MPW’s identical Strings.h functions.
Friday, November 20, 1992 1:58:07 PM
	bug fix to new general_filter_proc features and ScaleRect.
Monday, November 30, 1992 4:29:20 PM
	initialize_system_colors now creates a palette of sixteen colors, including the Apple menu
	colors, the system’s window colors, the highlight color and the five grays used in scrollbars,
	grow icons and window title bars.
Tuesday, February 16, 1993 1:19:08 PM
	removed two return TRUE’s in general_filter_proc (so our alert’s wouldn’t strangely disappear),
	and added support for hitting buttons by typing their first letter.
Wednesday, March 10, 1993 11:42:51 AM
	if a dialog contains any editText items, you must hold the command key to hit a button by
	typing its first letter.
Saturday, March 20, 1993 10:28:30 AM
	added myGetNewAlert and myAlert which handle automatic window positioning for dialogs and
	alerts under system six.  the dialog header is now a procedure pointer; a standard one is
	provided which paints PICT resource 256.
Friday, April 9, 1993 11:52:14 AM
	added support for Shift-TAB in dialogs, out of boredom.
Saturday, July 10, 1993 10:55:49 PM
	if iOK is dimmed or non-existant, RETURN and ENTER try iCANCEL, and vice versa.
Monday, August 30, 1993 2:57:36 PM
	hide/show_menu_bar work with multiple monitors, to an extent.
Wednesday, September 1, 1993 12:11:33 PM
	moved MostDevice and menu bar routines to DEVICES.C.
Friday, April 14, 1995 9:50:43 AM  (Jason')
	added screen saver voodoo.
*/

#include "macintosh_cseries.h"

#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#define AD20x
#include "AfterDarkGestalt.h"

#ifdef mpwc
#pragma segment modules
#endif

/* ---------- constants */

#define VISIBLE_DELAY 7

#define POPUP_ARROW_OFFSET 5
#define POPUP_ARROW_HEIGHT 6
#define POPUP_ARROW_WIDTH (2*(POPUP_ARROW_HEIGHT-1))

#define VERS_RESOURCE_ID   1

/* missing from Window.h, probably for a damn good reason :) */
enum
{
	wHighlightColor= 11,
	wLowlightColor= 12
};

/* ---------- resources */

#define alrtFATAL_ERROR 128
#define alrtNONFATAL_ERROR 129

/* ---------- structures */

/* ---------- globals */

/* for anybody */
char temporary[256];

/* for convenience */
RGBColor rgb_black= {0x0000, 0x0000, 0x0000};
RGBColor rgb_white= {0xFFFF, 0xFFFF, 0xFFFF};
RGBColor rgb_dark_gray= {0x4000, 0x4000, 0x4000};
RGBColor rgb_light_gray= {0xc000, 0xc000, 0xc000};

/* initialized when get_system_colors is called */
RGBColor system_colors[NUMBER_OF_SYSTEM_COLORS]=
{
	/* window highlight/lowlight colors (may be changed) */
	{0x3333, 0x3333, 0x6666},
	{0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0x0000},
	{0xCCCC, 0xCCCC, 0xFFFF},
	
	/* highlight color (will be changed) */
	{0xB000, 0xB000, 0xFFFF},
	
	/* grays */
	{0x2666, 0x2666, 0x2666}, /* 15% */
	{0x5555, 0x5555, 0x5555}, /* 33% */
	{0x7777, 0x7777, 0x7777}, /* 47% */
	{0x87AD, 0x87AD, 0x87AD}, /* 53% */
	{0xAAAA, 0xAAAA, 0xAAAA}, /* 67% */
	{0xBAE0, 0xBAE0, 0xBAE0}, /* 73% */
	{0xDDDD, 0xDDDD, 0xDDDD}, /* 87% */
	{0xEEEE, 0xEEEE, 0xEEEE}, /* 93% */
	
	/* active apple colors */
	{0, 48059, 0},
	{65535, 65535, 0},
	{65535, 26214, 0},
	{56797, 0, 0},
	{65535, 0, 39321},
	{0, 0, 56797},  /* was 0,0,56797, changed to 0,30146,52428 to fix apple, now back.  the old
						color was needed to make the apple stay in color, and is added as
						‘stupidColor1’ below */
	
	/* inactive apple colors */
	{0x6666, 0xCCCC, 0x6666},
	{0xFFFF, 0xFFFF, 0x6666},
	{0xFFFF, 0xCCCC, 0x6666},
	{0xFFFF, 0x6666, 0x6666},
	{0xFFFF, 0x6666, 0xCCCC},
	{0x6666, 0x6666, 0xFFFF}, /* changed green from 0xFFFF */
	
	{0,30146,52428}
};

#ifdef DEBUG
static boolean debug_status= TRUE;
#else
static boolean debug_status= FALSE;
#endif

static boolean debugger_installed= FALSE;

/* ---------- private prototypes */
static OSErr FSSpecAppendSuffix(FSSpec *source, FSSpec *destination, char *new_suffix);

/* ---------- code */

/* the worst possible thing that could happen here is for a later release of the system
	software to change how the default window colors are stored, which would cause this
	function to assume the default blue/purple colors.  this is only a problem because
	the system then won’t have the colors it wants to draw window frames and controls.
	but this itself is ok, because the system is smart about doing this and there might
	even be enough shades of gray to go around. */
void initialize_system_colors(
	void)
{
	CTabHandle default_wctb;

	default_wctb= (CTabHandle) GetResource('wctb', 0);
	if (default_wctb)
	{
		if ((*default_wctb)->ctSize>=12)
		{
			if ((*default_wctb)->ctTable[wHighlightColor].value==wHighlightColor&&
				(*default_wctb)->ctTable[wLowlightColor].value==wLowlightColor)
			{
				/* we have system seven or later with the right part codes! */
				system_colors[windowLowlight]= (*default_wctb)->ctTable[wLowlightColor].rgb;
				system_colors[windowHighlight]= (*default_wctb)->ctTable[wHighlightColor].rgb;
			}
		}
		
		/* we can’t dispose of this, because other people are using it */
	}
	
	/* interpolate to get the two shades between the highlight and lowlight */
	SetRGBColor(system_colors+window33Percent,
		(2*(long)system_colors[windowLowlight].red+(long)system_colors[windowHighlight].red)/3,
		(2*(long)system_colors[windowLowlight].green+(long)system_colors[windowHighlight].green)/3,
		(2*(long)system_colors[windowLowlight].blue+(long)system_colors[windowHighlight].blue)/3);
	SetRGBColor(system_colors+window66Percent,
		((long)system_colors[windowLowlight].red+2*(long)system_colors[windowHighlight].red)/3,
		((long)system_colors[windowLowlight].green+2*(long)system_colors[windowHighlight].green)/3,
		((long)system_colors[windowLowlight].blue+2*(long)system_colors[windowHighlight].blue)/3);

	/* get the highlight color */
	LMGetHiliteRGB(system_colors+highlightColor);

	return;
}

void hold_for_visible_delay(
	void)
{
	long start= TickCount();
	
	while(TickCount()<start+VISIBLE_DELAY);
}

void get_window_frame(
	WindowPtr window,
	Rect *frame)
{
	GrafPtr old_port;
	Point corner;
	
	GetPort(&old_port);
	SetPort(window);
	SetPt(&corner, 0, 0);
	LocalToGlobal(&corner);
	SetPort(old_port);
	
	*frame= window->portRect;
	OffsetRect(frame, corner.h-frame->left, corner.v-frame->top);

	return;
}

void modify_menu_item(
	short menu,
	short item,
	boolean status,
	short check)
{
	MenuHandle menu_handle;
	
	menu_handle= GetMHandle(menu);
	assert(menu_handle);

	if (menu_handle)
	{
		if (status)
		{
			EnableItem(menu_handle, item);
		}
		else
		{
			DisableItem(menu_handle, item);
		}
		
		if (check!=NONE)
		{
			CheckItem(menu_handle, item, check);
		}
	}
	
	return;
}

void _assertion_failure(
	char *information,
	char *file,
	int line,
	boolean fatal)
{
	Str255 buffer;

#ifdef DEBUG	
	if (debugger_installed)
	{
		psprintf((char *)buffer, "%s in %s,#%d: %s", fatal ? "halt" : "pause", file, line,
			information ? information : "<no reason given>");
		DebugStr(buffer);
	}
	else
#endif
	{
		Handle fkeyHandle= GetResource('FKEY', 3);
	
		/* load and call FKEY 3 to get a screen shot before we draw a dialog over everything */
	
		if (fkeyHandle)
		{
			LoadResource(fkeyHandle);
			if (*fkeyHandle)
			{
				HLock(fkeyHandle);
				CallFKEYProc((FKEYUPP) *fkeyHandle);
				HUnlock(fkeyHandle);
			}
		}

		psprintf((char *)buffer, "Sorry, a run-time assertion failed in “%s” at line #%d.  PLEASE WRITE THIS DOWN AND MAKE A BUG REPORT!", file, line);
		ParamText(buffer, "\p-1", (StringPtr)"", (StringPtr)"");
		Alert(fatal ? alrtFATAL_ERROR : alrtNONFATAL_ERROR, (ModalFilterUPP) NULL);
	}
	
	if (fatal)
	{
		exit(-1);
	}
	else
	{
		return;
	}
}

void alert_user(
	short type,
	short resource_number,
	short error_number,
	short identifier)
{
	char buffer[50];
	
	psprintf(buffer, "%d", identifier);
	ParamText((StringPtr)getpstr(temporary, resource_number, error_number), (StringPtr)buffer, (StringPtr)"", (StringPtr)"");
	
	switch(type)
	{
		case fatalError:
			Alert(alrtFATAL_ERROR, (ModalFilterUPP) NULL);
			ParamText((StringPtr)"", (StringPtr)"", (StringPtr)"", (StringPtr)"");
			exit(-1);
		
		case infoError:
			Alert(alrtNONFATAL_ERROR, (ModalFilterUPP) NULL);
			break;
		
		default:
			halt();
	}
	
	ParamText((StringPtr)"", (StringPtr)"", (StringPtr)"", (StringPtr)"");
	return;
}

boolean is_application_window(
	WindowPtr window)
{
	if (!window)
	{
		return FALSE;
	}

	if (((WindowPeek)window)->windowKind<0)
	{
		/* DA window */
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

long whats_on_top(
	void)
{
	WindowPtr window= FrontWindow(); /* FLOATS */
	
	if (window)
	{
		return GetWRefCon(window);
	}
	
	return NONE;
}

#ifdef OBSOLETE
OSErr choose_new_file(
	short *file_handle,
	short *reference_number,
	char *file_name,
	char *prompt,
	long creator,
	long file_type)
{
	Point location;
	SFReply reply;
	OSErr error;
	
	SetPt(&location, 40, 60);
	SFPutFile(location, prompt, file_name, (DlgHookProcPtr) NULL, &reply);
	if (reply.good)
	{
		error= Create(reply.fName, reply.vRefNum, creator, file_type);
		if (error==dupFNErr||error==noErr)
		{
			strncpy(file_name, reply.fName, *reply.fName+1);
			*reference_number= reply.vRefNum;
			
			error= FSOpen(reply.fName, reply.vRefNum, file_handle);
		}
	}
	else
	{
		error= userCanceledErr;
	}
	
	return error;
}

OSErr choose_and_open_file(
	short *file_handle,
	short *reference_number,
	char *file_name,
	long spec1,
	long spec2,
	long spec3,
	long spec4)
{
	Point location;
	SFTypeList filetypes;
	SFReply reply;
	OSErr error;
	short count;

	filetypes[0]= spec1, filetypes[1]= spec2, filetypes[2]= spec3, filetypes[3]= spec4;
	for (count=0;count<4;++count)
	{
		if (!filetypes[count])
		{
			break;
		}
	}
	
	SetPt(&location, 40, 60);
	SFGetFile(location, (char *) NULL, NULL, count, filetypes, NULL, &reply);

	if (reply.good)
	{
		strncpy(file_name, reply.fName, reply.fName[0]+1);
		*reference_number= reply.vRefNum;

		error= FSOpen(reply.fName, reply.vRefNum, file_handle);
	}
	else
	{
		error= userCanceledErr;
	}
	
	return error;
}
#endif

void mark_for_update(
	GrafPtr port,
	Rect *rectangle)
{
	GrafPtr old_port;
	
	GetPort(&old_port);
	SetPort(port);

	if (!rectangle) rectangle= &port->portRect;
	EraseRect(rectangle);
	InvalRect(rectangle);

	SetPort(old_port);
	
	return;
}

RGBColor *SetRGBColor(
	RGBColor *color,
	unsigned short red,
	unsigned short green,
	unsigned short blue)
{
	color->red= red;
	color->green= green;
	color->blue= blue;
	
	return color;
}

boolean wait_for_mouse_to_move(
	Point origin,
	short threshold)
{
	Point mouse;
	Rect bounds;
	
	SetRect(&bounds, -threshold, -threshold, threshold, threshold);
	OffsetRect(&bounds, origin.h, origin.v);
	
	do
	{
		GetMouse(&mouse);
		if (!(PtInRect(mouse, &bounds)))
		{
			return TRUE;
		}
	}
	while(Button());
	
	return FALSE;
}

#define FAKE_KEYBOARD_EVENT_PERIOD (60*30)
unsigned long last_fake_keyboard_event= 0;

void stay_awake(
	void)
{
	if (TickCount()>last_fake_keyboard_event+FAKE_KEYBOARD_EVENT_PERIOD)
	{
		last_fake_keyboard_event= TickCount();
		PostEvent(keyDown, 0);
	}
	
	return;
}

void *malloc(
	long size)
{
	return NewPtr(size);
}

void free(
	void *pointer)
{
	DisposePtr((Ptr)pointer);
}

// This is the most overloaded routine I've seen in a long time.
void *realloc(
	void *pointer,
	size_t size)
{
	if ((pointer==NULL) && (size!=0))
	{
		return NewPtr(size);
	}
	
	if (size==0)
	{
		if (pointer) DisposePtr((Ptr)pointer);
		return NULL;
	}
	
	if (size==GetPtrSize((Ptr)pointer)) return pointer;
	
	SetPtrSize((Ptr)pointer, size);
	
	// SetPtrSize can fail if the pointer couldn't be expanded in place
	if (MemError())
	{
		Size old_size= GetPtrSize((Ptr)pointer);
		Ptr	realloced_pointer= NewPtr(size);

		// so we make a whole new one if possible
		if (MemError()) return NULL;
		
		// and copy the data into it.
		BlockMoveData(pointer, realloced_pointer, old_size > size ? size : old_size);
		// and then destroy the old pointer		
		DisposePtr((Ptr)pointer);
		
		return realloced_pointer;
	}
	return pointer;
}

handle rmalloc(
	long size)
{
	return (handle) NewHandle(size);
}

void rfree(
	handle h)
{
	DisposeHandle((Handle)h);
}

void *rlock(
	handle h)
{
	HLock((Handle)h);
	
	return (void *) *((Handle)h);
}

void runlock(
	handle h)
{
	HUnlock((Handle)h);
	
	return;
}

void draw_popup_frame(
	Rect *bounds)
{
	short i;
	short y= bounds->top+(bounds->bottom-bounds->top-(2*POPUP_ARROW_HEIGHT)/3)/2;

	FrameRect(bounds);
	MoveTo(bounds->left+3, bounds->bottom);
	LineTo(bounds->right, bounds->bottom);
	LineTo(bounds->right, bounds->top+3);
	
	for (i=0;i<POPUP_ARROW_HEIGHT;++i)
	{
		MoveTo(bounds->right-POPUP_ARROW_WIDTH-POPUP_ARROW_OFFSET+i, y+i);
		LineTo(bounds->right-POPUP_ARROW_OFFSET-i, y+i);
	}

	return;
}

char *getpstr(
	char *buffer,
	short resource_number,
	short string_number)
{
	Handle strings= GetResource('STR#', resource_number);

	assert(strings);
	if (strings)
	{
		short count;
		byte *string_address;
		
		vassert(string_number>=0&&string_number<**((short**)strings),
			csprintf(temporary, "asked for #%d/#%d in 'STR#' ID#%d", string_number, **((short**)strings), resource_number));
		
		HLock(strings);
		count= 0;
		string_address= ((byte *)*strings)+2;
		while (count++<string_number)
		{
			string_address+= *string_address+1;
		}
		pstrcpy(buffer, (const char *)string_address);
		HUnlock(strings);
	}
	else
	{
		*buffer= '\0';
	}
	
	return buffer;
}

void pstrcat(
	unsigned char *str1,
	unsigned char *str2)
{
	memmove(str1 + *str1 + 1, str2+1, *str2);
	*str1+= *str2;
	
	return;
}

OSErr HOpenPath(
	short vRefNum,
	long dirID,
	ConstStr255Param fileName,
	char permission,
	short *refNum,
	short resource_number)
{
	OSErr error= HOpen(vRefNum, dirID, fileName, permission, refNum);
	short path_count= countstr(resource_number);
	Str255 new_filename;
	short i;
	
	for (i= 0; i<path_count && error!=noErr; ++i)
	{
		getpstr((char *)new_filename, resource_number, i);
		pstrcat(new_filename, (unsigned char *) fileName);
		
		error= HOpen(0, 0, new_filename, permission, refNum);
		if (error==noErr) pstrcpy((char *) fileName, (const char *)new_filename);
	}
	
	return error;
}

short HOpenResFilePath(
	short vRefNum,
	long dirID,
	ConstStr255Param fileName,
	char permission,
	short resource_number)
{
	short hndl= HOpenResFile(vRefNum, dirID, fileName, permission);
	short path_count= countstr(resource_number);
	Str255 new_filename;
	short i;
	
	for (i= 0; i<path_count && hndl==NONE; ++i)
	{
		getpstr((char *)new_filename, resource_number, i);
		pstrcat(new_filename, (unsigned char *) fileName);
		
		hndl= HOpenResFile(0, 0, new_filename, permission);
	}
	
	return hndl;
}

short countstr(
	short resource_number)
{
	Handle strings= GetResource('STR#', resource_number);
	
	return strings ? **((short**)strings) : 0;
}

char *getcstr(
	char *buffer,
	short resource_number,
	short string_number)
{
	getpstr(buffer, resource_number, string_number);
	p2cstr((unsigned char *)buffer);
	
	return buffer;
}

char *pstrcpy(
	char *destStr,
	const char *srcStr)
{
	BlockMove(srcStr, destStr, *((byte*)srcStr)+1);
	return destStr;
}

char *strupr(
	char *string)
{
	char *p;
	
	for (p=string;*p;p++)
	{
		*p= toupper(*p);
	}
	
	return string;
}

char *strlwr(
	char *string)
{
	char *p;
	
	for (p=string;*p;p++)
	{
		*p= tolower(*p);
	}
	
	return string;
}

unsigned long machine_tick_count(
	void)
{
	return TickCount();
}

/* calculate a new stdState rectangle based on the maximum bounds, positioned on the device which
	the given window overlaps most */
void build_zoom_rectangle(
	WindowPtr window,
	Rect *bounds)
{
	GDHandle device;
	WStateDataHandle window_state;
	Rect zoomed, unzoomed;
	Rect device_bounds;
	short bias;
	
	window_state= (WStateDataHandle) (((CWindowRecord *)window)->dataHandle);
	zoomed= (*window_state)->stdState;
	unzoomed= (*window_state)->userState;

	/* official Apple source sends the title bar along with this rectangle, but that isn’t useful */
	device= MostDevice(&unzoomed);
	if (!device) device= GetMainDevice();
	
	bias= unzoomed.top-(*((WindowPeek)window)->strucRgn)->rgnBBox.top-1;
	if (device==GetMainDevice()) bias+= GetMBarHeight();
	device_bounds= (*device)->gdRect;
	InsetRect(&device_bounds, 3, 3);
	device_bounds.top+= bias;
	
	zoomed= *bounds;
	OffsetRect(&zoomed, unzoomed.left-zoomed.left, unzoomed.top-zoomed.top);
	if (RECTANGLE_HEIGHT(&zoomed)>RECTANGLE_HEIGHT(&device_bounds))
	{
		zoomed.top= device_bounds.top;
		zoomed.bottom= device_bounds.bottom;
	}
	else
	{
		if (zoomed.bottom>device_bounds.bottom) OffsetRect(&zoomed, 0, device_bounds.bottom-zoomed.bottom);
		if (zoomed.top<device_bounds.top) OffsetRect(&zoomed, 0, device_bounds.top-zoomed.top);
	}
	if (RECTANGLE_WIDTH(&zoomed)>RECTANGLE_WIDTH(&device_bounds))
	{
		zoomed.left= device_bounds.left;
		zoomed.right= device_bounds.right;
	}
	else
	{
		if (zoomed.right>device_bounds.right) OffsetRect(&zoomed, device_bounds.right-zoomed.right, 0);
		if (zoomed.left<device_bounds.left) OffsetRect(&zoomed, device_bounds.left-zoomed.left, 0);
	}
	
	(*window_state)->stdState= zoomed;
	
	return;
}

#define LONG_ONE 0x10000L

/* scale source to fit within bounds with correct aspect ratio and stuff that into destination
	(centered, of course) */
void ScaleRect(
	Rect *bounds,
	Rect *source,
	Rect *destination)
{
	long horizontal_scale, vertical_scale;
	Rect new_rectangle;
	short width, height;
	
	/* the largest value will be the constraint (or, the axis we can peg) */
	horizontal_scale= (RECTANGLE_WIDTH(source)*LONG_ONE)/RECTANGLE_WIDTH(bounds);
	vertical_scale= (RECTANGLE_HEIGHT(source)*LONG_ONE)/RECTANGLE_HEIGHT(bounds);
	
	if (horizontal_scale>vertical_scale)
	{
		width= RECTANGLE_WIDTH(bounds);
		height= (width*RECTANGLE_HEIGHT(source))/RECTANGLE_WIDTH(source);
	}
	else
	{
		height= RECTANGLE_HEIGHT(bounds);
		width= (height*RECTANGLE_WIDTH(source))/RECTANGLE_HEIGHT(source);
	}
	
	SetRect(&new_rectangle, 0, 0, width, height);
	OffsetRect(&new_rectangle, bounds->left + (RECTANGLE_WIDTH(bounds)-width)/2,
		bounds->top + (RECTANGLE_HEIGHT(bounds)-height)/2);
	*destination= new_rectangle;
	
	return;
}

/* centers source within bounds */
void AdjustRect(
	Rect *bounds,
	Rect *source,
	Rect *destination,
	short mode)
{
	Rect result;
	
	result= *source;
	switch (mode)
	{
		case centerRect:
			OffsetRect(&result, bounds->left+(RECTANGLE_WIDTH(bounds)-RECTANGLE_WIDTH(source))/2-source->left,
				bounds->top+(RECTANGLE_HEIGHT(bounds)-RECTANGLE_HEIGHT(source))/2-source->top);
			break;
		case alertRect:
			OffsetRect(&result, bounds->left+(RECTANGLE_WIDTH(bounds)-RECTANGLE_WIDTH(source))/2-source->left,
				bounds->top+(RECTANGLE_HEIGHT(bounds)-RECTANGLE_HEIGHT(source))/3-source->top);
			break;
		
		default:
			halt();
	}
	
	*destination= result;
	
	return;
}

#ifdef env68k
/* prsprintf and rsprintf must be carefully tested with PPC before being used */
int prsprintf(
	char *buffer,
	short resource_number, /* STR# resource */
	short string_number,
	...)
{
	char format[256];
	va_list arglist;
	int return_value;
	short length;
	
	getcstr(format, resource_number, string_number);
	
	va_start(arglist, string_number);
	return_value= vsprintf(buffer+1, format, arglist);
	va_end(arglist);
	
	*buffer= ((length= strlen(buffer+1))>255) ? 255 : length;
	
	return return_value;
}

int rsprintf(
	char *buffer,
	short resource_number, /* STR# resource */
	short string_number,
	...)
{
	char format[256];
	va_list arglist;
	int return_value;

	getcstr(format, resource_number, string_number);
	
	va_start(arglist, string_number);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	return return_value;
}
#endif

boolean toggle_debug_status(
	void)
{
	debug_status= !debug_status;
	return debug_status;
}

void initialize_debugger(
	boolean force_debugger_on)
{
#ifdef DEBUG

	/* make sure the standard c library is loaded */
	sprintf(temporary, "Segment Loader— you’re fired!");
	
	debugger_installed= 
#ifdef env68k
	/* 0x120 is non-zero if macsbug is installed on 68k machines */
	(*((Ptr*)0x120) && GetResource('dbug', 128))
#else
	(GetResource('dbug', 128))
#endif
	? TRUE : force_debugger_on;

#endif

	return;
}

int dprintf(
	const char *format,
	...)
{
	char buffer[257]; /* [length byte] + [255 string bytes] + [null] */
	va_list arglist;
	int return_value;
	
	if (debug_status)
	{
		va_start(arglist, format);
		return_value= vsprintf(buffer+1, format, arglist);
		va_end(arglist);
		
		*buffer= strlen(buffer+1);
#ifdef DEBUG
		if (debugger_installed)
		{
			DebugStr((StringPtr)buffer);
		}
		else
#endif
		{
			ParamText((StringPtr)buffer, (StringPtr)"\p?", (StringPtr)"", (StringPtr)"");
			Alert(alrtNONFATAL_ERROR, (ModalFilterUPP) NULL);
			ParamText((StringPtr)"", (StringPtr)"", (StringPtr)"", (StringPtr)"");
		}
	}
	else
	{
		return_value= 0;
	}
	
	return return_value;
}

int psprintf(
	char *buffer,
	const char *format,
	...)
{
	va_list arglist;
	int return_value;
	short length;
	
	va_start(arglist, format);
	return_value= vsprintf(buffer+1, format, arglist);
	va_end(arglist);
	
	*buffer= ((length= strlen(buffer+1))>255) ? 255 : length;
	
	return return_value;
}

char *csprintf(
	char *buffer,
	char *format,
	...)
{
	va_list arglist;
	
	va_start(arglist, format);
	vsprintf(buffer, format, arglist);
	va_end(arglist);
	
	return buffer;
}

OSErr FSFlush(
	short hndl,
	short VRefNum)
{
	ParamBlockRec block;
	OSErr error;
	
	memset(&block, 0, sizeof(ParamBlockRec));
	block.fileParam.ioCompletion= (IOCompletionUPP) NULL;
	block.fileParam.ioFRefNum= hndl;
	block.fileParam.ioFVersNum= 0;
	
	error= PBFlushFile(&block, FALSE);
	if (error==noErr)
	{
		block.fileParam.ioNamePtr= (StringPtr) NULL;
		block.fileParam.ioVRefNum= VRefNum;
		error= PBFlushVol(&block, FALSE);
	}
	
	return error;
}

void GetNewTextSpec(
	TextSpecPtr font_info,
	short resource_number, 
	short font_index)
{
	Handle resource;
	
	resource= GetResource('finf', resource_number);
	assert(resource);
	if (resource)
	{
		/* First short is the count */
		assert(font_index>=0 && font_index<*((short *) *resource));
		
		/* Get to the right one.. */
		*font_info= *((TextSpecPtr)(*resource+sizeof(short))+font_index);
	}
	else
	{
		memset(font_info, 0, sizeof(TextSpecPtr));
	}
	
	return;
}

void SetFont(
	TextSpecPtr font_info)
{
	TextFont(font_info->font);
	TextFace(font_info->face);
	TextSize(font_info->size);
	
	return;
}

void GetFont(
	TextSpecPtr font_info)
{
	GrafPtr port;
	
	GetPort(&port);
	font_info->font= port->txFont;
	font_info->face= port->txFace;
	font_info->size= port->txSize;
	
	return;
}

short get_our_country_code(
	void)
{
	short       country_code;
	Handle      vers_rsrc;
	
	country_code = 0; // this is the U.S. code. an ok default.
	vers_rsrc = GetResource('vers', VERS_RESOURCE_ID);
	if (vers_rsrc)
	{
		VersRec  *vers;
		
		vers = (VersRec *) *vers_rsrc;
		country_code = vers->countryCode;
	}
	
	return country_code;
}

short wait_for_click_or_keypress(
	long maximum_delay)
{
	short keycode;
	EventRecord event;
	long first_tick= TickCount();

	FlushEvents(keyDownMask|keyUpMask|autoKeyMask|mDownMask|mUpMask, 0);

	do
	{
		global_idle_proc();
	}
	while (!GetOSEvent(keyDownMask|autoKeyMask|mDownMask, &event) &&
		(maximum_delay<0 || (TickCount()-first_tick)<maximum_delay));

	switch (event.what)
	{
		case nullEvent:
		case mouseDown:
			keycode= NONE;
			break;
		
		case keyDown:
		case autoKey:
			keycode= (event.message & 0x0000ffff) >> 8;
			break;
		
		default:
			halt();
	}

	while (GetOSEvent(keyDownMask|autoKeyMask|mDownMask, &event));

	return keycode;
}

/* ---------- screen saver voodoo */

enum
{
	uppScreenSaverControlProcInfo= kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSErr)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))
};

static SaverControlProcPtr control_proc;
static UniversalProcPtr control_proc_upp;

void kill_screen_saver(
	void)
{
	long attributes;
	OSErr error;
	
	error= Gestalt(gestaltScreenSaverAttr, &attributes);
	if (error==noErr && (attributes&FLAG(gestaltSaverTurnedOn)))
	{
		error= Gestalt(gestaltScreenSaverControl, (long *) &control_proc);
		if (error==noErr)
		{
#ifdef env68k
			error= control_proc(eSaverOff);
#else	
			control_proc_upp= (UniversalProcPtr) NewRoutineDescriptor((ProcPtr)control_proc,
				uppScreenSaverControlProcInfo, kM68kISA);

			if (control_proc_upp)
			{
				error= CallUniversalProc(control_proc_upp, uppScreenSaverControlProcInfo, eSaverOff);
			}
			else
			{
				error= MemError();
			}
#endif
			if (error==noErr)
			{
				atexit(restore_screen_saver);
			}
		}
	}
	
	return;
}

void restore_screen_saver(
	void)
{
	OSErr error;
	
#ifdef env68k
	error= control_proc(eSaverOn);
#else	
	error= CallUniversalProc(control_proc_upp, uppScreenSaverControlProcInfo, eSaverOn);
#endif
	
	return;
}

#include <processes.h>
#include <aliases.h>

static OSErr FSSpecAppendSuffix(
	FSSpec *source,
	FSSpec *destination,
	char *new_suffix)
{
	Str255 name;
	OSErr error;
	short i;
	
	pstrcpy((char *)name, (const char *)source->name);
	p2cstr(name);
	for (i= strlen((const char *)name)-1; i; --i)
	{
		if (name[i]=='.') name[i]= 0;
		if (name[i]==':') break;
	}
	strcat((char *)name, new_suffix);
	c2pstr((char *)name);
	error= FSMakeFSSpec(0, source->parID, name, destination);

	return error;
}

/* Get a fsspec from a given file, searching the given path... */
OSErr get_file_spec(
	FSSpec *spec,
	short string_resource_id,
	short file_name_index,
	short path_resource_id)
{
	OSErr err;
	FSSpec app_spec;

	/* Look for the files in the same directory that we are in.. */	
	err= get_my_fsspec(&app_spec);
	if(!err)
	{
		Boolean is_folder, was_alias;
		Str255 fileName, new_filepath;
		short ii;

		/* Get their name.. */
		getpstr((char *)fileName, string_resource_id, file_name_index);
		
		/* Check the first path.. */
		memcpy(new_filepath, fileName, *fileName+1);
		err= FSMakeFSSpec(app_spec.vRefNum, app_spec.parID, new_filepath, spec);

		/* Didn't get the file- search for the paths... */		
		if(err==fnfErr)
		{
			short path_count= countstr(path_resource_id);

			for(ii= 0; ii<path_count; ++ii)
			{
				getpstr((char *)new_filepath, path_resource_id, ii);
				pstrcat(new_filepath, fileName);
				
				/* Hmm... check FSMakeFSSpec... */
				/* Relative pathname.. */
				err= FSMakeFSSpec(app_spec.vRefNum, app_spec.parID, new_filepath, spec);
				
				/* IF we found one... */
				if(!err) break;
			}
		}

		/* Try to resolve it (for aliases) */
		if(!err)
		{
			err= ResolveAliasFile(spec, true, &is_folder, &was_alias);
		}
	}
	
	return err;
}

/* Return the process spec of the current process */
OSErr get_my_fsspec(
	FSSpec *file)
{
	ProcessSerialNumber psn;
	ProcessInfoRec info;
	char name[64];

	/* Set our process serial number */
	psn.highLongOfPSN= 0;
	psn.lowLongOfPSN= kCurrentProcess;

	/* Setup the parameter block */
	memset(&info, 0, sizeof(ProcessInfoRec));
	info.processInfoLength= sizeof(ProcessInfoRec);
	info.processName= (StringPtr)name;
	info.processAppSpec= file;

	return (GetProcessInformation(&psn, &info));
}
