#include <Arduino.h>

#include <Arduino_FreeRTOS.h>
#include <semphr.h>


/*--------------------------------*/
/*----------DECLARATIONS----------*/
/*--------------------------------*/

// ---Data types---
enum State {
  OFF = 0,
  GREEN = 1,
  YELLOW = 2,
  RED = 3,
  ERROR_ON = 4,
  ERROR_OFF = 5
};

// ---Global variables & constants---
const String VERSION = "1.2.1";
// Traffic light timer
TickType_t timers[4];
#define GREEN_T 0
#define YELLOW_T 1
#define RED_T 2
#define BLINKING_T 3
// Traffic light state
SemaphoreHandle_t stateMutex; // Mutex for mutual state access
State currentState;
// LED pins
#define GREEN_LED 4
#define YELLOW_LED 3
#define RED_LED 2

// ---Functions---
bool updateState(State newState);
// Entry state actions
State entryOFF();
State entryGREEN();
State entryYELLOW();
State entryRED();
State entryERROR_ON();
State entryERROR_OFF();
// Exit state actions
void exitGREEN();
void exitYELLOW();
void exitRED();
void exitERROR_ON();
// String representation of state
String stateToString(State state);

// ---Tasks declaration---
void TaskStartingSequence(void *pvParameters);
TaskHandle_t xSemaphoreTaskHandle = NULL;
void TaskSemaphoreOn(void *pvParameters);
void TaskSerialComuniaction(void *pvParameters);


/*------------------------*/
/*----------MAIN----------*/
/*------------------------*/

// ---Setup---
void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Mutex initialization
  stateMutex = xSemaphoreCreateMutex();
  // Check if semaphores were created
  if(stateMutex == NULL) {
    Serial.println("ERROR: Creation of semaphore failed");
  }
  
  // Pin setup
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Task starting sequence
  xTaskCreate(TaskStartingSequence, "StartingSequence", 128, NULL, 3, NULL);

  // Initialize timers
  timers[GREEN_T] = pdMS_TO_TICKS(5000);
  timers[YELLOW_T] = pdMS_TO_TICKS(2000);
  timers[RED_T] = pdMS_TO_TICKS(7000);
  timers[BLINKING_T] = pdMS_TO_TICKS(1000);

  // Tasks creation
  xTaskCreate(TaskSemaphoreOn, "SemaphoreOn", 128, NULL, 1, &xSemaphoreTaskHandle);
  xTaskCreate(TaskSerialComuniaction, "SerialComuniaction", 128, NULL, 1, NULL);
}

// ---Loop---
void loop() {
  /* EM*/
}


/*----------------------------------*/
/*----------IMPLEMENTATION----------*/
/*----------------------------------*/

// ---Functions---
bool updateState(State newState) {
  bool r = false;

  if(xSemaphoreTake(stateMutex, portMAX_DELAY) == pdTRUE) {
    switch(newState) {
      case State::OFF:
        currentState = entryOFF();
        r = true;
        break;
      
      case State::ERROR_ON:
        switch(currentState) {
          case State::GREEN:
            exitGREEN();
            break;

          case State::YELLOW:
            exitYELLOW();
            break;

          case State::RED:
            exitRED();
            break;

          default:
            break;
        }
        currentState = entryERROR_ON();
        r = true;
        break;

      case State::ERROR_OFF:
        switch(currentState) {
          case State::ERROR_ON:
            exitERROR_ON();
            currentState = entryERROR_OFF();
            r = true;
            break;

          default:
            break;
        }
        break;

      case State::GREEN:
        switch(currentState) {
          case State::RED:
            exitRED();
            currentState = entryGREEN();
            r = true;
            break;
          case State::ERROR_ON:
            exitERROR_ON();
          case State::OFF:
            currentState = entryGREEN();
            r = true;
            break;

          default:
            break;
        }
        break;

      case State::YELLOW:
        switch(currentState) {
          case State::GREEN:
            exitGREEN();
            currentState = entryYELLOW();
            r = true;
            break;

          default:
            break;
        }
        break;

      case State::RED:
        switch(currentState) {
          case State::YELLOW:
            exitYELLOW();
            currentState = entryRED();
            r = true;
            break;

          default:
            break;
        }
        break;
    }

    if(currentState != State::OFF) xTaskNotifyGive(xSemaphoreTaskHandle);
    xSemaphoreGive(stateMutex);
  }

  return r;
}
// Entry state actions
State entryOFF() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  return State::OFF;
}
State entryGREEN() {
  digitalWrite(GREEN_LED, HIGH);
  return State::GREEN;
}
State entryYELLOW() {
  digitalWrite(YELLOW_LED, HIGH);
  return State::YELLOW;
}
State entryRED() {
  digitalWrite(RED_LED, HIGH);
  return State::RED;
}
State entryERROR_ON() {
  digitalWrite(YELLOW_LED, HIGH);
  return State::ERROR_ON;
}
State entryERROR_OFF() {
  return State::ERROR_OFF;
}
// Exit state actions
void exitGREEN() {
  digitalWrite(GREEN_LED, LOW);
}
void exitYELLOW() {
  digitalWrite(YELLOW_LED, LOW);
}
void exitRED() {
  digitalWrite(RED_LED, LOW);
}
void exitERROR_ON() {
  digitalWrite(YELLOW_LED, LOW);
}
String stateToString(State state) {
  switch(state) {
    case State::OFF:
      return "OFF";
    case State::GREEN:
      return "GREEN";
    case State::YELLOW:
      return "YELLOW";
    case State::RED:
      return "RED";
    case State::ERROR_ON:
      return "ERROR_ON";
    case State::ERROR_OFF:
      return "ERROR_OFF";
    default:
      return "UNKNOWN";
  }
}

