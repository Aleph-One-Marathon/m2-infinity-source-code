/*
SOUND_MACINTOSH.C
Friday, August 25, 1995 4:51:06 PM  (Jason)

Tuesday, August 29, 1995 8:56:06 AM  (Jason)
	running.
Thursday, August 31, 1995 9:09:46 AM  (Jason)
	pitch changes without bogus sound headers.
Saturday , January 27, 1996 9:26 AM  (Jason)
	use rateMultiplierCmd instead of rateCmd to adjust channel pitch before bufferCmd.
*/


#ifdef SUPPORT_SOUND_SPROCKET
#include "fp.h"

static SSpListenerReference 	gListener = NULL;
static float					gSprocketSinTable[NUMBER_OF_ANGLES];
static float					gSprocketCosTable[NUMBER_OF_ANGLES];

void InitializeTables(void);
SSpSourceReference 		CreateSource (void);
SSpListenerReference 	CreateListener(void);

void ConvertMarathonCoordinatesToSoundSprocket (world_location3d *theWorld, 
												TQ3CameraPlacement *outCamera);
												
void CalcSourceInfo(SSpSourceReference theSource, world_location3d 	*theWorld);
void CalcListenerInfo (world_location3d *marathonCoords);

void InitializeTables(void)
{
	int loop;
	long double angle = 0;
	
	for (loop = 0; loop < NUMBER_OF_ANGLES; loop++)
	{
		
		long double angle = 360*loop/(57.296*NUMBER_OF_ANGLES);
		
		gSprocketSinTable[loop] = sin (angle);
		gSprocketCosTable[loop] = cos (angle);
	}
	
}

SSpSourceReference CreateSource ()
{
	OSStatus 				theErr = noErr;
	SSpSourceReference 		theSource = NULL;
	
	theErr = SSpSource_New (&theSource);
	
	if (theErr != noErr)
	{
		DebugStr ("\pFailed to create a source");
		theSource = NULL;
	}
	
	return theSource;
}


SSpListenerReference CreateListener()
{
	OSStatus 				theErr = noErr;
	SSpListenerReference 	theSource = NULL;
	
	theErr = SSpListener_New (&theSource);
	
	if (theErr != noErr)
	{
		DebugStr ("\pFailed to create a listener");
		theSource = NULL;
	}
	
	return theSource;
}

void ConvertMarathonCoordinatesToSoundSprocket (world_location3d *theWorld, 
												TQ3CameraPlacement *outCamera)
{
	// up vector and orientation are arbitrary, will fix later.
	angle pitch = normalize_angle (theWorld->pitch);
	angle yaw = normalize_angle (theWorld->yaw);
	
	outCamera->cameraLocation.x = (float) theWorld->point.x;
	outCamera->cameraLocation.y = (float) theWorld->point.y;
	outCamera->cameraLocation.z = (float) theWorld->point.z;

	outCamera->upVector.x = gSprocketSinTable[pitch];
	outCamera->upVector.y = 0;
	outCamera->upVector.z = gSprocketCosTable[pitch];
	
	outCamera->pointOfInterest.x = (float) theWorld->point.x + (gSprocketCosTable[yaw]*gSprocketCosTable[pitch]);
	outCamera->pointOfInterest.y = (float) theWorld->point.y - gSprocketSinTable[yaw];
	outCamera->pointOfInterest.z = (float) theWorld->point.z + (gSprocketCosTable[yaw]*gSprocketSinTable[pitch]);
}

// Only needs to be called once per frame
void CalcListenerInfo (world_location3d *marathonCoords)
{
	TQ3CameraPlacement 	listenerPosition;
	OSStatus 			theErr = noErr;

	// need to get the world coordinate -- how?

	ConvertMarathonCoordinatesToSoundSprocket (marathonCoords, &listenerPosition);
	
	theErr = SSpListener_SetCameraPlacement (gListener, &listenerPosition);
	
	if (theErr != noErr)
		DebugStr ("\pFailed to set the camera position of the listener");
}


// Needs to be called once per sound source that is being dynamically changed
void CalcSourceInfo(SSpSourceReference theSource, world_location3d 	*theWorld)
{
	TQ3CameraPlacement 	sourcePosition;
	OSStatus 			theErr = noErr;

	// need to get the world coordinate -- how?

	ConvertMarathonCoordinatesToSoundSprocket (theWorld, &sourcePosition);
	
	theErr = SSpSource_SetCameraPlacement (theSource, &sourcePosition);
	
	if (theErr != noErr)
		DebugStr ("\pFailed to set the camera position of the source");
}

