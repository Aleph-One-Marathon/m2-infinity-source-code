
#ifdef SUPPORT_INPUT_SPROCKET

enum
{
	//input sprocket needs that are flags only
	
	_input_sprocket_left_trigger,					// #  0 
	_input_sprocket_right_trigger,					// #  1
	_input_sprocket_moving_forward,					// #  2
	_input_sprocket_moving_backward,				// #  3

	_input_sprocket_turning_left,					// #  4
	_input_sprocket_turning_right,					// #  5
	_input_sprocket_yaw,							// #  6 horizontal axis	
	_input_sprocket_yaw_delta,						// #  7

	_input_sprocket_look_down,						// #  8
	_input_sprocket_look_up,						// #  9
	_input_sprocket_pitch,							// # 10 vertical axis 
	_input_sprocket_pitch_delta,					// # 11

	_input_sprocket_sidestep_left,					// # 12
	_input_sprocket_sidestep_right,					// # 13
	_input_sprocket_action_trigger,					// # 14
	
	_input_sprocket_previous_weapon,				// # 15
	_input_sprocket_next_weapon,					// # 16
	_input_sprocket_run_dont_walk,					// # 17
	_input_sprocket_sidestep_dont_turn,				// # 18 
	_input_sprocket_look_dont_turn,					// # 19
	
	_input_sprocket_looking_center,					// # 20
	_input_sprocket_looking_left,					// # 21 glance left
	_input_sprocket_looking_right,					// # 22 glance right
	_input_sprocket_toggle_map,						// # 23
	_input_sprocket_microphone_button,				// # 24	

	_input_sprocket_quit,							// # 25

	// utility keys here and below
	
	_input_sprocket_volume_up,						// # 26
	_input_sprocket_volume_down,
	_input_sprocket_change_view,
	
	_input_sprocket_zoom_map_in,					// # 29
	_input_sprocket_zoom_map_out,
	_input_sprocket_scroll_back_decrement_replay,
	_input_sprocket_scroll_forward_increment_replay,

	_input_sprocket_full_screen,							// f1
	_input_sprocket_100_percent,							// f2

	_input_sprocket_75_percent,								// f3
	_input_sprocket_50_percent,								// f4
	_input_sprocket_low_res,								// f5
#ifdef ALEX_DISABLED
	_input_sprocket_high_res,								// f6
	_input_sprocket_texture_floor_toggle,					// f7

	_input_sprocket_texture_ceiling_toggle,					// f8
#endif
	_input_sprocket_gamma_minus,							// f11
	_input_sprocket_gamma_plus,								// f12
	
	_input_sprocket_show_fps,								// # 30

#ifdef ALEX_DISABLED
	_input_sprocket_toggle_background_tasks,
#endif	
	NUMBER_OF_INPUT_SPROCKET_NEEDS,
};

extern int input_sprocket_needs_to_flags[];

#endif