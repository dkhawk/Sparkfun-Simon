/*
  6-19-2007
  Copyright Spark Fun Electronics© 2009
  Nathan Seidle
  nathan at sparkfun.com

  Simon Game ported for the ATmega168

  Generates random sequence, plays music, and displays button lights.

  Simon tones from Wikipedia

  * A (red, upper left) - 440Hz - 2.272ms - 1.136ms pulse
  * a (green, upper right, an octave higher than the upper right) - 880Hz - 1.136ms - 0.568ms pulse
  * D (blue, lower left, a perfect fourth higher than the upper left) - 587.33Hz - 1.702ms - 0.851ms pulse
  * G (yellow, lower right, a perfect fourth higher than the lower left) - 784Hz - 1.276ms - 0.638ms pulse

  The tones are close, but probably off a bit, but they sound all right.

  Timer rolls over at 256 and is reset to 131 (Timer 2 ISR) so 125 possible numbers
  131 + 31 = 162 will be button 1
  162 + 31 = 193 button 2
  193 + 31 = 224 button 3
  224 + 31 = 255 button 4

  Prescalar of 1024
  Clock = 16MHz
  15,625 clicks per second
  64us per click

  So each button has an equal chance of being added to the game sequence
  Each timer2 click is every 64us (16MHz / 1024) so the button possibility
  changes every 2ms or 500 times a second
  Not random, but no one can play the same way to within 2ms!
  The sequence changes *every* game

  The old version of SparkFun simon used an ATmega8. An ATmega8 ships with a default internal 1MHz oscillator.
  You will need to set the internal fuses	to operate at the correct external 16MHz oscillator.
  Original Fuses : avrdude -p atmega8 -P lpt1 -c stk200 -U lfuse:w:0xE1:m -U hfuse:w:0xD9:m
  Command to set to fuses to use external 16MHz : avrdude -p atmega8 -P lpt1 -c stk200 -U lfuse:w:0xEE:m -U hfuse:w:0xC9:m

  The current version of Simon uses the ATmega168. The external osciallator was removed to reduce component count.
  This version of simon relies on the internal default 1MHz osciallator. Do not set the external fuses.
*/

//#define CHIP_ATMEGA8
#define CHIP_ATMEGA168

#include <avr/io.h>
#include <avr/interrupt.h>

// #define DEBUG
#ifdef DEBUG
#include "debug.h"
#endif

#define LED_BLUE			0
#define LED_BLUE_PORT	PORTC
#define BLUE_BUTTON     (PINC & (1<<1))
#define BLUE_STATE        0b0001

#define LED_YELLOW			5
#define LED_YELLOW_PORT	PORTD
#define YELLOW_BUTTON   (PIND & (1<<6))
#define YELLOW_STATE        0b0010

#define LED_RED				3
#define LED_RED_PORT	PORTC
#define RED_BUTTON      (PINC & (1<<2))
#define RED_STATE        0b0100

#define LED_GREEN			2
#define LED_GREEN_PORT	PORTD
#define GREEN_BUTTON    (PINC & (1<<5))
#define GREEN_STATE        0b1000

#define LED_OFF	0
#define LED_ON	1

#define BUZZER1		3
#define BUZZER1_PORT	PORTD
#define BUZZER2		4
#define BUZZER2_PORT	PORTD

#define sbi(port_name, pin_number)   (port_name |= 1<<pin_number)
#define cbi(port_name, pin_number)   ((port_name) &= (uint8_t)~(1 << pin_number))

uint16_t random_byte;

uint8_t game_string[32];
uint8_t game_string_position;

volatile uint16_t global_clock;
volatile uint8_t flag;

//Define functions
//======================
uint8_t check_button(void);
void toner(uint8_t, uint16_t);
void add_to_string(void);
void play_string(void);
void play_loser(void);
void play_winner(void);

void ioinit(void);      //Initializes IO
void delay_ms(uint16_t x); //General purpose delay
void delay_us(uint16_t x);
//======================

ISR (SIG_OVERFLOW2)
{
  //Prescalar of 1024
  //Clock = 16MHz
  //15,625 clicks per second
  //64us per click

  TCNT2 = 131; //256 - 125 = 131 - Preload timer 2 for 125 clicks. Should be 8ms per ISR call
}

