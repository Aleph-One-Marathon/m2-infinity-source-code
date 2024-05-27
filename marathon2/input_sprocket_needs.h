
// Input Sprocket need grouping hints for auto configuration
enum
{
	_isp_group_movement= 1,			// forward, backward
	_isp_group_yaw,					// turning
	_isp_group_pitch,				// looking up/down
	_isp_group_sidestepping,		// sidestep left/right
	_isp_group_weapons_cycle,		// cycle weapons
	_isp_group_glancing,			// glance left/right
	_isp_group_volume,				// volume up/down
	_isp_group_map_zoom,			// zoom in/out
	_isp_group_scroll_inventory,	// scroll inv fwd/back (and replay speed)
	_isp_group_gamma_correction		// gamma increase decrease
};

//input sprocket needs mapped to flags
int input_sprocket_needs_to_flags[NUMBER_OF_INPUT_SPROCKET_NEEDS]=
{
	_left_trigger_state,
	_right_trigger_state,			
	_moving_forward,				
	_moving_backward,
	
	_turning_left,
	_turning_right,
	0,	// yaw
	0,	// yaw delta

	_looking_down,
	_looking_up,
	0,	// pitch
	0,	// pitch delta

	_sidestepping_left,
	_sidestepping_right,
	_action_trigger_state,

	_cycle_weapons_backward,
	_cycle_weapons_forward,
	_run_dont_walk,
	_sidestep_dont_turn,
	_look_dont_turn,
	_looking_center,
	_looking_left,	// glance left
	_looking_right,	// glance right
	_toggle_map,

	_microphone_button,
	0,	// quit
	0,	// volume up
	
	0,	// volume down
	0,	// change view
	0,	// zoom map in
	0,	// zoom map out
	0,	// scroll back
	
	0,	// scroll forward
	0,	// full screen
	0,	// 100 percent

	0,	// 75 percent
	0,	// 50 percent
	0,	// low res
#ifdef ALEX_DISABLED
	0,	// high res
	0,	// texture_floor_toggle

	0,	// toggle texture mapping on ceiling
#endif
	0,	// gamma correction plus
	0,	// gamma correction minus
	0	// show fps
#ifdef ALEX_DISABLED
	0,	// toggle background tasks
#endif
};

enum
{
	_pitch_icon= 1000,
	_yaw_icon,
	_primary_trigger_icon,
	_secondary_trigger_icon,
	_select_icon,
	_sidestep_left_icon,
	_sidestep_right_icon,
	_moving_forward_icon,
	_moving_backward_icon,
	_turning_left_icon,
	_turning_right_icon,
	_glance_left_icon,
	_glance_right_icon,
	_next_weapon_icon,
	_previous_weapon_icon,
	_sidestep_modifier_icon,
	_run_modifier_icon,
	_look_icon,
	_quit_icon,
	_look_up_icon,
	_look_down_icon,
	_action_icon,
	_map_icon,
	_generic_icon= 2000
};

