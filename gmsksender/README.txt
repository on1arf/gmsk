***************************************************
** GMSK sender:  demonstration code and educational toolkit


* About this project:

GMSK, or "Gaussian Minimal Shift Keying" is the modulation system used by
a number of digital communication systems; among others GSM (mobile phone
system) and the ham D-STAR system.

For most hams, GMSK is still a technology scrouded in a cloud of mystery;
partly because it is not part required knowledge for radio-amateur
examens, but also because there are not easy tools to allow hams to
experiment with this technology.

Althou there is open source software available that turns any computer
with a soundcard into a gmsk modem (and, as an extension, a D-STAR
terminal or repeater); using it to learn more about gmsk or digital
communication in general or about D-STAR in particular is not
easy. This is something that this software is designed to do something
about.


In addition to this, this software package serves a second purpose:

In the last one to two years, a new kind of computer-devices has made
its appeareance in our daily life: smartphones and tablet computers.
These mobile devices we carry around in our pockets are in fact
nothing less then small computers.

As a side effect of this, development boards, based on the technology
of these smartphones and tablet computers have become readily available.
At the time publication of this text, boards like the beagleboard, the
pandaboard, the hawkboard, the friendlyarm mini2440 and others have
filled the space between -on one side- the "controller" boards (like
the arduino, PÏC controllers, MBED and others) and -on the other side-
the plugcomputers (sheevaplug, guruplug, ...) or "mini PC" (like
ALIX computers).

These board have the additional advantage that they are powerfull enough
to run a "full blown" linux distribution; making software developement
on them very much like developement on a normal linux box.
Therefor, they have the potential to be used for quite a number of
ham-related projects, ranging from repeater or parrot controllers,
over packet or APRS nodes, as gmsk or other "digital mode" modem, to
calculation backends for SDR radios.

This project is therefor also intended as demostration that these boards
can be used as a gmsk modem for D-STAR or other Digital Voice projects.




* The objectives.

In total, this project has three different objectives:

1/
As an education tool, this project is intented to be used alongside
information that will be posted on the internet (in particular, my blog).
The basic goal is to provide to allow people to learn about gmsk,
about digital communication and about D-STAR.

Question that will be asked are:
- how does gmsk work? how is a gmsk signal transmitted or received? How
is it demodulated.

- what does a D-STAR gmsk stream look like? What information is actually
carried inside a D-STAR gmsk stream?
Also, what measures are taken to help protect certain information
inside the gmsk stream against transmission-errors?

- What are the problems related to the reception and transmission of a
gmsk stream? One of the main issues in this deals with syncronisation:
how does a  receiver know where what information in the gmsk is located?


The programming code is not only intended for "demostration" use, it
also allows people to experiment with it.
The goal is that whoever who want to try out ideas about how to
make gmsk reception or transmision can just hack the source-code and
give it a ride.


2/ Appart from being an education tool, it IS a fully functional gmsk
sender. If used on a computer or developement board that is attached
to a normal FM transceiver via its "9K6 packet" port, it
will send out any file as a GMSK or D-STAR stream.

This means that it can form the low-level interface for D-STAR or
other DV-applications. Some possible applications are:

- D-RATS "store and forward" system
- D-STAR voice-announcement system
- DPRS terminal

It can also be used as a gmsk transmitter for network-protocols that
go beyond the basic D-STAR specification:
- 4.8 Kbps Digital Data transmission on 2 meter or 70 cm DV channels
- codec2 based DV transmissions.



3/
As a "developement board demonstration platform", is tries to deal with
the concequences of porting a GMSK modem to these platforms.
Some of these boards (like friendlyarm mini2440) have limited resources
(like the lack of a Floating-point unit in the CPU) which has implications
on how the code is written.


The software is therefor also intended to experiment with the different
kinds of developement boards.




* Running the code:
See the document "RUNNINGIT.txt" for more information about this.



73
Kristoff - ON1ARF
