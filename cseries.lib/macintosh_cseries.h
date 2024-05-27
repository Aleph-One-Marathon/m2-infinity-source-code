#ifndef __MACINTOSH_CSERIES_H
#define __MACINTOSH_CSERIES_H

/*
MACINTOSH_CSERIES.H
Wednesday, December 26, 1990 9:02:07 PM

this header file loads CSERIES.H and then does a #pragma load on MACINTOSH_INTERFACES.D.
every macintosh-specific file should use this header file instead of CSERIES.H or any
specific macintosh library.

Tuesday, June 25, 1991 1:57:43 AM
	changes to reflect things we now consider to be a bad idea, such as the old window
	manager we tried to use for Discord.  MACINTOSH_INTERFACES.D is now loaded in a very
	non-standard way; this should be fixed as soon as we learn how to rip the values of
	environment variables out of the shell (ie., {Libraries}).
Thursday, July 11, 1991 1:18:41 PM
	in fact, it seems to be impossible to get {Libraries} nicely, as we keep generating
	‘String Range Error Check Abort’ messages.  kinda makes you want to say, ‘Hmmmmmm.’
Sunday, August 4, 1991 4:45:59 PM
	IMAGE_WINDOW_REF is no longer defined; we may want to have several different types of
	image-displaying windows with different reference numbers.  we have ditched the floating
	window WDEF too, and just use the standard document window.
Friday, August 16, 1991 4:51:47 PM
	UPPER_LEFT_CORNER, LOWER_RIGHT_CORNER added.  modifications for magnification.
Wednesday, August 28, 1991 7:36:31 PM
	preprocessor conditional mc6881 used during #pragma load directive.
Saturday, December 12, 1992 9:50:43 AM
	added kRETURN, kENTER, stuff for system_colors.
Saturday, March 20, 1993 8:34:14 PM
	there is absolutely no reason an editor should support a ‘delete everything from the
	cursor position to the EOF’.  glad i had a backup ...
*/

#define mac

#ifdef __MWERKS__
	#ifndef POWERPLANT
		#ifdef powerc
			#include "macintosh_interfaces.ppc"
		#else
			#include "macintosh_interfaces.68k"
		#endif
	#else
		#ifdef powerc
			#include "PP_interfaces.ppc"
		#else
			#include "PP_interfaces.68k"
		#endif
	#endif
#else
	#ifdef powerc
		#include "macintosh_interfaces.c" /* gak */
	#else
		#ifndef mc68881
			#pragma load "macintosh_interfaces.d"
		#else
			#pragma load "macintosh_interfaces881.d"
		#endif
	#endif
#endif

#include "cseries.h"

/* ---------- constants */

#define OSEvt app4Evt
#define MouseMovedMessage 0xfa
#define SuspendResumeMessage 1
#define ResumeMask 1

#define asyncUncompleted 1

#define MOUSE_MOVED_THRESHOLD 2

#define CONTROL_ACTIVE 0
#define CONTROL_HIGHLIGHTED 1
#define CONTROL_INACTIVE 255

#define CONTROL_VISIBLE 255
#define CONTROL_INVISIBLE 0

#define SCROLLBAR_WIDTH 16

#define iOK 1
#define iCANCEL 2

#define MACINTOSH_TICKS_PER_SECOND 60

#define NO_PROCEDURE ((void (*)(void)) NULL)

#define kUP_ARROW 0x1e
#define kLEFT_ARROW 0x1c
#define kRIGHT_ARROW 0x1d
#define kDOWN_ARROW 0x1f

#define kENTER 0x3
#define kRETURN 0xd
#define kTAB 0x9
#define kESCAPE 0x1b
#define kDELETE 0x08

#define kPAGE_UP 0xb
#define kPAGE_DOWN 0xc
#define kHOME 0x1
#define kEND 0x4

// keycodes for the function keys
#define kcF1 0x7a
#define kcF2 0x78
#define kcF3 0x63
#define kcF4 0x76
#define kcF5 0x60
#define kcF6 0x61
#define kcF7 0x62
#define kcF8 0x64
#define kcF9 0x65
#define kcF10 0x6d
#define kcF11 0x67
#define kcF12 0x6f

#define DIALOG_INSET 4

