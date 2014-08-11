#include <DS1307new.h>

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
#define ALARM_LED_MAX 255
uint8_t A_LED_state = 0;
uint8_t A_LED_level = 0;
unsigned long start_toggle;

void toggle_a_led()
{
  A_LED_state = !A_LED_state;
  start_toggle = millis();
}

void mon_cmd(void)
{
  mon_skip_space();
  if ( *mon_curr == '\0' )
    return;
  if ( mon_check('?') )
    mon_help();
  else if ( mon_check('h') )
    mon_help();
  else if ( mon_check('s') )
    mon_set_date_time();
  else if ( mon_check('i') )
    mon_info();
  else if ( mon_check('n') )
    mon_list();
  else if ( mon_check('m') )
    mem_memory();
  else if ( mon_check('d') )
    mon_dst();
  else if ( mon_check('l') )
    toggle_a_led();
  else
    ;
}

// *********************************************
// SETUP
// *********************************************
void setup()
{
  // Wait a second for the clock to get itself together
  delay(1000);

  // LEDs
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  analogWrite(RED_LED_PIN, 0);
  analogWrite(GREEN_LED_PIN, 0);
  analogWrite(BLUE_LED_PIN, 0);
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
// LED flashing
// *********************************************
uint8_t LED_state = 0;
uint32_t LED_next_change = 0L;
uint32_t LED_on_time = 100L;
uint32_t LED_off_time = 1000L;

void LED_flashing(void)
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

  // Alarm LED
  if (!A_LED_state && ! A_LED_level)
    return;
  else if (A_LED_state && A_LED_level == ALARM_LED_MAX)
    return;

  long level = ((millis() - start_toggle) * ALARM_LED_MAX) / ALARM_TOGGLE_TIME;
  if (!A_LED_state)
    level = ALARM_LED_MAX - level;

  A_LED_level = constrain(level, 0, ALARM_LED_MAX);
  analogWrite(GREEN_LED_PIN, A_LED_level);
}

// *********************************************
// MAIN (LOOP)
// *********************************************
void loop()
{
  LED_flashing();

  if ( RTC.isPresent() == 0 )
    LED_off_time = 100L;                // fast flashing if device is not available

  if ( Serial.available() )
  {
    mon_handler();
  }

  // Actual alarm clock bit:
  RTC.getTime();
  if (RTC.hour == ALARM_HOUR && !A_LED_state)
    toggle_a_led();
  else if (RTC.hour != ALARM_HOUR && A_LED_state) 
    toggle_a_led();
  delay(50);

  if (!digitalRead(SW_1_PIN))
     Serial.println("Switch 1 pressed");

  if (!digitalRead(SW_2_PIN))
     Serial.println("Switch 2 pressed");
}
