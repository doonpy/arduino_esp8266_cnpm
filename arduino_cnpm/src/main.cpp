//Thư viện
#include <Arduino.h>
//#include <LiquidCrystal_I2C.h>; // thư viện cho màn hình LCD
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
// #include <SerialCommands.h>
#include <SerialCommand.h>

//Khởi tạo biến toàn cục
const int mositureSensor = A0; //Chân analog cảm biển độ ẩm
int mositureValue = -1;        //Giá trị độ ẩm
const int waterPump = 4;       //Chân máy bơm
const byte RX = 3;             // Chân 6 được dùng làm chân RX
const byte TX = 2;             // Chân 5 được dùng làm chân TX
// char serialCommandBuffer[255]; //bộ nhớ đệm dùng cho command
bool PM = true;    //cờ check xem chế độ tưới là auto hay manual (true: auto, false: manual)
bool PS = false; //cờ check xem máy bơm mở hay tắt
int MM = 35;    //độ ẩm tối thiểu
//Khởi tạo với các chân (LCD)
// LiquidCrystal_I2C lcd(0x27,16,2);

//Khởi tạo Serial
SoftwareSerial thisSerial(RX, TX);
// Khai báo biến sử dụng thư viện Serial Command
// SerialCommands sCmd(&thisSerial, serialCommandBuffer, sizeof(serialCommandBuffer), "\n", " ");
SerialCommand sCmd(thisSerial);

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
  if (mositureValue < MM)
    digitalWrite(waterPump, HIGH);
  else
    digitalWrite(waterPump, LOW);
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
  const int capacity = JSON_OBJECT_SIZE(5);
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
  docJson["PS"] = PS;
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
  const int capacity = JSON_OBJECT_SIZE(10);
  String output;
  StaticJsonDocument<capacity> docJson; //Tao doi tuong docJson
  DeserializationError err = deserializeJson(docJson, port_str);
  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    // sender->GetSerial()->print("resRESU ");
    // sender->GetSerial()->println(msg);
    thisSerial.print("resRESU"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print(msg);
    thisSerial.print('\r');
    return;
  }

  docJson["status"] = "Ok";
  docJson["value"] = mositureValue;
  serializeJson(docJson, output); //Convert JSON -> String
  // sender->GetSerial()->println("resMOSI " + output); //gửi lệnh
  thisSerial.print("resMOSI"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(output);
  thisSerial.print('\r');

  //Xuất ra monitor
  Serial.print("->>>");
  Serial.println("resMOSI " + output);
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
    thisSerial.print("resINFO"); //gửi tên lệnh
    thisSerial.print('\r');
    thisSerial.print(msg);
    thisSerial.print('\r');
    Serial.print(F("resRESU "));
    Serial.println(msg);
    return;
  }

  //Change
  if (PM != docJson["PM"])
    PM = docJson["PM"];
  if (PS != docJson["PS"])
  {
    PS = docJson["PS"];
    //Mở/tắt máy bơm bằng tay
    if (PS == true)
    {
      if (PM == true)
        PM = false;
      digitalWrite(waterPump, HIGH);
    }
    else
      digitalWrite(waterPump, LOW);
  }

  //Đổi mức độ ẩm
  MM = docJson["MM"];

  char *idSent = docJson["idSent"];
  char msg[strlen("{\"status\":\"\",\"msg\":\"Success\"}") + strlen(err.c_str()) + strlen(idSent)];
  sprintf(msg, "{\"status\":\"%s\",\"msg\":\"Success\", \"idSent\":\"%s\"}", err.c_str(), idSent);
  thisSerial.print("resRESU"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(msg);
  thisSerial.print('\r');
}
// -> set cho tớ các biến như thế này
// SerialCommand setCommand_("set", setCommand);

//==============================XỬ LÝ LỆNH TÀO LAO =========================
// void cmdUnrecognized(SerialCommands *sender, const char *cmd)
// {
//   sender->GetSerial()->print("ERROR: Unrecognized command [");
//   sender->GetSerial()->print(cmd);
//   sender->GetSerial()->println("]");
// }

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
  thisSerial.begin(57600);

  //Setup chế độ của cổng
  pinMode(mositureSensor, INPUT);
  pinMode(waterPump, OUTPUT);

  //Khai báo xử lý khi có lệnh từ ESP8266
  // sCmd.AddCommand(&getInfo_);
  // sCmd.AddCommand(&getMositure_);
  // sCmd.AddCommand(&setCommand_);
  // sCmd.SetDefaultHandler(&cmdUnrecognized);

  sCmd.addCommand("getINFO", getInfo);
  sCmd.addCommand("getMOSI", getMositure);
  sCmd.addCommand("setCMD", setCommand);

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