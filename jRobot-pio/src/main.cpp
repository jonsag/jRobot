#include <Arduino.h>

// B-ROBOT  SELF BALANCE ARDUINO ROBOT WITH STEPPER MOTORS
// JJROBOTS BROBOT KIT: (Arduino Leonardo + BROBOT ELECTRONIC BRAIN v2 + STEPPER MOTOR drivers)
// This code is prepared for new BROBOT shield v2.0 with ESP8266 Wifi module
// You need to install libraries JJROBOTS_OSC, JJROBOTS_BROBOT, I2Cdev and MPU6050 (from Github repository)
// Author: JJROBOTS.COM (Jose Julio & Juan Pedro)
// Date: 02/09/2014
// Updated: 26/10/2015
// Version: 2.21
// License: GPL v2
// Project URL: http://jjrobots.com/b-robot (Features,documentation,build instructions,how it works, SHOP,...)
// New updates:
//   - New default parameters specially tuned for BROBOT EVO version
//   - Support for TouchMessages(/z). Main controls resets when user lift the fingers
//   - BROBOT sends the battery status to the interface
//   Remember to update the libraries and install the new TouchOSC layout
//  Thanks to our users on the forum for the new ideas. Specially sasa999, KomX, ...

// The board needs at least 10-15 seconds with no motion (robot steady) at beginning to give good values...
// MPU6050 IMU using internal DMP processor. Connected via I2C bus
// Angle calculations and control part is running at 200Hz from DMP solution
// DMP is using the gyro_bias_no_motion correction method.

// The robot is OFF when the angle is high (robot is horizontal). When you start raising the robot it
// automatically switch ON and start a RAISE UP procedure.
// You could RAISE UP the robot also with the robot arm servo [OPTIONAL] (Push1 on the interface)
// To switch OFF the robot you could manually put the robot down on the floor (horizontal)

// We use a standard PID controllers (Proportional, Integral derivative controller) for robot stability
// More info on the project page
// We have a PI controller for speed control and a PD controller for stability (robot angle)
// The output of the control (motors speed) is integrated so it´s really an acceleration not an speed.

// We control the robot from a WIFI module using OSC standard UDP messages (JJROBOTS_OSC library)
// You need an OSC app to control de robot (example: TouchOSC for IOS and Android)
// Join the module Wifi Access Point (by default: JJROBOTS) with your Smartphone/Tablet...
// Install the BROBOT layout into the OSC app (Touch OSC) and start play! (read the project page)
// OSC controls:
//    fader1: Throttle (0.0-1.0) OSC message: /1/fader1
//    fader2: Steering (0.0-1.0) OSC message: /1/fader2
//    push1: Move servo arm (and robot raiseup) OSC message /1/push1 [OPTIONAL]
//    if you enable the touchMessage on TouchOSC options, controls return to center automatically when you lift your fingers
//    toggle1: Enable PRO mode. On PRO mode steering and throttle are more aggressive
//    PAGE2: PID adjustments [optional][don´t touch if you don´t know what you are doing...;-) ]

#include "config.h"
#include "dmpFunctions.h"
#include "PIDController.h"
#include "delay.h"
#include "timers.h"
#include "motorControl.h"
#include "oscControl.h"
#include "ESPSetup.h"

