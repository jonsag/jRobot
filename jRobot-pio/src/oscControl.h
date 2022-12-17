
// Read control PID parameters from user. This is only for advanced users that want to "play" with the controllers...
void readControlParameters()
{
    // Parameter initialization (first time we enter page2)
    if ((OSC.page == 2) && (!modifing_control_parameters))
    {
        OSC.fadder1 = 0.5;
        OSC.fadder2 = 0.5;
        OSC.fadder3 = 0.5;
        OSC.fadder4 = 0.0;
        // OSC.toggle1 = 0;
        modifing_control_parameters = true;
    }
    // Parameters Mode (page2 controls)
    // User could adjust KP, KD, KP_THROTTLE and KI_THROTTLE (fadder1,2,3,4)
    // Now we need to adjust all the parameters all the times because we don´t know what parameter has been moved
    if (OSC.page == 2)
    {
        Kp_user = KP * 2 * OSC.fadder1;
        Kd_user = KD * 2 * OSC.fadder2;
        Kp_thr_user = KP_THROTTLE * 2 * OSC.fadder3;
        Ki_thr_user = (KI_THROTTLE + 0.1) * 2 * OSC.fadder4;
#if DEBUG > 0
        Serial.print("Par: ");
        Serial.print(Kp_user);
        Serial.print(" ");
        Serial.print(Kd_user);
        Serial.print(" ");
        Serial.print(Kp_thr_user);
        Serial.print(" ");
        Serial.println(Ki_thr_user);
#endif
        // Kill robot => Sleep
        while (OSC.toggle2)
        {
            // Reset external parameters
            mpu.resetFIFO();
            PID_errorSum = 0;
            timer_old = millis();
            setMotorSpeedM1(0);
            setMotorSpeedM2(0);
            OSC.MsgRead();
        }
        newControlParameters = true;
    }
    if ((newControlParameters) && (!modifing_control_parameters))
    {
        // Reset parameters
        OSC.fadder1 = 0.5;
        OSC.fadder2 = 0.5;
        // OSC.toggle1 = 0;
        newControlParameters = false;
    }
}
