#include "I2Cdev.h"

#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"

MPU6050 mpu;
#define OUTPUT_READABLE_YAWPITCHROLL
// #define OUTPUT_READABLE_REALACCEL
#define OUTPUT_READABLE_WORLDACCEL
//#define OUTPUT_TEAPOT

#define NUM_TO_AVERAGE 100

#define INTERRUPT_PIN 2 // use pin 2 on Arduino Uno & most boards
#define LED_PIN 13      // (Arduino is 13, Teensy is 11, Teensy++ is 6)
bool blinkState = false;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;        // [w, x, y, z]         quaternion container
VectorInt16 aa;      // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;  // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld; // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity; // [x, y, z]            gravity vector
float euler[3];      // [psi, theta, phi]    Euler angle container
float ypr[3];        // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

unsigned long prev_sample_time;
unsigned long loop_end_time;

//COMMS CODE---------------------------------
bool handshake_completed;
bool dataCommand;
long beginTime;
struct IntDataPacket
{
  int acc_x;
  int acc_y;
  int acc_z;
  int yaw;
  int pitch;
  int roll;
  uint32_t crc;

};

IntDataPacket TxPacket;
void resetFlags(){
  handshake_completed = false;
  dataCommand= false;
  
  }

//COMMS CODE---------------------------------

void setup()
{
  Wire.begin();
  Wire.setClock(400000);

  // initialize serial communication
//  Serial.begin(115200);
//  while (!Serial);

  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  // load and configure the DMP
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

  // make sure it worked (returns 0 if so)
  if (devStatus == 0)
  {
    // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    // turn on the DMP, now that it's ready
    mpu.setDMPEnabled(true);
    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    dmpReady = true;
    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else
  {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }
  Serial.begin(115200);
  while (!Serial);
  // configure LED for output
  pinMode(LED_PIN, OUTPUT);
}

// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop()
{
  //COMMS CODE---------------------------------
  loop_end_time = millis() + 50;
  if (Serial.available() > 0 ) {
    byte cmd = Serial.read();
    switch (char(cmd)) {
      case 'S':
        Serial.write('A');
        handshake_completed = false;
        dataCommand = false;
        break;
      case 'A':
        handshake_completed = true;
        break;
      case 'D':
        dataCommand = true;
        beginTime = millis();
        break;
      case 'R':
        resetFlags();
        break;
      case 'G':
        beginTime = millis();
        break;
    }

  }
  if (handshake_completed && dataCommand) {
    if (!dmpReady)
      return;

    // read a packet from FIFO
    int yaw = 0;
    int pitch = 0;
    int roll = 0;

    int acc_x = 0;
    int acc_y = 0;
    int acc_z = 0;

    for (int i = 0; i < NUM_TO_AVERAGE; i++)
    {
      if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer))
      {
        prev_sample_time = micros();
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        yaw += (ypr[0] * 180 / M_PI);
        pitch += (ypr[1] * 180 / M_PI);
        roll += (ypr[2] * 180 / M_PI);
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetAccel(&aa, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
        mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
        acc_x += (aaWorld.x);
        acc_y += (aaWorld.y);
        acc_z += (aaWorld.z);
        // blink LED to indicate activity
        blinkState = !blinkState;
        digitalWrite(LED_PIN, blinkState);
      }
      while (micros() < (prev_sample_time + 500));
    }

    yaw /= (NUM_TO_AVERAGE / 25);
    pitch /= (NUM_TO_AVERAGE / 25);
    roll /= (NUM_TO_AVERAGE / 25);

    acc_x /= (10 * NUM_TO_AVERAGE);
    acc_y /= (10 * NUM_TO_AVERAGE);
    acc_z /= (10 * NUM_TO_AVERAGE);

    TxPacket.acc_x = acc_x;
    TxPacket.acc_y = acc_y;
    TxPacket.acc_z = acc_z;
    TxPacket.yaw = yaw;
    TxPacket.pitch = pitch;
    TxPacket.roll = roll;

    Serial.write((uint8_t*) &TxPacket, sizeof(TxPacket));
    long timeSinceLastReply = millis() - beginTime;
    if (timeSinceLastReply >= 6000) {
      delay(1000);
    }

  }
  while(millis() < loop_end_time);
  //COMMS CODE---------------------------------
}