#endif
/* --------- constants */

enum
{
	MINIMUM_SOUND_BUFFER_SIZE= 300*KILO,
	MORE_SOUND_BUFFER_SIZE= 600*KILO,
	AMBIENT_SOUND_BUFFER_SIZE= 1*MEG,
	MAXIMUM_SOUND_BUFFER_SIZE= 1*MEG,

	kAMBIENT_SOUNDS_HEAP= 6*MEG,
	kMORE_SOUNDS_HEAP= 4*MEG,
	k16BIT_SOUNDS_HEAP= 8*MEG,
	kEXTRA_MEMORY_HEAP= 12*MEG
};

/* --------- macros */

#define BUILD_STEREO_VOLUME(l, r) ((((long)(r))<<16)|(l))

/* --------- globals */

/* --------- private prototypes */

static pascal void sound_callback_proc(SndChannelPtr channel, SndCommand command);

static long sound_level_to_sound_volume(short level);

static void close_sound_file(void);
static void shutdown_sound_manager(void);

/* --------- code */

void set_sound_manager_parameters(
	struct sound_manager_parameters *parameters)
{
	if (_sm_initialized)
	{
		boolean initial_state= _sm_active;

		verify_sound_manager_parameters(parameters);
		
		/* if it was initially on, turn off the sound manager */
		if (initial_state) set_sound_manager_status(FALSE);		
		
		/* we need to get rid of the sounds we have in memory */
		unload_all_sounds();
		
		/* stuff in our new parameters */
		*_sm_parameters= *parameters;
		
		/* if it was initially on, turn the sound manager back on */
		if (initial_state) set_sound_manager_status(TRUE);
	}
	
	return;
}

/* passing FALSE disposes of all existing sound channels and sets _sm_active to FALSE,
	TRUE reallocates everything and sets _sm_active to TRUE */
void set_sound_manager_status(
	boolean active)
{
	if (_sm_initialized)
	{
		short i;
		OSErr error;
		struct channel_data *channel;
#ifdef SUPPORT_SOUND_SPROCKET
		SoundComponentLink		myLink;
#endif
	
		if (active != _sm_active)
		{
			if (active)
			{
				_sm_globals->total_channel_count= _sm_parameters->channel_count;
				if (_sm_parameters->flags&_ambient_sound_flag) _sm_globals->total_channel_count+= MAXIMUM_AMBIENT_SOUND_CHANNELS;

				_sm_globals->total_buffer_size= (_sm_parameters->flags&_more_sounds_flag) ?
					MORE_SOUND_BUFFER_SIZE : MINIMUM_SOUND_BUFFER_SIZE;
				if (_sm_parameters->flags&_ambient_sound_flag) _sm_globals->total_buffer_size+= AMBIENT_SOUND_BUFFER_SIZE;
				if (_sm_parameters->flags&_16bit_sound_flag) _sm_globals->total_buffer_size*= 2;
				if (_sm_globals->available_flags&_extra_memory_flag) _sm_globals->total_buffer_size*= 2;

				_sm_globals->sound_source= (_sm_parameters->flags&_16bit_sound_flag) ? _16bit_22k_source : _8bit_22k_source;
				_sm_globals->base_sound_definitions= sound_definitions + _sm_globals->sound_source*NUMBER_OF_SOUND_DEFINITIONS;

				GetDefaultOutputVolume(&_sm_globals->old_sound_volume);
				SetDefaultOutputVolume(sound_level_to_sound_volume(_sm_parameters->volume));
				
#ifdef SUPPORT_SOUND_SPROCKET
				InitializeTables();
				gListener = CreateListener();
				SSpListener_SetMetersPerUnit (gListener, 0.0001);
#endif
				
				for (i= 0, channel= _sm_globals->channels; i<_sm_globals->total_channel_count; ++i, ++channel)
				{
					/* initialize the channel */
					channel->flags= 0;
					channel->callback_count= FALSE;
					channel->sound_index= NONE;
					memset(channel->channel, 0, sizeof(SndChannel));
					channel->channel->qLength= stdQLength;
					channel->channel->userInfo= (long) &channel->callback_count;

					error= SndNewChannel(&channel->channel, sampledSynth, (_sm_parameters->flags&_stereo_flag) ? initStereo : initMono,
						_sm_globals->sound_callback_upp);
					if (error!=noErr)
					{
						for (channel= _sm_globals->channels; i; --i, ++channel)
						{
							error= SndDisposeChannel(channel->channel, TRUE);
							assert(error==noErr);
						}
						
 						alert_user(infoError, strERRORS, badSoundChannels, error);
						_sm_globals->total_channel_count= 0;
						active= _sm_active= _sm_initialized= FALSE;
						
						break;
					}
					
#ifdef SUPPORT_SOUND_SPROCKET
					channel->sspSource = CreateSource();

					myLink.description.componentType = kSoundEffectsType;
					myLink.description.componentSubType = kSSpLocalizationSubType;
					myLink.description.componentManufacturer = 0;
					myLink.description.componentFlags = 0;
					myLink.description.componentFlagsMask = 0;
					myLink.mixerID = nil;
					myLink.linkID = nil;
					
					error = SndSetInfo(channel->channel, siPreMixerSoundComponent, &myLink);
					if (error != noErr)
					{
						DebugStr("\pError in SOUND_SPROCKET - set_sound_manager_status");
					}
#endif		
					
				}
			}
			else
			{
				stop_all_sounds();
				for (i= 0, channel= _sm_globals->channels; i<_sm_globals->total_channel_count; ++i, ++channel)
				{
					error= SndDisposeChannel(channel->channel, TRUE);
					assert(error==noErr);
#ifdef SUPPORT_SOUND_SPROCKET
				SSpSource_Dispose (channel->sspSource);
#endif
				}

#ifdef SUPPORT_SOUND_SPROCKET
				SSpListener_Dispose (gListener);
#endif

				SetDefaultOutputVolume(_sm_globals->old_sound_volume);
				
				// noticing that the default sound volume wasn’t correctly restored until the
				// next sound command was issued (sysbeep, etc.) we do this to assure volume
				// is correct on exit
				{
					long unused;
					
					GetDefaultOutputVolume(&unused);
				}

				_sm_globals->total_channel_count= 0;
			}
			
			_sm_active= active;
		}
	}
	
	return;
}

