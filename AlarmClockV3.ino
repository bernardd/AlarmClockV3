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
		{RED_LED_PIN, 0, 0, 0},
		{GREEN_LED_PIN, 0, 0, 0},
		{BLUE_LED_PIN, 0, 0, 0}
	};

Button B[2] =
	{
		{SW_1_PIN, 0, false, 0},
		{SW_2_PIN, 0, false, 0}
	};

void set_LED(Colour c, byte level, int period)
{
	if (L[c].target != level) {
		L[c].target = level;
		L[c].changeStartTime = millis();
		L[c].changeStartLevel = L[c].level;
		L[c].period = period;
	}
}

State state = IDLE;

// *********************************************
// SETUP
// *********************************************
void setup()
{
	// Wait a second for the clock to get itself together
	delay(1000);

	// LEDs
	for (int i=0; i<NUM_COLOURS; i++) {
		pinMode(L[i].pin, OUTPUT);
		analogWrite(L[i].pin, 0);
	}

	pinMode(STATUS_LED_PIN, OUTPUT);
	digitalWrite(STATUS_LED_PIN, 0);

	// Switches
	pinMode(SW_1_PIN, INPUT_PULLUP);
	pinMode(SW_2_PIN, INPUT_PULLUP);
	pinMode(SW_GND_PIN, OUTPUT);
	digitalWrite(SW_GND_PIN, 0);

	// Serial
	Serial.begin(9600);
	Serial.println("DS1307 Monitor (enable LF/CR, type 'h' for help)");
	Serial.println();
}

// *********************************************
// Status LED flashing
// *********************************************
uint8_t LED_state = 0;
uint32_t LED_next_change = 0L;
uint32_t LED_on_time = 100L;
uint32_t LED_off_time = 1000L;

void flashStatusLED()
{
	// Status LED
	if ( LED_next_change < millis() )
	{
		if ( LED_state == 0 )
		{
			LED_next_change = millis() + LED_on_time;
			LED_state = 1;
		}
		else
		{
			LED_next_change = millis() + LED_off_time;
			LED_state = 0;
		}
		digitalWrite(STATUS_LED_PIN, LED_state);
	}
}

void updateLEDs(void)
{
	flashStatusLED();

	unsigned long now = millis();
	for (int i=0; i<NUM_COLOURS; i++) {
		LED *l = &L[i];
		if (l->target == l->level)
			continue;

		// If we've overshot without hitting the exact target due to rounding or
		// timing, fix it immediately.
		if (l->changeStartTime + l->period > now) {
			l->level = l->target;
			analogWrite(l->pin, l->level);
			continue;
		}

		long change = l->target - l->changeStartLevel;
		long level = l->changeStartLevel + (change * ((float)l->period / (float)(now - l->changeStartTime)));
		l->level = constrain(level, 0, LED_MAX);
		analogWrite(l->pin, l->level);
	}
}

// *********************************************
// Buttons
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
	if ( !RTC.isPresent() )
		LED_off_time = 100L;                // fast flashing if device is not available

	updateLEDs();

	if ( Serial.available() )
		mon_handler();

	// Actual alarm clock bit:
	RTC.getTime();
	if (RTC.hour == ALARM_HOUR)
		set_LED(ALARM, LED_MAX, ALARM_TOGGLE_TIME);
	else if (RTC.hour != ALARM_HOUR)
		set_LED(ALARM, 0, ALARM_TOGGLE_TIME);

	for (int i=0; i<2; i++)
		updateButton(i);
	handleButtons();
}