int main (void)
{
  uint8_t choice;
  uint8_t current_pos;
  uint16_t time_limit;

  ioinit(); //Setup IO pins and defaults

 BEGIN_GAME:

  //Display fancy LED pattern waiting for two buttons to be pressed
  //Wait for user to begin game
  while(1)
    {
      sbi(LED_RED_PORT, LED_RED);
      cbi(LED_BLUE_PORT, LED_BLUE);
      cbi(LED_GREEN_PORT, LED_GREEN);
      cbi(LED_YELLOW_PORT, LED_YELLOW);
      delay_ms(100);
      if(check_button())
        break;

      cbi(LED_RED_PORT, LED_RED);
      sbi(LED_BLUE_PORT, LED_BLUE);
      cbi(LED_GREEN_PORT, LED_GREEN);
      cbi(LED_YELLOW_PORT, LED_YELLOW);
      delay_ms(100);
      if(check_button())
        break;

      cbi(LED_RED_PORT, LED_RED);
      cbi(LED_BLUE_PORT, LED_BLUE);
      sbi(LED_GREEN_PORT, LED_GREEN);
      cbi(LED_YELLOW_PORT, LED_YELLOW);
      delay_ms(100);
      if(check_button())
        break;

      cbi(LED_RED_PORT, LED_RED);
      cbi(LED_BLUE_PORT, LED_BLUE);
      cbi(LED_GREEN_PORT, LED_GREEN);
      sbi(LED_YELLOW_PORT, LED_YELLOW);
      delay_ms(100);
      if(check_button())
        break;
    }

  //Indicate the start of game play
  sbi(LED_RED_PORT, LED_RED);
  sbi(LED_BLUE_PORT, LED_BLUE);
  sbi(LED_GREEN_PORT, LED_GREEN);
  sbi(LED_YELLOW_PORT, LED_YELLOW);
  delay_ms(1000);
  cbi(LED_RED_PORT, LED_RED);
  cbi(LED_BLUE_PORT, LED_BLUE);
  cbi(LED_GREEN_PORT, LED_GREEN);
  cbi(LED_YELLOW_PORT, LED_YELLOW);
  delay_ms(250);

  game_string_position = 0; //Start new game

  while(1)
    {
      add_to_string(); //Add the first button to the string
      play_string(); //Play the current contents of the game string back for the player

      //Wait for user to input buttons until they mess up, reach the end of the current string, or time out
      for(current_pos = 0 ; current_pos < game_string_position ; current_pos++)
        {
          //Button scanning
          //=======================================
          time_limit = 0; //Run timer while we wait for the user to push a button


          int button_state;
          int last_button = check_button();
          while(1)
            {
              button_state = check_button();
              if(button_state != 0 && last_button == button_state)
                break;
              last_button = button_state;

              delay_ms(5);

              time_limit++; //3 second time limit
              if (time_limit > 3000)
                {
                  play_loser(); //Play anoying loser tones
                  delay_ms(1000);
                  play_string();
                  goto BEGIN_GAME;
                }
            }

          // Button debounce.
          delay_ms(10);

          //Wait for user to release button
          choice = 0;
          while(choice == 0)
            {
              int button = check_button();
              if(button == 1) choice = '3'; //Lower left button "Blue"
              if(button == 2) choice = '4'; //Lower right "Yellow"
              if(button == 4) choice = '1'; //Upper left "Red"
              if(button == 8) choice = '2'; //Upper right "Green"

              toner(choice, 150); //Fire the button and play the button tone
            }

          // Debounce
          delay_ms(10);

          //=======================================
          if (choice != game_string[current_pos])
            {
              play_loser(); //Play annoying loser tones
              delay_ms(1000);
              play_string();
              goto BEGIN_GAME;
            }

        }//End user input loop

      //If user reaches the game length of X, they win!
      if (current_pos == 13)
        {
          play_winner(); //Play winner tones
          goto BEGIN_GAME;
        }

      //Otherwise, we need to wait just a hair before we play back the last string
      delay_ms(1000);
    }

  return(0);
}

