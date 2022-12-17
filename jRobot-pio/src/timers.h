
// TIMER 1 : STEPPER MOTOR1 SPEED CONTROL
ISR(TIMER1_COMPA_vect)
{
    if (dir_M1 == 0) // If we are not moving we dont generate a pulse
        return;
    // We generate 1us STEP pulse
    SET(PORTE, 6); // STEP MOTOR 1
    delay_1us();
    CLR(PORTE, 6);
}
// TIMER 3 : STEPPER MOTOR2 SPEED CONTROL
ISR(TIMER3_COMPA_vect)
{
    if (dir_M2 == 0) // If we are not moving we dont generate a pulse
        return;
    // We generate 1us STEP pulse
    SET(PORTD, 6); // STEP MOTOR 2
    delay_1us();
    CLR(PORTD, 6);
}