// INITIALIZATION
void setup()
{
  // STEPPER PINS ON JJROBOTS BROBOT BRAIN BOARD
  pinMode(ENABLE_MOTORS_PIN, OUTPUT);    // ENABLE MOTORS
  pinMode(STEP_MOTOR_1_PIN, OUTPUT);    // STEP MOTOR 1 PORTE,6
  pinMode(DIR_MOTOR_1_PIN, OUTPUT);    // DIR MOTOR 1  PORTB,4
  pinMode(STEP_MOTOR_2_PIN, OUTPUT);   // STEP MOTOR 2 PORTD,6
  pinMode(DIR_MOTOR_2_PIN, OUTPUT);    // DIR MOTOR 2  PORTC,6
  digitalWrite(DISABLE_MOTORS_PIN, HIGH); // Disable motors

  pinMode(SERVO_1_PIN, OUTPUT); // Servo1 (arm)
  pinMode(SERVO_2_PIN, OUTPUT); // Servo2

  Serial.begin(115200); // Serial output to console

  // Wifi module initialization: Wifi module should be pre configured (use another sketch to configure the module)

  // Initialize I2C bus (MPU6050 is connected via I2C)
  Wire.begin();
  // I2C 400Khz fast mode
  TWSR = 0;
  TWBR = ((16000000L / I2C_SPEED) - 16) / 2;
  TWCR = 1 << TWEN;

#if DEBUG > 0
  delay(9000);
#else
  delay(2000);
#endif
  Serial.println("BROBOT by JJROBOTS v2.2");
  Serial.println("Initializing I2C devices...");
  // mpu.initialize();
  //  Manual MPU initialization... accel=2G, gyro=2000º/s, filter=20Hz BW, output=200Hz
  mpu.setClockSource(MPU6050_CLOCK_PLL_ZGYRO);
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_2000);
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  mpu.setDLPFMode(MPU6050_DLPF_BW_10); // 10,20,42,98,188  // Default factor for BROBOT:10
  mpu.setRate(4);                      // 0=1khz 1=500hz, 2=333hz, 3=250hz 4=200hz
  mpu.setSleepEnabled(false);

  delay(500);
  Serial.println("Initializing DMP...");
  devStatus = mpu.dmpInitialize();
  if (devStatus == 0)
  {
    // turn on the DMP, now that it's ready
    Serial.println("Enabling DMP...");
    mpu.setDMPEnabled(true);
    mpuIntStatus = mpu.getIntStatus();
    dmpReady = true;
  }
  else
  { // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print("DMP Initialization failed (code ");
    Serial.print(devStatus);
    Serial.println(")");
  }

  // Gyro calibration
  // The robot must be steady during initialization
  delay(500);
  Serial.println("Gyro calibration!!  Dont move the robot in 10 seconds... ");
  delay(500);

#ifdef WIFI_ESP
  // With the new ESP8266 WIFI MODULE WE NEED TO MAKE AN INITIALIZATION PROCESS
  Serial.println("Initializing ESP Wifi Module...");
  Serial.println("WIFI RESET");
  Serial1_flush();
  Serial1_print("+++"); // To ensure we exit the transparent transmission mode
  delay(100);
  ESPsendCommand("AT", "OK", 1);
  ESPsendCommand("AT+RST", "OK", 2); // ESP Wifi module RESET
  ESPwait("ready", 6);
  ESPsendCommand("AT+GMR", "OK", 5);
  Serial1_println("AT+CIPSTAMAC?");
  ESPgetMac();
  Serial.print("MAC:");
  Serial.println(MAC);
  delay(250);
  ESPsendCommand("AT+CWQAP", "OK", 3);
  ESPsendCommand("AT+CWMODE=2", "OK", 3); // Soft AP mode
  // Generate Soft AP. SSID=JJROBOTS, PASS=87654321
  char *cmd = "AT+CWSAP=\"JJROBOTS_XX\",\"87654321\",5,3";
  // Update XX characters with MAC address (last 2 characters)
  cmd[19] = MAC[10];
  cmd[20] = MAC[11];
  // const char cmd[50] = "AT+CWSAP=\"JJROBOTS_"+MAC.substring(MAC.length()-2)+"\",\"87654321\",5,3";
  ESPsendCommand(cmd, "OK", 6);
  // Start UDP SERVER on port 2222
  Serial.println("Start UDP server at port 2222");
  ESPsendCommand("AT+CIPMUX=0", "OK", 3);  // Single connection mode
  ESPsendCommand("AT+CIPMODE=1", "OK", 3); // Transparent mode
  // ESPsendCommand("AT+CIPSTART=\"UDP\",\"0\",2223,2222,0", "OK", 3);
  ESPsendCommand("AT+CIPSTART=\"UDP\",\"192.168.4.2\",2223,2222,0", "OK", 3);
  delay(250);
  ESPsendCommand("AT+CIPSEND", ">", 2); // Start transmission (transparent mode)
  delay(5000);                          // Time to settle things... the bias_from_no_motion algorithm needs some time to take effect and reset gyro bias.
