#include <Arduino.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

SoftwareSerial BT(8,7); // Using Bleutooth Tx == 8; Rx == 7

// Define the pins for SPI communication.
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

char * url = "{\"GPS\":\"36.7249122:3.1542043\",\"CONSTITUTION\":\"boom\",\"ND1\":\"035853195\",\"ND2\":\"0\",\"ND3\":\"0\",\"ND4\":\"0\",\"ND5\":\"0\",\"ND6\":\"0\",\"ND7\":\"0\"}";
uint8_t ndefprefix = NDEF_URIPREFIX_NONE;

const int buttonPin = 9; // Botton

DynamicJsonDocument doc(1024);

void setup(void) 
{
  Serial.begin(115200);
  BT.begin(115200); // For Bleutooth
  while (!Serial) delay(10); 

  nfc.begin();

  pinMode(buttonPin, INPUT); // Declare the botton 
                                                           
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) 
  {
    Serial.print("Didn't find PN53x board");
    BT.write("Didn't find NFC Mudole");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
    while (1);
  }
  
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  BT.write("Found NFC Module");
  
  // configure board to read RFID tags
  nfc.SAMConfig();   
}


void loop(void) 
{
  
  int buttonState = digitalRead(buttonPin);
  
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  
  uint8_t uidLength;                        
  uint8_t dataLength;

  Serial.println("\r\nPlace your NDEF formatted NTAG2xx tag on the reader to update the");
  Serial.println("NDEF record and press any key to continue ...\r\n");
 
  while (BT.available())
  {
    char messagechar = BT.read();
    String message;
    if(messagechar != '/n')
    {
       message += String(messagechar);
    } 
    else
     {
      deserializeJson(doc, message);
      JsonObject obj = doc.as<JsonObject>();
      String mode = obj["mode"].as<String>();  
    
    if(mode == "write")
    {
      if (buttonState == HIGH) 
      {
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) 
        {
    BT.write("Found NFC Tag");
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength != 7)
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
    else
    {
      uint8_t data[32];
      Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");    
      memset(data, 0, 4);
      success = nfc.ntag2xx_ReadPage(3, data);
      
      if (!success)
      {
        Serial.println("Unable to read the Capability Container (page 3)");
        return;
      }
      else
      {
       
        if (!((data[0] == 0xE1) && (data[1] == 0x10)))
        {
          Serial.println("This doesn't seem to be an NDEF formatted tag.");
          Serial.println("Page 3 should start with 0xE1 0x10.");
        }
        else
        {
          
          dataLength = data[2]*8;
          Serial.print("Tag is NDEF formatted. Data area size = ");
          Serial.print(dataLength);
          Serial.println(" bytes");
          
          Serial.print("Erasing previous data area ");
          for (uint8_t i = 4; i < (dataLength/4)+4; i++) 
          {
            memset(data, 0, 4);
            success = nfc.ntag2xx_WritePage(i, data);
            Serial.print(".");
            if (!success)
            {                                            
              Serial.println(" ERROR!");
              BT.write("ERROR!");
           
              return;
            }

          }
          Serial.println(" DONE!");
          BT.write("DONE");
        
          Serial.print("Writing URI as NDEF Record ... ");
          success = nfc.ntag2xx_WriteNDEFURI(ndefprefix, url, dataLength);
          if (success)
          {
            Serial.println("DONE!");
            BT.write("DONE");
          }
          else
          {
            Serial.println("ERROR! (URI length?)");
            BT.write("ERROR!");
          }
                    
        } 
      } 
    } 
    
    Serial.flush();
    while (!Serial.available());
    while (Serial.available()) 
    {
    Serial.read();
    }
    Serial.flush();    
     }
   } 
} 

else if(mode=="read")
{
      if (buttonState == HIGH) 
 {
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) 
    {
    
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    BT.write("Find NFC Tag");
    
    if (uidLength == 7)
    {
      uint8_t data[32];
      
      Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");    
          
      for (uint8_t i = 0; i < 42; i++) 
      {
        success = nfc.ntag2xx_ReadPage(i, data);
        
        Serial.print("PAGE ");
        if (i < 10)
        {
          Serial.print("0");
          Serial.print(i);
        }
        else
        {
          Serial.print(i);
        }
        Serial.print(": ");

        if (success) 
        {
          
          nfc.PrintHexChar(data, 4);
        }
        else
        {
          Serial.println("Unable to read the requested page!");
          BT.write("Can't read");
        }
      }      
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
      BT.write("Can't read");
    }
    
    Serial.println("\n\nSend a character to scan another tag!");
    Serial.flush();
    while (!Serial.available());
    while (Serial.available()) {
    Serial.read();
    }
    Serial.flush();    
  }
}
} 

else {
      BT.write("BAD REQUEST");
    }
        Serial.print("BAD REQUEST");
     }
  }
}
