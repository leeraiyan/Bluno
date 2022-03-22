#include <Arduino_FreeRTOS.h>

//HARDWARE CODE----------------
#include <MPU6050.h>
#include "Wire.h"

MPU6050 imu;
int16_t ax, ay, az;
int16_t gx, gy, gz;
//HARDWARE CODE----------------


bool handshake_completed;
bool dataCommand;
long randNumber;
long minIMUdummyValue = -3600000;
long maxIMUdummyValue = 3600000;

int intminIMUdummyValue = -30000;
int intmaxIMUdummyValue = 30000;

long beginTime;

struct DataPacket
{
  long acc_x;
  long acc_y;
  long acc_z;
  long yaw;
  long pitch;
  long roll;
  uint32_t crc;
};


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

uint32_t checksumCalculator(uint16_t * data, uint32_t length)
{
   uint32_t curr_crc = 0x00000000;
   uint32_t sum1 =  curr_crc;
   uint32_t sum2 = (curr_crc >> 16);
   int index;
   for(index = 0; index < length; index = index+1)
   {
      sum1 = (sum1 + data[index]) % 65535;
      sum2 = (sum2 + sum1) % 65535;
   }
   return (sum2 << 16) | sum1;
}

IntDataPacket TxPacket;

// define two tasks for Blink & AnalogRead
void TaskSend( void *pvParameters );
void TaskPrepare( void *pvParameters );

TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  randomSeed(199);

  // Now set up two tasks to run independently.
  xTaskCreate(TaskSend, "Task1", 128, NULL, 1, &TaskHandle_1);
  xTaskCreate(TaskPrepare, "Task2", 128, NULL, 2, &TaskHandle_2);

  //HARDWARE CODE----------------
  pinMode(5, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  imu.initialize();
  Serial.println(imu.testConnection() ? "Connected" : "Error");
  //HARDWARE CODE----------------
}

long dummyIMUData(){
  randNumber = random(minIMUdummyValue, maxIMUdummyValue);
  return randNumber;
  }

int dummyIMUDataInt(){
  randNumber = random(intminIMUdummyValue, intmaxIMUdummyValue);
  return randNumber;
  }

void(* resetFunc) (void) = 0;

void loop() {

}


void TaskSend(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    
    if(Serial.available() > 0 ){
      byte cmd = Serial.read();
      switch(char(cmd)){
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
          resetFunc();
          break;
        case 'G':
          beginTime = millis();
          break;
        }
  
      }
      if(handshake_completed && dataCommand){
        Serial.write((uint8_t*) &TxPacket, sizeof(TxPacket));
        long timeSinceLastReply = millis() - beginTime;
        if (timeSinceLastReply < 6000)
          vTaskDelay( 50 / portTICK_PERIOD_MS );
        else{
          vTaskDelay( 1000 / portTICK_PERIOD_MS );
          } 
  
        }
  }
}

void TaskPrepare(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;)
  {
    imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    TxPacket.acc_x = ax;
    TxPacket.acc_y = ay;
    TxPacket.acc_z = az;
    TxPacket.yaw = gx;
    TxPacket.pitch = gy;
    TxPacket.roll = gz;
    uint16_t buf[6] = {TxPacket.acc_x, TxPacket.acc_y ,TxPacket.acc_z, TxPacket.yaw, TxPacket.pitch, TxPacket.roll};
    uint32_t crc = checksumCalculator(buf, 6);
    TxPacket.crc = crc;
    vTaskDelay( 50 / portTICK_PERIOD_MS ); 
  }
}
