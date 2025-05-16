#include <Arduino.h>

#include <Arduino_FreeRTOS.h>


// -Global variables-
int timer = 1000;

// -Tasks declaration-
void TaskSemaphore(void *pvParameters);
void TaskSerialComuniaction(void *pvParameters);

// -Setup-
void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Pin setup
  pinMode(LED_BUILTIN, OUTPUT);

  // LED initialization
  digitalWrite(LED_BUILTIN, LOW);

  // Tasks creation
  xTaskCreate(TaskSemaphore, "Semaphore", 128, NULL, 2, NULL);
  xTaskCreate(TaskSerialComuniaction, "SerialComuniaction", 128, NULL, 1, NULL);
}

// -Loop-
void loop() {
  // Empty
}

// -Task definition-
void TaskSemaphore(void *pvParameters) {
  (void) pvParameters;

  for(;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay( timer / portTICK_PERIOD_MS );
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay( timer / portTICK_PERIOD_MS );
  }
}

void TaskSerialComuniaction(void *pvParameters) {
  (void) pvParameters;
  
  for(;;) {
    if(Serial.available() > 0) {
      String input = Serial.readString();
      if(input.startsWith("T:") || input.startsWith("t:")) {
        timer = input.substring(2).toInt();

        Serial.print("U:");
        Serial.println(timer);
      }
    }
  }
}
