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


void * funct_alsaout (void * c_globaldatain) {

// vars
int state=0;

int16_t * p_audioafterend;
int silencecounter=0;
int silencesize=0;

// local vars
int nextbuffer;
char alsaoutbuffer[3840]; // buffer is 960 samples (20 ms at 48Khz)
										// multiplied by 4: stereo and 16 bit size
char silence[3840]; // buffer containing all 0 (silence)
int16_t * p_in; // pointer to audio (global.buffer_audio)
int16_t * p_out; // pointer to audio "out" (alsabuffer)
int loop;
int ret;
int queuesize;
int thisalsastate;


char retmsg[INITALSARETMSGSIZE];


// import global data
c_globaldatastr * p_c_global;
s_globaldatastr * p_s_global;
g_globaldatastr * p_g_global;

p_c_global=(c_globaldatastr *) c_globaldatain;

p_s_global=p_c_global->p_s_global;
p_g_global=p_c_global->p_g_global;


// flag to syncronise between sender and receiver. Disable decoding when sending
p_g_global->sending=0;

// no audio in queue
p_s_global->waiting_audio=1;

//note: this points just AFTER the end of the audio-buffer
p_audioafterend = &p_s_global->buffer_audio[BUFFERSIZE_AUDIO];

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


// if alsa card not fully configured, wait here
while (!p_s_global->alsaready) {
	// try again in 5 ms
	usleep(5000);
}; // end while

/// ENDLESS LOOP

while (FOREVER) {
	int alsanotok;
	int sendsilence;

	// check ALSA state.
	thisalsastate = snd_pcm_state(p_s_global->alsahandle);

	alsanotok=1;
	// wait for alsa to get ready
	while (alsanotok) {
		if (thisalsastate == SND_PCM_STATE_OPEN) {
		// alsa card still in "open" state. Wait some more to give it some time to get into "running" or "prepared" mode
			fprintf(stderr, "ALSAOUT: ALSA still in \"OPEN\" STATE! \n");

			// sleep 2 ms and try again
			usleep(2000);
		} else if (thisalsastate == SND_PCM_STATE_SETUP) {
		// alsa card still in "setup" state. Wait some more to give it some time to get into "running" or "prepated" mode
			fprintf(stderr, "ALSAOUT: ALSA still in \"SETUP\" STATE! \n");

			// sleep 2 ms and try again
			usleep(2000);
		} else {
			// ALSA is OK
			alsanotok=0;

			if (thisalsastate == SND_PCM_STATE_XRUN) {
				// if state is underrun, try "snd_pcm_prepate"
				ret = snd_pcm_prepare(p_s_global->alsahandle);

				if (ret < 0) {
					fprintf(stderr, "ALSAOUT: underrun recovery failed! \n");
				} else {
					fprintf(stderr, "ALSAOUT: underrun recovery done! \n");
				}; // end else - if
			}; // end if (state XRUN)
		}; // end else - elsif - elsif - if (alsa state)

	}; // end while (ALSA is OK)


	// is there audiodata in the queue?


	queuesize = p_s_global->ptr_a_fillup - p_s_global->ptr_a_read - 1;

	// wrapping of counters around end-of-buffer
	if (queuesize < 0) {
		queuesize +=  BUFFERSIZE_AUDIO;
	}; // end if


	// STATE of modem
	// state = 0: waiting for audio
	// state = 1: begin silence
	// state = 2: playout out
	// state = 3: end silence

	// set "sendsilence" to 0. Will be set to 1 if silence need to be
	// send, otherwize, (if still set to 0 at end of all check)
	// voice data will be send
	sendsilence=0;

	if (state == 0)  {
	// state 0: waiting for at least AUDIOMINBUFFER frames received

		if (queuesize < AUDIOMINBUFFER) {
			sendsilence=1;

		} else if (p_s_global->silencebegin) {
		// sufficient audio in buffer. Start sending it out
		// set new state:
		// if begin-silence to be played: go to state 1
		// if no begin-silence: go directly to state 2
			state=1;
			silencesize=p_s_global->silencebegin*5; // function runs every 20 ms (silence audio-frame = 960 octets), so multiply by 50 times / second; however, silence counter is in tenth of second)
			silencecounter=0;

			// lock PTT lockfile
			p_g_global->sending=1;

			// play out one sample of silence before going to next 20ms timeslot
			sendsilence=1;

		} else {
			state=2;
		}; // end else - if

		// audio in queue
		// go on to bottom of all "state" checks
		p_s_global->waiting_audio=0;


	} else if (state == 1) {
		// state = 1: play out silence
		silencecounter++;

		// all silence send?
		if (silencecounter >= silencesize) {
			state = 2;
		}; // end if

		// lock PTT lockfile (just to be sure)
		p_g_global->sending=1;

		// play out silence and done
		sendsilence=1;

	} else if (state == 2) {
		// state = 2: play out data

		if (queuesize < PERIOD ) {
			// no data anymore in buffer

			// determine next state
			if (p_s_global->silenceend) {
				// silence to be added at the end
				state=3;
				silencesize=p_s_global->silenceend*5; // function is started 50 times / second (silence value is tenth of second)
				silencecounter=0;

				p_g_global->sending=1;

			} else {
				// no end silence, reset state
				state=0;

				// no audio in queue
				p_s_global->waiting_audio=1;

				// unlock PTT lockfile
				p_g_global->sending=0;

			}; // end else - if


			// finally, play out silence
			sendsilence=1;
		}; // end if

		// if there IS audio to be send, go on below to play out audio

	} else if (state == 3) {
		// state = 3: play out silence
		silencecounter++;

		// all silence send?
		if (silencecounter >= silencesize) {
			// reset state
			state = 0;

			// no audio in queue
			p_s_global->waiting_audio=1;

			// unlock PTT lockfile
			p_g_global->sending=0;

		}; // end if

		// play out silence and done
		sendsilence=1;

	} else {
		// unknown value for "state"
		fprintf(stderr,"Error: unknow state %d in alsaout!\n",state);

		// reinit
		state=0;

		// no audio in queue
		p_s_global->waiting_audio=1;

		// play out silence and done
		sendsilence=1;

		// unlock PTT lockfile
		p_g_global->sending=0;

	}; // end else - if


	// end of processing "state"

	if (sendsilence) {

		ret=snd_pcm_wait(p_s_global->alsahandle,100);
		if (!ret) {
			fprintf(stderr,"WARNING, WAIT DID NOT COME BACK FOR 100 ms! \n");
		}; // end if
		ret=snd_pcm_writei(p_s_global->alsahandle,silence,PERIOD);

		if (ret < 0) {
				fprintf(stderr, "ALSAOUT SILENCE state %d: unable to write to pcm device: %s\n", state,snd_strerror(ret));
			// break out
			continue;
		}; // end if
	} else {
		// this part of the function is reached when audio from the queue
		// needs to be played out

		// lock PTT lockfile (just be sure)
		p_g_global->sending=1;


		/// write data to buffer
		// 20 ms of audio at 48000 Khz samples = 960 samples
		p_in=&p_s_global->buffer_audio[p_s_global->ptr_a_read];
		p_out=(int16_t *)alsaoutbuffer;

		for (loop=0; loop<PERIOD; loop++) {
			// copy audio 2 times (interleave left channel and right channel)
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
				p_in=p_s_global->buffer_audio;
			}; // end if

		}; // end for

		// move up buffer
		nextbuffer=p_s_global->ptr_a_read+PERIOD;
		// counter wrapped end of buffer
		if (nextbuffer >= BUFFERSIZE_AUDIO) {
			nextbuffer -= BUFFERSIZE_AUDIO;
		}; 
		p_s_global->ptr_a_read=nextbuffer;


		// write audio to capture buffer, write 960 samples
		ret=snd_pcm_wait(p_s_global->alsahandle,100);
		if (!ret) {
			fprintf(stderr,"WARNING, WAIT DID NOT COME BACK FOR 100 ms! \n");
		}; // end if
		ret=snd_pcm_writei(p_s_global->alsahandle,alsaoutbuffer,PERIOD);

		if (ret < 0) {
		// error
			fprintf(stderr, "ALSAOUT/playaudio/voice state %d: unable to write to pcm device: %s\n", state,snd_strerror(ret));
			// break out
			continue;
		}; // end if

	}; // end else (voice) - if (silence)

}; // end while forever


// we broke out of the "forever" loop. This should never happen

fprintf(stderr,"Error: END OF FUNCTION OF S_ALSAOUT REACHED. SHOULD NOT HAPPEN!\n");

return(NULL);
}; // end function funct_alsaout
