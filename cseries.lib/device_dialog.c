/*
DEVICE_DIALOG.C
Sunday, December 4, 1994 4:50:19 PM  (Jason')
*/

#include "macintosh_cseries.h"

/* ---------- constants */

#define dlogDEVICE 20000
	#define iCOLORS 3
	#define iGRAYS 4
	#define iDEVICE_AREA 5
	#define iDEVICE_NAME 9

#define DEVICE_AREA_MENU_BAR_HEIGHT 6
#define DEVICE_AREA_SCALE 16

/* ---------- structures */

struct device_dialog_global_data
{
	GDSpec device_spec;
	
	GDHandle device;
};

/* ---------- globals */

static struct device_dialog_global_data device_dialog_globals;

/* ---------- private code */

static device_dialog_instantiate_proc(DialogPtr dialog);
static pascal Boolean device_dialog_filter_proc(DialogPtr dialog, EventRecord *event, short *item_hit);

static void draw_device_area(GDSpecPtr device_spec, GDHandle selected_device, Rect *frame);
static GDHandle click_in_device_area(GDSpecPtr device_spec, Rect *frame, Point mouse);

static void get_device_area_offset(Rect *frame, Point *offset);
static void get_device_area_frame(GDHandle device, Rect *frame, Point offset);

/* ---------- code */

GDHandle display_device_dialog(
	GDSpecPtr device_spec)
{
	GDHandle device= BestDevice(device_spec);
	
	if (device)
	{
		DialogPtr dialog= myGetNewDialog(dlogDEVICE, NULL, (WindowPtr) -1, 0);
		ModalFilterUPP device_dialog_filter_upp= NewModalFilterProc(device_dialog_filter_proc);
		short item_hit;
		
		assert(dialog);
		assert(device_dialog_filter_upp);
		
		/* setup globals */
		device_dialog_globals.device_spec= *device_spec;
		device_dialog_globals.device= device;
		
		/* setup and show dialog */
		device_dialog_instantiate_proc(dialog);
		ShowWindow(dialog);
	
		do
		{
			boolean reinstantiate= FALSE;
			
			ModalDialog(device_dialog_filter_upp, &item_hit);
			switch(item_hit)
			{
				case iCOLORS: device_dialog_globals.device_spec.flags|= deviceIsColor; reinstantiate= TRUE; break;
				case iGRAYS: device_dialog_globals.device_spec.flags&= ~deviceIsColor; reinstantiate= TRUE; break;
				
				case iDEVICE_AREA: reinstantiate= TRUE; break;
				
				case iOK:
					*device_spec= device_dialog_globals.device_spec;
					device= device_dialog_globals.device;
					break;
			}
			
			if (reinstantiate) device_dialog_instantiate_proc(dialog);
		}
		while (item_hit!=iOK && item_hit!=iCANCEL);
		
		DisposeDialog(dialog);
		DisposeRoutineDescriptor(device_dialog_filter_upp);
	}

	return device;
}

/* ---------- private code */

static device_dialog_instantiate_proc(
	DialogPtr dialog)
{
	short item_type;
	Rect item_rectangle;
	Handle item_handle;
	OSErr error;
	
	modify_radio_button_family(dialog, iCOLORS, iGRAYS, (device_dialog_globals.device_spec.flags&deviceIsColor) ? iCOLORS : iGRAYS);

	error= GetNameFromGDevice(device_dialog_globals.device, temporary);
	if (error!=noErr) temporary[0]= 0;
	GetDItem(dialog, iDEVICE_NAME, &item_type, &item_handle, &item_rectangle);
	SetIText(item_handle, (const unsigned char *)temporary);
	
	return;
}

