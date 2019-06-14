//Thư viện
#include <SocketIoClient.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <SerialCommands.h>
#include <ArduinoJson.h>
#include <string.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <time.h>

//Khởi tạo biến toàn cục
const byte RX = D1; //cổng RX
const byte TX = D2; //cổng TX
char serial_command_buffer_[256];
struct tm timeinfo; //Đồng hồ

//Khởi tạo Serial với Arduino
SoftwareSerial thisSerial(RX, TX, false, 256);
//Khởi tạo lệnh
SerialCommands sCmd(&thisSerial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " "); // Khai báo biến sử dụng thư viện Serial Command
//Khởi tạo client socket io
SocketIoClient client;                                        //Khai bao socket client
char host[] = "192.168.1.33";                                 //Địa chỉ IP socket server
int port = 3000;                                              //Port của server
const int FW_VERSION = 1245;                                  //version firmware
const char *fwUrlBase = "http://192.168.1.33:3000/firmware/"; //đường dẫn chứa folder firmware

//========================= CHẾ ĐỘ ĐIỂM TRUY CẬP (KHI KO TÌM ĐC WIFI NÀO) ======================================
// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// WiFiManager
// Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;

// Khai báo các xử lý:
//===================THÔNG TIN CHUNG=======================================
//gửi yêu cầu lấy thông tin cho arduino
void getInfo(const char *payload, size_t length)
{
  Serial.println("->>>getINFO {}");
  thisSerial.println("getINFO {}"); //gửi tên lệnh
}

//Phản hồi với socket server
void resInfo(SerialCommands *sender)
{
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    return;
  }

  //Xuất ra monitor
  Serial.print("<<<- ");
  Serial.println(port_str);

  //Gửi cho socket server
  client.emit("resInfomation", port_str);
}

//tao lenh rieng
SerialCommand resInfo_("resINFO", resInfo);

//===================MÁY BƠM=======================================
//Set mode cho máy bơm
void setPumbMode(const char *payload, size_t length)
{
  Serial.println("->>>setPUMB {}");
  thisSerial.println("setPUMB {}"); //gửi tên lệnh
}

//Tắt mở máy bơm
void controlPumb(const char *payload, size_t length)
{
  Serial.println("->>>contrPUMB {}");
  thisSerial.println("contrPUMB {}"); //gửi tên lệnh
}

//===================== KẾT QUẢ THỰC THI ===========================
void resResult(SerialCommands *sender)
{
  //Lấy data
  const char *port_str = sender->Next();
  if (port_str == NULL)
  {
    Serial.println("ERROR NO_PORT");
    return;
  }
  Serial.print("<<<-resRESULT ");
  Serial.println(port_str);
  client.emit("resResult", port_str);
}

SerialCommand resResult_("resRESULT", resResult);

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

//=========================== UPDATE FIRMWARE =========================
void checkForUpdates()
{
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
      client.emit("resResult", "{\"value\":false,\"msg\":\"Update successed\"}");
    }
    else
    {
      Serial.println("=> Already on latest version");
      client.emit("resResult", "{\"value\":false,\"msg\":\"Already on latest version\"}");
    }
  }
  else
  {
    Serial.print("=> Firmware version check failed, got HTTP response code ");
    Serial.println(httpCode);
    char msg[] = "";
    sprintf(msg, "{\"value\":false,\"msg\":\"Firmware version check failed, got HTTP response code %d\"}", httpCode);
    client.emit("resResult", msg);
  }
  httpClient.end();
}
void updateFW(const char *payload, size_t length)
{
  Serial.print("<<<-updateFW ");
  Serial.println(payload);
  checkForUpdates();
}

void setup()
{
  //Bật baudrate ở mức 115200 để giao tiếp với máy tính qua Serial
  Serial.begin(115200);
  thisSerial.begin(9600); //Bật software serial để giao tiếp với Arduino
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
  client.on("getInfomation", getInfo);
  client.on("setPumbMode", setPumbMode);
  client.on("controlPumb", controlPumb);
  client.on("updateFW", updateFW);
  //Kết nối với socket server
  client.begin(host, port);

  //Với Arduino
  sCmd.AddCommand(&resInfo_); //Thêm lệnh đọc thông tin độ ẩm, pumbMode, trạng thái máy bơm
  sCmd.AddCommand(&resResult_);

  //Check update firmware
  checkForUpdates();

  Serial.println("Da san sang nhan lenh");
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    //  Kết nối lại!============================
    client.loop();

    sCmd.ReadSerial();
    delay(10);
  }
}