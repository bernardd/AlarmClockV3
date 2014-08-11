#ifndef _ALARMCLOCKV3_H_
#define _ALARMCLOCKV3_H_

typedef enum {
	RED = 0,
	GREEN = 1,
	BLUE = 2,
	NUM_COLOURS = 3
} Colour;

typedef struct {
	byte pin;
	byte level; // Current level
	byte target; // Target level
	long changeStart; // Time we started the change
	int period; // Period of level change
} LED;

#endif // _ALARMCLOCKV3_H_
