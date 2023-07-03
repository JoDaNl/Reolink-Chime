// ==================================================================
// MAIN.CPP
// ==================================================================
//
// This small sketch will act as a doorbell chime controller for
// a Reolink Doorbell. It uses a basic 433Mhz receiver hooked up to
// an input-pin.
// ==================================================================

#define CHIME_ON_TIME   (600)   // milliseconds
#define CHIME_OFF_TIME  (1500)  // milliseconds
#define CHIME_REPEAT    (2)     // repeat n times

// define IO-pins

#define PIN_REVEIVER    (D1)
#define PIN_RELAY       (D2)
#define PIN_LED         (LED_BUILTIN)
#define PIN_BUTTON      (D3)

// ==================================================================

#define LOOP_DELAY      (10)    // milliseconds
#define DETECT_TIMEOUT  (300)   // milliseconds
#define LED_FLASH_TIME  (4000)  // flash led briefly every N milliseconds

#define LED_ON          (LOW)
#define LED_OFF         (HIGH)  
#define RELAY_ON        (HIGH)  
#define RELAY_OFF       (LOW)

#define EEPROM_SIZE     (4)     // nr of bytes to store in EEPROM

// ==================================================================

#include <RCSwitch.h>           // 433Mhz receiver library
#include <EEPROM.h>

// ==================================================================


static unsigned long doorbellCode = 0xB65902;
static const int nrOfvalidCodes = 5;

static RCSwitch recv433 = RCSwitch();

// reolink uses 300us protocol-timing
static const RCSwitch::Protocol reolink_protocol = { 300, {  1, 31 }, {  1,  3 }, {  3,  1 }, false };

// by default we are not in learn mode
static bool learn_mode = false;

// ==================================================================
// SETUP
// ==================================================================

void setup() 
{
  uint8_t code[4];
  uint8_t check;

  Serial.begin(115200);
  while (!Serial)
    ; // wait for Serial to become available
  delay(200);

  // Welcome message
  printf("\n");
  printf("========================\n");
  printf("     REOLINK CHIME\n");
  printf("========================\n\n");

  // led
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_OFF);

  // relay
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, RELAY_OFF);

  // 433Mhz receiver
  recv433.enableReceive(PIN_REVEIVER);
  recv433.setProtocol(reolink_protocol);

  // eeprom
  EEPROM.begin(EEPROM_SIZE); 

  // get code from eeprom. code is 24-bits, stored in an unsigned long (32-bits)
  // MS-byte is written as 5A as sanity check.
  EEPROM.get(0, doorbellCode);

  // MS-byte of code must be 0x5A...if not the code is not valid
  // and we will start in learn-mode
  if ( ( (doorbellCode & 0xFF000000) >> 24) != 0x0000005A)
  {
    learn_mode = true;
    printf("Invalid code at start-up...going into learn-mode\n");
  }
  else
  {
    printf("MS-byte of code is 0x5A so seems to be valid\n");
  }

  doorbellCode = (doorbellCode & 0x00FFFFFF);
  printf("Code from EEPROM = %lX\n",doorbellCode);

  // test-code for writing code
  // doorbellCode = 0xB65902;
  // doorbellCode = doorbellCode | 0x5A000000;
  // EEPROM.put(0, doorbellCode);
  // EEPROM.commit();

  EEPROM.end();

}


// ==================================================================
// LOOP
// ==================================================================

void loop() 
{
  unsigned long recv;
  static unsigned long time=0;
  static unsigned long prev_time=0;
  static int  cnt=0;

  bool        chime_trigger=false;
  static int  chime_repeat = 0;
  static int  chime_on_count = 0;
  static int  chime_off_count = 0;

  static int led_flash_count = LED_FLASH_TIME / LOOP_DELAY;

  // check is a code has been received
  if (recv433.available()) 
  {

    recv = recv433.getReceivedValue(); 
    recv433.resetAvailable();

    // we will chime if 
    time = millis();  
    if (recv == doorbellCode)
    {
      if (time-prev_time > 1000)
      {
        cnt = 0;
      }
      else
      {
        cnt++;
      }  

      chime_trigger = (cnt == nrOfvalidCodes);

    }

    prev_time = time;
  }

  // chime_trigger will be true for one loop iteration
  // set chime-counters to chime will do the ding-dong

  if (chime_trigger)
  {
     printf("Valid code received.\n");

     chime_repeat = CHIME_REPEAT;
     chime_on_count = CHIME_ON_TIME / LOOP_DELAY;
     chime_off_count = 0;    
  }

  // chime_repeat will down down...and chime will sound until chime_count is zero
  if (chime_repeat)
  {
    if (chime_on_count)
    {
      chime_on_count--;
      digitalWrite(PIN_RELAY, RELAY_ON);
      digitalWrite(PIN_LED, LED_ON);
      
      if (chime_on_count ==0)
      {
        chime_off_count = CHIME_OFF_TIME / LOOP_DELAY;
      }
    }

    if (chime_off_count)
    {
      chime_off_count--;
      digitalWrite(PIN_RELAY, RELAY_OFF);
      digitalWrite(PIN_LED, LED_OFF);
      
      if (chime_off_count ==0)
      {
        chime_on_count = CHIME_ON_TIME / LOOP_DELAY;
        chime_repeat--;
      }
    }

    // (p)reset flash counter after ringing
    led_flash_count = LED_FLASH_TIME / LOOP_DELAY;
  }

  else // chime is not ringing

  {
    // LED FLASH
    if (led_flash_count == 1)
    {
      digitalWrite(PIN_LED, LED_ON);
    }

    if (led_flash_count == 0)
    {
      digitalWrite(PIN_LED, LED_OFF);
      led_flash_count = LED_FLASH_TIME / LOOP_DELAY;
    }
    else
    {
      led_flash_count--;
    }

  }

  delay(LOOP_DELAY);

}

// ==================================================================
