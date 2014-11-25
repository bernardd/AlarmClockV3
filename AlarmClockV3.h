#ifndef _ALARMCLOCKV3_H_
#define _ALARMCLOCKV3_H_

typedef enum {
	RED = 0,
	GREEN = 1,
	BLUE = 2,
	STATUS = 3,
	NUM_COLOURS = 4
} Colour;

typedef struct {
	byte pin;
	byte level; // Current level
	int target; // Target level
	unsigned long changeStartTime; // Time we started the change or last time we toggled the flash
	int changeStartLevel; // Level prior to starting the change
	unsigned long period; // Period of level change or flash interval if we're flashing (light will be on for period then off for period)
	bool flash;
	int flashCount; // -1 to flash forever
	bool isAnalog;
} LED;

typedef struct {
	byte pin;
	unsigned long startPressed;
	bool pressHandled;
	unsigned long lastPress;
} Button;

typedef struct {
	byte h;
	byte m;
} Alarm;

typedef enum {
	IDLE,
	SET_ALARM_H,
	SET_ALARM_M,
	SET_TIME_H,
	SET_TIME_M
} State;

#endif // _ALARMCLOCKV3_H_
