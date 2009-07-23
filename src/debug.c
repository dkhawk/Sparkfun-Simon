void button_hardware_check(void) {
  while(1)
    {
      if( BLUE_BUTTON == 0)
        sbi(LED_BLUE_PORT, LED_BLUE);
      else
        cbi(LED_BLUE_PORT, LED_BLUE);

      if( YELLOW_BUTTON == 0)
        sbi(LED_YELLOW_PORT, LED_YELLOW);
      else
        cbi(LED_YELLOW_PORT, LED_YELLOW);

      if( RED_BUTTON == 0)
        sbi(LED_RED_PORT, LED_RED);
      else
        cbi(LED_RED_PORT, LED_RED);

      if( GREEN_BUTTON == 0)
        sbi(LED_GREEN_PORT, LED_GREEN);
      else
        cbi(LED_GREEN_PORT, LED_GREEN);

      delay_ms(50);
    }

  while(1) {
    int buttons = check_button();

    if (buttons & RED_STATE) {
      sbi(LED_RED_PORT, LED_RED);
    } else {
      cbi(LED_RED_PORT, LED_RED);
    }

    if (buttons & BLUE_STATE) {
      sbi(LED_BLUE_PORT, LED_BLUE);
    } else {
      cbi(LED_BLUE_PORT, LED_BLUE);
    }

    if (buttons & YELLOW_STATE) {
      sbi(LED_YELLOW_PORT, LED_YELLOW);
    } else {
      cbi(LED_YELLOW_PORT, LED_YELLOW);
    }

    if (buttons & GREEN_STATE) {
      sbi(LED_GREEN_PORT, LED_GREEN);
    } else {
      cbi(LED_GREEN_PORT, LED_GREEN);
    }

  }

}


void ExerciseHardware() {
  /*
    while(1)
    {
    sbi(PORTB, 5);
    delay_ms(1000);

    cbi(PORTB, 5);
    delay_ms(1000);
    }
  */
  //Test routines
  /*toner('1', 100);
    delay_ms(100);
    toner('2', 100);
    delay_ms(100);
    toner('3', 100);
    delay_ms(100);
    toner('4', 100);
    delay_ms(100);*/

  //LED Testing
  /*
    sbi(LED_RED_PORT, LED_RED);
    sbi(LED_BLUE_PORT, LED_BLUE);
    sbi(LED_GREEN_PORT, LED_GREEN);
    sbi(LED_YELLOW_PORT, LED_YELLOW);
    while(1);
  */

  /*while(1)
    {
    sbi(LED_RED_PORT, LED_RED);
    sbi(LED_BLUE_PORT, LED_BLUE);
    sbi(LED_GREEN_PORT, LED_GREEN);
    sbi(LED_YELLOW_PORT, LED_YELLOW);
    delay_ms(1000);

    cbi(LED_RED_PORT, LED_RED);
    cbi(LED_BLUE_PORT, LED_BLUE);
    cbi(LED_GREEN_PORT, LED_GREEN);
    cbi(LED_YELLOW_PORT, LED_YELLOW);
    delay_ms(1000);
    }*/

  //Button testing
  /*while(1)
    {
    if(check_button() == 1)
    sbi(LED_BLUE_PORT, LED_BLUE);

    if(check_button() == 2)
    sbi(LED_YELLOW_PORT, LED_YELLOW);

    if(check_button() == 4)
    sbi(LED_RED_PORT, LED_RED);

    if(check_button() == 8)
    sbi(LED_GREEN_PORT, LED_GREEN);
    }*/

  //Buzzer testing
  /*
    uint32_t buzz_length, buzz_delay;
    buzz_length = 1000;

    buzz_length *= 1000;
    buzz_delay = 1232;
    while(1)
    {
    //Toggle the buzzer at various speeds
    cbi(BUZZER1_PORT, BUZZER1);
    sbi(BUZZER2_PORT, BUZZER2);
    delay_us(buzz_delay);

    sbi(BUZZER1_PORT, BUZZER1);
    cbi(BUZZER2_PORT, BUZZER2);
    delay_us(buzz_delay);

    //Subtract the buzz_delay from the overall length
    if(buzz_delay > buzz_length) break;
    buzz_length -= buzz_delay;
    if(buzz_delay > buzz_length) break;
    buzz_length -= buzz_delay;
    }
  */
}
