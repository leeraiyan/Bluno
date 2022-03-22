#include <Arduino_FreeRTOS.h>
bool handshake_completed;
bool dataCommand;
int randNumber;
long beginTime;

struct DataPacket
{
  char hit; 
};
DataPacket TxPacket;

// define two tasks for Blink & AnalogRead
void TaskSend( void *pvParameters );
void TaskPrepare( void *pvParameters );

TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  randomSeed(500);
  // Now set up two tasks to run independently.
  xTaskCreate(TaskSend, "Task1", 128, NULL, 1, &TaskHandle_1);
  xTaskCreate(TaskPrepare, "Task2", 128, NULL, 2, &TaskHandle_2);
}

char dummyVestData(){
  randNumber = random(0, 10);
  if(randNumber == 5){
    return 255;
    }
  else{
    return 0;
    }
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
    TxPacket.hit = dummyVestData();
    vTaskDelay( 50 / portTICK_PERIOD_MS ); 
  }
}
