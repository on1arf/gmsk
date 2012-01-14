// global vars

// global defs
#define FOREVER 1
#define RETMSGSIZE 200                  // maximum size of return message of parsecli

// global data structure definition

typedef struct {
	int verboselevel;
	char * lockfilename;
	char * serialdevicename;
	int invert;
} globaldatastr;

globaldatastr global;
