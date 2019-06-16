//Thư viện
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>; // thư viện cho màn hình LCD
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
// #include <SerialCommands.h>
#include <SerialCommand.h>

//Khởi tạo biến toàn cục
const int mositureSensor_1 = A0; //Chân analog cảm biển độ ẩm 1
const int mositureSensor_2 = A1; //Chân analog cảm biển độ ẩm 2
int mositureValue_1 = -1;        //Giá trị độ ẩm 1
int mositureValue_2 = -1;        //Giá trị độ ẩm 2
const int waterPump_1 = 4;       //Chân máy bơm 1
const int waterPump_2 = 5;       //Chân máy bơm 2
const byte RX = 3;               // Chân 6 được dùng làm chân RX
const byte TX = 2;               // Chân 5 được dùng làm chân TX
// char serialCommandBuffer[255]; //bộ nhớ đệm dùng cho command
bool PM = true;    //cờ check xem chế độ tưới là auto hay manual (true: auto, false: manual)
String PS_1 = "0"; //cờ check xem máy bơm 1 mở hay tắt
String PS_2 = "0"; //cờ check xem máy bơm 2 mở hay tắt
int MM = 35;       //độ ẩm tối thiểu

//Khởi tạo với các chân (LCD)
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Khởi tạo Serial
SoftwareSerial thisSerial(RX, TX);
// Khai báo biến sử dụng thư viện Serial Command
// SerialCommands sCmd(&thisSerial, serialCommandBuffer, sizeof(serialCommandBuffer), "\n", " ");
SerialCommand sCmd(thisSerial);

// Khai báo các xử lý:
//================== CHECK MOSITURE =================================
void printLCD()
{
  //Set lcd
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  String status;
  if (PS_1 == "1")
    status = "ON";
  else
    status = "OFF";
  lcd.print("PB1:" + status + " SS1:" + String(mositureValue_1) + "%");
  lcd.setCursor(0, 1);
  if (PS_2 == "1")
    status = "ON";
  else
    status = "OFF";
  lcd.print("PB2:" + status + " SS2:" + String(mositureValue_2) + "%");
}

void checkMositure()
{
  //Thay đổi độ ẩm nếu có gì khác
  //cảm biến 1
  // int temp = analogRead(mositureSensor_1);
  // temp = map(analogRead(mositureSensor_1), 0, 1023, 100, 0);
  if (map(analogRead(mositureSensor_1), 0, 1023, 100, 0) != mositureValue_1)
    mositureValue_1 = map(analogRead(mositureSensor_1), 0, 1023, 100, 0);

  //cảm biến 2
  // temp = analogRead(mositureSensor_2);
  // temp = map(analogRead(mositureSensor_2), 0, 1023, 100, 0);
  if (map(analogRead(mositureSensor_2), 0, 1023, 100, 0) != mositureValue_2)
    mositureValue_2 = map(analogRead(mositureSensor_2), 0, 1023, 100, 0);
  //thay đổi màn hình LCD
  printLCD();
}

//================== AUTO MODE =====================================
void autoMode()
{
  checkMositure();
  //máy bơm 1
  if (mositureValue_1 < MM)
  {
    digitalWrite(waterPump_1, HIGH);
    PS_1 = "1";
  }
  else
  {
    digitalWrite(waterPump_1, LOW);
    PS_1 = "0";
  }
  //máy bơm 2
  if (mositureValue_2 < MM)
  {
    digitalWrite(waterPump_2, HIGH);
    PS_2 = "1";
  }
  else
  {
    digitalWrite(waterPump_2, LOW);
    PS_2 = "0";
  }
}