// ---Task definition---
void TaskStartingSequence(void *pvParameters) {
  (void) pvParameters;

  digitalWrite(GREEN_LED, HIGH);
  vTaskDelay(pdMS_TO_TICKS(150));
  digitalWrite(YELLOW_LED, HIGH);
  vTaskDelay(pdMS_TO_TICKS(150));
  digitalWrite(RED_LED, HIGH);
  vTaskDelay(pdMS_TO_TICKS(700));

  currentState = entryOFF();

  vTaskDelete(NULL);
}
void TaskSemaphoreOn(void *pvParameters) {
  (void) pvParameters;

  bool skipWait = false;

  for(;;) {
    if(skipWait || xTaskNotifyWait(0, 0, NULL, portMAX_DELAY) == pdTRUE) {
      skipWait = false;

      switch(currentState) {
        case State::GREEN:
          if(xTaskNotifyWait(0, 0, NULL, timers[GREEN_T]) == pdFALSE) {
            if(currentState == State::GREEN) updateState(State::YELLOW);
          } else {
            skipWait = true;
          }
          break;
        
        case State::YELLOW:
          if(xTaskNotifyWait(0, 0, NULL, timers[YELLOW_T]) == pdFALSE) {
            if(currentState == State::YELLOW) updateState(State::RED);
          } else {
            skipWait = true;
          }
          break;
        
        case State::RED:
          if(xTaskNotifyWait(0, 0, NULL, timers[RED_T]) == pdFALSE) {
            if(currentState == State::RED) updateState(State::GREEN);
          } else {
            skipWait = true;
          }
          break;

        case State::ERROR_ON:
          if(xTaskNotifyWait(0, 0, NULL, timers[BLINKING_T]) == pdFALSE) {
            if(currentState == State::ERROR_ON) updateState(State::ERROR_OFF);
          } else {
            skipWait = true;
          }
          break;

        case State::ERROR_OFF:
          if(xTaskNotifyWait(0, 0, NULL, timers[BLINKING_T]) == pdFALSE) {
            if(currentState == State::ERROR_OFF) updateState(State::ERROR_ON);
          } else {
            skipWait = true;
          }
          break;
        
        default:
          break;
      }
    }
  }
}
void TaskSerialComuniaction(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    if(Serial.available() > 0) {
      String input = Serial.readString();

      if (input.startsWith("set_")) {
        input = input.substring(4);

        if (input.startsWith("g ")) {
          timers[GREEN_T] = pdMS_TO_TICKS(input.substring(2).toInt());
          Serial.println("New GREEN timer: " + String(pdTICKS_TO_MS(timers[GREEN_T])));
        } else if (input.startsWith("y ")) {
          timers[YELLOW_T] = pdMS_TO_TICKS(input.substring(2).toInt());
          Serial.println("New YELLOW timer: " + String(pdTICKS_TO_MS(timers[YELLOW_T])));
        } else if (input.startsWith("r ")) {
          timers[RED_T] = pdMS_TO_TICKS(input.substring(2).toInt());
          Serial.println("New RED timer: " + String(pdTICKS_TO_MS(timers[RED_T])));
        } else if(input.startsWith("b ")) {
          timers[BLINKING_T] = pdMS_TO_TICKS(input.substring(2).toInt());
          Serial.println("New ERROR timer: " + String(pdTICKS_TO_MS(timers[BLINKING_T])));
        } else if(input == "error") {
          if(updateState(State::ERROR_ON)) {
            Serial.println("New state: " + stateToString(currentState));
          } else {
            Serial.println("FAIL");
          }
        } else {
          Serial.println("Invalid set command: " + input);
        }
      } else if (input.startsWith("get_")) {
        input = input.substring(4);

        if (input.startsWith("g")) {
          Serial.println("GREEN timer: " + String(pdTICKS_TO_MS(timers[GREEN_T])));
        } else if (input.startsWith("y")) {
          Serial.println("YELLOW timer: " + String(pdTICKS_TO_MS(timers[YELLOW_T])));
        } else if (input.startsWith("r")) {
          Serial.println("RED timer: " + String(pdTICKS_TO_MS(timers[RED_T])));
        } else if(input.startsWith("b")) {
          Serial.println("ERROR timer: " + String(pdTICKS_TO_MS(timers[BLINKING_T])));
        } else if(input == "state") {
          Serial.print("State: " + stateToString(currentState));
        } else if(input == "version") {
          Serial.println(VERSION);
        } else {
          Serial.println("Invalid get command: " + input);
        }
      } else if(input == "turn_on") {
        if(updateState(State::GREEN)) {
          Serial.println("New state: " + stateToString(currentState));
        } else {
          Serial.println("FAIL");
        }
      } else if(input == "turn_off") {
        if(updateState(State::OFF)) {
          Serial.println("New state: " + stateToString(currentState));
        } else {
          Serial.println("FAIL");
        }
      } else {
        Serial.println("Invalid command: " + input);
      }
    }
  }
}
