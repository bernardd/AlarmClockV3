#include <DS1307new.h>
#include "AlarmClockV3.h"

void mon_handler();

// *********************************************
// Alarm LED toggling
// *********************************************
#define RED_LED_PIN 3
#define GREEN_LED_PIN 6
#define BLUE_LED_PIN 5
#define STATUS_LED_PIN 13

#define SW_1_PIN 15
#define SW_2_PIN 16
#define SW_GND_PIN 17

#define STD_PRESS_TIME 300
#define SET_TOGGLE_TIME 2000
#define ALARM_TOGGLE_TIME 2000
#define STATE_FADE_TIME 100
#define TIME_FLASH_PERIOD 250
#define LED_MAX 255
#define MINUTE_DIVISOR 4

#define ALARM_SET_BUTTON 0
#define TIME_SET_BUTTON 1

#define ALARM GREEN

LED L[NUM_COLOURS] =
	{
		{RED_LED_PIN, 0, 0, 0, 0, 0, false, 0, true},
		{GREEN_LED_PIN, 0, 0, 0, 0, 0, false, 0, true},
		{BLUE_LED_PIN, 0, 0, 0, 0, 0, false, 0, true},
		{STATUS_LED_PIN, 0, 0, 0, 0, 1000, true, -1, false}
	};

Button B[2] =
	{
		{SW_1_PIN, 0, false, 0},
		{SW_2_PIN, 0, false, 0}
	};

void updateLED(LED* l)
{
	if (l->isAnalog)
		analogWrite(l->pin, l->level);
	else
		digitalWrite(l->pin, l->level);
}

void set_LED(Colour c, byte level, int period)
{
	L[c].flash = false;
	if (L[c].target != level) {
		L[c].target = level;
		L[c].changeStartTime = millis();
		L[c].changeStartLevel = L[c].level;
		L[c].period = period;
	}
}

void flash_LED(Colour c, byte level, unsigned long period, unsigned int count)
{
	if (count != 0) {
		L[c].flash = true;
		L[c].target = level;
	} else {
		L[c].flash = false;
		L[c].target = 0;
	}
	L[c].level = 0;
	L[c].flashCount = count;
	L[c].period = period;
	L[c].changeStartTime = millis();
	updateLED(&L[c]);
}

State state = IDLE;

Alarm alarm;

unsigned int wrapHour(unsigned int h)
{
	while (h > 23) h -= 24;
	return h;
}
unsigned int wrapMin(unsigned int m)
{
	while (m > 59) m -= 60;
	return m;
}

// *********************************************
// SETUP
// *********************************************
void setup()
{
	Serial.begin(9600);
	// Wait a second for the clock to get itself together
	delay(1000);

	// LEDs
	for (int i=0; i<NUM_COLOURS; i++) {
		pinMode(L[i].pin, OUTPUT);
		updateLED(&L[i]);
	}

	if ( !RTC.isPresent() )
		flash_LED(STATUS, 1, 100, -1);                // fast flashing if device is not available
	else
		flash_LED(STATUS, 1, 1000, -1);

	// Switches
	pinMode(SW_1_PIN, INPUT_PULLUP);
	pinMode(SW_2_PIN, INPUT_PULLUP);
	pinMode(SW_GND_PIN, OUTPUT);
	digitalWrite(SW_GND_PIN, 0);

	// Load saved alarm time
	loadAlarm();

	// Serial
	Serial.println("DS1307 Monitor (enable LF/CR, type 'h' for help)");
	Serial.println();
}

void updateLEDs(void)
{
	unsigned long now = millis();
	for (int i=0; i<NUM_COLOURS; i++) {
		LED *l = &L[i];

		if (l->flash) {
			if (l->changeStartTime + l->period > millis())
				continue;

			l->changeStartTime += l->period;
			if (l->level == l->target) {
				l->level = 0;
				if (l->flashCount != -1 && --(l->flashCount) == 0) {
					l->target = 0;
					l->flash = false;
				}
			} else {
				l->level = l->target;
			}

			updateLED(l);

		} else {
			if (l->target == l->level)
				continue;

			// If we've overshot without hitting the exact target due to rounding or
			// timing, fix it immediately.
			if (now > l->changeStartTime + l->period) {
				l->level = l->target;
				updateLED(l);
				continue;
			}

			long change = l->target - l->changeStartLevel;
			long level = l->changeStartLevel + (long)((float)change * ((float)(now - l->changeStartTime) / (float)l->period));
			l->level = constrain(level, 0, LED_MAX);
			updateLED(l);
		}
	}
}

// *********************************************
// Buttons/State
// *********************************************

void saveAlarm()
{
  RTC.setRAM(0, (uint8_t *)&alarm, sizeof(alarm));
}

void loadAlarm()
{
  RTC.getRAM(0, (uint8_t *)&alarm, sizeof(alarm));
}


// *********************************************
// Buttons/State
// *********************************************

void updateButton(byte index)
{
	Button *b = &B[index];
	if (digitalRead(b->pin)) {
		// Button is UP
		if (b->startPressed)
			b->lastPress = millis() - b->startPressed;
		b->startPressed = 0;
	} else if (!b->startPressed) {
		// Button is newly DOWN
		b->pressHandled = false;
		b->startPressed = millis();
	}
}

unsigned long timePressed(byte index)
{
	if (!B[index].pressHandled && B[index].startPressed)
		return millis() - B[index].startPressed;
	return 0;
}

