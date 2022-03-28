#include <Arduino_FreeRTOS.h>
#include "cppQueue.h"

bool handshake_completed;
bool dataCommand = false;
bool isAwaitingACK = false;
int ACKTimeOut = 0;

int randNumber;
long beginTime;

struct DataPacket
{
  char shoot; 
};

DataPacket TxPacket;


cppQueue  q(sizeof(TxPacket), 10, FIFO); // Instantiate queue


//HARDWARE CODE-------------------------------------------
#include <Arduino.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>

#define GOD_MODE 1
#define GUN_CODE 0xDEAD

int ammo_count = 100;
long prev_shot_time = 0;

void button_ISR() {
  long current_time = millis();
  if ((ammo_count > 0 || GOD_MODE) && (current_time - prev_shot_time > 500)){
    prev_shot_time = current_time;
    IrSender.sendNECRaw(GUN_CODE);
    ammo_count -= 1;

    //INTERNAL COMMS CODE------------------------------------------------
    DataPacket ShootPacket;
    ShootPacket.shoot = 255;
    q.push(&ShootPacket);
    //INTERNAL COMMS CODE------------------------------------------------
  }
}
//HARDWARE CODE--------------------------------------------


// define two tasks for Serial Comms & Preparing Packet
void TaskSend( void *pvParameters );

TaskHandle_t TaskHandle_1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  randomSeed(199);

  // Now set up two tasks to run independently.
  xTaskCreate(TaskSend, "Task1", 128, NULL, 1, &TaskHandle_1);


//HARDWARE CODE----------------------------------------------------
  Serial.begin(115200);
  IrSender.begin();
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), button_ISR, FALLING);
//HARDWARE CODE----------------------------------------------------


}


char dummyGunData(){
  randNumber = random(0, 10);
  if(randNumber == 5){
    return 255;
    }
  else{
    return 0;
    }
  }

void resetFlags(){
  handshake_completed = false;
  dataCommand = false;
}

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
          resetFlags();
          break;
        case 'G':
          beginTime = millis();
          break;
        case 'Y':
          isAwaitingACK = false;
          break;
        }
  
      }
      if(handshake_completed && dataCommand){

        DataPacket ShootPacket;

        //i am awaiting an ACK for a shot, send the prompt command
        if (isAwaitingACK){
          if (ACKTimeOut >= 50){
            ShootPacket.shoot = 170;
            Serial.write((uint8_t*) &ShootPacket, sizeof(ShootPacket));
            ACKTimeOut = 0;
            long timeSinceLastReply = millis() - beginTime;
            if (timeSinceLastReply < 6000)
              vTaskDelay( 50 / portTICK_PERIOD_MS );
            else{
              vTaskDelay( 1000 / portTICK_PERIOD_MS );
            }
            
            }
          else{
            ShootPacket.shoot = 0;
            ACKTimeOut += 1;
            Serial.write((uint8_t*) &ShootPacket, sizeof(ShootPacket));
            long timeSinceLastReply = millis() - beginTime;
            if (timeSinceLastReply < 6000)
              vTaskDelay( 50 / portTICK_PERIOD_MS );
            else{
              vTaskDelay( 1000 / portTICK_PERIOD_MS );
            } 
            
            
            }
          
        }

        //im not awaiting an ACK, you can go ahead and send the next shoot packet
        else{

          //nothing in the ququq
          if (q.pop(&ShootPacket) == false){
            ShootPacket.shoot = 0;
            Serial.write((uint8_t*) &ShootPacket, sizeof(ShootPacket));
            long timeSinceLastReply = millis() - beginTime;
            if (timeSinceLastReply < 6000)
              vTaskDelay( 50 / portTICK_PERIOD_MS );
            else{
              vTaskDelay( 1000 / portTICK_PERIOD_MS );
            } 
          }
          //someting in the q
          else{
            Serial.write((uint8_t*) &ShootPacket, sizeof(ShootPacket));
            isAwaitingACK = true;
            ACKTimeOut = 0;
            long timeSinceLastReply = millis() - beginTime;
            if (timeSinceLastReply < 6000)
              vTaskDelay( 50 / portTICK_PERIOD_MS );
            else{
              vTaskDelay( 1000 / portTICK_PERIOD_MS );
            } 
          }
          
          }
        

        
//        Serial.write((uint8_t*) &TxPacket, sizeof(TxPacket));
//        long timeSinceLastReply = millis() - beginTime;
//        if (timeSinceLastReply < 6000)
//          vTaskDelay( 50 / portTICK_PERIOD_MS );
//        else{
//          vTaskDelay( 1000 / portTICK_PERIOD_MS );
//          } 
     }
  }
}
