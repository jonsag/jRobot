
#define WIFI_ESP // New WIFI ESP8266 Module. Comment this line if you are using the old Wifi module (RN-171)

#include <JJROBOTS_OSC.h>
#include <JJROBOTS_BROBOT.h>
#include <Wire.h>
#include <I2Cdev.h>               // I2Cdev lib from www.i2cdevlib.com
#include <JJ_MPU6050_DMP_6Axis.h> // Modified version of the MPU6050 library to work with DMP (see comments inside)
// This version optimize the FIFO (only contains quaternion) and minimize code size

// PINS
#define ENABLE_MOTORS_PIN 4
#define STEP_MOTOR_1_PIN 7
#define DIR_MOTOR_1_PIN 8
#define STEP_MOTOR_2_PIN 12
#define DIR_MOTOR_2_PIN 5
#define DISABLE_MOTORS_PIN 4

#define SERVO_1_PIN 10
#define SERVO_2_PIN 13

#define SONAR_TRIG_PIN 6
#define SONAR_ECHO_PIN 9

// NORMAL MODE PARAMETERS (MAXIMUM SETTINGS)
#define MAX_THROTTLE 580
#define MAX_STEERING 150
#define MAX_TARGET_ANGLE 12

// PRO MODE = MORE AGGRESSIVE (MAXIMUM SETTINGS)
#define MAX_THROTTLE_PRO 980 // 680
#define MAX_STEERING_PRO 250
#define MAX_TARGET_ANGLE_PRO 40 // 20

// Default control terms
#define KP 0.19
#define KD 28
#define KP_THROTTLE 0.07
#define KI_THROTTLE 0.04

// Control gains for raiseup (the raiseup movement require special control parameters)
#define KP_RAISEUP 0.16
#define KD_RAISEUP 36
#define KP_THROTTLE_RAISEUP 0 // No speed control on raiseup
#define KI_THROTTLE_RAISEUP 0.0

#define MAX_CONTROL_OUTPUT 500

// Servo definitions
#define SERVO_AUX_NEUTRO 1550 // Servo neutral position
#define SERVO_MIN_PULSEWIDTH 650
#define SERVO_MAX_PULSEWIDTH 2600

// Battery management [optional]. This is not needed for alkaline or Ni-Mh batteries but useful for if you use lipo batteries
#define BATTERY_CHECK 1             // 0: No  check, 1: check (send message to interface)
#define LIPOBATT 0                  // Default 0: No Lipo batt 1: Lipo batt (to adjust battery monitor range)
#define BATTERY_WARNING 105         // (10.5 volts) aprox [for lipo batts, disable by default]
#define BATTERY_SHUTDOWN 95         // (9.5 volts) [for lipo batts]
#define SHUTDOWN_WHEN_BATTERY_OFF 0 // 0: Not used, 1: Robot will shutdown when battery is off (BATTERY_CHECK SHOULD BE 1)

#define DEBUG 0 // 0 = No debug info (default)

#define CLR(x, y) (x &= (~(1 << y)))
#define SET(x, y) (x |= (1 << y))

#define ZERO_SPEED 65535
#define MAX_ACCEL 7 // Maximum motor acceleration (MAX RECOMMENDED VALUE: 8) (default:7)

#define MICROSTEPPING 16 // 8 or 16 for 1/8 or 1/16 driver microstepping (default:16)

#define I2C_SPEED 400000L // 400kHz I2C speed

#define RAD2GRAD 57.2957795
#define GRAD2RAD 0.01745329251994329576923690768489

#define ITERM_MAX_ERROR 25 // Iterm windup constants for PI control //40
#define ITERM_MAX 8000     // 5000

bool Robot_shutdown = false; // Robot shutdown flag => Out of

String MAC; // MAC address of Wifi module

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (for us 18 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[18]; // FIFO storage buffer
Quaternion q;

uint8_t loop_counter;        // To generate a medium loop 40Hz
uint8_t slow_loop_counter;   // slow loop 2Hz
uint8_t sendBattery_counter; // To send battery status

long timer_old;
long timer_value;
int debug_counter;
float debugVariable;
float dt;

// class default I2C address is 0x68 for MPU6050
MPU6050 mpu;

// Angle of the robot (used for stability control)
float angle_adjusted;
float angle_adjusted_Old;

// Default control values from constant definitions
float Kp = KP;
float Kd = KD;
float Kp_thr = KP_THROTTLE;
float Ki_thr = KI_THROTTLE;
float Kp_user = KP;
float Kd_user = KD;
float Kp_thr_user = KP_THROTTLE;
float Ki_thr_user = KI_THROTTLE;
bool newControlParameters = false;
bool modifing_control_parameters = false;
float PID_errorSum;
float PID_errorOld = 0;
float PID_errorOld2 = 0;
float setPointOld = 0;
float target_angle;
float throttle;
float steering;
float max_throttle = MAX_THROTTLE;
float max_steering = MAX_STEERING;
float max_target_angle = MAX_TARGET_ANGLE;
float control_output;

uint8_t mode; // mode = 0 Normal mode, mode = 1 Pro mode (More aggressive)

int16_t motor1;
int16_t motor2;

int16_t speed_M1, speed_M2; // Actual speed of motors
int8_t dir_M1, dir_M2;      // Actual direction of steppers motors
int16_t actual_robot_speed; // overall robot speed (measured from steppers speed)
int16_t actual_robot_speed_Old;
float estimated_speed_filtered; // Estimated robot speed