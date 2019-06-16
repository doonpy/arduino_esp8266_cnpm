//Thư viện
#include <SocketIoClient.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
// #include <SerialCommands.h>
#include <SerialCommand.h>
#include <ArduinoJson.h>
#include <string.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <time.h>

//include thư viện để kiểm tra free RAM trên con esp8266
extern "C"
{
#include "user_interface.h"
}

//Khởi tạo biến toàn cục
const byte RX = D1; //cổng RX
const byte TX = D2; //cổng TX
// char serialCommandBuffer[1024];
struct tm timeinfo; //Đồng hồ

//Khởi tạo Serial với Arduino
SoftwareSerial thisSerial(RX, TX);
//Khởi tạo lệnh
// SerialCommands sCmd(&thisSerial, serialCommandBuffer, sizeof(serialCommandBuffer), "\n", " "); // Khai báo biến sử dụng thư viện Serial Command
SerialCommand sCmd(thisSerial);

//Khởi tạo client socket io
SocketIoClient client;          //Khai bao socket client
char host[] = "192.168.43.103"; //Địa chỉ IP socket server
int port = 3000;                //Port của server
const int FW_VERSION = 1245;    //version firmware
// const char *fwUrlBase = "http://192.168.0.104/firmware/"; //đường dẫn chứa folder firmware

//========================= CHẾ ĐỘ ĐIỂM TRUY CẬP (KHI KO TÌM ĐC WIFI NÀO) ======================================
// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// WiFiManager
// Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;

// Khai báo các xử lý:
//========================SET CLOCK================================
// Set time via NTP, as required for x.509 validation
void setClock()
{
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // UTC+7

  Serial.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }
  Serial.println(F(""));
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

//===================THÔNG TIN CHUNG=======================================
//gửi yêu cầu lấy thông tin cho arduino
void getInfo(const char *payload, size_t length)
{
  Serial.print("->>>getINFO ");
  Serial.println(payload);
  thisSerial.print("getINFO"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(payload);
  thisSerial.print('\r');
  uint32_t free = system_get_free_heap_size();
  Serial.println(free);
}

//Phản hồi với socket server
void resInfo()
{
  char *port_str = sCmd.next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    client.emit("resError", "{\"status\":\"NoPort\", \"msg\":\"No_port\"}");
    return;
  }
  //Xuất ra monitor
  Serial.print("<<<-resINFO ");
  Serial.println(port_str);

  //JSON handle
  const int capacity = JSON_OBJECT_SIZE(5);
  StaticJsonDocument<capacity> docJson;
  DeserializationError err = deserializeJson(docJson, port_str);

  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    client.emit("resError", msg);
    return;
  }

  //Thêm phiên bản vào
  String output;
  docJson["firmware"] = FW_VERSION;
  serializeJson(docJson, output);

  //Gửi cho socket server
  client.emit("resInfomation", output.c_str());
}
//tao lenh rieng
// SerialCommand resInfo_("resINFO", resInfo);

//========================MOSITURE===========================================
void getMositure(const char *payload, size_t length)
{
  Serial.print("->>>getMOSI ");
  Serial.println(payload);
  thisSerial.print("getMOSI"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(payload);
  thisSerial.print('\r');
}
//Phản hồi với socket server
void resMositure()
{
  char *port_str = sCmd.next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    client.emit("resError", "{\"status\":\"NoPort\", \"msg\":\"No_port\"}");
    return;
  }

  //Xuất ra monitor
  Serial.print("<<<- ");
  Serial.println(port_str);

  //JSON handle
  const int capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> docJson;
  DeserializationError err = deserializeJson(docJson, port_str);

  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    client.emit("resError", msg);
    return;
  }

  //Thêm ngày giờ vào
  setClock();
  String output;
  docJson["time"] = asctime(&timeinfo);
  docJson["status"] = "Ok";
  serializeJson(docJson, output);

  //Gửi cho socket server
  client.emit("resMositure", output.c_str());
}
//tao lenh rieng
// SerialCommand resMositure_("resMOSI", resMositure);

//===================SET COMMAND=======================================
//Set trạng thái máy bơm và chế độ tưới
void setCommand(const char *payload, size_t length)
{
  sCmd.clearBuffer();
  Serial.print("->>>setCMD ");
  Serial.println(payload);
  thisSerial.print("setCMD"); //gửi tên lệnh
  thisSerial.print('\r');
  thisSerial.print(payload);
  thisSerial.print('\r');
}

//===================== KẾT QUẢ THỰC THI ===========================
void resResult()
{
  //Lấy data
  char *port_str = sCmd.next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    client.emit("resError", "{\"status\":\"NoPort\", \"msg\":\"No_port\"}");
    return;
  }
  Serial.print("<<<-resRESU ");
  Serial.println(port_str);

  //JSON handle
  const int capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> docJson;
  DeserializationError err = deserializeJson(docJson, port_str);

  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    char msg[strlen("{\"status\":\"\",\"msg\":\"deserializeJson()_failed\"}") + strlen(err.c_str())];
    sprintf(msg, "{\"status\":\"%s\",\"msg\":\"deserializeJson()_failed\"}", err.c_str());
    client.emit("resError", msg);
    return;
  }

  if (docJson["status"] == "Ok")
  {
    String output;
    serializeJson(docJson, output);
    client.emit("resResult", output.c_str());
  }
  else
  {
    String output;
    serializeJson(docJson, output);
    client.emit("resResult", output.c_str());
  }
}

