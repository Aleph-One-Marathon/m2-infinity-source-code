/*
	OVERLAY.H
	Sunday, February 6, 1994 7:41:10 PM
*/


/* ----- Constants */
#define WINDOW_MASK 0x8000

/* ---- Macros */
#define GET_WINDOW_INDEX(x) (((shape_desc) (x)) & ~WINDOW_MASK)
#define HAS_WINDOW(x) (((shape_desc) (x) & WINDOW_MASK) ? (TRUE) : (FALSE))

/* ----- Code */
void initialize_wall_windows(void);

shape_desc new_wall_window(
	shape_desc base_shape, 	
	short animation_ticks,
	shape_desc *windows,
	short count);

void get_wall_window_information(
	shape_desc texture,
	register struct wall_information_data *data);
	
void free_wall_window(shape_desc window);

void update_wall_windows(short time_elapsed);