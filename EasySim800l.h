#ifndef ES
#define ES

#if (ARDUINO >=100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class EasySim800l  {
  public:
    boolean Connected = false;
    boolean gprs = false;
    boolean sleep = false;

    String APN = "internet"; //"airtelgprs.com"; //For airtel india
    // Constructor
    EasySim800l(String apn = "internet", uint8_t uart_n = 1, bool ESP = true);

    // Methods
    void begin(uint32_t baud = 9600, uint8_t RXD = 16, uint8_t TXD = 17);
    int getBatteryPer();
    bool initGPRS();
    bool disconGPRS();
    String getIP();
    int postJson(String URL, String dataString);
    int getJson(String URL);
    String getJsonString();
    int RSSI();
    boolean disconnect();
    boolean goToSleep();


  private:
    HardwareSerial *_serial; // Pointer to the class
    // AT commands
    char* chargeStat = "AT+CBC";
    char* awake = "AT+CSCLK=0";
    char* fullFun = "AT+CFUN=1";
    char* attachPDS = "AT+CGATT=1";
    char* GPRSbp = "AT+SAPBR=3,1,Contype,GPRS";
    char* setAPN = "AT+SAPBR=3,1,APN,";
    char* GPRSenb = "AT+SAPBR=1,1";
    char* chkbr = "AT+SAPBR=2,1";
    char* HTTPINIT = "AT+HTTPINIT";
    char* CID = "AT+HTTPPARA=CID,1";
    char* setURL = "AT+HTTPPARA=URL,";
    char* cJson = "AT+HTTPPARA=CONTENT,APPLICATION/JSON";
    char* strPost = "AT+HTTPDATA=1024,1000";
    char* blank = " ";
    char* Post = "AT+HTTPACTION=1";
    char* Get = "AT+HTTPACTION=0";
    char* chkSleep = "AT+CSCLK?";
    char* HTTPTERM = "AT+HTTPTERM";
    char* GPRSdis = "AT+CIPSHUT";
    char* dattachPDS = "AT+CGATT=0";
    char* noFun = "AT+CFUN=0";
    char* goSleep = "AT+CSCLK=2";
    char* HTTPREAD = "AT+HTTPREAD";
    char* chkFun = "AT+CFUN?";
    char* rssicom = "AT+CSQ";
    char* chkcops = "AT+COPS?";

    // Private methods
    int8_t sendATcommand(char* ATcommand = "ATTTTTTT", char* expected_answer = "OK", unsigned int timeout = 6000);
    String getATRes(char* ATcommand = "ATTTTTTT", char* expected_answer  = "OK", unsigned int timeout = 6000);
    bool checkConnection();
    void updateStat(); // Update status values
    bool connect();
    bool wakeUp();
    boolean checkSleep();
    
};
#endif
