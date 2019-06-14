//Thư viện
#include <Arduino.h>
//#include <LiquidCrystal_I2C.h>; // thư viện cho màn hình LCD
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <SerialCommands.h>

//Khởi tạo biến toàn cục
const int mositureSensor = A0;       //Chân analog cảm biển độ ẩm
int mositureValue = -1;              //Giá trị độ ẩm
const int waterPump = 4;             //Chân máy bơm
const byte RX = 6;                   // Chân 6 được dùng làm chân RX
const byte TX = 5;                   // Chân 5 được dùng làm chân TX
const char serialCommandBuffer[256]; //bộ nhớ đệm dùng cho command
bool pumbMode = true;                //cờ check xem máy bơm là auto hay manual (true: auto, false: manual)

//Khởi tạo với các chân (LCD)
// LiquidCrystal_I2C lcd(0x27,16,2);

//Khởi tạo Serial
SoftwareSerial thisSerial(RX, TX);
// Khai báo biến sử dụng thư viện Serial Command
SerialCommands sCmd(&thisSerial, serialCommandBuffer, sizeof(serialCommandBuffer), "\r\n", " ");

// Khai báo các xử lý:
//===================THÔNG TIN CHUNG=======================================
void resInfo() //phản hồi yêu cầu độ ẩm của ESP8266
{
  //Tao doi tuong Json
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(3);
  String jsonCmd;
  StaticJsonDocument<BUFFER_SIZE> jsonDoc; //Tao doi tuong JsonDoc
  jsonDoc["mositure"] = mositureValue;
  jsonDoc["pumbMode"] = pumbMode;
  jsonDoc["pumbStatus"] = digitalRead(waterPump);
  serializeJson(jsonDoc, jsonCmd);          //Convert JSON -> String
  thisSerial.println("resINFO " + jsonCmd); //gửi lệnh

  //Xuất ra monitor
  Serial.print("->>>");
  Serial.println("resINFO " + jsonCmd);
}

void getInfo(SerialCommands *sender)
{

  //Lấy data
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    thisSerial.println("resRESULT {\"value\":false,\"msg\":\"ERROR NO_PORT\"}");
    return;
  }

  //Xuất ra monitor
  Serial.print("<<<-getINFO");
  Serial.println(port_str);

  //Gọi hàm phản hồiy
  resInfo();
}

void resMositure() //gửi độ ẩm cho ESP8266 nếu có thay đổi
{
  //Tao doi tuong Json
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(1);
  String jsonCmd;
  StaticJsonDocument<BUFFER_SIZE> jsonDoc; //Tao doi tuong JsonDoc
  jsonDoc["value"] = mositureValue;
  serializeJson(jsonDoc, jsonCmd);          //Convert JSON -> String
  thisSerial.println("resMOSI " + jsonCmd); //gửi lệnh

  //Xuất ra monitor
  Serial.print("->>>");
  Serial.println("resMOSI " + jsonCmd);
}

// void getMositure(SerialCommands *sender)
// {

//   //Lấy data
//   const char *port_str = sender->Next();
//   if (port_str == NULL)
//   {
//     Serial.println("ERROR NO_PORT");
//     thisSerial.println("resRESULT {\"value\":false,\"msg\":\"ERROR NO_PORT\"}");
//     return;
//   }

//   //Xuất ra monitor
//   Serial.print("<<<-getMOSI");
//   Serial.println(port_str);

//   //Gọi hàm phản hồiy
//   resInfo();
// }

//Khởi tạo biến nhận lệnh từ ESP8266
SerialCommand getInfo_("getINFO", getInfo);
// SerialCommand getMositure_("getINFO", getMositure);