ISpNeed input_sprocket_needs[NUMBER_OF_INPUT_SPROCKET_NEEDS] =
{
	// 0
	{
		"\pPrimary Trigger",
		_primary_trigger_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Fire,
		0,
		0,
		0,
		0
	},
	{
		"\pSecondary Trigger",
		_secondary_trigger_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_SecondaryFire,
		0,
		0,
		0,
		0
	},
	{
		"\pForward",
		_moving_forward_icon,
		0,
		_isp_group_movement,
		kISpElementKind_Button,
		kISpElementLabel_Btn_MoveForward,
		0,
		0,
		0,
		0
	},
	{
		"\pBackward",
		_moving_backward_icon,
		0,
		_isp_group_movement,
		kISpElementKind_Button,
		kISpElementLabel_Btn_MoveBackward,
		0,
		0,
		0,
		0
	},
	// turning (yaw)
	{
		"\pTurn Left",
		_turning_left_icon,
		0,
		_isp_group_yaw,
		kISpElementKind_Button,
		kISpElementLabel_Btn_TurnLeft,
		kISpNeedFlag_Button_AlreadyAxis | kISpNeedFlag_Button_AlreadyDelta,
		0,
		0,
		0
	},
	{
		"\pTurn Right",
		_turning_right_icon,
		0,
		_isp_group_yaw,
		kISpElementKind_Button,
		kISpElementLabel_Btn_TurnRight,
		kISpNeedFlag_Button_AlreadyAxis | kISpNeedFlag_Button_AlreadyDelta,
		0,
		0,
		0
	},
	{
		"\pYaw",
		_yaw_icon,
		0,
		_isp_group_yaw,
		kISpElementKind_Axis,
		kISpElementLabel_Axis_Yaw,
		kISpNeedFlag_Axis_AlreadyButton | kISpNeedFlag_Axis_AlreadyDelta,
		0,
		0,
		0
	},
	{
		"\pYaw (Classic Mouse)",
		_yaw_icon,
		0,
		_isp_group_yaw,
		kISpElementKind_Delta,
		kISpElementLabel_Delta_Yaw,
		kISpNeedFlag_Delta_AlreadyButton | kISpNeedFlag_Delta_AlreadyAxis,
		0,
		0,
		0
	},
	
	// looking up/down (pitch)
	{
		"\pLook Down",
		_look_down_icon,
		0,
		_isp_group_pitch,
		kISpElementKind_Button,
		kISpElementLabel_Btn_LookDown,
		kISpNeedFlag_Button_AlreadyAxis | kISpNeedFlag_Button_AlreadyDelta,
		0,
		0,
		0
	},
	{
		"\pLook Up",
		_look_up_icon,
		0,
		_isp_group_pitch,
		kISpElementKind_Button,
		kISpElementLabel_Btn_LookUp,
		kISpNeedFlag_Button_AlreadyAxis | kISpNeedFlag_Button_AlreadyDelta,
		0,
		0,
		0
	},
	{
		"\pPitch",
		_pitch_icon,
		0,
		_isp_group_pitch,
		kISpElementKind_Axis,
		kISpElementLabel_Axis_Pitch,
		kISpNeedFlag_Axis_AlreadyButton | kISpNeedFlag_Axis_AlreadyDelta,
		0,
		0,
		0
	},
	{
		"\pPitch (Classic Mouse)",
		_pitch_icon,
		0,
		_isp_group_pitch,
		kISpElementKind_Delta,
		kISpElementLabel_Delta_Pitch,
		kISpNeedFlag_Delta_AlreadyButton | kISpNeedFlag_Delta_AlreadyAxis,
		0,
		0,
		0
	},
	
	{
		"\pSidestep Left",
		_sidestep_left_icon,
		0,
		_isp_group_sidestepping,
		kISpElementKind_Button,
		kISpElementLabel_Btn_SlideLeft,
		0,
		0,
		0,
		0
	},
	{
		"\pSidestep Right",
		_sidestep_right_icon,
		0,
		_isp_group_sidestepping,
		kISpElementKind_Button,
		kISpElementLabel_Btn_SlideRight,
		0,
		0,
		0,
		0
	},
	{
		"\pAction Key",
		_action_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	{
		"\pPrevious Weapon",
		_previous_weapon_icon,
		0,
		_isp_group_weapons_cycle,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Previous,
		0,
		0,
		0,
		0
	},
	{
		"\pNext Weapon",
		_next_weapon_icon,
		0,
		_isp_group_weapons_cycle,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Next,
		0,
		0,
		0,
		0
	},
	{
		"\pRun or Swim",
		_run_modifier_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Run,
		0,
		0,
		0,
		0
	},
	{
		"\pSidestep",
		_sidestep_modifier_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_SideStep,
		0,
		0,
		0,
		0
	},
	{
		"\pLook",
		_look_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Look,
		kISpNeedFlag_Button_AlreadyAxis,
		0,
		0,
		0
	},
	{
		"\pLook Center",
		_generic_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Btn_LookUp,
		kISpNeedFlag_Button_AlreadyAxis,
		0,
		0,
		0
	},
	{
		"\pGlance Left",
		_glance_left_icon,
		0,
		_isp_group_glancing,
		kISpElementKind_Button,
		kISpElementLabel_Btn_LookLeft,
		0,
		0,
		0,
		0
	},
	{
		"\pGlance Right",
		_glance_right_icon,
		0,
		_isp_group_glancing,
		kISpElementKind_Button,
		kISpElementLabel_Btn_LookRight,
		0,
		0,
		0,
		0
	},
	{
		"\pMap",
		_map_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	{
		"\pMicrophone",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		0,
		0,
		0,
		0
	},
	{
		"\pQuit",
		_quit_icon,
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_Start,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pVolume Up",
		_generic_icon,	//!!! should be icon number
		0,
		_isp_group_volume,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pVolume Down",
		_generic_icon,	//!!! should be icon number
		0,
		_isp_group_volume,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pChange Replay View",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pZoom Map In",
		_generic_icon,	//!!! should be icon number
		0,
		_isp_group_map_zoom,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pZoom Map Out",
		_generic_icon,	//!!! should be icon number
		0,
		_isp_group_map_zoom,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pScroll Inventory Back",	// also replay speed
		_select_icon,
		0,
		_isp_group_scroll_inventory,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Previous,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pScroll Inventory Forward",	// also replay speed
		_select_icon,
		0,
		_isp_group_scroll_inventory,
		kISpElementKind_Button,
		kISpElementLabel_Btn_Next,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pSwitch to full screen size",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pSwitch to 100% screen size",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pSwitch to 75% screen size",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pSwitch to 50% screen size",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pToggle resolution",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
#ifdef ALEX_DISABLED
	{
		"\pSwitch to high res",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pToggle floor textures",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pToggle ceiling textures",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
#endif	
	{
		"\pGamma Correction Decrement",
		_generic_icon,	//!!! should be icon number
		0,
		_isp_group_gamma_correction,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pGamma Correction Increment",
		_generic_icon,	//!!! should be icon number
		0,
		_isp_group_gamma_correction,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
	{
		"\pShow FPS Counter",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	}
#ifdef ALEX_DISABLED
	{
		"\pToggle Background Tasks",
		_generic_icon,	//!!! should be icon number
		0,
		0,
		kISpElementKind_Button,
		kISpElementLabel_None,
		kISpNeedFlag_Utility,	// utility function
		0,
		0,
		0
	},
#endif
};