static pascal Boolean device_dialog_filter_proc(
	DialogPtr dialog,
	EventRecord *event,
	short *item_hit)
{
	GrafPtr old_port;
	short item_type;
	Handle item_handle;
	Rect item_rectangle;
	boolean handled= FALSE;

	GetPort(&old_port);
	SetPort(dialog);
	
	GetDItem(dialog, iDEVICE_AREA, &item_type, &item_handle, &item_rectangle);

	/* preprocess events */	
	switch(event->what)
	{
		case mouseDown:
		{
			Point where= event->where;
			
			GlobalToLocal(&where);
			switch (FindDItem(dialog, where)+1)
			{
				case iDEVICE_AREA:
				{
					GDHandle new_device= click_in_device_area(&device_dialog_globals.device_spec, &item_rectangle, where);
					
					if (new_device && new_device!=device_dialog_globals.device)
					{
						device_dialog_globals.device= new_device;
						BuildExplicitGDSpec(&device_dialog_globals.device_spec, new_device, device_dialog_globals.device_spec.flags, device_dialog_globals.device_spec.bit_depth, 0, 0);
						draw_device_area(&device_dialog_globals.device_spec, device_dialog_globals.device, &item_rectangle);
						
						*item_hit= iDEVICE_AREA;
					}
					handled= TRUE;
					
					break;
				}
			}
			break;
		}
		
		case updateEvt:
			if ((DialogPtr)event->message==dialog)
			{
				draw_device_area(&device_dialog_globals.device_spec, device_dialog_globals.device, &item_rectangle);
			}
			break;
	}

	SetPort(old_port);

	return handled ? TRUE : general_filter_proc(dialog, event, item_hit);
}

static void draw_device_area(
	GDSpecPtr device_spec,
	GDHandle selected_device,
	Rect *frame)
{
	Point offset;
	GDHandle device;

	get_device_area_offset(frame, &offset);
	
	EraseRect(frame);
	FrameRect(frame);
	
	for (device= GetDeviceList(); device; device= GetNextDevice(device))
	{
		if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive))
		{
			GDSpec spec;
			Rect bounds;
			
			BuildExplicitGDSpec(&spec, device, device_spec->flags, device_spec->bit_depth, 0, 0);
			
			get_device_area_frame(device, &bounds, offset);
			
			RGBForeColor(HasDepthGDSpec(&spec) ? &rgb_dark_gray : &rgb_white);
			PaintRect(&bounds);
			RGBForeColor(&rgb_black);
			
			if (device==selected_device) PenSize(2, 2);
			FrameRect(&bounds);
			PenSize(1, 1);
			
			if (device==GetMainDevice())
			{
				bounds.bottom= bounds.top + DEVICE_AREA_MENU_BAR_HEIGHT;
				EraseRect(&bounds);
				FrameRect(&bounds);
			}
		}
	}
	
	return;
}

static GDHandle click_in_device_area(
	GDSpecPtr device_spec,
	Rect *frame,
	Point mouse)
{
	Point offset;
	GDHandle device, intersected_device= (GDHandle) NULL;

	get_device_area_offset(frame, &offset);
	
	for (device= GetDeviceList(); device; device= GetNextDevice(device))
	{
		if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive))
		{
			GDSpec spec;
			Rect bounds;
			
			BuildExplicitGDSpec(&spec, device, device_spec->flags, device_spec->bit_depth, 0, 0);
			if (HasDepthGDSpec(&spec))
			{
				get_device_area_frame(device, &bounds, offset);
				if (PtInRect(mouse, &bounds))
				{
					intersected_device= device;
					break;
				}
			}
		}
	}
	
	return intersected_device;
}

static void get_device_area_offset(
	Rect *frame,
	Point *offset)
{
	GDHandle device= GetMainDevice();
	
	SetPt(offset, (*device)->gdRect.left + RECTANGLE_WIDTH(&(*device)->gdRect)/2,
		(*device)->gdRect.top + RECTANGLE_HEIGHT(&(*device)->gdRect)/2);
	SetPt(offset, frame->left + RECTANGLE_WIDTH(frame)/2 - offset->h/DEVICE_AREA_SCALE,
		frame->top + RECTANGLE_HEIGHT(frame)/2 - offset->v/DEVICE_AREA_SCALE);
	
	return;
}

static void get_device_area_frame(
	GDHandle device,
	Rect *frame,
	Point offset)
{
	*frame= (*device)->gdRect;
	frame->left/= DEVICE_AREA_SCALE, frame->right/= DEVICE_AREA_SCALE;
	frame->top/= DEVICE_AREA_SCALE, frame->bottom/= DEVICE_AREA_SCALE;
	OffsetRect(frame, offset.h, offset.v);
	
	return;
}