void ioinit (void)
{
  //1 = output, 0 = input
  DDRB = 0b11111100; //button 2,3 on PB0,1
  DDRD = 0b00111110; //LEDs, buttons, buzzer, TX/RX
  DDRC = (1<<LED_RED) | (1<<LED_BLUE);

  PORTC = 0b00010110;  // Red and Blue Buttons
  PORTB = 0b00000011; //Enable pull-ups on buttons 2,3
  PORTD = 0b11000000; //Enable pull-up on button 0,1

#ifdef CHIP_ATMEGA168
  //Init timer 0 for delay_us timing
  //1,000,000 / 1 = 1,000,000
  TCCR0B = (1<<CS00); //Set Prescaler to 1. CS00=1

  //Init timer 2
  ASSR = 0;
  TCCR2B = (1<<CS22)|(1<<CS21)|(1<<CS20); //Set Prescaler to 1024. CS22=1, CS21=1,CS20=1
  TIMSK2 = (1<<TOIE2); //Enable Timer 2 Interrupt
#endif

#ifdef CHIP_ATMEGA8
  //Init timer 0 for delay_us timing
  //1,000,000 / 1 = 1,000,000
  TCCR0 = (1<<CS00); //Set Prescaler to 1. CS00=1

  //Init timer 2
  ASSR = 0;
  TCCR2 = (1<<CS22)|(1<<CS21)|(1<<CS20); //Set Prescaler to 1024. CS22=1, CS21=1,CS20=1
  TIMSK = (1<<TOIE2); //Enable Timer 2 Interrupt
#endif

  sei();
}

//Returns a '1' bit in the position of button 3,2,1,0
uint8_t check_button(void)
{
  uint8_t button_pressed = 0;
  button_pressed =
    (!BLUE_BUTTON ? BLUE_STATE : 0) |
    (!RED_BUTTON ? RED_STATE : 0) |
    (!GREEN_BUTTON ? GREEN_STATE : 0) |
    (!YELLOW_BUTTON ? YELLOW_STATE : 0);
  return(button_pressed);
}

//Plays the loser sounds
void play_loser(void)
{
  //printf("You failed!\n", 0);

  sbi(LED_RED_PORT, LED_RED);
  sbi(LED_GREEN_PORT, LED_GREEN);
  toner('0', 255);

  sbi(LED_BLUE_PORT, LED_BLUE);
  sbi(LED_YELLOW_PORT, LED_YELLOW);
  toner('0', 255);

  sbi(LED_RED_PORT, LED_RED);
  sbi(LED_GREEN_PORT, LED_GREEN);
  toner('0', 255);

  sbi(LED_BLUE_PORT, LED_BLUE);
  sbi(LED_YELLOW_PORT, LED_YELLOW);
  toner('0', 255);
}

//Plays the winner sounds
void play_winner(void)
{
  uint8_t x, y, z;

  //printf("You win!\n", 0);

  sbi(LED_RED_PORT, LED_RED);
  sbi(LED_YELLOW_PORT, LED_YELLOW);
  sbi(LED_BLUE_PORT, LED_BLUE);
  sbi(LED_GREEN_PORT, LED_GREEN);

  for(z = 0 ; z < 4 ; z++)
    {
      if(z == 0 || z == 2)
        {
          cbi(LED_RED_PORT, LED_RED);
          cbi(LED_YELLOW_PORT, LED_YELLOW);
          sbi(LED_BLUE_PORT, LED_BLUE);
          sbi(LED_GREEN_PORT, LED_GREEN);
        }
      else
        {
          sbi(LED_RED_PORT, LED_RED);
          sbi(LED_YELLOW_PORT, LED_YELLOW);
          cbi(LED_BLUE_PORT, LED_BLUE);
          cbi(LED_GREEN_PORT, LED_GREEN);
        }

      for(x = 250 ; x > 70 ; x--)
        {
          for(y = 0 ; y < 3 ; y++)
            {
              //Toggle the buzzer at various speeds
              sbi(BUZZER2_PORT, BUZZER2);
              cbi(BUZZER1_PORT, BUZZER1);

              delay_us(x);

              cbi(BUZZER2_PORT, BUZZER2);
              sbi(BUZZER1_PORT, BUZZER1);

              delay_us(x);
            }
        }
    }

}

//Plays the current contents of the game string
void play_string(void)
{
  uint8_t string_pos;
  uint8_t button;

  for(string_pos = 0 ; string_pos < game_string_position ; string_pos++)
    {
      button = game_string[string_pos];
      toner(button, 150);
      delay_ms(150);
    }
}