/* indexes into the system_colors array */
enum
{
	windowLowlight= 0,
	window33Percent,
	window66Percent,
	windowHighlight,
	highlightColor,
	gray15Percent,
	gray33Percent,
	gray47Percent,
	gray53Percent,
	gray67Percent,
	gray73Percent,
	gray87Percent,
	gray93Percent,
	activeAppleGreen,
	activeAppleYellow,
	activeAppleOrange,
	activeAppleRed,
	activeApplePurple,
	activeAppleBlue,
	inactiveAppleGreen,
	inactiveAppleYellow,
	inactiveAppleOrange,
	inactiveAppleRed,
	inactiveApplePurple,
	inactiveAppleBlue,
	
	stupidColor1,
	NUMBER_OF_SYSTEM_COLORS
};

enum /* modes for AdjustRect */
{
	centerRect,
	alertRect
};

/* --------- types */

typedef void (*dialog_header_proc_ptr)(DialogPtr dialog, Rect *frame);
typedef void (*update_any_window_proc_ptr)(WindowPtr window);
typedef void (*suspend_resume_proc_ptr)(boolean resume);

/* --------- structures */

struct TextSpec /* 'finf' resource */
{
	short font;
	short face;
	short size;
};
typedef struct TextSpec TextSpec, *TextSpecPtr;

/* ---------- globals */

extern RGBColor rgb_black;
extern RGBColor rgb_dark_gray;
extern RGBColor rgb_light_gray;
extern RGBColor rgb_white;

extern RGBColor system_colors[];

/* ---------- macros */

#define RECTANGLE_WIDTH(r) ((r)->right-(r)->left)
#define RECTANGLE_HEIGHT(r) ((r)->bottom-(r)->top)

#define HIGH_WORD(n) (((n)>>16)&0xffff)
#define LOW_WORD(n) ((n)&0xffff)

#define UPPER_LEFT_CORNER(r) ((Point *)r)
#define LOWER_RIGHT_CORNER(r) (((Point *)r)+1)

#define COMPACT_ARRAY(array, element, nmemb, size) if ((element)<(nmemb)-1) \
	BlockMove(((byte*)(array))+((element)+1)*(size), ((byte*)(array))+(element)*(size), ((nmemb)-(element)-1)*(size))

#define GET_DIALOG_ITEM_COUNT(dialog) (*((short*)*((DialogPeek)(dialog))->items)+1)
#define GET_DIALOG_EDIT_ITEM(dialog) (((DialogPeek)(dialog))->editField+1)

#define IS_KEYPAD(virtual) ((virtual)>=0x52&&(virtual)<=0x5c)

#ifdef env68k
#pragma parameter __D0 get_a5
long get_a5(void)= {0x200d};
#pragma parameter __D0 set_a5(__D1)
long set_a5(long a5)= {0x200d, 0x2a41};
#pragma parameter __D0 get_a0
long get_a0(void)= {0x2008};
#pragma parameter __D0 get_a1
long get_a1(void)= {0x2009};
#endif

#define AbsRandom() (Random()&0x7fff)

/* ---------- external calls (yuck) and their callers */

/* called by general_filter_proc() */
void update_any_window(WindowPtr window, EventRecord *event);
void activate_any_window(WindowPtr window, EventRecord *event, boolean activate);
void global_idle_proc(void);

/* ---------- prototypes: MACINTOSH_UTILITIES.C */
	
long whats_on_top(void);
boolean is_application_window(WindowPtr window);

void modify_menu_item(short menu, short item, boolean status, short check);

void get_window_frame(WindowPtr window, Rect *frame);
void mark_for_update(GrafPtr port, Rect *rectangle);
void build_zoom_rectangle(WindowPtr window, Rect *bounds);

OSErr choose_new_file(short *file_handle, short *reference_number, char *file_name,
	char *prompt, long creator, long file_type);
OSErr choose_and_open_file(short *file_handle, short *reference_number,
	char *file_name, long spec1, long spec2, long spec3, long spec4);

void draw_popup_frame(Rect *bounds);

RGBColor *SetRGBColor(RGBColor *color, word red, word green, word blue);

boolean wait_for_mouse_to_move(Point origin, short threshold);
short wait_for_click_or_keypress(long maximum_delay);

void hold_for_visible_delay(void);
void stay_awake(void);

/* getcstr is in cseries.h */
short countstr(short resource_number);
char *getpstr(char *buffer, short collection_number, short string_number);

