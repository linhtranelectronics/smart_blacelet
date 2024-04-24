#include "defintion.h"

const byte
  BUTTON_PIN(14),  // connect a button switch from this pin to ground
  LED_PIN(2);      // the standard Arduino "pin 13" LED

Button myBtn(BUTTON_PIN);  // define the button

int maxIndexSMS = 0;
String phoneNumber1 = "";
String phoneNumber2= "";
String phoneNumber3 = "";
String password = "123456";
bool enableChange = false;
uint32_t startChange = 0;

sensors_event_t a, g, temp;
float avgx, avgy, avgz;

enum Action
{
  NONE,
  PASSWORD,
  LOCALATION
}action;
Action run;
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
  mpu.setInterruptPinLatch(true); 
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);
  // read phone number from Memory
  savePhoneNumber.begin("PHONENUMBER", RO_MODE);
  //phoneNumber1 = savePhoneNumber.getString("num1");
  phoneNumber1 = "0335644677";
  phoneNumber2 = savePhoneNumber.getString("num2");
  phoneNumber3 = savePhoneNumber.getString("num3");
  savePhoneNumber.end();

  savePassword.begin("PASS", RO_MODE);
  password = savePassword.getString("pass1");
  Serial.println("pass " + password);
  savePassword.end();
  delAllSMS();

}

void loop() {

  myBtn.read();
  readSensor(millis(), 100);
  if (millis() > enableChange + 60000)
  {
    enableChange = false;
    Serial.println("end change");
    //modem.sendSMS(phoneNumber1, "sdt " + phoneNumber1+ "  "+ phoneNumber2 + "  "+ phoneNumber3 + 'end');
  }
  
 if (detectFall(7)) {
    Serial.println("detect");
    //modem.sendSMS(phoneNumber1, "nguoi dung bi nga');
 }

  if (SerialAT.available()) {

   // Serial.write(SerialAT.read());
    String s = _readSerial();
    if (s.indexOf("+CMTI:") != -1) {
      Serial.print("receive a SMS  ");
      String num = s.substring(s.indexOf(",") + 1, s.indexOf("\n") - s.indexOf(","));
      maxIndexSMS = num.toInt();
      Serial.println(maxIndexSMS);
      getContentSMS(maxIndexSMS);
    }
    else
    {
      Serial.println(s);
    }
  }
  if (Serial.available()) {
    SerialAT.write(Serial.read());
  }
  if (myBtn.wasReleased()) {
    delAllSMS();

    //sendSMS();
  }

}

void getContentSMS(int index)
{
  String _buffer = readSMS(index);
  String fromNum = getNumberSMS(index);
  if (_buffer.indexOf("Pass ") != -1)
  {
    uint8_t _idx1 = _buffer.indexOf("Pass ");
    if (_idx1 != -1)
    {
      String p1 = _buffer.substring(_idx1+5, _idx1+11);
      String p2 = _buffer.substring(_idx1+12, _idx1+18);
      String password = changePassword(p1, p2);
      sendSMS(phoneNumber1, "pass: " + password);
    }
  }
   if (_buffer.indexOf("Vitri") != -1)
  {
    Serial.print("from "+ fromNum + " myNum "+ phoneNumber1);
    if (fromNum == phoneNumber1 || fromNum == phoneNumber2|| fromNum == phoneNumber3)
    {
      sendSMS(fromNum, "vi tri hien tai");
    }
  }
  if (_buffer.indexOf("Change ") != -1)
  {
     uint8_t _idx1 = _buffer.indexOf("Change ");
    String p1 = _buffer.substring(_idx1+7, _idx1+13);
    if (p1 == password)
    {
      enableChange = true;
      startChange = millis();
      delay(100);
      modem.sendSMS(fromNum, "sdt " + phoneNumber1+ "  "+ phoneNumber2 + "  "+ phoneNumber3);
    }
    else sendSMS(fromNum, "sai password");

  }
  if (_buffer.indexOf("Sdt1 ") != -1 && enableChange)
  {
    uint8_t _idx1 = _buffer.indexOf("Sdt1 ");
    String p1 = _buffer.substring(_idx1+5, _idx1+15);
    savePhoneNumber.begin("PHONENUMBER", RW_MODE);
    savePhoneNumber.putString("num1", p1);
    phoneNumber1 = savePhoneNumber.getString("num1");
    Serial.println(phoneNumber1);
    savePhoneNumber.end();
    modem.sendSMS(fromNum, "sdt1 " + phoneNumber1);

  }
  if (_buffer.indexOf("Sdt2 ") != -1 && enableChange)
  {
    uint8_t _idx1 = _buffer.indexOf("Sdt2 ");
    String p1 = _buffer.substring(_idx1+5, _idx1+15);
    savePhoneNumber.begin("PHONENUMBER", RW_MODE);
    savePhoneNumber.putString("num2", p1);
    phoneNumber2 = savePhoneNumber.getString("num2");
    Serial.println(phoneNumber2);
    savePhoneNumber.end();
    modem.sendSMS(fromNum, "sdt2 " + phoneNumber2);
    
  }
  if (_buffer.indexOf("Sdt3 ") != -1 && enableChange)
  {
    uint8_t _idx1 = _buffer.indexOf("Sdt3 ");
    String p1 = _buffer.substring(_idx1+5, _idx1+15);
    savePhoneNumber.begin("PHONENUMBER", RW_MODE);
    savePhoneNumber.putString("num3", p1);
    phoneNumber1 = savePhoneNumber.getString("num3");
    Serial.println(phoneNumber3);
    savePhoneNumber.end();
    modem.sendSMS(fromNum, "sdt3 " + phoneNumber3);
    
  }

}
String changePassword(String oldPass, String newPass)
{
  if(oldPass == password)
  {
    savePassword.begin("PASS", RW_MODE);
    savePassword.putString("pass1", newPass);
    String p = savePassword.getString("pass1");
    savePassword.end();
    return p;
  }
  return password;
}
void unEnableChange(uint16_t after)
{
  if (millis() > startChange + after)
  {
    enableChange = false;
  }
  
}
bool receiveSMS() {
}
void sendSMS(String num, String content) {
  //Serial.print("Init success, start to send message to  ");
  //Serial.println(num);

  bool res = modem.sendSMS(num, content);

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
    return "0" + _buffer.substring(_idx1 + 3, _buffer.indexOf("\",\"", _idx1 + 4)).substring(3, 12);
  } else {
    return "";
  }
}
bool delAllSMS() {
  SerialAT.print(F("at+cmgd=1\n\r"));
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