//===================THÔNG TIN CHUNG=======================================
void getInfo()
{

  //Lấy data
  char *port_str = sCmd.next();
  if (port_str == NULL)
  {
    Serial.println("No_port");
    // thisSerial.println("resRESU {\"status\":NoPort,\"msg\":\"No_port\"}");
    thisSerial.print("resRESU"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print("{\"status\":NoPort,\"msg\":\"No_port\"}");
    thisSerial.print('\r');
    return;
  }

  //Xuất ra monitor
  Serial.print("<<<-getINFO ");
  Serial.println(port_str);
  // sender->GetSerial()->println("resINFO haha");

  //Tao doi tuong Json
  const int capacity = JSON_OBJECT_SIZE(6);
  String output;
  StaticJsonDocument<capacity> docJson; //Tao doi tuong docJson
  DeserializationError err = deserializeJson(docJson, port_str);

  if (err)
  {
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    Serial.print(F("resRESU "));
    Serial.println(msg);
    thisSerial.print("resRESU"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print(msg);
    thisSerial.print('\r');
    return;
  }
  docJson["status"] = "Ok";
  docJson["PM"] = PM;
  // String ps = String(PS_1) + '_' + String(PS_2);
  docJson["PS"] = PS_1 + '_' + PS_2;
  docJson["MM"] = MM;
  serializeJson(docJson, output); //Convert JSON -> String

  thisSerial.print("resINFO"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(output);
  thisSerial.print('\r');

  //Xuất ra monitor
  Serial.print("->>>");
  Serial.println("resINFO " + output);
  // sCmd.clearBuffer();
}
//Khởi tạo biến nhận lệnh từ ESP8266
// SerialCommand getInfo_("getINFO", getInfo);

//================= MOSITURE =========================
void getMositure()
{

  //Lấy data
  char *port_str = sCmd.next();
  if (port_str == NULL)
  {
    Serial.println("No_port");
    // thisSerial.println("resRESU {\"status\":NoPort,\"msg\":\"No_port\"}");
    thisSerial.print("resRESU"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print("{\"status\":NoPort,\"msg\":\"No_port\"}");
    thisSerial.print('\r');
    return;
  }

  //Xuất ra monitor
  Serial.print("<<<-getMOSI ");
  Serial.println(port_str);

  //Check độ ẩm
  checkMositure();

  //Tao doi tuong Json
  const int capacity = JSON_OBJECT_SIZE(7);
  String output;
  StaticJsonDocument<capacity> docJson; //Tao doi tuong docJson
  DeserializationError err = deserializeJson(docJson, port_str);

  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    thisSerial.print("resRESU"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print(msg);
    thisSerial.print('\r');
    return;
  }

  docJson["status"] = "Ok";
  docJson["value"] = String(mositureValue_1) + '_' + String(mositureValue_2);
  serializeJson(docJson, output); //Convert JSON -> String
  // sender->GetSerial()->println("resMOSI " + output); //gửi lệnh
  thisSerial.print("resMOSI"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(output);
  thisSerial.print('\r');

  //Xuất ra monitor
  Serial.print("->>>");
  Serial.println("resMOSI " + output);

  sCmd.clearBuffer();
}
//Khởi tạo biến nhận lệnh từ ESP8266
// SerialCommand getMositure_("getMOSI", getMositure);

//=================== FROM ESP8266 =======================================
//Command handle
void setCommand()
{
  //Lấy data
  char *port_str = sCmd.next();
  if (port_str == NULL)
  {
    Serial.println("No_port");
    Serial.println("->>>resRESU {\"status\":NoPort,\"msg\":\"No_port\"}");
    // sender->GetSerial()->println("resRESU {\"status\":NoPort,\"msg\":\"No_port\"}");
    thisSerial.print("resRESU"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print("{\"status\":NoPort,\"msg\":\"No_port\"}");
    thisSerial.print('\r');
    return;
  }
  //Xuất ra monitor
  Serial.print("<<<-setCMD ");
  Serial.println(port_str);
  // sCmd.clearBuffer();

  //JSON handle
  const int capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> docJson;
  DeserializationError err = deserializeJson(docJson, port_str);

  // Serial.println(err.c_str());
  if (err)
  {
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    thisSerial.print("resRESU"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print(msg);
    thisSerial.print('\r');
    Serial.print(F("resRESU "));
    Serial.println(msg);
    return;
  }

  String tempPS = docJson["PS"].as<String>();

  //thay đổi mode

  PM = docJson["PM"];

  //đóng mở máy bơm 1
  if (PS_1 != tempPS[0])
  {
    if (PM == true)
      PM = false;
    PS_1 = tempPS[0];
    //Mở/tắt máy bơm bằng tay
    if (PS_1 == "1")
      digitalWrite(waterPump_1, HIGH);
    else
      digitalWrite(waterPump_1, LOW);
  }

  //đóng mở máy bơm 1
  if (PS_2 != tempPS[2])
  {
    if (PM == true)
      PM = false;
    PS_2 = tempPS[2];
    //Mở/tắt máy bơm bằng tay
    if (PS_2 == "1")
      digitalWrite(waterPump_2, HIGH);
    else
      digitalWrite(waterPump_2, LOW);
  }

  //Đổi mức độ ẩm
  MM = docJson["MM"];

  int idSent = docJson["IS"].as<int>();
  String idSent_str = String(idSent);
  char msg[strlen("{\"status\":\"\",\"msg\":\"Success\",\"idSent\":\"\"}") + strlen(idSent_str.c_str())];
  sprintf(msg, "{\"status\":\"Ok\",\"msg\":\"Success\", \"idSent\":\"%d\"}", idSent);

  Serial.print("->>>resRESU ");
  Serial.println(msg);

  thisSerial.print("resRESU"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(msg);
  thisSerial.print('\r');
  //thay đổi màn hình LCD
  printLCD();
}
// -> set cho tớ các biến như thế này
// SerialCommand setCommand_("set", setCommand);

//==============================XỬ LÝ LỆNH TÀO LAO =========================
void cmdUnrecognized(String cmd)
{
  Serial.print("==> ERROR: Unrecognized command ");
  Serial.println(cmd);
}

//=============================== MAIN ===================================
void setup()
{
  lcd.init();      //Khởi động màn hình. Bắt đầu cho phép Arduino sử dụng màn hình
  lcd.backlight(); //Bật đèn nền
  lcd.setCursor(0, 0);
  lcd.print("ExplodingKettens");

  //Khởi tạo Serial ở baudrate 115200 để debug ở serial monitor
  Serial.begin(115200);
  //Khởi tạo Serial ở baudrate 57600 cho cổng Serial thứ hai, dùng cho việc kết nối với ESP8266
  thisSerial.begin(57600);

  //Setup chế độ của cổng
  pinMode(mositureSensor_1, INPUT);
  pinMode(mositureSensor_2, INPUT);

  pinMode(waterPump_1, OUTPUT);
  pinMode(waterPump_2, OUTPUT);

  //Khai báo xử lý khi có lệnh từ ESP8266
  // sCmd.AddCommand(&getInfo_);
  // sCmd.AddCommand(&getMositure_);
  // sCmd.AddCommand(&setCommand_);
  // sCmd.SetDefaultHandler(&cmdUnrecognized);

  sCmd.addCommand("getINFO", getInfo);
  sCmd.addCommand("getMOSI", getMositure);
  sCmd.addCommand("setCMD", setCommand);
  sCmd.addDefaultHandler(cmdUnrecognized);

  //Khi đã xong
  Serial.println("Setup completed! I'm ready :)");
}

void loop()
{
  //Đọc lệnh được gửi từ ESP8266
  sCmd.readSerial();
  //Tự động tưới nếu cờ auto đang bật
  // checkMositure();
  if (PM == true)
    autoMode();
}