void AdjustRect(Rect *bounds, Rect *source, Rect *destination, short mode);
void ScaleRect(Rect *bounds, Rect *source, Rect *destination);

void initialize_system_colors(void);

char *pstrcpy(char *destStr, const char *srcStr);
void pstrcat(unsigned char *str1, unsigned char *str2);

/* rsprintf and dprintf are in cseries.h */
int prsprintf(char *s, short resource_number, short string_number, ...);
int psprintf(char *s, const char *format, ...);

OSErr FSFlush(short hndl, short VRefNum);

void GetNewTextSpec(TextSpecPtr font_info, short resource_number, short font_index);
void SetFont(TextSpecPtr font_info);
void GetFont(TextSpecPtr font_info);

short get_our_country_code(void);

short HOpenResFilePath(short vRefNum, long dirID, ConstStr255Param fileName,
	char permission, short resource_number);
short HOpenPath(short vRefNum, long dirID, ConstStr255Param fileName, char permission,
	short *refNum, short resource_number);

void kill_screen_saver(void);
void restore_screen_saver(void);

OSErr get_file_spec(FSSpec *spec, short string_resource_id, short file_name_index, short path_resource_id);
OSErr get_my_fsspec(FSSpec *file);

/* ---------- prototypes: DEVICES.C */

#define deviceIsGrayscale 0x0000
#define deviceIsColor 0x0001

struct GDSpec
{
	short slot;
	
	word flags; /* deviceIsGrayscale || deviceIsColor */
	short bit_depth; /* bits per pixel */
	
	short width, height;
};
typedef struct GDSpec GDSpec, *GDSpecPtr;

GDHandle MostDevice(Rect *bounds);
GDHandle BestDevice(GDSpecPtr device_spec);

void BuildExplicitGDSpec(GDSpecPtr device_spec, GDHandle device, word flags,
	short bit_depth, short width, short height);
void BuildGDSpec(GDSpecPtr device_spec, GDHandle device);
GDHandle MatchGDSpec(GDSpecPtr device_spec);
Boolean EqualGDSpec(GDSpecPtr device_spec1, GDSpecPtr device_spec2);

boolean HasDepthGDSpec(GDSpecPtr device_spec);
void SetDepthGDSpec(GDSpecPtr device_spec);

boolean machine_has_display_manager(void);
void SetResolutionGDSpec(GDSpecPtr device_spec, VDSwitchInfoPtr switchInfo);

short GetSlotFromGDevice(GDHandle device);
OSErr GetNameFromGDevice(GDHandle device, char *name);

void HideMenuBar(GDHandle device);
void ShowMenuBar(void);

void LowLevelSetEntries(short start, short count, CSpecArray aTable);

CTabHandle build_macintosh_color_table(struct color_table *color_table);
struct color_table *build_color_table(struct color_table *color_table, CTabHandle macintosh_color_table);

/* ---------- prototypes/DEVICE_DIALOG.C */

GDHandle display_device_dialog(GDSpecPtr device_spec);

/* ---------- prototypes: DIALOGS.C */

short myAlert(short alertID, ModalFilterUPP filterProc);
DialogPtr myGetNewDialog(short dialogID, void *dStorage, WindowPtr behind, long refCon);

void set_dialog_header_proc(dialog_header_proc_ptr proc);
void set_update_any_window_proc(update_any_window_proc_ptr proc);
void set_suspend_resume_proc(suspend_resume_proc_ptr proc);

void set_dialog_cursor_tracking(boolean status);

ModalFilterUPP get_general_filter_upp(void);
pascal Boolean general_filter_proc(DialogPtr dialog, EventRecord *event, short *itemhit);

void modify_control(DialogPtr dialog, short control, short status, short value);
void modify_radio_button_family(DialogPtr dialog, short start, short end, short current);

boolean hit_dialog_button(DialogPtr dialog, short item);
void standard_dialog_header_proc(DialogPtr dialog, Rect *frame);

Boolean CmdPeriodEvent(EventRecord *anEvent);

long extract_number_from_text_item(DialogPtr dialog, short item_number);
float extract_float_from_text_item(DialogPtr dialog, short item_num);
void insert_number_into_text_item(DialogPtr dialog, short item_number, long new_value);
void insert_float_into_text_item(DialogPtr dialog, short item_num, float new_value);

MenuHandle get_popup_menu_handle(DialogPtr dialog, short item);
#endif
