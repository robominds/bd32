
#include <stdio.h>
#include "subs.h"

void setvect( unsigned Vector, void (*NewVector)()) {
        printf("Not sure what to do here for setvect()\n");
}

void (*getvect(void)) (unsigned Vector) {
        printf("Not sure what to do here for getvect()\n");
	return(0);
}

void enable(void) {
        printf("Not sure what to do here for enable()\n");
}

void disable(void) {
        printf("Not sure what to do here for disable()\n");
}

int inportb(int port) {
        printf("Not sure what to do here for outportb()\n");
	return(0);
}

void outportb(int port, int val) {
        printf("Not sure what to do here for outportb()\n");
}

