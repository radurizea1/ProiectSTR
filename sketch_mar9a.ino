#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <semphr.h>
#include <task.h> 
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ACS712.h"
#include <ZMPT101B.h>
#define SENSITIVITY 500.0f//stabilitate pt senzorul de tensiune

///////////////////////////pt temp
#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;
int deviceCount = 0;
float tempC1 = 0;
float tempC2 = 0;
///////////////////////////

///////////////////////pt ampermetru
ACS712  ACS(A1, 5.0, 1023, 100);
 /////////////////////////////////////

////////////////////////pt voltmetru
ZMPT101B voltageSensor(A0, 50.0);
/////////////////////////////////////

LiquidCrystal_I2C lcd(0x27,16,2);
//valori globale
volatile float valorSensorTemperatura;
volatile int T = 50;
const int trigPin = 9;
const int echoPin = 10;
long duration;
int distance = 15;
int stare_proces = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println(F("Start!"));
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(53, OUTPUT);
  digitalWrite(53, HIGH);
  pinMode(50, INPUT);
  pinMode(51, INPUT);
  pinMode(52, INPUT);
  lcd.init();
  lcd.clear();
  lcd.backlight();
  ACS.autoMidPoint();
  voltageSensor.setSensitivity(SENSITIVITY);
  //initializare task-uri
  xTaskCreate(Tasktemp1,"task7",configMINIMAL_STACK_SIZE,NULL, 6, NULL);
  xTaskCreate(TaskA,"task6",configMINIMAL_STACK_SIZE,NULL, 7, NULL);
  xTaskCreate(TaskV,"task5",configMINIMAL_STACK_SIZE,NULL, 8, NULL);
  xTaskCreate(Taskprint,"task1",configMINIMAL_STACK_SIZE,NULL, 4, NULL);
  xTaskCreate(Taskreferinta,"task2",configMINIMAL_STACK_SIZE,NULL, 5, NULL);
  xTaskCreate(Taskdist,"task3",configMINIMAL_STACK_SIZE,NULL, 2, NULL);
  xTaskCreate(Taskbutton,"task4",configMINIMAL_STACK_SIZE,NULL, 3, NULL);
  vTaskStartScheduler();
}