#else
  delay(10000); // Time to settle things... the bias_from_no_motion algorithm needs some time to take effect and reset gyro bias.
#endif

  // Verify connection
  Serial.println("Testing device connections...");
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  timer_old = millis();

  // Init servos
  Serial.println("Servo initialization...");
  BROBOT.initServo();
  BROBOT.moveServo1(SERVO_AUX_NEUTRO);

  // STEPPER MOTORS INITIALIZATION
  Serial.println("Stepper motors initialization...");
  // MOTOR1 => TIMER1
  TCCR1A = 0;                          // Timer1 CTC mode 4, OCxA,B outputs disconnected
  TCCR1B = (1 << WGM12) | (1 << CS11); // Prescaler=8, => 2Mhz
  OCR1A = ZERO_SPEED;                  // Motor stopped
  dir_M1 = 0;
  TCNT1 = 0;

  // MOTOR2 => TIMER3
  TCCR3A = 0;                          // Timer3 CTC mode 4, OCxA,B outputs disconnected
  TCCR3B = (1 << WGM32) | (1 << CS31); // Prescaler=8, => 2Mhz
  OCR3A = ZERO_SPEED;                  // Motor stopped
  dir_M2 = 0;
  TCNT3 = 0;

  // Adjust sensor fusion gain
  Serial.println("Adjusting DMP sensor fusion gain...");
  dmpSetSensorFusionAccelGain(0x20);

  delay(200);

  // Enable stepper drivers and TIMER interrupts
  digitalWrite(4, LOW); // Enable stepper drivers
  // Enable TIMERs interrupts
  TIMSK1 |= (1 << OCIE1A); // Enable Timer1 interrupt
  TIMSK3 |= (1 << OCIE1A); // Enable Timer1 interrupt

  // Little motor vibration and servo move to indicate that robot is ready
  for (uint8_t k = 0; k < 5; k++)
  {
    setMotorSpeedM1(5);
    setMotorSpeedM2(5);
    BROBOT.moveServo1(SERVO_AUX_NEUTRO + 100);
    delay(200);
    setMotorSpeedM1(-5);
    setMotorSpeedM2(-5);
    BROBOT.moveServo1(SERVO_AUX_NEUTRO - 100);
    delay(200);
  }
  BROBOT.moveServo1(SERVO_AUX_NEUTRO);

  Serial.print("BATTERY:");
  Serial.println(BROBOT.readBattery());

  // OSC initialization
  OSC.fadder1 = 0.5;
  OSC.fadder2 = 0.5;

  Serial.println("Let´s start...");

  mpu.resetFIFO();
  timer_old = millis();
  Robot_shutdown = false;
  mode = 0;
}