OSErr open_sound_file(
	FSSpec *spec)
{
	OSErr error= noErr;
	short refNum;
	
	error= FSpOpenDF(spec, fsRdPerm, &refNum);
	if (error==noErr)
	{
		long count;

		// read header		
		{
			struct sound_file_header header;
			
			count= sizeof(struct sound_file_header);
			error= FSRead(refNum, &count, (void *) &header);
			if (error==noErr)
			{
				if (header.version!=SOUND_FILE_VERSION ||
					header.tag!=SOUND_FILE_TAG ||
					header.sound_count!=NUMBER_OF_SOUND_DEFINITIONS ||
					header.source_count!=NUMBER_OF_SOUND_SOURCES)
				{
					dprintf("sound file discarded %p 0x%x '%4.4s' #%d/#%d;g;", &header, header.version, &header.tag, header.sound_count, NUMBER_OF_SOUND_DEFINITIONS);
					error= -1;
				}
			}
		}
		
		if (error==noErr)
		{
			count= NUMBER_OF_SOUND_SOURCES*NUMBER_OF_SOUND_DEFINITIONS*sizeof(struct sound_definition);
			error= FSRead(refNum, &count, (void *) sound_definitions);
			if (error==noErr)
			{
			}
			else
			{
				_sm_initialized= FALSE;
			}
		}
		
		if (error!=noErr)
		{
			FSClose(refNum);
		}
		else
		{
			close_sound_file();
			_sm_globals->sound_file_refnum= refNum;
		}
	}
	
	return error;
}

boolean adjust_sound_volume_up(
	struct sound_manager_parameters *parameters,
	short sound_index)
{
	boolean changed= FALSE;

	if (_sm_active)
	{
		if (parameters->volume<NUMBER_OF_SOUND_VOLUME_LEVELS)
		{
			_sm_parameters->volume= (parameters->volume+= 1);
			SetDefaultOutputVolume(sound_level_to_sound_volume(parameters->volume));
			play_sound(sound_index, (world_location3d *) NULL, NONE);
			changed= TRUE;
		}
	}
	
	return changed;
}

boolean adjust_sound_volume_down(
	struct sound_manager_parameters *parameters,
	short sound_index)
{
	boolean changed= FALSE;

	if (_sm_active)
	{
		if (parameters->volume>0)
		{
			_sm_parameters->volume= (parameters->volume-= 1);
			SetDefaultOutputVolume(sound_level_to_sound_volume(parameters->volume));
			play_sound(sound_index, (world_location3d *) NULL, NONE);
			changed= TRUE;
		}
	}
	
	return changed;
}