void loop() {
}
//citirea butoanelor
int buton1;
int buton2;
int buton3;
void Taskbutton(void *pvParameters) {
  while(1){
    buton1 = digitalRead(50);
    buton2 = digitalRead(51);
    buton3 = digitalRead(52);
    if(buton1 == HIGH){
      Serial.println(F("buton1 apasat"));////////ok button
    }
    if(buton2 == HIGH){
      Serial.println(F("buton2 apasat"));
    }
    if(buton3 == HIGH){
      Serial.println(F("buton3 apasat"));
    }
      vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

//citirea temperaturii
void Tasktemp1(void *pvParameters) {
  while (1) {
    Serial.println("merge T");
    if(stare_proces == 5){
      
      sensors.requestTemperatures();      
      tempC1 = sensors.getTempCByIndex(0);
      Serial.print("temperatura1: ");
      Serial.println(tempC1);
      tempC2 = sensors.getTempCByIndex(1);
      Serial.print("temperatura2: ");
      Serial.print(tempC2);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }
}

//citirea curentului
float A;
void TaskA(void *pvParameters) {
  while (1) {
    Serial.println("merge A");
    if(stare_proces == 5){
      float average = 0;
       uint32_t start = millis();
       for (int i = 0; i < 10; i++)
       {
         //  select appropriate function
         //  average += ACS.mA_AC_sampling();
         average += ACS.mA_AC();
       }
       A = average / 10000.0;
       uint32_t duration = millis() - start;
       Serial.print("amp : ");
       Serial.print(A);
       vTaskDelay(501 / portTICK_PERIOD_MS);
    }
  }
}

//citirea tensiunii
float voltage;
void TaskV(void *pvParameters) {
  while (1) {
    Serial.println("merge v");
    if(stare_proces == 5){
      
      voltage = voltageSensor.getRmsVoltage();
      voltage = voltage * 0.88;
      Serial.print("voltaj: ");
      Serial.print(voltage);
      vTaskDelay(502 / portTICK_PERIOD_MS); 
    }
  }
}

//introducerea referintei
void Taskreferinta(void *pvParameters) {
  while (1) {
    if(stare_proces == 3 ){
      int sensorValue = analogRead(A4);
      T = 50 + 0.058 * (sensorValue + 1 );
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

//evaluarea distantei pt calcularea volumului lchidului
void Taskdist(void *pvParameters)  {
  while(1)
  { 
    if(stare_proces == 1){
      digitalWrite(trigPin, LOW);
      vTaskDelay(2 / portTICK_PERIOD_MS);
      digitalWrite(trigPin, HIGH);
      vTaskDelay(10 / portTICK_PERIOD_MS);
      digitalWrite(trigPin, LOW);
      duration = pulseIn(echoPin, HIGH);
      distance = duration * 0.034 / 2;
      distance = 12;
      vTaskDelay(500 / portTICK_PERIOD_MS);
    } 
  }
}

////hmi
long consum;
double t_inceput;
double t_curent;
double t_acum;
float consum_acum = 0;
void Taskprint(void *pvParameters)  {
  while(1)
  { /////////////////////////////////////////////////////////////////starea 0(Buna ziua)    
    if(stare_proces == 0) {
      lcd.setCursor(0,0);
      lcd.print("Buna ziua!");
      vTaskDelay(1800 / portTICK_PERIOD_MS);
      lcd.clear();
      stare_proces++;
    }
    /////////////////////////////////////////////////////////////////starea 1 / inceperea procesului
    if(stare_proces == 1) {
      ///////////////////ALG MESAJ MESAJ LUNG ON
      String message = "Lasati capacul jos";
      for (int i=0; i < 16; i++) {
        message = " " + message;  
      }  
      message = message + " "; 
      for (int position = 0; position < message.length(); position++) {
        if(buton1 == HIGH)
          break;
        lcd.setCursor(0, 0);
        lcd.print(message.substring(position, position + 16));
        lcd.setCursor(0,1);
        lcd.print("Apasati ok");
        vTaskDelay(400 / portTICK_PERIOD_MS);
      }
      ///////////////////ALG MESAJ MESAJ LUNG off
      if(buton1 == HIGH){
        lcd.clear();
        stare_proces ++;
      }
    }
    /////////////////////////////////////////////////////////////////starea 2 / calcularea volumului
    if(stare_proces == 2) {
      lcd.setCursor(0,0);
      lcd.print("Se proceseaza");
      vTaskDelay(600 / portTICK_PERIOD_MS);
      if(distance < 20 && distance > 1){/////////////valori experimentale
        lcd.clear();
        stare_proces ++;
      }else{
        lcd.clear();
        lcd.setCursor(0,0);
        /////////////////////////scrollMessage(0,"Cantitate de apa nonconforma", 250, 16);
        ///////////////////ALG MESAJ MESAJ LUNG ON
      String message = "Cantitate de apa nonconforma";
      for (int i=0; i < 16; i++) {
        message = " " + message;  
      }  
      message = message + " "; 
      for (int position = 0; position < message.length(); position++) {
        if(buton1 == HIGH)
          break;
        lcd.setCursor(0, 0);
        lcd.print(message.substring(position, position + 16));
        vTaskDelay(400 / portTICK_PERIOD_MS);
      }
      ///////////////////ALG MESAJ MESAJ LUNG off
      String message1 = "Schimbati catitatea de lichid";
      for (int i=0; i < 16; i++) {
        message1 = " " + message1;  
      }  
      message1 = message1 + " "; 
      for (int position = 0; position < message1.length(); position++) {
        if(buton1 == HIGH)
          break;
        lcd.setCursor(0, 0);
        lcd.print(message1.substring(position, position + 16));
        vTaskDelay(400 / portTICK_PERIOD_MS);
      }
      ///////////////////ALG MESAJ MESAJ LUNG off
        lcd.setCursor(0,1);
        lcd.print(distance);
        vTaskDelay(1600 / portTICK_PERIOD_MS);
        lcd.clear();
        stare_proces = 1;
      }
    }
    /////////////////////////////////////////////////////////////////starea 3 / alegerea referintei
    if(stare_proces == 3){
      lcd.setCursor(0,0);
       ///////////////////ALG MESAJ MESAJ LUNG ON
      String message = "Introduceti referinta";
      for (int i=0; i < 16; i++) {
        message = " " + message;  
      }  
      message = message + " "; 
      for (int position = 0; position < message.length(); position++) {
        if(buton1 == HIGH)
          break;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(message.substring(position, position + 16));
        lcd.setCursor(0,1);
        lcd.print("Ref = ");
        lcd.setCursor(6,1);
        lcd.print(T);
        lcd.setCursor(10,1);
        lcd.print("ok");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        
      }
      ///////////////////ALG MESAJ MESAJ LUNG off
      if(buton1 == HIGH){
        lcd.clear();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        stare_proces ++;
      }
    }
    /////////////////////////////////////////////////////////////////starea 4 / calculul volumului si algoritmul de predictie
    if(stare_proces == 4){
      consum = 63.27 * ((18 - distance) * 0.176625) + 1.37 * (T - 22) - 64.01;
      lcd.setCursor(0, 0);
      lcd.print("predict cons:");
      lcd.setCursor(14, 0);
      lcd.print(consum);
      lcd.setCursor(0, 1);
      lcd.print("kWh ok/back/tea");
      if(buton1 == HIGH){
        lcd.clear();
        A = 0;
        T = T - 2;
        lcd.setCursor(0, 0);
        t_inceput = millis();
        lcd.print("Consum:");
        lcd.setCursor(8, 0);
        lcd.print("Temps:");
        digitalWrite(53, LOW);
        stare_proces = 5;
      }
      if(buton2 == HIGH){///////////////////////////////////reintroduce referinta
        lcd.clear();
        stare_proces = 3;
      }
      if(buton3 == HIGH){///////////////////////////////////mod ceai
        lcd.clear();
        A = 0;
        T = 70;
        lcd.setCursor(0, 0);
        lcd.print("Noul consum:");
        consum = 63.27 * (18 - distance) * 7.25 * 7.25 * 3.14 + 1.37 * (T - 22) - 64.01;
        lcd.setCursor(0, 1);
        lcd.print(consum);
        t_inceput = 0;
        digitalWrite(53, LOW);
        stare_proces = 8;
      }
    }
    /////////////////////////////////////////////////////////////////starea 5 / urmarirea referintei si calculul consumului de energie live
    if(stare_proces == 5){
      t_curent =  millis();
      t_acum =(t_curent - t_inceput) / 1000 * 0.000277778;
      if(voltage > 235)
        voltage = 235;
      if(A > 0.2)
        consum_acum = A * voltage * t_acum ;
      lcd.setCursor(0, 1);
      lcd.print(consum_acum);
      lcd.setCursor(6, 1);
      lcd.print(tempC1+2);
      lcd.setCursor(12, 1);
      lcd.print(tempC2);
      if(tempC1 > T || tempC2 > T){
        digitalWrite(53, HIGH);
        lcd.clear();
        stare_proces ++;
      }
    }
    //////////////////////////////////////////////////////////////////starea 6 / final
    if(stare_proces == 6){
      lcd.setCursor(0, 0);
      lcd.print("Consumul este:");
      lcd.setCursor(0, 1);
      lcd.print(consum_acum);
    }
    //////////////////////////////////////////////////////////////////starea 8 / mod ceai step1
    if(stare_proces == 8){
      t_curent =  millis();
      t_acum =(t_curent - t_inceput) / 1000 * 0.000277778;
      if(voltage > 235)
        voltage = 235;
      if(A > 0.2)
        consum_acum = A * voltage * t_acum ;
      lcd.setCursor(0, 1);
      lcd.print(consum_acum);
      lcd.setCursor(6, 1);
      lcd.print(tempC1+2);
      lcd.setCursor(12, 1);
      lcd.print(tempC2);
      if(tempC1 > T || tempC2 > T){
        digitalWrite(53, HIGH);
        lcd.clear();
        stare_proces = 6;
   }
}
}
}
