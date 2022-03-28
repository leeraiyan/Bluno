#include <Arduino_FreeRTOS.h>
bool handshake_completed;
bool dataCommand = false;
bool isAwaitingACK = false;
int ACKTimeOut = 0;

int randNumber;
long beginTime;


struct DataPacket
{
  char hit; 
};
DataPacket TxPacket;

// define two tasks for Blink & AnalogRead
void TaskSend( void *pvParameters );

TaskHandle_t TaskHandle_1;

//HARDWARE CODE-------------------------------------------
#include <IRremote.hpp>
#define IR_RECEIVE_PIN 2
//HARDWARE CODE-------------------------------------------


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  randomSeed(500);
  // Now set up two tasks to run independently.
  xTaskCreate(TaskSend, "Task1", 128, NULL, 1, &TaskHandle_1);


//HARDWARE CODE-------------------------------------------
  IrReceiver.begin(IR_RECEIVE_PIN, false);
//HARDWARE CODE-------------------------------------------
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

void resetFlags(){
  handshake_completed = false;
  dataCommand = false;
}

void loop() {
}


void TaskSend(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  int queueCounter = 0;
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
        DataPacket HitPacket;
        
        // put your main code here, to run repeatedly:
        if(IrReceiver.decodeNEC()) {
          if(IrReceiver.read()->decodedRawData == 0xDEAD) {
            queueCounter++;
            } 



           //now check if you are still awaiting an ACK
           if (isAwaitingACK){
            
            //i need to send a prompt
            if (ACKTimeOut >= 20){
              HitPacket.hit = 170;
              Serial.write((uint8_t*) &HitPacket, sizeof(HitPacket));
              ACKTimeOut = 0;
              long timeSinceLastReply = millis() - beginTime;
              if (timeSinceLastReply < 6000)
                vTaskDelay( 50 / portTICK_PERIOD_MS );
              else{
                vTaskDelay( 1000 / portTICK_PERIOD_MS );  
              }
            
            }

            //there is not need to send a prompt
            else{
            HitPacket.hit  = 0;
            ACKTimeOut += 1;
            Serial.write((uint8_t*) &HitPacket, sizeof(HitPacket));
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
            if (queueCounter == 0){
              HitPacket.hit = 0;
              Serial.write((uint8_t*) &HitPacket, sizeof(HitPacket));
              long timeSinceLastReply = millis() - beginTime;
              if (timeSinceLastReply < 6000)
                vTaskDelay( 50 / portTICK_PERIOD_MS );
              else{
                vTaskDelay( 1000 / portTICK_PERIOD_MS );
              } 
            }

            //someting in the q
            else{
              queueCounter --;
              HitPacket.hit = 255;
              Serial.write((uint8_t*) &HitPacket, sizeof(HitPacket));
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





           
          }
        IrReceiver.resume();
     
        long timeSinceLastReply = millis() - beginTime;
        if (timeSinceLastReply < 6000)
          vTaskDelay( 50 / portTICK_PERIOD_MS );
        else{
          vTaskDelay( 1000 / portTICK_PERIOD_MS );
          } 
        }
  }
}
