#include "EasySim800l.h"

// Initialize UART
EasySim800l::EasySim800l(String apn, uint8_t uart_n, bool ESP) {
  if (ESP) {
    _serial = new HardwareSerial(uart_n);
    return;
  }
  APN = apn;
  return;
}

// Initialize Serial communcation on with sim800l
void EasySim800l::begin( uint32_t baud, uint8_t RXD, uint8_t TXD) {
  _serial->begin(baud, SERIAL_8N1, RXD, TXD);
  delay(1000);
  updateStat();
}

// Check response of at command
int8_t EasySim800l::sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout) {
  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;

  // Initialice the null string
  memset(response, '\0', 100);
  // delay(100);

  //Clean the input buffer
  while ( _serial->available() > 0) _serial->read();

  if (ATcommand[0] != '\0') {
    // Send the AT command
    _serial->println(ATcommand);
  }

  x = 0;
  previous = millis();

  // this loop waits for the answer with time out
  do {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (_serial->available() != 0) {
      response[x] = _serial->read();
      //Serial.print(response[x]); // !!UNCOMENT FOR DEBUG
      x++;
      // check if the desired answer (OK) is in the response of the module
      if (strstr(response, expected_answer) != NULL) {
        answer = 1;
      }
    }
  } while ((answer == 0) && ((millis() - previous) < timeout));
  //Serial.println(response); // !!UNCOMENT FOR DEBUG
  delay(10);
  return answer;
}

String EasySim800l::getATRes(char* ATcommand, char* expected_answer, unsigned int timeout) {
  uint8_t x = 0,  answer = 0;
  uint32_t maxResSize = 1024;
  char response[maxResSize];
  unsigned long previous;
  unsigned long resCount = 0;

  // Initialice the null string
  memset(response, '\0', maxResSize);
  // delay(maxResSize);

  //Clean the input buffer
  while ( _serial->available() > 0) _serial->read();

  if (ATcommand[0] != '\0') {
    // Send the AT command
    _serial->println(ATcommand);
  }

  x = 0;
  previous = millis();

  // this loop waits for the answer with time out
  do {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (_serial->available() != 0) {
      response[x] = _serial->read();
      // Serial.print(response[x]); // !!UNCOMENT FOR DEBUG
      x++;
      // check if the desired answer (OK) is in the response of the module
      if (strstr(response, expected_answer) != NULL) {
        resCount = millis();
      }
    }
    if ((millis() - resCount) > 10 && resCount > 0) {
      answer = 1;
    }
  } while ((answer == 0) && ((millis() - previous) < timeout));
  // Serial.println(response); // !!UNCOMENT FOR DEBUG
  return response;
}

boolean EasySim800l::checkSleep(){
  boolean isSleep = true;
  sendATcommand("AT", "OK", 100);
  if (sendATcommand(chkSleep, "+CSCLK: 0", 500) == 1) isSleep = false;
  
  return isSleep;
}

boolean EasySim800l::checkConnection(){
  if(checkSleep()) return false;
  
  if(sendATcommand(chkFun, "+CFUN: 1", 500) == 1) return true;

  if(sendATcommand(chkcops, "+COPS: 0,0,\"", 500) == 1) return true;

  return false;
}

String EasySim800l::getIP() {
  gprs = false;
  String ipStr = "0.0.0.0";
  if (!checkConnection()) return ipStr;

  String outPutStr = getATRes(chkbr);
  outPutStr = outPutStr.substring(outPutStr.indexOf("\n+SAPBR: 1,") + 11, outPutStr.indexOf("\nOK"));
  if (outPutStr.length() <= 0) return ipStr;

  uint8_t stat = outPutStr.substring(0, outPutStr.indexOf(',')).toInt();
  if (stat != 1) return ipStr;

  ipStr = outPutStr.substring(outPutStr.indexOf(",\"") + 2);
  ipStr = ipStr.substring(0, ipStr.indexOf("\""));
  if (ipStr == "0.0.0.0") return ipStr;

  gprs = true;
  return ipStr;
}

void EasySim800l::updateStat() {
  unsigned long start = millis();
  sendATcommand("ATT", "OK", 100);
  sleep = checkSleep();
  Connected = checkConnection();
  getIP();
  //Serial.println("Time Taken In Update Fun: "+String(millis()-start));
}

int EasySim800l::getBatteryPer() {
  sendATcommand("AT", "OK", 500);
  String batStr = getATRes(chargeStat);
  batStr = batStr.substring(batStr.indexOf(',') + 1);
  batStr = batStr.substring(0, batStr.indexOf(','));
  return batStr.toInt();
}

