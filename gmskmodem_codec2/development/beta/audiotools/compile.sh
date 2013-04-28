
gcc -O3 -Wall -o audiocap_c2udp_keyin_multirate audiocap_c2udp_keyin_multirate.c -lportaudio -lcodec2 -lsamplerate -lpthread

gcc -O3 -Wall -o audioplay_multirate_callback audioplay_multirate_callback.c  -lportaudio -lcodec2 -lsamplerate
