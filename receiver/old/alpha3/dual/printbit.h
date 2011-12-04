// function printbit
void printbit(int bit, int numgroup, int grouplines, char text) {

// declare and init vars
static int cr=0;
static int lf=0;
static int lines=0;

// bit=-1 -> reset all counters
if (bit == -1) {
	cr=0;lf=0;lines=0;
	return;
}; // end if

if (bit) {
	putchar('1');
} else {
	putchar('0');
}; // end else - if

if (text) {
	putchar(text);
}; // end if

cr++;

if (cr >= 8) {
	lf++;
	if (lf >= numgroup) {
		putchar(0x0a); // 0x0A = LineFeed
		lf=0; cr=0;

		lines++;

		if ((grouplines > 0) && (lines >= grouplines)) {
			// insert extra LF to group lines
			putchar(0x0a);
			lines=0;
		}
	} else {
		putchar(0x20); // 0x20 = ' '
		cr=0;
	}; // else else - if
}; // end if

}; // end function printbit

