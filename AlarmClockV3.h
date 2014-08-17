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
	unsigned long changeStartTime; // Time we started the change
	unsigned long changeStartLevel; // Level prior to starting the change
	unsigned long period; // Period of level change
} LED;

typedef struct {
	byte pin;
	unsigned long startPressed;
	bool pressHandled;
	unsigned long lastPress;
} Button;

typedef enum {
	IDLE,
	SET_ALARM_H,
	SET_ALARM_M,
	SET_TIME_H,
	SET_TIME_M
} State;

#endif // _ALARMCLOCKV3_H_
