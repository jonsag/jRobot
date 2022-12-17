
// Set speed of Stepper Motor1
// tspeed could be positive or negative (reverse)
void setMotorSpeedM1(int16_t tspeed)
{
    long timer_period;
    int16_t speed;

    // Limit max speed?

    // WE LIMIT MAX ACCELERATION of the motors
    if ((speed_M1 - tspeed) > MAX_ACCEL)
        speed_M1 -= MAX_ACCEL;
    else if ((speed_M1 - tspeed) < -MAX_ACCEL)
        speed_M1 += MAX_ACCEL;
    else
        speed_M1 = tspeed;

#if MICROSTEPPING == 16
    speed = speed_M1 * 46; // Adjust factor from control output speed to real motor speed in steps/second
#else
    speed = speed_M1 * 23; // 1/8 Microstepping
#endif

    if (speed == 0)
    {
        timer_period = ZERO_SPEED;
        dir_M1 = 0;
    }
    else if (speed > 0)
    {
        timer_period = 2000000 / speed; // 2Mhz timer
        dir_M1 = 1;
        SET(PORTB, 4); // DIR Motor 1 (Forward)
    }
    else
    {
        timer_period = 2000000 / -speed;
        dir_M1 = -1;
        CLR(PORTB, 4); // Dir Motor 1
    }
    if (timer_period > 65535) // Check for minimum speed (maximum period without overflow)
        timer_period = ZERO_SPEED;

    OCR1A = timer_period;
    // Check  if we need to reset the timer...
    if (TCNT1 > OCR1A)
        TCNT1 = 0;
}

// Set speed of Stepper Motor2
// tspeed could be positive or negative (reverse)
void setMotorSpeedM2(int16_t tspeed)
{
    long timer_period;
    int16_t speed;

    // Limit max speed?

    // WE LIMIT MAX ACCELERATION of the motors
    if ((speed_M2 - tspeed) > MAX_ACCEL)
        speed_M2 -= MAX_ACCEL;
    else if ((speed_M2 - tspeed) < -MAX_ACCEL)
        speed_M2 += MAX_ACCEL;
    else
        speed_M2 = tspeed;

#if MICROSTEPPING == 16
    speed = speed_M2 * 46; // Adjust factor from control output speed to real motor speed in steps/second
#else
    speed = speed_M2 * 23; // 1/8 Microstepping
#endif

    if (speed == 0)
    {
        timer_period = ZERO_SPEED;
        dir_M2 = 0;
    }
    else if (speed > 0)
    {
        timer_period = 2000000 / speed; // 2Mhz timer
        dir_M2 = 1;
        CLR(PORTC, 6); // Dir Motor2 (Forward)
    }
    else
    {
        timer_period = 2000000 / -speed;
        dir_M2 = -1;
        SET(PORTC, 6); // DIR Motor 2
    }
    if (timer_period > 65535) // Check for minimum speed (maximum period without overflow)
        timer_period = ZERO_SPEED;

    OCR3A = timer_period;
    // Check  if we need to reset the timer...
    if (TCNT3 > OCR3A)
        TCNT3 = 0;
}