void test_sound_volume(
	short volume,
	short sound_index)
{
	if (_sm_active)
	{
		if (volume= PIN(volume, 0, NUMBER_OF_SOUND_VOLUME_LEVELS))
		{
			SetDefaultOutputVolume(sound_level_to_sound_volume(volume));
			play_sound(sound_index, (world_location3d *) NULL, NONE);
			while (sound_is_playing(sound_index));
			SetDefaultOutputVolume(sound_level_to_sound_volume(_sm_parameters->volume));
		}
	}
	
	return;
}

/* ---------- private code (SOUND.C) */

static void initialize_machine_sound_manager(
	struct sound_manager_parameters *parameters)
{
	OSErr error;

	assert(kFullVolume==MAXIMUM_SOUND_VOLUME);

	_sm_globals->sound_callback_upp= NewSndCallBackProc((ProcPtr)sound_callback_proc);
	if ((error= MemError())==noErr)
	{
		short i;
		
		// allocate sound channels
		for (i= 0; i<MAXIMUM_SOUND_CHANNELS+MAXIMUM_AMBIENT_SOUND_CHANNELS; ++i)
		{
			_sm_globals->channels[i].channel= (SndChannelPtr) NewPtr(sizeof(SndChannel));
		}
		
		if ((error= MemError())==noErr)
		{
			FSSpec sounds_file;
			
			/* initialize _sm_globals */
			_sm_globals->loaded_sounds_size= 0;
			_sm_globals->total_channel_count= 0;
			_sm_globals->sound_file_refnum= -1;

			error= get_file_spec(&sounds_file, strFILENAMES, filenameSOUNDS8, strPATHS);
			if (error==noErr)
			{
				error= open_sound_file(&sounds_file);
				if (error==noErr)
				{
					atexit(shutdown_sound_manager);
					
					_sm_globals->available_flags= _stereo_flag | _dynamic_tracking_flag;
					{
						long heap_size= FreeMem();
						
						if (heap_size>kAMBIENT_SOUNDS_HEAP) _sm_globals->available_flags|= _ambient_sound_flag;
						if (heap_size>kMORE_SOUNDS_HEAP) _sm_globals->available_flags|= _more_sounds_flag;
						if (heap_size>k16BIT_SOUNDS_HEAP) _sm_globals->available_flags|= _16bit_sound_flag;
						if (heap_size>kEXTRA_MEMORY_HEAP) _sm_globals->available_flags|= _extra_memory_flag;
					}
					
					/* fake a set_sound_manager_parameters() call */
					_sm_parameters->flags= 0;
					_sm_initialized= _sm_active= TRUE;
					GetDefaultOutputVolume(&_sm_globals->old_sound_volume);
					
					set_sound_manager_parameters(parameters);
				}
			}
		}
	}

	vwarn(error==noErr, csprintf(temporary, "initialize_sound_manager() == #%d", error));

	return;
}

static boolean channel_busy(
	struct channel_data *channel)
{
	assert(SLOT_IS_USED(channel));
	
	return (channel->callback_count) ? FALSE : TRUE;
}

static void unlock_sound(
	short sound_index)
{
	struct sound_definition *definition= get_sound_definition(sound_index);
	
	assert(definition->hndl);
	
	if (definition->hndl)
	{
		HUnlock((Handle)definition->hndl);
	}
	
	return;
}

static void dispose_sound(
	short sound_index)
{
	struct sound_definition *definition= get_sound_definition(sound_index);
	
	assert(definition->hndl);
	
	_sm_globals->loaded_sounds_size-= GetHandleSize((Handle)definition->hndl);
	DisposeHandle((Handle)definition->hndl);
	definition->hndl= 0;
	
	return;
}

