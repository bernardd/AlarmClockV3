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

#define ALARM_TOGGLE_TIME 2000
#define LED_MAX 255

#define ALARM GREEN

LED L[NUM_COLOURS] =
			{
				{RED_LED_PIN, 0, 0, 0},
				{GREEN_LED_PIN, 0, 0, 0},
				{BLUE_LED_PIN, 0, 0, 0}
			};

void set_LED(Colour c, byte level, int period)
{
	if (L[c].target != level) {
		L[c].target = level;
		L[c].changeStart = millis();
		L[c].period = period;
	}
}

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

void flash_status_LED()
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

void LED_flashing(void)
{
	flash_status_LED();

	for (int i=0; i<NUM_COLOURS; i++) {
		if (L[i].target == L[i].level)
			continue;

	long level = ((millis() - start_toggle) * ALARM_LED_MAX) / ALARM_TOGGLE_TIME;
	if (!A_LED_state)
		level = ALARM_LED_MAX - level;

	A_LED_level = constrain(level, 0, ALARM_LED_MAX);
	analogWrite(L[i].pin, L[i].level);
	}
}

// *********************************************
// MAIN (LOOP)
// *********************************************
void loop()
{
	if ( !RTC.isPresent() )
		LED_off_time = 100L;                // fast flashing if device is not available

	LED_flashing();

	if ( Serial.available() )
		mon_handler();

	// Actual alarm clock bit:
	RTC.getTime();
	if (RTC.hour == ALARM_HOUR)
		set_LED(ALARM, LED_MAX, ALARM_TOGGLE_TIME);
	else if (RTC.hour != ALARM_HOUR)
		set_LED(ALARM, 0, ALARM_TOGGLE_TIME);

	if (!digitalRead(SW_1_PIN))
		Serial.println("Switch 1 pressed");

	if (!digitalRead(SW_2_PIN))
		Serial.println("Switch 2 pressed");
}