// SerialCommand resResult_("resRESU", resResult);

//=========================== UPDATE FIRMWARE =========================
void checkForUpdates()
{
  const String portTemp = String(port);
  char fwUrlBase[strlen("http:///firmware/") + strlen(host) + strlen(portTemp.c_str())];
  sprintf(fwUrlBase, "http://%s:%s/firmware/", host, portTemp.c_str());
  String fwURL = String(fwUrlBase);
  fwURL.concat("firmware");
  String fwVersionURL = fwURL;
  fwVersionURL.concat(".version");

  Serial.println("=> Checking for firmware updates.");
  Serial.print("=> Firmware version URL: ");
  Serial.println(fwVersionURL);

  HTTPClient httpClient;
  httpClient.begin(fwVersionURL);
  int httpCode = httpClient.GET();
  if (httpCode == 200)
  {
    String newFWVersion = httpClient.getString();

    Serial.print("=> Current firmware version: ");
    Serial.println(FW_VERSION);
    Serial.print("=> Available firmware version: ");
    Serial.println(newFWVersion);

    int newVersion = newFWVersion.toInt();

    if (newVersion > FW_VERSION)
    {
      Serial.println("=> Preparing to update");

      String fwImageURL = fwURL;
      fwImageURL.concat(String(newVersion));
      fwImageURL.concat(".bin");
      t_httpUpdate_return ret = ESPhttpUpdate.update(fwImageURL);

      switch (ret)
      {
      case HTTP_UPDATE_FAILED:
        Serial.printf("=> HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("=> HTTP_UPDATE_NO_UPDATES");
        break;
      }
      client.emit("resResult", "{\"status\":\"Ok\",\"msg\":\"Update successed\"}");
    }
    else
    {
      Serial.println("=> Already on latest version");
      client.emit("resResult", "{\"status\":\"UpdateFailed\",\"msg\":\"Already on latest version\"}");
    }
  }
  else
  {
    Serial.print("=> Firmware version check failed, got HTTP response code ");
    Serial.println(httpCode);
    char msg[] = "";
    sprintf(msg, "{\"status\":\"UpdateFailed\",\"msg\":\"Firmware version check failed, got HTTP response code %d\"}", httpCode);
    client.emit("resError", msg);
  }
  httpClient.end();
}
void updateFirmware(const char *payload, size_t length)
{
  Serial.print("<<<-updateFirmware ");
  Serial.println(payload);
  checkForUpdates();
}

//This is the default handler, and gets called when no other command matches.
// void cmdUnrecognized(SerialCommands *sender, const char *cmd)
// {
//   sender->GetSerial()->print("Unrecognized command [");
//   sender->GetSerial()->print(cmd);
//   sender->GetSerial()->println("]");
// }

//============================ AUTHORIZATION ==========================
void getAuthorization(const char *payload, size_t length)
{
  char msg[strlen("{\"type\":false,\"mac\":\"\"}") + strlen(WiFi.macAddress().c_str())];
  sprintf(msg, "{\"type\":false,\"mac\":\"%s\"}", WiFi.macAddress().c_str());
  client.emit("resAuth", msg);
  checkForUpdates();
}

void setup()
{
  //Bật baudrate ở mức 115200 để giao tiếp với máy tính qua Serial
  Serial.begin(115200);
  thisSerial.begin(57600); //Bật software serial để giao tiếp với Arduino
  delay(30);

  // WiFiManager - quan ly wifi
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  // wifiManager.resetSettings();
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  //Hàm lặp liên tục cho đến khi nào connect (để trống nó tự tạo tên wifi)
  wifiManager.autoConnect("Exploding Kettens");
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  server.begin();

  //Lấy đồng hồ
  setClock();

  //Với socketserver - lắng nghe socket server ra lệnh
  client.on("getInfomation", getInfo);         //yêu cầu lấy thông tin của thiết bị Arduino
  client.on("getMositure", getMositure);       //yêu cầu lấy độ ẩm
  client.on("setCommand", setCommand);         //gửi lệnh setting các thông số Arduino
  client.on("updateFirmware", updateFirmware); //gửi lệnh update Firmware
  client.on("getAuth", getAuthorization);      //gửi lệnh update Firmware

  //Kết nối với socket server
  client.begin(host, port);

  //Với Arduino
  // sCmd.AddCommand(&resInfo_);     //Thêm lệnh đọc thông tin độ ẩm, pumbMode, trạng thái máy bơm
  // sCmd.AddCommand(&resMositure_); //Lệnh lấy độ ẩm
  // sCmd.AddCommand(&resResult_);
  // sCmd.SetDefaultHandler(&cmdUnrecognized);
  sCmd.addCommand("resINFO", resInfo);
  sCmd.addCommand("resMOSI", resMositure);
  sCmd.addCommand("resRESU", resResult);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    //  Kết nối lại!============================
    client.loop();
  }
  sCmd.readSerial();
}