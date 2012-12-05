/* ALSA OUT */

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
// version 20120105: initial release


static void funct_alsaout (int sig) {

// static vars (kept from one cycle to another)
static int state;
static int running;
static int init=1;

static int overrun;

static int16_t * p_audioafterend;
static int silencecounter;
static int silencesize;

// local vars
int nextbuffer;
static char alsaoutbuffer[3840]; // buffer is 960 samples (20 ms at 48Khz)
										// multiplied by 4: stereo and 16 bit size
static char silence[3840]; // buffer containing all 0 (silence)
int16_t * p_in; // pointer to audio (global.buffer_audio)
int16_t * p_out; // pointer to audio "out" (alsabuffer)
int loop;
int ret;
int queuesize;
int thisalsastate;


// init vars first time
if (init) {
	char retmsg[INITALSARETMSGSIZE];

	state=0;
	running=0;
	init=0;
	overrun=0;

	// flag to syncronise between sender and receiver. Disable decoding when sending
	g_global.sending=0;

	// no audio in queue
	s_global.waiting_audio=1;

	//note: this points just AFTER the end of the audio-buffer
	p_audioafterend = &s_global.buffer_audio[BUFFERSIZE_AUDIO];

	// init alsa
	ret=init_alsa_playback(retmsg);

	if (ret) {
		// retval != 0: error or warning
		fprintf(stderr,"%s",retmsg);

		// retval < 0: error -> exit
		if (ret <0 ) {
			exit(-1);
		};
	}; // end if

	// create silence
	memset(silence,0,3840);
	
}; // end if

// as this function is called every 20 ms, there is a change
// the earlier instance is still running. Let's check
if (running) {
	putc('A',stderr);
	return;
}; // end if


// set "running" flag
running=1;


// check "overrun" state. If the previous frame could not be completely
// writen, this means we are in "overrun" state. Audio data is being fed
// to fast to the ALSA device. This probably means that the 20 ms
// kernel heartbeat is to fast compaired to the ALSA audio driver.

// skip this frame to give the alsa driver "some breathing-space"
if (overrun) {
	overrun=0;

	// stop now
	g_global.sending=0;
	running=0;
}; // end if


// check ALSA state.
thisalsastate = snd_pcm_state(s_global.alsahandle);

if (thisalsastate == SND_PCM_STATE_OPEN) {
// alsa card still in "open" state. Wait some more to give it some time to get into "running" or "prepared" mode
	fprintf(stderr, "ALSAOUT: ALSA still in \"OPEN\" STATE! \n");

	// stop now
	g_global.sending=0;
	running=0;

	return;

} else if (thisalsastate == SND_PCM_STATE_SETUP) {
// alsa card still in "setup" state. Wait some more to give it some time to get into "running" or "prepated" mode
	fprintf(stderr, "ALSAOUT: ALSA still in \"SETUP\" STATE! \n");

	// stop now
	g_global.sending=0;
	running=0;

	return;
} else if (thisalsastate == SND_PCM_STATE_XRUN) {
// if state is underrun, try "snd_pcm_prepate"
	ret = snd_pcm_prepare(s_global.alsahandle);

	if (ret < 0) {
		fprintf(stderr, "ALSAOUT: underrun recovery failed! \n");
	} else {
		fprintf(stderr, "ALSAOUT: underrun recovery done! \n");
	}; // end if
}; // end if


queuesize = s_global.ptr_a_fillup - s_global.ptr_a_read - 1;

// wrapping of counters around end-of-buffer
if (queuesize < 0) {
	queuesize +=  BUFFERSIZE_AUDIO;
}; // end if


// check state
// state = 0: waiting for audio
// state = 1: begin silence
// state = 2: playout out
// state = 3: end silence


if (state == 0)  {
// state 0: waiting for at least AUDIOMINBUFFER frames received

	if (queuesize < AUDIOMINBUFFER) {
		// not yet enough info in buffer -> exit
		// play out silence
		ret=snd_pcm_writei(s_global.alsahandle,silence,PERIOD);
if (ret < 0) {
	// error
   fprintf(stderr, "ALSAOUT state 0/waiting: unable to write to pcm device: %s\n", snd_strerror(ret));
fprintf(stderr, "alsastate was %d \n",thisalsastate);
} else if (ret < PERIOD) {
	// Not all audiodata could be writen: "overrun" state
	overrun=1;

	fprintf(stderr,"O%d",ret);
}; // end if


		// unlock, just to be sure
		g_global.sending=0;
		running=0;

		return;
	}; // end if

	// sufficient audio in buffer. Start sending it out
	// set new state:
	// if begin-silence to be played: go to state 1
	// if no begin-silence: go directly to state 2
	if (s_global.silencebegin) {
		state=1;
		silencesize=s_global.silencebegin*5; // function is started 50 times / second (silence counter is in tenth of second)
		silencecounter=0;

		// lock PTT lockfile
		g_global.sending=1;

		// play out one sample of silence before going to next 20ms timeslot
		ret=snd_pcm_writei(s_global.alsahandle,silence,PERIOD);
if (ret < 0) {
	// error
   fprintf(stderr, "ALSAOUT state0/begin-silence: unable to write to pcm device: %s\n", snd_strerror(ret));
fprintf(stderr, "alsastate was %d \n",thisalsastate);
} else if (ret < PERIOD) {
	// Not all audiodata could be writen: "overrun" state
	overrun=1;

	fprintf(stderr,"O%d",ret);
}; // end if


		running=0;
		return;
	} else {
		state=2;
	}; // end else - if

	// audio in queue
	// go on below
	s_global.waiting_audio=0;


} else if (state == 1) {
	// state = 1: play out silence
	silencecounter++;

	// all silence send?
	if (silencecounter >= silencesize) {
		state = 2;
	}; // end if

	// lock PTT lockfile (just to be sure)
	g_global.sending=1;

	// play out silence and done
	ret=snd_pcm_writei(s_global.alsahandle,silence,PERIOD);
if (ret < 0) {
	// error
   fprintf(stderr, "ALSAOUT/state1: unable to write to pcm device: %s\n", snd_strerror(ret));
fprintf(stderr, "alsastate was %d \n",thisalsastate);
} else if (ret < PERIOD) {
	// Not all audiodata could be writen: "overrun" state
	overrun=1;

	fprintf(stderr,"O%d",ret);
}; // end if


	// clear "running" marker
	running=0;

	return;

} else if (state == 2) {
	if (queuesize < PERIOD ) {
		// no data anymore in buffer

		// determine next state
		if (s_global.silenceend) {
			// silence to be added at the end
			state=3;
			silencesize=s_global.silenceend*5; // function is started 50 times / second (silence value is tenth of second)
			silencecounter=0;

			g_global.sending=1;

		} else {
			// no end silence, reset state
			state=0;
			running=0;

			// no audio in queue
			s_global.waiting_audio=1;

			// unlock PTT lockfile
			g_global.sending=0;

		}; // end else - if


		// finally, play out silence and stop
		ret=snd_pcm_writei(s_global.alsahandle,silence,PERIOD);
if (ret < 0) {
	// error
   fprintf(stderr, "ALSAOUT/state2: unable to write to pcm device: %s\n", snd_strerror(ret));
fprintf(stderr, "alsastate was %d \n",thisalsastate);
} else if (ret < PERIOD) {
	// Not all audiodata could be writen: "overrun" state
	overrun=1;

	fprintf(stderr,"O%d",ret);
}; // end if


		// clear "running" marker
		running=0;

		return;
	}; // end if

	// if there IS audio to be send, go on below to play out audio

} else if (state == 3) {
	// state = 3: play out silence
	silencecounter++;

	// all silence send?
	if (silencecounter >= silencesize) {
		// reset state
		state = 0;
		running=0;

		// no audio in queue
		s_global.waiting_audio=1;

		// unlock PTT lockfile
		g_global.sending=0;

	}; // end if

	// play out silence and done
	ret=snd_pcm_writei(s_global.alsahandle,silence,PERIOD);
if (ret < 0) {
	// error
   fprintf(stderr, "ALSAOUT/state3: unable to write to pcm device: %s\n", snd_strerror(ret));
fprintf(stderr, "alsastate was %d \n",thisalsastate);
} else if (ret < PERIOD) {
	// Not all audiodata could be writen: "overrun" state
	overrun=1;

	fprintf(stderr,"O%d",ret);
}; // end if


	// clear "running" marker
	running=0;

	return;

} else {
	fprintf(stderr,"Error: unknow state %d in alsaout!\n",state);
	// reinit
	state=0;
	running=0;

	// no audio in queue
	s_global.waiting_audio=1;

	// play out silence and done
	ret=snd_pcm_writei(s_global.alsahandle,silence,PERIOD);
if (ret < 0) {
	// error
   fprintf(stderr, "ALSAOUT/unknown-state: unable to write to pcm device: %s\n", snd_strerror(ret));
fprintf(stderr, "alsastate was %d \n",thisalsastate);
} else if (ret < PERIOD) {
	// Not all audiodata could be writen: "overrun" state
	overrun=1;

	fprintf(stderr,"O%d",ret);
}; // end if


	// unlock PTT lockfile
	g_global.sending=0;

	return;
}; // end else - if


// lock PTT lockfile (just be sure)
g_global.sending=1;


/// write data to buffer
// 20 ms of audio at 48000 Khz samples = 960 samples
p_in=&s_global.buffer_audio[s_global.ptr_a_read];
p_out=(int16_t *)alsaoutbuffer;

for (loop=0; loop<PERIOD; loop++) {
	// copy audio 2 times (left channel and right channel)
	*p_out=*p_in;
	// move up "out" pointer
	p_out++;

	*p_out=*p_in;
	// move up "out" pointer
	p_out++;

	// move up "in" pointer
	p_in++;

	if (p_in >= p_audioafterend) {
		// move pointer to beginning of 
		p_in=s_global.buffer_audio;
	}; // end if
}; // end for

// move up buffer
nextbuffer=s_global.ptr_a_read+PERIOD;

// counter wrapped end of buffer
if (nextbuffer >= BUFFERSIZE_AUDIO) {
	nextbuffer -= BUFFERSIZE_AUDIO;
}; 
s_global.ptr_a_read=nextbuffer;

// write audio to capture buffer, write 960 samples
ret=snd_pcm_writei(s_global.alsahandle,alsaoutbuffer,PERIOD);

if (ret < 0) {
	// error
   fprintf(stderr, "ALSAOUT/playaudio: unable to write to pcm device: %s\n", snd_strerror(ret));
fprintf(stderr, "alsastate was %d \n",thisalsastate);
} else if (ret < PERIOD) {
	// Not all audiodata could be writen: "overrun" state
	overrun=1;

	fprintf(stderr,"O%d",ret);
}; // end if


running=0;
return;
}; // end function funct_alsaout