// should be asynchronous
// should only read a single sound unless _sm_parameters->flags&_more_sounds_flag
static long read_sound_from_file(
	short sound_index)
{
	struct sound_definition *definition= get_sound_definition(sound_index);
	boolean success= FALSE;
	Handle data= NULL;
	OSErr error= noErr;
	
	if (_sm_globals->sound_file_refnum!=-1)
	{
		long size= (_sm_parameters->flags&_more_sounds_flag) ? definition->total_length : definition->single_length;
		
		if (data= NewHandle(size))
		{
			ParamBlockRec param;
			
			HLock(data);
			
			param.ioParam.ioCompletion= (IOCompletionUPP) NULL;
			param.ioParam.ioRefNum= _sm_globals->sound_file_refnum;
			param.ioParam.ioBuffer= *data;
			param.ioParam.ioReqCount= size;
			param.ioParam.ioPosMode= fsFromStart;
			param.ioParam.ioPosOffset= definition->group_offset;
			
			error= PBReadSync(&param);
			if (error==noErr)
			{
				HUnlock(data);
				
				_sm_globals->loaded_sounds_size+= size;
			}
			else
			{
				DisposeHandle(data);
				
				data= NULL;
			}
		}
		else
		{
			error= MemError();
			dprintf("read_sound_from_file() couldn’t allocate #%d bytes", size);
		}
	}
	
	vwarn(error==noErr, csprintf(temporary, "read_sound_from_file(#%d) got error #%d", sound_index, error));
	
	return (long) data;
}

/* send flushCmd, quietCmd */
static void quiet_channel(
	struct channel_data *channel)
{
	SndCommand command;
	OSErr error;
	
	command.cmd= flushCmd;
	command.param1= 0;
	command.param2= 0;
	error= SndDoImmediate(channel->channel, &command);
	if (error==noErr)
	{
		command.cmd= quietCmd;
		command.param1= 0;
		command.param2= 0;
		error= SndDoImmediate(channel->channel, &command);
		if (error==noErr)
		{
		}
	}
	
	vwarn(error==noErr, csprintf(temporary, "SndDoImmediate() == #%d in quiet_channel()", error));
	
	return;
}

//••••••••••
#ifdef SUPPORT_SOUND_SPROCKET

static void instantiate_sound_variables(
	struct sound_variables *variables,
	struct channel_data *channel,
	boolean first_time)
{

	OSStatus 			error= noErr;
	SndCommand 			command;
	SSpLocalizationData stereoData;



	if (variables->useSprocketForSound)
	{
			// set the source location
	error = SSpSource_SetCameraPlacement (channel->sspSource, &variables->source);
	if (error != noErr) DebugStr ("\pFailed to set the source location");
	
	error = SSpSource_SetReferenceDistance (channel->sspSource, 2000.0);
	
		
	// calculate our channel parameters and pass them along to the channel
	error = SSpSource_CalcLocalization (channel->sspSource, gListener, &stereoData);
	if (error != noErr) DebugStr ("\pFailed to calculate the localization");
	
	if (first_time)
		stereoData.currentLocation.sourceVelocity = 0;
	
	stereoData.sourceMode = kSSpSourceMode_Localized;
	
	command.param1= 0;
	command.param2= BUILD_STEREO_VOLUME(variables->volume, variables->volume);
	error= SndDoImmediate(channel->channel, &command);
	
	SndSetInfo (channel->channel, siSSpLocalization, &stereoData);

	channel->variables= *variables;
	}
	else
	{
		SndCommand command;
		SSpLocalizationData bogusLocalization;

	if (first_time || variables->right_volume!=channel->variables.right_volume || variables->left_volume!=channel->variables.left_volume)
	{
		/* set the sound volume */
		command.cmd= volumeCmd;
		command.param1= 0;
		command.param2= BUILD_STEREO_VOLUME(variables->left_volume, variables->right_volume);
		error= SndDoImmediate(channel->channel, &command);
	}

	vwarn(error==noErr, csprintf(temporary, "SndDoImmediate() == #%d in instantiate_sound_variables()", error));

	channel->variables= *variables;
	
		bogusLocalization.sourceMode = kSSpSourceMode_Unfiltered;
		SndSetInfo (channel->channel, siSSpLocalization, &bogusLocalization);
		return;
	}

}

#else
static void instantiate_sound_variables(
	struct sound_variables *variables,
	struct channel_data *channel,
	boolean first_time)
{
	OSErr error= noErr;
	SndCommand command;

	if (first_time || variables->right_volume!=channel->variables.right_volume || variables->left_volume!=channel->variables.left_volume)
	{
		/* set the sound volume */
		command.cmd= volumeCmd;
		command.param1= 0;
		command.param2= BUILD_STEREO_VOLUME(variables->left_volume, variables->right_volume);
		error= SndDoImmediate(channel->channel, &command);
	}

	vwarn(error==noErr, csprintf(temporary, "SndDoImmediate() == #%d in instantiate_sound_variables()", error));

	channel->variables= *variables;
	
	return;
}
#endif

