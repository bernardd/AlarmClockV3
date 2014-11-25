#include <DS1307new.h>
#include "AlarmClockV3.h"

void mon_handler();

// *********************************************
// Alarm LED toggling
// *********************************************
#define ALARM_HOUR 7
#define RED_LED_PIN 3
#define GREEN_LED_PIN 6
#define BLUE_LED_PIN 5
#define STATUS_LED_PIN 13

#define SW_1_PIN 15
#define SW_2_PIN 16
#define SW_GND_PIN 17

#define SET_TOGGLE_TIME 2000
#define ALARM_TOGGLE_TIME 2000
#define STATE_FADE_TIME 100
#define LED_MAX 255

#define ALARM_SET_BUTTON 0
#define TIME_SET_BUTTON 1

#define ALARM GREEN

LED L[NUM_COLOURS] =
	{
		{RED_LED_PIN, 0, 0, 0, false, 0, true},
		{GREEN_LED_PIN, 0, 0, 0, false, 0, true},
		{BLUE_LED_PIN, 0, 0, 0, false, 0, true},
		{STATUS_LED_PIN, 0, 0, 1000, true, -1, false}
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
	L[c].flash = true;
	L[c].level = 0;
	L[c].target = level;
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

	Serial.println("Status LED flashing setup");
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
				if (l->flashCount != -1 && --(l->flashCount) == 0)
					l->flash = false;
			}
			else
				l->level = l->target;

			updateLED(l);

		} else {
			if (l->target == l->level)
				continue;

			// If we've overshot without hitting the exact target due to rounding or
			// timing, fix it immediately.
			if (now > l->changeStartTime + l->period) {
				Serial.println(now);
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
	state = newState;
	switch (state) {
		case IDLE:
			set_LED(RED, 0, STATE_FADE_TIME);
			set_LED(BLUE, 0, STATE_FADE_TIME);
			break;
		case SET_ALARM_H:
		case SET_ALARM_M:
			set_LED(RED, LED_MAX, STATE_FADE_TIME);
			set_LED(BLUE, 0, STATE_FADE_TIME);
			break;
		case SET_TIME_H:
		case SET_TIME_M:
			set_LED(RED, 0, STATE_FADE_TIME);
			set_LED(BLUE, LED_MAX, STATE_FADE_TIME);
			break;
	}
	B[0].pressHandled = B[1].pressHandled = true;
}

void idle()
{
	if (timePressed(ALARM_SET_BUTTON) > SET_TOGGLE_TIME && timePressed(TIME_SET_BUTTON) == 0)
		setState(SET_ALARM_H);
	else if (timePressed(TIME_SET_BUTTON) > SET_TOGGLE_TIME && timePressed(ALARM_SET_BUTTON) == 0)
		setState(SET_TIME_H);
}

void setAlarm()
{
	if (timePressed(ALARM_SET_BUTTON) > ALARM_TOGGLE_TIME && timePressed(TIME_SET_BUTTON) > ALARM_TOGGLE_TIME) {
		setState(IDLE);
		return;
	}

}

void setTime()
{
	if (timePressed(ALARM_SET_BUTTON) > ALARM_TOGGLE_TIME && timePressed(TIME_SET_BUTTON) > ALARM_TOGGLE_TIME) {
		setState(IDLE);
		return;
	}

}

void handleButtons()
{
	switch (state) {
		case IDLE:
			idle();
			break;
		case SET_ALARM_H:
		case SET_ALARM_M:
			setAlarm();
			break;
		case SET_TIME_H:
		case SET_TIME_M:
			setTime();
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