// MAIN LOOP
void loop()
{
  // If we run out of battery, we do nothing... STOP
#if SHUTDOWN_WHEN__OFF == 1
  if (Robot_shutdown)
    return;
#endif

  debug_counter++;
  OSC.MsgRead(); // Read UDP OSC messages
  if (OSC.newMessage)
  {
    OSC.newMessage = 0;
    if (OSC.page == 1) // Get commands from user (PAGE1 are user commands: throttle, steering...)
    {
      OSC.newMessage = 0;
      throttle = (OSC.fadder1 - 0.5) * max_throttle;
      // We add some exponential on steering to smooth the center band
      steering = OSC.fadder2 - 0.5;
      if (steering > 0)
        steering = (steering * steering + 0.5 * steering) * max_steering;
      else
        steering = (-steering * steering + 0.5 * steering) * max_steering;

      modifing_control_parameters = false;
      if ((mode == 0) && (OSC.toggle1))
      {
        // Change to PRO mode
        max_throttle = MAX_THROTTLE_PRO;
        max_steering = MAX_STEERING_PRO;
        max_target_angle = MAX_TARGET_ANGLE_PRO;
        mode = 1;
      }
      if ((mode == 1) && (OSC.toggle1 == 0))
      {
        // Change to NORMAL mode
        max_throttle = MAX_THROTTLE;
        max_steering = MAX_STEERING;
        max_target_angle = MAX_TARGET_ANGLE;
        mode = 0;
      }
    }
#if DEBUG == 1
    Serial.print(throttle);
    Serial.print(" ");
    Serial.println(steering);
#endif
  } // End new OSC message

  timer_value = millis();

  // New DMP Orientation solution?
  fifoCount = mpu.getFIFOCount();
  if (fifoCount >= 18)
  {
    if (fifoCount > 18) // If we have more than one packet we take the easy path: discard the buffer and wait for the next one
    {
      Serial.println("FIFO RESET!!");
      mpu.resetFIFO();
      return;
    }
    loop_counter++;
    slow_loop_counter++;
    dt = (timer_value - timer_old);
    timer_old = timer_value;

    angle_adjusted_Old = angle_adjusted;
    // Get new orientation angle from IMU (MPU6050)
    angle_adjusted = dmpGetPhi();

#if DEBUG == 8
    Serial.print(throttle);
    Serial.print(" ");
    Serial.print(steering);
    Serial.print(" ");
    Serial.println(mode);
#endif

#if DEBUG == 1
    Serial.println(angle_adjusted);
#endif
    // Serial.print("\t");
    mpu.resetFIFO(); // We always reset FIFO

    // We calculate the estimated robot speed:
    // Estimated_Speed = angular_velocity_of_stepper_motors(combined) - angular_velocity_of_robot(angle measured by IMU)
    actual_robot_speed_Old = actual_robot_speed;
    actual_robot_speed = (speed_M1 + speed_M2) / 2; // Positive: forward

    int16_t angular_velocity = (angle_adjusted - angle_adjusted_Old) * 90.0;                    // 90 is an empirical extracted factor to adjust for real units
    int16_t estimated_speed = -actual_robot_speed_Old - angular_velocity;                       // We use robot_speed(t-1) or (t-2) to compensate the delay
    estimated_speed_filtered = estimated_speed_filtered * 0.95 + (float)estimated_speed * 0.05; // low pass filter on estimated speed

#if DEBUG == 2
    Serial.print(" ");
    Serial.println(estimated_speed_filtered);
#endif
    // SPEED CONTROL: This is a PI controller.
    //    input:user throttle, variable: estimated robot speed, output: target robot angle to get the desired speed
    target_angle = speedPIControl(dt, estimated_speed_filtered, throttle, Kp_thr, Ki_thr);
    target_angle = constrain(target_angle, -max_target_angle, max_target_angle); // limited output

#if DEBUG == 3
    Serial.print(" ");
    Serial.println(estimated_speed_filtered);
    Serial.print(" ");
    Serial.println(target_angle);
#endif

    // Stability control: This is a PD controller.
    //    input: robot target angle(from SPEED CONTROL), variable: robot angle, output: Motor speed
    //    We integrate the output (summatory), so the output is really the motor acceleration, not motor speed.
    control_output += stabilityPDControl(dt, angle_adjusted, target_angle, Kp, Kd);
    control_output = constrain(control_output, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT); // Limit max output from control

    // The steering part from the user is injected directly on the output
    motor1 = control_output + steering;
    motor2 = control_output - steering;

    // Limit max speed (control output)
    motor1 = constrain(motor1, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT);
    motor2 = constrain(motor2, -MAX_CONTROL_OUTPUT, MAX_CONTROL_OUTPUT);

    // NOW we send the commands to the motors
    if ((angle_adjusted < 76) && (angle_adjusted > -76)) // Is robot ready (upright?)
    {
      // NORMAL MODE
      digitalWrite(4, LOW); // Motors enable
      setMotorSpeedM1(motor1);
      setMotorSpeedM2(motor2);

      // Push1 Move servo arm
      if (OSC.push1) // Move arm
      {
        // Update to correct bug when the robot is lying backward
        if (angle_adjusted > -40)
          BROBOT.moveServo1(SERVO_MIN_PULSEWIDTH + 100);
        else
          BROBOT.moveServo1(SERVO_MAX_PULSEWIDTH + 100);
      }
      else
        BROBOT.moveServo1(SERVO_AUX_NEUTRO);

      // Push2 reset controls to neutral position
      if (OSC.push2)
      {
        OSC.fadder1 = 0.5;
        OSC.fadder2 = 0.5;
      }

      // Normal condition?
      if ((angle_adjusted < 45) && (angle_adjusted > -45))
      {
        Kp = Kp_user; // Default user control gains
        Kd = Kd_user;
        Kp_thr = Kp_thr_user;
        Ki_thr = Ki_thr_user;
      }
      else // We are in the raise up procedure => we use special control parameters
      {
        Kp = KP_RAISEUP; // CONTROL GAINS FOR RAISE UP
        Kd = KD_RAISEUP;
        Kp_thr = KP_THROTTLE_RAISEUP;
        Ki_thr = KI_THROTTLE_RAISEUP;
      }
    }
    else // Robot not ready (flat), angle > 70º => ROBOT OFF
    {
      digitalWrite(4, HIGH); // Disable motors
      setMotorSpeedM1(0);
      setMotorSpeedM2(0);
      PID_errorSum = 0; // Reset PID I term
      Kp = KP_RAISEUP;  // CONTROL GAINS FOR RAISE UP
      Kd = KD_RAISEUP;
      Kp_thr = KP_THROTTLE_RAISEUP;
      Ki_thr = KI_THROTTLE_RAISEUP;

      // if we pulse push1 button we raise up the robot with the servo arm
      if (OSC.push1)
      {
        // Because we know the robot orientation (face down of face up), we move the servo in the appropiate direction for raise up
        if (angle_adjusted > 0)
          BROBOT.moveServo1(SERVO_MIN_PULSEWIDTH);
        else
          BROBOT.moveServo1(SERVO_MAX_PULSEWIDTH);
      }
      else
        BROBOT.moveServo1(SERVO_AUX_NEUTRO);
    }
    // Check for new user control parameters
    readControlParameters();

  } // End of new IMU data

  // Medium loop 40Hz
  if (loop_counter >= 5)
  {
    loop_counter = 0;
    // We do nothing here now...
#if DEBUG == 10
    Serial.print(angle_adjusted);
    Serial.print(" ");
    Serial.println(debugVariable);
#endif
  } // End of medium loop

  if (slow_loop_counter >= 99) // 2Hz
  {
    slow_loop_counter = 0;
    // Read  status
#if BATTERY_CHECK == 1
    int BatteryValue = BROBOT.readBattery();
    sendBattery_counter++;
    if (sendBattery_counter >= 10)
    { // Every 5 seconds we send a message
      sendBattery_counter = 0;
#if LIPOBATT == 0
      // From >10.6 volts (100%) to 9.2 volts (0%) (aprox)
      float value = constrain((BatteryValue - 92) / 14.0, 0.0, 1.0);
#else
      // For Lipo battery use better this config: (From >11.5v (100%) to 9.5v (0%)
      float value = constrain((BatteryValue - 95) / 20.0, 0.0, 1.0);
#endif
      // Serial.println(value);
      OSC.MsgSend("/1/rotary1\0\0,f\0\0\0\0\0\0", 20, value);
    }
    // if (Battery_value < BATTERY_WARNING){
    //   Serial.print("LOW BAT!! ");
    //   }
#endif // BATTERY_CHECK
#if DEBUG == 6
    Serial.print("B:");
    Serial.println(BatteryValue);
#endif
#if SHUTDOWN_WHEN_BATTERY_OFF == 1

    if (BROBOT.readBattery(); < BATTERY_SHUTDOWN)
    {
      // Robot shutdown !!!
      Serial.println("LOW BAT!! SHUTDOWN");
      Robot_shutdown = true;
      // Disable steppers
      digitalWrite(4, HIGH); // Disable motors
    }
#endif

  } // End of slow loop
}
