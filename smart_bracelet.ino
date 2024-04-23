

#include "utilities.h"
#include <JC_Button.h>
//#include <GSM.h>

#define TINY_GSM_DEBUG SerialMon
#define SerialMon Serial
#include <TinyGsmClient.h>
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
Adafruit_MPU6050 mpu;

#define SMS_TARGET "+84335644677"  //Change the number you want to send sms message
const byte
  BUTTON_PIN(14),  // connect a button switch from this pin to ground
  LED_PIN(2);      // the standard Arduino "pin 13" LED

Button myBtn(BUTTON_PIN);  // define the button
int maxIndexSMS = 0;

sensors_event_t a, g, temp;
float avgx, avgy, avgz;

void setup() {
  Serial.begin(115200);
  myBtn.begin();
  SerialAT.begin(115200, SERIAL_8N1);
  Serial.println("Start modem...");
  delay(3000);
  while (!modem.testAT()) {
    delay(10);
  }
  pinMode(14, INPUT_PULLUP);
    mpu.begin();
  //setupt motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);  // Keep it latched.  Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

}

void loop() {

  myBtn.read();
  readSensor(millis(), 100);

 if (detectFall(7)) {
    Serial.println("detect");
 }

  if (SerialAT.available()) {

   // Serial.write(SerialAT.read());
    String s = _readSerial();
    if (s.indexOf("+CMTI:") != -1) {
      Serial.print("receive a SMS  ");
      String num = s.substring(s.indexOf(",") + 1, s.indexOf("\n") - s.indexOf(","));
      maxIndexSMS = num.toInt();
      Serial.println(maxIndexSMS);
    }
    else
    {
      Serial.println(s);
    }
  }
  if (Serial.available()) {
    SerialAT.write(Serial.read());
  }
  delay(1);
  if (myBtn.wasReleased()) {

    Serial.print("read sms    ");
    Serial.print(maxIndexSMS);
    Serial.print("    ");
    String c = getContentSMS(maxIndexSMS);
    Serial.println(c);
    //sendSMS();
  }
}
String getContentSMS(int index)
{
  String _buffer = readSMS(index);
  uint8_t _idx1 = _buffer.indexOf("Pass ");
  if (_idx1 != -1)
  {
    return _buffer.substring(_idx1+5, _idx1+11);
    
  }
  
}

bool receiveSMS() {
}
void sendSMS() {
  Serial.print("Init success, start to send message to  ");
  Serial.println(SMS_TARGET);

  String imei = modem.getIMEI();
  bool res = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);

  Serial.print("Send sms message ");
  Serial.println(res ? "OK" : "fail");
}

String readSMS(uint8_t index) {
  String _buffer;
  SerialAT.print(F("AT+CMGF=1\r"));
  if ((_readSerial().indexOf("ER")) == -1) {
    SerialAT.print(F("AT+CMGR="));
    SerialAT.print(index);
    SerialAT.print("\r");
    _buffer = _readSerial();
    if (_buffer.indexOf("CMGR:") != -1) {
      return _buffer;
    } else return "";
  } else
    return "";
}
String _readSerial() {
  uint16_t _timeout = 0;
  while (!SerialAT.available() && _timeout < 12000) {
    delay(13);
    _timeout++;
  }
  if (SerialAT.available()) {
    return SerialAT.readString();
  }
}



String getNumberSMS(uint8_t index) {
  String _buffer = readSMS(index);
  Serial.println(_buffer.length());
  if (_buffer.length() > 10)  //avoid empty sms
  {
    uint8_t _idx1 = _buffer.indexOf("+CMGR:");
    _idx1 = _buffer.indexOf("\",\"", _idx1 + 1);
    return _buffer.substring(_idx1 + 3, _buffer.indexOf("\",\"", _idx1 + 4));
  } else {
    return "";
  }
}
bool delAllSMS() {
  SerialAT.print(F("at+cmgda=1,1\n\r"));
  String _buffer = _readSerial();
  if (_buffer.indexOf("OK") != -1) {
    maxIndexSMS = 0;
    return true;
  } else {
    return false;
  }
}
void readSensor(uint32_t now, uint16_t t) {
  static uint32_t lastRead;
  static float valX[3];
  static float valY[3];
  static float valZ[3];
  static uint8_t i = 0;
  if (now >= lastRead + t) {
    lastRead = now;

    mpu.getEvent(&a, &g, &temp);

    valX[i] = a.acceleration.x;
    valY[i] = a.acceleration.y;
    valZ[i] = a.acceleration.z;
    i += 1;
  }
  if (i >= 3) {
    float tempx = 0, tempy = 0, tempz = 0;
    for (uint8_t a = 0; a < 4; a++) {
      tempx += valX[a];
      tempy += valY[a];
      tempz += valZ[a];
    }
    avgx = tempx / 3;
    avgy = tempy / 3;
    avgz = tempz / 3;
    i = 0;
  }
}
bool detectFall(uint8_t val) {
  if (mpu.getMotionInterruptStatus()) {
    mpu.getEvent(&a, &g, &temp);
    float xchange = avgx - a.acceleration.x;
    float ychange = avgy - a.acceleration.y;
    float zchange = avgz - a.acceleration.z;
    float sum = abs(xchange) + abs(ychange) + abs(zchange);
    if (sum > val && (abs(g.gyro.x) > 1.2 || abs(g.gyro.y) > 1.2 || abs(g.gyro.z) > 1.2)) {
      return true;
    }
  }
  return false;
}