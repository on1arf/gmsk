/* initalsa.h */

int init_alsa(char * retmsg) {

int ret;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;
unsigned int val;
int dir;
int warning=0;


/* Open PCM device for recording (capture). */
ret = snd_pcm_open(&global.handle, global.capturedevice, SND_PCM_STREAM_CAPTURE, 0);
if (ret < 0) {
	snprintf(retmsg,160, "unable to open pcm device: %s\n", snd_strerror(ret));
	return(-1);
}

/* Allocate a hardware parameters object. */
snd_pcm_hw_params_alloca(&params);

/* Fill it in with default values. */
snd_pcm_hw_params_any(global.handle, params);

/* Set the desired hardware parameters. */

/* Interleaved mode */
snd_pcm_hw_params_set_access(global.handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

/* Signed 16-bit little-endian format */
snd_pcm_hw_params_set_format(global.handle, params, SND_PCM_FORMAT_S16_LE);

/* One channel (mono) */
snd_pcm_hw_params_set_channels(global.handle, params, 1);

/* 48 Khz sampling rate */
val=RATE;
snd_pcm_hw_params_set_rate_near(global.handle, params, &val, &dir);

/* Try to set the period size to 960 frames (20 ms at 48Khz sampling rate). */
frames = PERIOD;
snd_pcm_hw_params_set_period_size_near(global.handle, params, &frames, &dir);

// set_period_size_near TRIES to set the period size to a certain
// value. However, there is no garantee that the audio-driver will accept
// this. So read the actual configured value from the device

// get actual periode size from driver for capture and playback
// capture
snd_pcm_hw_params_get_period_size(params, &frames, &dir);
global.frames = frames;

if (frames != PERIOD) {
	snprintf(retmsg,160,"periode could not be set: wanted %d, got %d \n",PERIOD,(int) global.frames);
	warning=1;
}; // end if

/* Write the parameters to the driver */
ret = snd_pcm_hw_params(global.handle, params);
if (ret < 0) {
	snprintf(retmsg,160,"unable to set hw parameters: %s\n", snd_strerror(ret));
	return(-1);
}


// all done
if (warning) {
	return(1);
};

// no warning
return(0);

}; // end function init_alsa
