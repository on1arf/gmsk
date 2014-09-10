C2GMSK MODEM: API

API version: 20130314



The Application Programming Interface


* Introduction

Please find here the 20130314 release of the C2GMSK modem API.

The goal is to make the VHF/UHF modem more accessable and allow it to be
used inside other applications. Possible targets for this are FreeDV, the
standalone gmskmodem for embedded unix systems and TheLinkBox.

Appart of the basic API calls, the package also includes some support
functions, trying to make live a little bit more easy for the application
programmer. They mainly deal with creating/initialising certain structures
needed by the API and processing the return-messages received from the API.

Also added are  two test / demo-applications have been added:
testapi_modulate and testapi_demodulate.

In this verson of the API, there is no formal seperation between the
"visible" API-calls and the functions used internally in the package.



** General setup:

The API calls can be divided into four groups:

1/ Initialisation:
- c2gmsk_param_init: a support function that initiases the two structure used by API
- c2gmsk_sess_new: create a new c2gmsk API-session.
Multiple c2gmsk API-sessions can exist at the same time and all will run
independently from eachother

2/ For the modulation chain, there are 4 API calls .
- c2gmsk_mod_start: generate the "sync / version-id" preamble stream C2GMSK
stream
- c2gmsk_mod_voice1400: process a 56 bit codec2 frame (40 ms).
- c2gmsk_mod_voice1400_end: generate the postamble of the C2GMSK stream.
- c2gmsk_mod_audioflush: egt any audio that might still be present in the
audio-queue of the API

3/ For the "demodulation" chain: there is one single API call:
- c2gmsk_demod


4/ Support functions

- for initialisation:
c2gmsk_param_init

- for processing message from the message-queue:

c2gmsk_msgchain_search: retrieve next message from the message-queue,
from ANY time

c2gmsk_msgchain_search_tod: retrieve next message from the message-queue
of one particular Type-of-Data.
(usefull if you are only interested in (say) modulated audio and you do
not care about debug-information and so on


- "checksign"

The API depends heavily on data-structures that are passed from one piece
of code to other function. In some cases, data is passed via "void *" pointer
structures, which means that the compiler is not able to check if the data
passed from one function to another is indeed of the correct data-type.

To maintain overal robustness of the code, all structures -both used
internally and externally- contain a 4-character "signature" at the beginning
of the structure.
This allows all functions to verify that the data -as received- is indeed of
the data-type as expected.

The following APU-support functions are available
checksign_param
checksign_chain
checksign_msg


- c2gmsk_msgdecode:
These are a number of support-functions that help to decode and extract data
from a received API message-queue messages. The following functions are
available:
c2gmsk_msgdecode_printbit
c2gmsk_msgdecode_numeric
c2gmsk_msgdecode_c2
c2gmsk_msgdecode_pcm48k + c2gmsk_msgdecode_pcm48k_p
c2gmsk_msgdecode_gmsk + c2gmsk_msgdecode_gmsk_p

- "c2gmsk_getapiversion"
As the name implies.