//Adds a new random button to the game sequence based on the current timer elapsed
void add_to_string(void)
{
  uint8_t random_timer;
  uint8_t new_button;

  random_timer = TCNT2;

  //Timer rolls over at 256 and is reset to 131 (Timer 2 ISR) so 125 possible numbers
  //131 + 31 = 162 will be button 1
  //162 + 31 = 193 button 2
  //193 + 31 = 224 button 3
  //224 + 31 = 255 button 4

  //Prescalar of 1024
  //Clock = 16MHz
  //15,625 clicks per second
  //64us per click

  //So each button has an equal chance of being added to the game sequence
  //Each timer2 click is every 64us (16MHz / 1024) so the button possibility
  //changes every 2ms or 500 times a second
  //Not random, but no one can play the same way to within 2ms!
  //The sequence changes *every* game

  if(random_timer < 162)
    new_button = '1';
  else if(random_timer < 193)
    new_button = '2';
  else if(random_timer < 224)
    new_button = '3';
  else
    new_button = '4';

  game_string[game_string_position] = new_button;
  game_string_position++;
}

//Tone generator
//(red, upper left) - 440Hz - 2.272ms - 1.136ms pulse
//(green, upper right, an octave higher than the upper right) - 880Hz - 1.136ms - 0.568ms pulse
//(blue, lower left, a perfect fourth higher than the upper left) - 587.33Hz - 1.702ms - 0.851ms pulse
//(yellow, lower right, a perfect fourth higher than the lower left) - 784Hz - 1.276ms - 0.638ms pulse

//Loop length is calculated to run buzz tone for 1ms. Buzz_length of 50 means tone will play for 50ms
//Red = 2.272ms = 2272us
//loop_length = 1
void toner(uint8_t tone, uint16_t buzz_length_ms)
{
  uint32_t buzz_length_us;
  buzz_length_us = buzz_length_ms * (uint32_t)1000;

  uint16_t buzz_delay;

  switch (tone)
    {
    case 'z' :
    case '0' :
      //Failure Light up all LEDS
      buzz_delay = 1500;
      break;

    case 'A' :
    case '1' :
      buzz_delay = 1136; //440Hz = 2272us Upper left, Red
      sbi(LED_RED_PORT, LED_RED);
      break;

    case 'a' :
    case '2' :
      buzz_delay = 568; //Upper right, Green
      sbi(LED_GREEN_PORT, LED_GREEN);
      break;

    case 'D' :
    case '3' :
      buzz_delay = 851; //Lower left, Blue
      sbi(LED_BLUE_PORT, LED_BLUE);
      break;

    case 'G' :
    case '4' :
      buzz_delay = 638; //Lower right, Yellow
      sbi(LED_YELLOW_PORT, LED_YELLOW);
      break;

      //Special noises without LEDs
    case '5' :
      buzz_delay = 150;//300; //Special noise
      break;

    case '6' :
      buzz_delay = 160;//300; //Special noise
      break;
    }

  //Run buzzer for buzz_length_us
  while(1)
    {
      //Subtract the buzz_delay from the overall length
      if(buzz_delay > buzz_length_us) break;
      buzz_length_us -= buzz_delay;

      if(buzz_delay > buzz_length_us) break;
      buzz_length_us -= buzz_delay;

      //Toggle the buzzer at various speeds
      cbi(BUZZER1_PORT, BUZZER1);
      sbi(BUZZER2_PORT, BUZZER2);
      delay_us(buzz_delay);

      sbi(BUZZER1_PORT, BUZZER1);
      cbi(BUZZER2_PORT, BUZZER2);
      delay_us(buzz_delay);
    }

  //Turn off all LEDs
  cbi(LED_RED_PORT, LED_RED);
  cbi(LED_BLUE_PORT, LED_BLUE);
  cbi(LED_YELLOW_PORT, LED_YELLOW);
  cbi(LED_GREEN_PORT, LED_GREEN);
}

//General short delays
//Uses internal timer do a fairly accurate 1us
void delay_us(uint16_t x)
{
#ifdef CHIP_ATMEGA168
  while(x > 256)
    {
      TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer2
      TCNT0 = 0; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
      while( (TIFR0 & (1<<TOV0)) == 0);

      x -= 256;
    }

  TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer2
  TCNT0 = 256 - x; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
  while( (TIFR0 & (1<<TOV0)) == 0);
#endif

#ifdef CHIP_ATMEGA8
  while(x > 256)
    {
      TIFR = (1<<TOV0); //Clear any interrupt flags on Timer2
      TCNT0 = 0; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
      while( (TIFR & (1<<TOV0)) == 0);

      x -= 256;
    }

  TIFR = (1<<TOV0); //Clear any interrupt flags on Timer2
  TCNT0 = 256 - x; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
  while( (TIFR & (1<<TOV0)) == 0);
#endif

}

//General short delays
void delay_ms(uint16_t x)
{
  for ( ; x > 0 ; x--)
    delay_us(1000);
}
