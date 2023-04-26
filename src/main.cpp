#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEUtils.h>

#define NAME_DEVICE "ESP_BLE"

#define SERVICE_CONTROL_UUID   "6f59f19e-2f39-49de-8525-5d2045f4d999" 
#define SERVICE_WORK_TIME_UUID "f790145d-61dd-4464-9414-c058448ee9f2"

#define CONTROL_REQUEST_UUID "420ece2e-c66c-4059-9ceb-5fc19251e453"
#define CONTROL_RESPONSE_UUID "a9bf2905-ee69-4baa-8960-4358a9e3a558"

#define WORK_TIME_UUID "5e9f22d4-a305-4113-8fa2-55c5d497c3f2"

#define PIN_LED_FIRST  2
#define PIN_LED_SECOND 26
#define PIN_SERVO      27

#define CMD_ENABLE_LED  0x1
#define CMD_SERVO_ANGLE 0x2 

BLECharacteristic* controlRequest;
BLECharacteristic* controlResponse;
BLECharacteristic* workTime;

//Servo servo;

unsigned long timerMillis = 0;
bool isConnected = false;

void setupBluetooth();
void setupLeds();
void setupServo();
void enableLed(int pin, bool enable);
void rotateServo(int angle);
void sendWorkedTime(int seconds);
void sendDeviceState();


class BluetoothServerEventCallback : public BLEServerCallbacks {
  void onConnect(BLEServer * server) 
  {
    isConnected = true;
    sendDeviceState(); 
  } 
  void onDisconnect(BLEServer * server) 
  {
    isConnected = false;
  }
};

class BluetoothEventCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) 
  {
    uint8_t* data = characteristic->getData();
    if (data[0] == CMD_ENABLE_LED) { 
      enableLed(data[1], data[2] > 0); 
    } else if (data[0] == CMD_SERVO_ANGLE) {
      rotateServo(data[1]);
    }
    sendDeviceState();
  }
}; 

void setup() {
  Serial.begin(115200);
  Serial.println("Launching...");

  Serial1.begin(115200, SERIAL_8N1, -1, -1, true, 20000UL);


  setupBluetooth();
  setupLeds(); 
  setupServo();
}

void loop() { 
  if (millis() - timerMillis > 5000) {
    timerMillis = millis();

    sendWorkedTime(timerMillis / 1000); 
  } 
}

void setupBluetooth() 
{
  BLEDevice::init(NAME_DEVICE);

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new BluetoothServerEventCallback());
  BLEService* serviceControl  = server->createService(SERVICE_CONTROL_UUID);
  controlRequest = serviceControl->createCharacteristic(CONTROL_REQUEST_UUID, BLECharacteristic::PROPERTY_WRITE);
  controlRequest->setCallbacks(new BluetoothEventCallback());
  controlResponse = serviceControl->createCharacteristic(CONTROL_RESPONSE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  controlResponse->addDescriptor(new BLE2902()); 

  BLEService* serviceWorkTime = server->createService(SERVICE_WORK_TIME_UUID);
  workTime = serviceWorkTime->createCharacteristic(WORK_TIME_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  workTime->addDescriptor(new BLE2902());

  serviceControl->start();
  serviceWorkTime->start(); 

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_CONTROL_UUID);
  advertising->addServiceUUID(SERVICE_WORK_TIME_UUID);
  advertising->setMinPreferred(0x06); 
  advertising->setMinPreferred(0x12);
  advertising->start();  
}

void setupLeds() 
{
  pinMode(PIN_LED_FIRST, OUTPUT);
  pinMode(PIN_LED_SECOND, OUTPUT);
}

void setupServo()
{/*
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);

  servo.setPeriodHertz(50);// Standard 50hz servo
  servo.attach(PIN_SERVO, 500, 2400); */
} 

void enableLed(int pin, bool enable) 
{
  digitalWrite(pin, (enable) ? HIGH : LOW);
}

void rotateServo(int angle) 
{
  //servo.write(angle); 
}

void sendWorkedTime(int seconds)
{
  if (isConnected) {
    workTime->setValue(seconds);
    workTime->notify();
  }
}

void sendDeviceState() 
{
  if (isConnected) {
    uint8_t data[5];
    data[0] = PIN_LED_FIRST;                    
    data[1] = digitalRead(PIN_LED_FIRST);
    data[2] = PIN_LED_SECOND; 
    data[3] = digitalRead(PIN_LED_SECOND);
    //data[4] = servo.read();
    controlResponse->setValue(data, 5);
    controlResponse->notify();
    Serial.printf("LED %d: %d, LED %d: %d, Servo: %d\n", data[0], data[1], data[2], data[3], data[4]);
  }
}