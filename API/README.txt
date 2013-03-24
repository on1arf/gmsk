C2GMSK MODEM: API

API version: 20130314



The Application Programming Interface


* Introduction

Please find here the 20130314 release of the C2GMSK modem API.

The goal is to make the VHF/UHF modem more accessable and allow it to be used inside other applications. Possible targets for this are FreeDV, the standalone gmskmodem for embedded unix systems and TheLinkBox.

Appart of the basic API calls, the package also includes some support functions, trying to make live a little bit more easy for the application programmer. They mainly deal with creating/initialising certain structures needed by the API and processing the return-messages received from the API.
In addition to the API itself, two test / demo-applications have been added: testapi_modulate and testapi_demodulate.

In this verson of the API, there is no formal seperation between the "visible" API-calls and the functions used internally in the package.



** General setup:

The API calls can be divided into four groups:

1/ Initialisation:
- c2gmsk_param_init: a support function that initiases the two structure used by API
- c2gmsk_sess_new: create a new c2gmsk API-session.
Multiple c2gmsk API-sessions can exist at the same time and all will run independently from eachother

2/ For the modulation chain, there are 4 API calls .
- c2gmsk_mod_start: generate the "sync / version-id" preamble stream C2GMSK stream
- c2gmsk_mod_voice1400: process a 56 bit codec2 frame (40 ms).
- c2gmsk_mod_voice1400_end: generate the postamble of the C2GMSK stream.
- c2gmsk_mod_audioflush: egt any audio that might still be present in the audio-queue of the API

3/ For the "demodulation" chain: there is one single API call:
- c2gmsk_demod


4/ Support functions for processing messages from the message queue



** Returning data
The GMSK modulation and demodulation process can return 
