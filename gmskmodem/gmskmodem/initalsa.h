/* initalsa.h */


//// INIT ALSA ////


/*
 *      Copyright (C) 2011 by Kristoff Bonne, ON1ARF
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */


// Release information:
// 20120105: initial release



int init_alsa_playback(char * retmsg) {

int ret;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;
unsigned int val;
int dir;
int warning=0;


/* Open PCM device for playback. */
if (ALSAPLAYBACKNONBLOCK) {
	ret = snd_pcm_open(&s_global.alsahandle, s_global.alsaname, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
} else {
	ret = snd_pcm_open(&s_global.alsahandle, s_global.alsaname, SND_PCM_STREAM_PLAYBACK, 0);
}; // end else - if
if (ret < 0) {
	snprintf(retmsg,INITALSARETMSGSIZE, "unable to open pcm device: %s\n", snd_strerror(ret));
	return(-1);
}

/* Allocate a hardware parameters object. */
snd_pcm_hw_params_alloca(&params);

/* Fill it in with default values. */
snd_pcm_hw_params_any(s_global.alsahandle, params);

/* Set the desired hardware parameters. */

/* Interleaved mode */
snd_pcm_hw_params_set_access(s_global.alsahandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

/* Signed 16-bit little-endian format */
snd_pcm_hw_params_set_format(s_global.alsahandle, params, SND_PCM_FORMAT_S16_LE);


/* number of channels */
snd_pcm_hw_params_set_channels(s_global.alsahandle, params, 2);


/* 48 Khz sampling rate */
val=RATE;
snd_pcm_hw_params_set_rate_near(s_global.alsahandle, params, &val, &dir);

/* Try to set the period size to 960 frames (20 ms at 48Khz sampling rate). */
frames = PERIOD;
snd_pcm_hw_params_set_period_size_near(s_global.alsahandle, params, &frames, &dir);

// set_period_size_near TRIES to set the period size to a certain
// value. However, there is no garantee that the audio-driver will accept
// this. So read the actual configured value from the device

// get actual periode size from driver for capture and playback
// capture
snd_pcm_hw_params_get_period_size(params, &frames, &dir);
s_global.alsaframes = frames;

if (frames != PERIOD) {
	snprintf(retmsg,INITALSARETMSGSIZE,"periode could not be set: wanted %d, got %d \n",PERIOD,(int) frames);
	warning=1;
}; // end if

/* Write the parameters to the driver */
ret = snd_pcm_hw_params(s_global.alsahandle, params);
if (ret < 0) {
	snprintf(retmsg,INITALSARETMSGSIZE,"unable to set hw parameters: %s\n", snd_strerror(ret));
	return(-1);
}


// all done
if (warning) {
	return(1);
};

// no warning
return(0);

}; // end function init_alsa_playback


////////// INIT_ALSA CAPTURE

int init_alsa_capture(char * retmsg) {

int ret;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;
unsigned int val;
int dir;
int warning=0;

/* Open PCM device for recording (capture). */
if (ALSACAPTURENONBLOCK) {
	ret = snd_pcm_open(&r_global.handle, r_global.capturedevice, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
} else {
	ret = snd_pcm_open(&r_global.handle, r_global.capturedevice, SND_PCM_STREAM_CAPTURE, 0);
}; // end else - if

if (ret < 0) {
	snprintf(retmsg,INITALSARETMSGSIZE, "unable to open pcm device: %s\n", snd_strerror(ret));
	return(-1);
}

/* Allocate a hardware parameters object. */
snd_pcm_hw_params_alloca(&params);

/* Fill it in with default values. */
snd_pcm_hw_params_any(r_global.handle, params);

/* Set the desired hardware parameters. */

/* Interleaved mode */
snd_pcm_hw_params_set_access(r_global.handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

/* Signed 16-bit little-endian format */
snd_pcm_hw_params_set_format(r_global.handle, params, SND_PCM_FORMAT_S16_LE);


/* number of channels */
if (r_global.stereo) {
	snd_pcm_hw_params_set_channels(r_global.handle, params, 2);
} else {
	snd_pcm_hw_params_set_channels(r_global.handle, params, 1);
}; // end else - if


/* 48 Khz sampling rate */
val=RATE;
snd_pcm_hw_params_set_rate_near(r_global.handle, params, &val, &dir);

/* Try to set the period size to 960 frames (20 ms at 48Khz sampling rate). */
frames = PERIOD;
snd_pcm_hw_params_set_period_size_near(r_global.handle, params, &frames, &dir);

// set_period_size_near TRIES to set the period size to a certain
// value. However, there is no garantee that the audio-driver will accept
// this. So read the actual configured value from the device

// get actual periode size from driver for capture and playback
// capture
snd_pcm_hw_params_get_period_size(params, &frames, &dir);
r_global.frames = frames;

if (frames != PERIOD) {
	snprintf(retmsg,INITALSARETMSGSIZE,"periode could not be set: wanted %d, got %d \n",PERIOD,(int) r_global.frames);
	warning=1;
}; // end if

/* Write the parameters to the driver */
ret = snd_pcm_hw_params(r_global.handle, params);
if (ret < 0) {
	snprintf(retmsg,INITALSARETMSGSIZE,"unable to set hw parameters: %s\n", snd_strerror(ret));
	return(-1);
}


// all done
if (warning) {
	return(1);
};

// no warning
return(0);

}; // end function init_alsa_capture

