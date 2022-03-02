#include <Arduino_FreeRTOS.h>

bool handshake_completed;
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
  
};


struct IntDataPacket
{
  int acc_x;
  int acc_y;
  int acc_z;
  int yaw;
  int pitch;
  int roll;
  
};


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
  
//  TxPacket.acc_x = dummyIMUDataInt();
//  TxPacket.acc_y = dummyIMUDataInt();
//  TxPacket.acc_z = dummyIMUDataInt();
//  TxPacket.yaw = dummyIMUDataInt();
//  TxPacket.pitch = dummyIMUDataInt();
//  TxPacket.roll = dummyIMUDataInt();
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
    bool dataCommand = false;
    if(Serial.available() > 0 ){
      byte cmd = Serial.read();
      switch(char(cmd)){
        case 'H':
          Serial.write('A');
          handshake_completed = false;
          break;
        case 'A':
          delay(5000);
          handshake_completed = true;
          beginTime = millis();
          break;
        case 'D':
          dataCommand = true;
          break;
        case 'R':
          resetFunc();
          break;
        case 'G':
          beginTime = millis();
          break;
        }
  
      }
      if(handshake_completed){
        Serial.write((uint8_t*) &TxPacket, sizeof(TxPacket));
        dataCommand = false;
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
    TxPacket.acc_x = dummyIMUDataInt();
    TxPacket.acc_y = dummyIMUDataInt();
    TxPacket.acc_z = dummyIMUDataInt();
    TxPacket.yaw = dummyIMUDataInt();
    TxPacket.pitch = dummyIMUDataInt();
    TxPacket.roll = dummyIMUDataInt();
    vTaskDelay( 50 / portTICK_PERIOD_MS ); 
  }
}
