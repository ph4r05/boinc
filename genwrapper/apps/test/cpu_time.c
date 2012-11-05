#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#include <math.h>
#define usleep(usec) (Sleep ((usec) / 1000), 0)
#endif
#define MAX_I 655350

int main(int argc, char **argv) {
	int i,i2;
	int param=0;
	double cos_i;
	if (argc>1)
	    param=strtol(argv[1], NULL, 10);
	if (param==0)
	    param=10;
	for (i2=0; i2<param; i2++) {
		for (i=0; i<MAX_I; i++) {
			cos_i = cos(i);
		}
		printf("%d. done...\n", i2);
	}
}