int EasySim800l::RSSI() {
  sendATcommand("AT", "OK", 100);
  String rssiStr = getATRes(rssicom); // Get rssi string
  rssiStr = rssiStr.substring(rssiStr.indexOf("+CSQ: ") + 6);
  rssiStr = rssiStr.substring(0, rssiStr.indexOf(','));
  int rssi = rssiStr.toInt();
  int dbm = map(rssi, 0, 31, -115, -52);
  if (rssi >= 99) dbm = 99;
  return dbm;
}

bool EasySim800l::connect() {
  updateStat();
  if (Connected) return true;

  if (sleep) wakeUp();

  sendATcommand(); // Send synchronizing command
  if(sendATcommand(fullFun, "Call Ready", 60000) == 1) return true; // Set modem to full functionality mode

  return false;
}

bool EasySim800l::wakeUp() {
  updateStat();
  if (!sleep) return true;
  
  sendATcommand("AT", "OK", 100); // Send AT for wakeup
  sendATcommand(awake); // Unset the sleep mode
  updateStat();
  if (!sleep) return true;

  return false;
}

boolean EasySim800l::initGPRS() {
  updateStat();
  if (gprs) return true;

  if (!Connected) connect(); 
  
  sendATcommand(attachPDS); // Connect modem is attached to packet domain service
  sendATcommand(GPRSbp); // Connection type: GPRS - bearer profile 1
  sendATcommand(const_cast<char*>((String(setAPN) + APN).c_str())); // Sets the APN settings for default Vi-2G network change by class.APN = "string"
  sendATcommand(GPRSenb); // Enable the GPRS - enable bearer 1
  sendATcommand(chkbr); // Check whether bearer 1 is open.
  sendATcommand(HTTPINIT); // Init HTTP service
  sendATcommand(CID); // Set CID = 1
  updateStat();
  if (gprs) return true;

  return false;
}

int EasySim800l::postJson(String URL, String dataString) {
  updateStat();
  if (!gprs) return 650; // return response code.

  String URLstring = String(setURL) + URL; // Set url string
  sendATcommand(const_cast<char*>(URLstring.c_str())); // Set the HTTP URL
  sendATcommand(cJson); // Set content to application/json for json data in body
  sendATcommand(strPost, "DOWNLOAD", 2000); // Setup the data length and waiting time 1s
  _serial->print(dataString); // Send the Data String
  sendATcommand(blank); // Wait for response
  String resStr = getATRes(Post, "+HTTPACTION: 1,", 60000); // Set up the HTTP action to 1 (POST) wait 60sec max
  resStr = resStr.substring(resStr.indexOf(',') + 1, resStr.length());
  resStr = resStr.substring(0, resStr.indexOf(','));
  return resStr.toInt();
}

int EasySim800l::getJson(String URL) {
  updateStat();
  if (!gprs) return 650; // return response code.

  String URLstring = String(setURL) + URL; // Set url string
  sendATcommand(const_cast<char*>(URLstring.c_str())); // Set the HTTP URL
  sendATcommand(cJson); // Set content to application/json for json data in body
  String resStr = getATRes(Get, "+HTTPACTION: 0,", 60000); // Set up the HTTP action to 0 (GET) wait 60sec max
  resStr = resStr.substring(resStr.indexOf(',') + 1, resStr.length());
  resStr = resStr.substring(0, resStr.indexOf(','));
  return resStr.toInt();
}

String EasySim800l::getJsonString() {
  updateStat();
  if (!gprs) return "{}";
  
  String resStr = getATRes(HTTPREAD, "\nOK", 60000); //Get the json data wait for "OK" or 60sec max
  String dataSizeStr = resStr.substring(resStr.indexOf("+HTTPREAD: ") + 11);
  dataSizeStr = dataSizeStr.substring(0, dataSizeStr.indexOf('\n'));
  int dataSize = dataSizeStr.toInt();
  if (dataSize <= 0) return "{}";
  
  String Jdata = resStr.substring(resStr.indexOf("\n{"), resStr.indexOf("}\r\n"));
  return Jdata;
};

boolean EasySim800l::disconGPRS() {
  updateStat();
  if(!gprs) return true;

  sendATcommand(HTTPTERM, "OK", 500); // Terminate the HTTP service
  sendATcommand(GPRSdis, "SHUT OK"); // Shuts down the GPRS connection. This returns "SHUT OK"
  sendATcommand(dattachPDS); // Deattech from the GPRS
  getIP();
  return true;
}

boolean EasySim800l::disconnect(){
  updateStat();
  if(!Connected) return false;

  disconGPRS();
  if(sendATcommand(noFun) == 1) return false; // Disable device

  return true;
}

boolean EasySim800l::goToSleep(){
  updateStat();
  if(sleep) return true;

  Connected = disconnect();
  if(sendATcommand(goSleep) == 1) return true; // Enter the sleep mode 2

  return false;
}
