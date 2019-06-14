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
bool pumbMode = true;                //cờ check xem chế độ tưới là auto hay manual (true: auto, false: manual)
bool pumbStatus = false;             //cờ check xem máy bơm mở hay tắt
//Khởi tạo với các chân (LCD)
// LiquidCrystal_I2C lcd(0x27,16,2);

//Khởi tạo Serial
SoftwareSerial thisSerial(RX, TX);
// Khai báo biến sử dụng thư viện Serial Command
SerialCommands sCmd(&thisSerial, serialCommandBuffer, sizeof(serialCommandBuffer), "\r\n", " ");

// Khai báo các xử lý:
//================== CHECK MOSITURE =================================
void checkMositure()
{
  delay(3000);
  //Thay đổi độ ẩm nếu có gì khác
  int temp = analogRead(mositureSensor);
  temp = map(temp, 0, 1023, 100, 0);
  if (temp != mositureValue)
    mositureValue = temp;
}

//================== AUTO MODE =====================================
void autoMode()
{
  checkMositure();
  if (mositureValue < 35)
    digitalWrite(waterPump, HIGH);
  else
    digitalWrite(waterPump, LOW);
}

//===================THÔNG TIN CHUNG=======================================
void resInfo(char *port_str) //phản hồi yêu cầu độ ẩm của ESP8266
{
  //Tao doi tuong Json
  const int capacity = JSON_OBJECT_SIZE(8);
  String output;
  StaticJsonDocument<capacity> docJson; //Tao doi tuong docJson
  DeserializationError err = deserializeJson(docJson, port_str);

  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    thisSerial.print("resRESULT ");
    thisSerial.println(msg);
    return;
  }
  docJson["status"] = "Ok";
  docJson["pumbMode"] = pumbMode;
  docJson["pumbStatus"] = pumbStatus;
  serializeJson(docJson, output);          //Convert JSON -> String
  thisSerial.println("resINFO " + output); //gửi lệnh

  //Xuất ra monitor
  Serial.print("->>>");
  Serial.println("resINFO " + output);
}

void getInfo(SerialCommands *sender)
{

  //Lấy data
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("No_port");
    thisSerial.println("resRESULT {\"status\":NoPort,\"msg\":\"No_port\"}");
    return;
  }

  //Xuất ra monitor
  Serial.print("<<<-getINFO ");
  Serial.println(port_str);

  //Gọi hàm phản hồiy
  resInfo(port_str);
}

//Khởi tạo biến nhận lệnh từ ESP8266
SerialCommand getInfo_("getINFO", getInfo);

//================= MOSITURE =========================

void resMositure() //gửi độ ẩm cho ESP8266 nếu có thay đổi
{
  //Check độ ẩm
  checkMositure();

  //Tao doi tuong Json
  const int capacity = JSON_OBJECT_SIZE(5);
  String output;
  StaticJsonDocument<capacity> docJson; //Tao doi tuong docJson
  docJson["status"] = "Ok";
  docJson["value"] = mositureValue;
  serializeJson(docJson, output);          //Convert JSON -> String
  thisSerial.println("resMOSI " + output); //gửi lệnh

  //Xuất ra monitor
  Serial.print("->>>");
  Serial.println("resMOSI " + output);
}

void getMositure(SerialCommands *sender)
{

  //Lấy data
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("No_port");
    thisSerial.println("resRESULT {\"status\":NoPort,\"msg\":\"No_port\"}");
    return;
  }

  //Xuất ra monitor
  Serial.print("<<<-getMOSI");
  Serial.println(port_str);

  //Gọi hàm phản hồiy
  resMositure();
}

//Khởi tạo biến nhận lệnh từ ESP8266
SerialCommand getMositure_("getMOSI", getMositure);

//=================== FROM ESP8266 =======================================
//Command handle
void setCommand(SerialCommands *sender)
{
  //Lấy data
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("No_port");
    Serial.println("->>>resRESULT {\"status\":NoPort,\"msg\":\"No_port\"}");
    thisSerial.println("resRESULT {\"status\":NoPort,\"msg\":\"No_port\"}");
    return;
  }
  //Xuất ra monitor
  Serial.print("<<<-setCMD ");
  Serial.println(port_str);

  //JSON handle
  const int capacity = JSON_OBJECT_SIZE(5);
  StaticJsonDocument<capacity> docJson;
  DeserializationError err = deserializeJson(docJson, port_str);

  // Serial.println(err.c_str());
  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    thisSerial.print("resRESULT ");
    thisSerial.println(msg);
    return;
  }

  //Change
  if (pumbMode != docJson["pumbMode"])
    pumbMode = docJson["pumbMode"];
  if (pumbStatus != docJson["pumbStatus"])
  {
    pumbStatus = docJson["pumbStatus"];
    //Mở/tắt máy bơm bằng tay
    if (pumbStatus == true)
    {
      if (pumbMode == true)
        pumbMode = false;
      digitalWrite(waterPump, HIGH);
    }
    else
      digitalWrite(waterPump, LOW);
  }

  char msg[strlen("{\"status\":\"\",\"msg\":\"Success\"}") + strlen(err.c_str())];
  sprintf(msg, "{\"status\":\"%s\",\"msg\":\"Success\"}", err.c_str());
  thisSerial.print("resRESULT ");
  thisSerial.println(msg);
}

// -> set cho tớ các biến như thế này
SerialCommand setCommand_("setCMD", setCommand);

//=============================== MAIN ===================================
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
  sCmd.AddCommand(&getMositure_);
  sCmd.AddCommand(&setCommand_);
  // sCmd.AddCommand(&controlPumb_);
  //Khi đã xong
  Serial.println("Setup completed! I'm ready :)");
}

void loop()
{
  //Tự động tưới nếu cờ auto đang bật
  // checkMositure();
  if (pumbMode == true)
    autoMode();

  //Đọc lệnh được gửi từ ESP8266
  sCmd.ReadSerial();
}