//===================MÁY BƠM=======================================
//Set mode cho máy bơm
void setCommand(SerialCommands *sender)
{
  //Lấy data
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    Serial.println("->>>resRESULT {\"value\":false,\"msg\":\"ERROR NO_PORT\"}");
    thisSerial.println("resRESULT {\"value\":false,\"msg\":\"ERROR NO_PORT\"}");
    return;
  }

  if (pumbMode == false)
  {
    Serial.println("<<<-setPUMB {}");
    pumbMode = true;
    //Trả lại kết quả
    Serial.println("->>>resRESULT {\"value\":true,\"msg\":\"Success\"}");
    thisSerial.println("resRESULT {\"value\":true,\"msg\":\"Success\"}");
  }
  else
  {
    Serial.println("<<<-setPUMB {}");
    pumbMode = false;
    //Trả lại kết quả
    Serial.println("->>>resRESULT {\"value\":true,\"msg\":\"Success\"}");
    thisSerial.println("resRESULT {\"value\":true,\"msg\":\"Success\"}");
  }
}

//Tắt mở máy bơm
void controlPumb(SerialCommands *sender)
{
  //Lấy data
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    Serial.println("->>>resRESULT {\"value\":false,\"msg\":\"ERROR NO_PORT\"}");
    thisSerial.println("resRESULT {\"value\":false,\"msg\":\"ERROR NO_PORT\"}");
    return;
  }

  //Tắt chế độ bơm auto
  if (pumbMode == true)
    pumbMode = false;

  //control thôi
  if (digitalRead(waterPump) == 0)
  {
    Serial.println("<<<-contrPUMB {}");
    digitalWrite(waterPump, HIGH);
    Serial.println("->>>resRESULT {\"value\":true,\"msg\":\"Success\"}");
    thisSerial.println("resRESULT {\"value\":true,\"msg\":\"Success\"}");
  }
  else
  {
    Serial.println("<<<-contrPUMB {}");
    digitalWrite(waterPump, LOW);
    Serial.println("->>>resRESULT {\"value\":true,\"msg\":\"Success\"}");
    thisSerial.println("resRESULT {\"value\":true,\"msg\":\"Success\"}");
  }
}

// -> set cho tớ chế độ máy bơm như này
SerialCommand setPumbMode_("setPUMB", setPumbMode);
// -> máy bơm mở/tắt cho tớ
SerialCommand controlPumb_("contrPUMB", controlPumb);

//================== AUTO MODE =====================================
void autoMode()
{
  if (mositureValue < 35)
    digitalWrite(waterPump, HIGH);
  else
    digitalWrite(waterPump, LOW);
}

//================== CHECK MOSITURE =================================
void checkMositure()
{
  //Thay đổi độ ẩm nếu có gì khác
  if (analogRead(mositureSensor) != mositureValue)
  {
    mositureValue = analogRead(mositureSensor);
    mositureValue = map(mositureValue, 0, 1023, 100, 0);
    resMositure();
  }
}

void setup()
{
  // lcd.init();       //Khởi động màn hình. Bắt đầu cho phép Arduino sử dụng màn hình
  // lcd.backlight();   //Bật đèn nền
  // lcd.setCursor(0,0);
  // lcd.print("ExplodingKettens");

  //Khởi tạo Serial ở baudrate 115200 để debug ở serial monitor
  Serial.begin(115200);
  //Khởi tạo Serial ở baudrate 57600 cho cổng Serial thứ hai, dùng cho việc kết nối với ESP8266
  thisSerial.begin(9600);

  //Setup chế độ của cổng
  pinMode(mositureSensor, INPUT);
  pinMode(waterPump, OUTPUT);

  //Khai báo xử lý khi có lệnh từ ESP8266
  sCmd.AddCommand(&getInfo_);
  // sCmd.AddCommand(&getMositure_);
  sCmd.AddCommand(&setPumbMode_);
  sCmd.AddCommand(&controlPumb_);
  //Khi đã xong
  Serial.println("Setup completed! I'm ready :)");
}

void loop()
{
  //Tự động tưới nếu cờ auto đang bật
  checkMositure();
  if (pumbMode == true)
    autoMode();
  //Đọc lệnh được gửi từ ESP8266
  sCmd.ReadSerial();
}