static void buffer_sound(
	struct channel_data *channel,
	short sound_index,
	fixed pitch)
{
	struct sound_definition *definition= get_sound_definition(sound_index);
	short permutation= get_random_sound_permutation(sound_index);
	SoundHeaderPtr sound_header;
	SndCommand command;
	OSErr error;

	assert(definition->hndl);
	HLock((Handle)definition->hndl);
	
	assert(permutation>=0 && permutation<definition->permutations);
	sound_header= (SoundHeaderPtr) ((*(byte **)definition->hndl) + definition->sound_offsets[permutation]);

	command.cmd= rateMultiplierCmd;
	command.param1= 0;
	command.param2= (long) calculate_pitch_modifier(sound_index, pitch);
	error= SndDoImmediate(channel->channel, &command);
	if (error==noErr)
	{
		/* play the sound */
		command.cmd= bufferCmd; /* high bit not set: we’re sending a real pointer */
		command.param1= 0;
		command.param2= (long) sound_header;
		error= SndDoCommand(channel->channel, &command, FALSE);
		if (error==noErr)
		{
			/* queue the callback */
			command.cmd= callBackCmd;
			command.param1= 0;
			command.param2= 0;
			error= SndDoCommand(channel->channel, &command, FALSE);
			if (error==noErr)
			{
			}
		}
	}

	vwarn(error==noErr, csprintf(temporary, "SndDoCommand() == #%d in buffer_sound()", error));
	
	return;
}

/* ---------- private code (SOUND_MACINTOSH.C) */

static void shutdown_sound_manager(
	void)
{
	set_sound_manager_status(FALSE);

	close_sound_file();	
	
	return;
}

static pascal void sound_callback_proc(
	SndChannelPtr channel,
	SndCommand command)
{
	#pragma unused (command)
	
	*((short *)channel->userInfo)+= 1;

	return;
}

static void close_sound_file(
	void)
{
	OSErr error= noErr;
	
	if (_sm_globals->sound_file_refnum!=-1)
	{
		error= FSClose(_sm_globals->sound_file_refnum);
		if (error!=noErr)
		{
			_sm_globals->sound_file_refnum= -1;
		}
	}
	
	return;
}

static long sound_level_to_sound_volume(
	short level)
{
	short volume= level*SOUND_VOLUME_DELTA;
	
	return BUILD_STEREO_VOLUME(volume, volume);
}


#if 0
enum
{
	FADE_OUT_DURATION= 2*MACINTOSH_TICKS_PER_SECOND,
	FADE_OUT_STEPS= 20,
	
	TICKS_PER_FADE_OUT_STEP= FADE_OUT_DURATION/FADE_OUT_STEPS
};

// doesn’t work
static synchronous_global_fade_to_silence(
	void)
{
	short i;
	boolean fade_out= FALSE;
	struct channel_data *channel;
	struct sound_variables old_variables[MAXIMUM_SOUND_CHANNELS+MAXIMUM_AMBIENT_SOUND_CHANNELS];
	
	for (i= 0, channel= _sm_globals->channels; i<_sm_globals->total_channel_count; ++i, ++channel)
	{
		if (SLOT_IS_USED(channel))
		{
			old_variables[i]= channel->variables;
			
			fade_out= TRUE;
		}
	}
	
	if (fade_out)
	{
		short step;
		
		for (step= 0; step<FADE_OUT_STEPS; ++step)
		{
			long tick_at_last_update= TickCount();
			
			while (TickCount()-tick_at_last_update<TICKS_PER_FADE_OUT_STEP);
			
			for (i= 0, channel= _sm_globals->channels; i<_sm_globals->total_channel_count; ++i, ++channel)
			{
				if (SLOT_IS_USED(channel))
				{
					struct sound_variables *old= old_variables + i;
					struct sound_variables *new= &channel->variables;
					
					new->volume= (old->volume*(FADE_OUT_STEPS-step-1))/FADE_OUT_STEPS;
					new->left_volume= (old->left_volume*(FADE_OUT_STEPS-step-1))/FADE_OUT_STEPS;
					new->right_volume= (old->right_volume*(FADE_OUT_STEPS-step-1))/FADE_OUT_STEPS;
			
#ifdef SUPPORT_SOUND_SPROCKET
					variables->useSprocketForSound = false;
#endif
					instantiate_sound_variables(&channel->variables, channel, TRUE);
				}
			}
		}
	}
	
	return;
}
#endif