void setState(State newState)
{
	Serial.print("New State: ");
	Serial.println(newState);
	state = newState;
	switch (state) {
		case IDLE:
			set_LED(RED, 0, STATE_FADE_TIME);
			set_LED(BLUE, 0, STATE_FADE_TIME);
			break;
		case SET_ALARM_H:
			set_LED(RED, LED_MAX, STATE_FADE_TIME);
			Serial.println(alarm.h);
			flash_LED(BLUE, LED_MAX, TIME_FLASH_PERIOD, alarm.h);
			break;
		case SET_ALARM_M:
			set_LED(RED, LED_MAX/MINUTE_DIVISOR, STATE_FADE_TIME);
			Serial.println(alarm.m);
			flash_LED(BLUE, LED_MAX/MINUTE_DIVISOR, TIME_FLASH_PERIOD, alarm.m / 10);
			break;
		case SET_TIME_H:
			flash_LED(RED, LED_MAX, TIME_FLASH_PERIOD, RTC.hour);
			Serial.println(RTC.hour);
			set_LED(BLUE, LED_MAX, STATE_FADE_TIME);
			break;
		case SET_TIME_M:
			flash_LED(RED, LED_MAX/MINUTE_DIVISOR, TIME_FLASH_PERIOD, RTC.minute / 10);
			Serial.println(RTC.minute);
			set_LED(BLUE, LED_MAX/MINUTE_DIVISOR, STATE_FADE_TIME);
			break;
	}
	B[0].pressHandled = B[1].pressHandled = true;
}

void checkNextState(State nextState)
{
	if (timePressed(TIME_SET_BUTTON) > STD_PRESS_TIME && timePressed(ALARM_SET_BUTTON) == 0)
		setState(nextState);
}

bool checkAdjust()
{
	if (timePressed(ALARM_SET_BUTTON) > STD_PRESS_TIME && timePressed(TIME_SET_BUTTON) == 0) {
		B[0].pressHandled = B[1].pressHandled = true;
		return true;
	}
	return false;
}

bool checkRefreshState(State thisState)
{
	if (timePressed(ALARM_SET_BUTTON) > SET_TOGGLE_TIME && timePressed(TIME_SET_BUTTON) > SET_TOGGLE_TIME) {
		setState(thisState);
		return true;
	}
	return false;
}

void idle()
{
	if (timePressed(ALARM_SET_BUTTON) > SET_TOGGLE_TIME && timePressed(TIME_SET_BUTTON) == 0)
		setState(SET_ALARM_H);
	else if (timePressed(TIME_SET_BUTTON) > SET_TOGGLE_TIME && timePressed(ALARM_SET_BUTTON) == 0)
		setState(SET_TIME_H);
}

void setAlarmHour()
{
	if (checkRefreshState(SET_ALARM_H)) return;

	if (checkAdjust()) {
		Serial.print("Setting alarm hour ");
		if (++alarm.h > 23)
			alarm.h = 0;
		Serial.println(alarm.h);
		flash_LED(BLUE, LED_MAX, TIME_FLASH_PERIOD, 1);
		saveAlarm();
		return;
	}

	checkNextState(SET_ALARM_M);
}

void setAlarmMinute()
{
	if (checkRefreshState(SET_ALARM_M)) return;

	if (checkAdjust()) {
		Serial.print("Setting alarm minute ");
		if ((alarm.m += 10) > 50)
			alarm.m = 0;
		Serial.println(alarm.m);
		flash_LED(BLUE, LED_MAX, TIME_FLASH_PERIOD, 1);
		saveAlarm();
		return;
	}

	checkNextState(IDLE);
}

void writeTime()
{
	RTC.setTime();
	RTC.startClock();
	RTC.getTime();
}

void setTimeHour()
{
	if (checkRefreshState(SET_TIME_H)) return;

	if (checkAdjust()) {
		Serial.print("Setting clock hour ");
		if (++RTC.hour > 23)
			RTC.hour = 0;
		writeTime();
		flash_LED(RED, LED_MAX, TIME_FLASH_PERIOD, 1);
		Serial.println(RTC.hour);
		return;
	}

	checkNextState(SET_TIME_M);
}

void setTimeMinute()
{
	if (checkRefreshState(SET_TIME_M)) return;

	if (checkAdjust()) {
		Serial.print("Setting clock minute ");
		int over = RTC.minute % 10;
		RTC.minute = (RTC.minute - over) + 10;
		if (RTC.minute > 50)
			RTC.minute = 0;
		writeTime();
		flash_LED(RED, LED_MAX, TIME_FLASH_PERIOD, 1);
		Serial.println(RTC.minute);
		return;
	}

	checkNextState(IDLE);
}

void handleButtons()
{
	switch (state) {
		case IDLE:
			idle();
			break;
		case SET_ALARM_H:
			setAlarmHour();
			break;
		case SET_ALARM_M:
			setAlarmMinute();
			break;
		case SET_TIME_H:
			setTimeHour();
			break;
		case SET_TIME_M:
			setTimeMinute();
			break;
	}
}

// *********************************************
// MAIN (LOOP)
// *********************************************
void loop()
{
	updateLEDs();

	if ( Serial.available() )
		mon_handler();

	// Actual alarm clock bit:
	RTC.getTime();
	if ((RTC.hour == alarm.h && RTC.minute >= alarm.m) ||
		 (RTC.hour == wrapHour(alarm.h+1) && RTC.minute < alarm.m))
		set_LED(ALARM, LED_MAX, ALARM_TOGGLE_TIME);
	else
		set_LED(ALARM, 0, ALARM_TOGGLE_TIME);


	for (int i=0; i<2; i++)
		updateButton(i);
	handleButtons();
}
