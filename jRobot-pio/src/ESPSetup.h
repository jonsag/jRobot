
#ifdef WIFI_ESP

int ESPwait(String stopstr, int timeout_secs)
{
    String response;
    bool found = false;
    char c;
    long timer_init;
    long timer;

    timer_init = millis();
    while (!found)
    {
        timer = millis();
        if (((timer - timer_init) / 1000) > timeout_secs)
        { // Timeout?
            Serial.println("!Timeout!");
            return 0; // timeout
        }
        if (Serial1_available())
        {
            c = Serial1_read();
            Serial.print(c);
            response += c;
            if (response.endsWith(stopstr))
            {
                found = true;
                delay(10);
                Serial1_flush();
                Serial.println();
            }
        } // end Serial1_available()
    }     // end while (!found)
    return 1;
}

// getMacAddress from ESP wifi module
int ESPgetMac()
{
    char c1, c2;
    bool timeout = false;
    long timer_init;
    long timer;
    uint8_t state = 0;
    uint8_t index = 0;

    MAC = "";
    timer_init = millis();
    while (!timeout)
    {
        timer = millis();
        if (((timer - timer_init) / 1000) > 5) // Timeout?
            timeout = true;
        if (Serial1_available())
        {
            c2 = c1;
            c1 = Serial1_read();
            Serial.print(c1);
            switch (state)
            {
            case 0:
                if (c1 == ':')
                    state = 1;
                break;
            case 1:
                if (c1 == '\r')
                {
                    MAC.toUpperCase();
                    state = 2;
                }
                else
                {
                    if ((c1 != '"') && (c1 != ':'))
                        MAC += c1; // Uppercase
                }
                break;
            case 2:
                if ((c2 == 'O') && (c1 == 'K'))
                {
                    Serial.println();
                    Serial1_flush();
                    return 1; // Ok
                }
                break;
            } // end switch
        }     // Serial_available
    }         // while (!timeout)
    Serial.println("!Timeout!");
    Serial1_flush();
    return -1; // timeout
}

int ESPsendCommand(char *command, String stopstr, int timeout_secs)
{
    Serial1_println(command);
    ESPwait(stopstr, timeout_secs);
    delay(250);
}
#endif
