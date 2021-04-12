#include <Adafruit_Fingerprint.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <HTTPClient.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  //sometimes the adress is not 0x3f. Change to 0x27 if it dosn't work.  

AsyncWebServer server(80);
const char* ssid = "INFINITUM4304_2.4";  //replace
const char* password =  "vxMr7ddEx3"; //replace

String host = "https://huellascheck.000webhostapp.com/api/Api.php?apicall=";
HTTPClient http;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

uint8_t id;
bool cancel = false;
String siglas;
String nombre;
String correo;
String area;
bool userSavedDB = false;
bool userDeletedDB = false;

/////////////////////////////////////Input and outputs////////////////////////////////////////////////
int scan_pin = 14;      //Pin for the scan push button
int add_id_pin = 26;    //Pin for the add new ID push button
int cancelButton = 32;     //Pin to close the door button
int green_led = 8;      //Extra LEDs for open or close door labels
int red_led = 7;
int btnPress = 0;
//int servo = 6;        //Pin for the Servo PWM signal
const int pinRele = 23; //PIN DIGITAL UTILIZADO PARA MÓDULO RELÉ 

int main_user_ID = 1;                 //Change this value if you want a different main user
bool scanning = false;
int counter = 0;
int id_ad_counter = 0;
bool id_ad = false;
uint8_t num = 1;
//bool id_selected = false;
//uint8_t id;
bool first_read = false;
bool main_user = false;
bool add_new_id = false;
bool door_locked = true;

void setup()
{
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);

  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Sensor encontrado!");
  } else {
    Serial.println("Sensor NO encontrado");
    while (1) { delay(1); }
  }
  
  pinMode(scan_pin,INPUT_PULLUP);  
  attachInterrupt(scan_pin, ISR, FALLING);  // LOW/HIGH/FALLING/RISING/CHANGE
  pinMode(cancelButton,INPUT_PULLUP);  
  attachInterrupt(cancelButton, ISR, FALLING);  // LOW/HIGH/FALLING/RISING/CHANGE
  
  pinMode(pinRele, OUTPUT); //DEFINE EL PIN COMO SALIDA
  digitalWrite(pinRele, HIGH); //MÓDULO RELÉ INICIA DESLIGADO
  
  lcd.init();                     //Init the LCD with i2c communication and print text
  lcd.backlight();
  lcd.setCursor(0,0);
  
  connectToNetwork();
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
  server.on("/AddFingerPrint", HTTP_GET, [](AsyncWebServerRequest *request){
    /*lcd.print("    Recivido    ");
    delay(500);
    lcd.clear();*/
    int paramsNr = request->params();
    for(int i=0;i<paramsNr;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->name()== "id"){
          id = p->value().toInt();
          //Serial.println("ID = "+id);
        }else
        if(p->name()== "siglas"){
          siglas = p->value();
          //Serial.println("Siglas = "+siglas);
        }else
        if(p->name()== "nombre"){
          nombre = p->value();
          //Serial.println("Siglas = "+nombre);
        }else
        if(p->name()== "correo"){
          correo = p->value();
          //Serial.println("Correo = "+correo);
        }else
        if(p->name()== "area"){
          area = p->value();
          //Serial.println("Area = "+area);
        }
    }
    if(!id){
      id = 0;
    }{
      id_ad = true;
    }
    
    //delay(4000);
    //while (! userSavedDB );
    //request->send(200, "text/plain", "parametros recibidos");
    request->send(200, "application/json", "{\"message\":\"recibidos\"}");
  });
  
  server.on("/ScannFinger", HTTP_GET, [](AsyncWebServerRequest *request){
    //lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("     IP  OK     ");
    lcd.setCursor(0,1);
    lcd.print("       :)       "); 
    delay(500);
    lcd.setCursor(0,0);
    lcd.print(" Precione para ");
    lcd.setCursor(0,1);
    lcd.print("escanear huella"); 
    request->send(200, "application/json", "{\"message\":\"correcto\"}");
  });
  
  server.on("/DeleteFingerIDUser", HTTP_GET, [](AsyncWebServerRequest *request){
    int paramsNr = request->params();
    int idToDelete;
    for(int i=0;i<paramsNr;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->name()== "id"){
          idToDelete = p->value().toInt();
          //lcd.print("  Eliminando... ");
          while(! deleteFingerprint(idToDelete));
          //lcd.clear();
          lcd.setCursor(0,0);
          Serial.println("Eliminado "+siglas);
          lcd.print("    ID #       ");
          lcd.setCursor(9,0);
          lcd.print(siglas);
          lcd.setCursor(0,1);
          lcd.print("    eliminado!  ");
          delay(700);
          lcd.clear();
          lcd.print(" Precione para ");
          lcd.setCursor(0,1);
          lcd.print("escanear huella"); 
          request->send(200, "application/json", "{\"message\":\"user deleted\"}");
        }
    }
    request->send(200, "application/json", "{\"message\":\"user not deleted\"}");
  });
  
  lcd.clear();
  lcd.print(" Precione para ");
  lcd.setCursor(0,1);
  lcd.print("escanear huella"); 
}

void ISR(){
  if (!digitalRead(scan_pin)) btnPress = 1;
  else if (!digitalRead(cancelButton)){
    if(!cancel){
      cancel = true;
    }
    if(!cancel){
      cancel = true;
    }
    if(scanning){
      scanning = false;
    }
    //counter = 0;
    /*lcd.setCursor(0,0);
    lcd.print(" Precione para ");
    lcd.setCursor(0,1);
    lcd.print("escanear huella"); */
    Serial.print("cancel");
    btnPress = 2;
  }
  else btnPress = 0;
}

void postAccess(int idUser, int idSensor)
{
  if(WiFi.status()== WL_CONNECTED)
    {
      http.begin(host + "createAccessRegister");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String postData = "iduser= "+ String(idUser) + "&idsensor=" + String(idSensor); 
      int httpCode = http.POST(postData);
      String payload = http.getString();
      //Serial.println(payload);
      payload.replace("users","");
      payload.replace(":","");
      payload.replace("{","");
      payload.replace("}","");
      payload.replace("\"","");
      payload.replace("Siglas","Bienvenido ");
      payload.replace("users","");
      payload.replace("\n","");
      payload.trim();
      //payload.remove(0,);
      if(payload == "closed"){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(" Sesion cerrada "); 
        lcd.setCursor(0,1);
        lcd.print(" ID: #");
        
        lcd.setCursor(6,1);
        lcd.print(finger.fingerID);
        //Serial.println(finger.fingerID);
      
        lcd.setCursor(9,1);
        lcd.print("%: ");
      
        lcd.setCursor(12,1);
        lcd.print(finger.confidence);
      }else{
        lcd.clear();
        lcd.setCursor(0,0);
        //Serial.println(payload+" abierta");
        digitalWrite(pinRele, LOW); //Abre el paso de corriente al RELE
        //Serial.println(payload+" closed");
        String Bienvenida = payload;
        lcd.print(Bienvenida);   //Print HTTP return code
        lcd.setCursor(0,1);
        lcd.print(" ID: #");
        
        lcd.setCursor(6,1);
        lcd.print(finger.fingerID);
        Serial.println(finger.fingerID);
      
        lcd.setCursor(9,1);
        lcd.print("%: ");
      
        lcd.setCursor(12,1);
        lcd.print(finger.confidence);
      }
      //Serial.println(Bienvenida);
      digitalWrite(pinRele, HIGH); //Cierra el paso de corriente al RELE
      delay(2000);
      lcd.clear();
   
      http.end();
    }
    else 
    {
      Serial.println("WiFi Desconectado");
      lcd.print("WiFi Desconectado");
      digitalWrite(pinRele, HIGH);
    }
    
  
  btnPress = 0;
  lcd.clear();
  lcd.print(" Precione para ");
  lcd.setCursor(0,1);
  lcd.print("escanear huella"); 
}

void connectToNetwork() 
{
  WiFi.scanNetworks();
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(900);
    lcd.print("Conectando...");
    Serial.println("Conectando...");
    lcd.clear();
  }
  lcd.clear();
  Serial.println("CONECTADO");
  lcd.print("   Conectado!   ");
  
  lcd.setCursor(2,1); 
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();
}
void loop()                     // run over and over again
{/*
  lcd.clear();
  lcd.print(" Seleccione una ");
  lcd.setCursor(0,1);
  lcd.print(" opcion  -check "); */
  ////////////////////////////////Scan button pressed/////////////////////////////////////////////////
if(btnPress == 2)
{
  lcd.setCursor(0,0);
  lcd.print(" Precione para  ");
  lcd.setCursor(0,1);
  lcd.print("escanear huella "); 
}
 
 if(btnPress == 1 && !id_ad)
 {
  scanning = true;
  lcd.setCursor(0,0);
  lcd.print(" Coloca el dedo ");
  lcd.setCursor(0,1);
  lcd.print("ESCANEANDO------");
 }
  
 while(scanning && counter <= 46)
 {
  getFingerprintID();
  delay(100); 
  counter = counter + 1;
  if(!scanning){
    Serial.print(scanning);
    break;
  }
  if(counter == 7)
  {
    lcd.setCursor(0,0);
    lcd.print(" Coloca el dedo ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO -----");
  }

  if(counter == 14)
  {
    lcd.setCursor(0,0);
    lcd.print(" Coloca el dedo ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO  ----");
  }

  if(counter == 21)
  {
    lcd.setCursor(0,0);
    lcd.print(" Coloca el dedo ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO   ---");
  }

  if(counter == 28)
  {
    lcd.setCursor(0,0);
    lcd.print(" Coloca el dedo ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO    --");
  }
  if(counter == 35)
  {
    lcd.setCursor(0,0);
    lcd.print(" Coloca el dedo ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO     -");
  }
  if(counter == 42)
  {
    lcd.setCursor(0,0);
    lcd.print(" Coloca el dedo ");
    lcd.setCursor(0,1);
    lcd.print("ESCANEANDO      ");
  }
  if(counter == 45)
  {
    lcd.setCursor(0,0);
    lcd.print(" Tiempo agotado ");
    lcd.setCursor(0,1);
    lcd.print("                ");

    delay(2000);
    btnPress = 0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" Precione para ");
    lcd.setCursor(0,1);
    lcd.print("escanear huella "); 

   }
   
 }
 scanning = false;
 //btnPress = 0;
 counter = 0;
///////////////////////////////END WITH SCANNING PART
  if(id > 0 && id_ad)
   {
    while (!  getFingerprintEnroll() && !cancel);
    cancel = false;
    id_ad = false;
   }
/*
   if(userDeletedDB){
   }
   userDeletedDB = false;*/
}

void SaveUserToServer(int idU, String nombreU, String siglasU, String correoU, String areaU){
  
  //Serial.println("ID: "+String(idU)+" Nombre: "+nombreU+" SIGLAS: "+siglasU+" CORREO: "+correoU+" AREA: "+areaU);
  
  if(WiFi.status()== WL_CONNECTED)
    {
      http.begin(host + "createUser");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String postData = "id= "+ String(idU) + "&nombre=" + nombreU + "&siglas=" + siglasU+ "&correo=" + correoU+ "&area=" + areaU; 
      int httpCode = http.POST(postData);
      String payload = http.getString();
      lcd.clear();
      Serial.print(payload);
      http.end();
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Usuario guardado");
      lcd.setCursor(0,1);
      lcd.print("   como ID #   "); 
      lcd.setCursor(12,1);
      lcd.print(id);  
      id = 0;
      delay(3000);
      lcd.clear();
      lcd.print(" Precione para ");
      lcd.setCursor(0,1);
      lcd.print("escanear huella"); 
      userSavedDB = true;
    }
    else 
    {
      Serial.println("WiFi Desconectado");
      lcd.print("WiFi Desconectado");
    }
  
}

void DeleteUserFromServer(int idU){
   
  if(WiFi.status()== WL_CONNECTED)
    {
      http.begin(host + "deleteUser");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String postData = "id= "+ String(idU); 
      int httpCode = http.POST(postData);
      String payload = http.getString();
      Serial.print(payload);
      http.end();
      
      //return true;
      userDeletedDB = true;
    }
    else 
    {
      Serial.println("WiFi Desconectado");
      lcd.print("WiFi Desconectado");
    }
  
}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;
/*
  lcd.print("    Eliminando  ");
  Serial.println("Eliminando usuario #"+id);*/
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    DeleteUserFromServer(id);
    return true;    
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}

uint8_t getFingerprintID()
{
  uint8_t p = finger.getImage();
  switch (p)
  {
    case FINGERPRINT_OK:
    break;
    case FINGERPRINT_NOFINGER: return p;
    case FINGERPRINT_PACKETRECIEVEERR: return p;
    case FINGERPRINT_IMAGEFAIL: return p;
    default: return p; 
  }
// OK success!

  p = finger.image2Tz();
  switch (p) 
  {
    case FINGERPRINT_OK: break;    
    case FINGERPRINT_IMAGEMESS: return p;    
    case FINGERPRINT_PACKETRECIEVEERR: return p;  
    case FINGERPRINT_FEATUREFAIL: return p;  
    case FINGERPRINT_INVALIDIMAGE: return p;    
    default: return p;
  }
// OK converted!

p = finger.fingerFastSearch();

if (p == FINGERPRINT_OK)
{
  scanning = false;
  counter = 0;
  if(add_new_id)
  {
    /*if(finger.fingerID == main_user_ID)
    {
      main_user = true;
      id_ad = false;
    }
    else
    {
      add_new_id = false;
      main_user = false;
      id_ad = false;
    }*/
    
  }
  else
  {
  //miServo.write(door_open_degrees); //degrees so door is open
  //digitalWrite(pinRele, LOW); //LIGA O MÓDULO RELÉ (LÂMPADA ACENDE)        
  //digitalWrite(red_led,LOW);       //Red LED turned OFF
  //digitalWrite(green_led,HIGH);    //Green LED turned ON, shows door OPEN
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" verificando... ");
  
  postAccess(finger.fingerID, 107);
  /*lcd.setCursor(0,1);
  lcd.print(" ID: #");
  
  lcd.setCursor(6,1);
  lcd.print(finger.fingerID);

  lcd.setCursor(9,1);
  lcd.print("%: ");

  lcd.setCursor(12,1);
  lcd.print(finger.confidence);*/

  /*myFile = SD.open(file_name_to_save, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.print(" -- User match for ID# "); myFile.print(finger.fingerID);
  myFile.print(" with confidence: ");   myFile.print(finger.confidence); myFile.println(" - door open");
  myFile.close(); */
  door_locked = false;
  delay(500);
  digitalWrite(pinRele, HIGH); //Dierra el paso de corriente al RELE
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.print(" Precione para ");
  lcd.setCursor(0,1);
  lcd.print("escanear huella"); 
  delay(50);
  }
}//end finger OK

else if(p == FINGERPRINT_NOTFOUND)
{
  scanning = false;
  id_ad = false;
  counter = 0;
  lcd.setCursor(0,0);
  lcd.print("  No coincide  ");
  lcd.setCursor(0,1);
  lcd.print("Intenta de vuelta");
  add_new_id = false;
  main_user = false;  

  /*myFile = SD.open(file_name_to_save, FILE_WRITE);
  myFile.print(rtc.getDateStr()); myFile.print(" -- "); myFile.print(rtc.getTimeStr());
  myFile.print(" -- No match for any ID. Door status still the same"); 
  myFile.close(); */
  delay(2000);
  if(door_locked)
    {
      lcd.clear();
      lcd.print(" Precione para ");
      lcd.setCursor(0,1);
      lcd.print("escanear huella"); 
    }
    else
    {
      lcd.clear();
      lcd.print(" Precione para ");
      lcd.setCursor(0,1);
      lcd.print("escanear huella"); 
    }
  delay(2);
  return p;
}//end finger error
}// returns -1 if failed, otherwise returns ID #


int getFingerprintIDez() {
uint8_t p = finger.getImage();
if (p != FINGERPRINT_OK) return -1;
p = finger.image2Tz();
if (p != FINGERPRINT_OK) return -1;
p = finger.fingerFastSearch();
if (p != FINGERPRINT_OK) return -1;
// found a match!
return finger.fingerID;
}

/*This function will add new ID to the database*/

uint8_t getFingerprintEnroll() {

  int p = -1;
  if(!first_read)
  {
  lcd.setCursor(0,0);
  lcd.print("New User ID#");
  lcd.setCursor(13,0);
  lcd.print(id);
  lcd.setCursor(0,1);
  lcd.print(" Coloca el dedo ");  
  }
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(" Imagen tomada!  ");
      lcd.setCursor(0,1);
      lcd.print("                ");  
      delay(300);
      first_read = true;
      break;
    case FINGERPRINT_NOFINGER:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("New User ID#");
      lcd.setCursor(13,0);
      lcd.print(id);
      lcd.setCursor(0,1);
      lcd.print(" Coloca el dedo "); 
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("    ERROR DE    ");
      lcd.setCursor(0,1);
      lcd.print("  Comunicación  ");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.setCursor(0,0);
      lcd.print("    -Imagen     ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("  -Desconocido   ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print(" Imagen tomada!  ");
      lcd.setCursor(0,1);
      lcd.print("                 ");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEMESS:
      lcd.setCursor(0,0);
      lcd.print("Imagen desordenada!");
      lcd.setCursor(0,1);
      lcd.print("                ");
      delay(1000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("  Comunicación  ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      lcd.setCursor(0,0);
      lcd.print("  Huella no    ");
      lcd.setCursor(0,1);
      lcd.print(" coincidente  ");
      delay(1000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      lcd.setCursor(0,0);
      lcd.print("  Huella no   ");
      lcd.setCursor(0,1);
      lcd.print(" coincidente  ");
      delay(1000);
      return p;
    default:
      lcd.setCursor(0,0);
      lcd.print("     ERROR!     ");
      lcd.setCursor(0,1);
      lcd.print("    Desconocido ");
      delay(1000);
      return p;
  }
  
  lcd.setCursor(0,0);
  lcd.print(" Quita el dedo! ");
  lcd.setCursor(0,1);
  lcd.print("                ");
  delay(1000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  lcd.setCursor(0,1);
  lcd.print("ID# ");
  lcd.setCursor(4,1);
  lcd.print(id);
  p = -1;
  lcd.setCursor(0,0);
  lcd.print("Vuelve a colocar");
  lcd.setCursor(0,1);
  lcd.print(" el mismo dedo ");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print(" Imagen tomada!  ");
      lcd.setCursor(0,1);
      lcd.print("                "); 
      lcd.clear();
      delay(400);
      break;
    case FINGERPRINT_NOFINGER:
      lcd.setCursor(0,0);
      lcd.print("Vuelve a colocar");
      lcd.setCursor(0,1);
      lcd.print(" el mismo dedo ");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("  Comunicación  ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.setCursor(0,0);
      lcd.print("  Error   ");
      lcd.setCursor(0,1);
      lcd.print("  Error   ");
      delay(1000);
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("     ERROR!     ");
      lcd.setCursor(0,1);
      lcd.print("   -Desconocido    ");
      delay(1000);
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      lcd.setCursor(0,0);
      lcd.print("Image converted!");
      lcd.setCursor(0,1);
      lcd.print("                ");
      break;
    case FINGERPRINT_IMAGEMESS:
      lcd.setCursor(0,0);
      lcd.print("Image too messy!");
      lcd.setCursor(0,1);
      lcd.print("                ");
      delay(1000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.setCursor(0,0);
      lcd.print("  Comunicacion  ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      lcd.setCursor(0,0);
      lcd.print(" No fingerprint ");
      lcd.setCursor(0,1);
      lcd.print("features found  ");
      delay(1000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      lcd.setCursor(0,0);
      lcd.print("  Comunication  ");
      lcd.setCursor(0,1);
      lcd.print("     ERROR!     ");
      delay(1000);
      return p;
    default:
      lcd.setCursor(0,0);
      lcd.print("     ERROR!     ");
      lcd.setCursor(0,1);
      lcd.print("    Desconocido   ");
      delay(1000);
      return p;
  }
  /*
  // OK converted!
 lcd.setCursor(0,0);
 lcd.print("Creando modelo");
 lcd.setCursor(0,1);
 lcd.print("para ID# ");
 lcd.setCursor(8,1);
 lcd.print(id);
 delay(550);*/
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
   /*lcd.setCursor(0,0);
    lcd.print("      Huella     ");
    lcd.setCursor(0,1);
    lcd.print("   coincidente   ");
    //id=0;
    delay(1000);*/
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.setCursor(0,0);
    lcd.print("  Comunicación  ");
    lcd.setCursor(0,1);
    lcd.print("     ERROR!     ");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    lcd.setCursor(0,0);
    lcd.print("  Huella no      ");
    lcd.setCursor(0,1);
    lcd.print(" coincidente ");
    delay(1000);
    return p;
  } else {
    lcd.setCursor(0,0);
    lcd.print("     ERROR!     ");
    lcd.setCursor(0,1);
    lcd.print("    -Desconocido    ");
    delay(1000);
    return p;
  }   
  
  lcd.setCursor(0,1);
  lcd.print("ID# ");
  lcd.setCursor(4,1);
  lcd.print(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {/*
    lcd.setCursor(0,0);
    lcd.print("Huella dactilar ");
    lcd.setCursor(0,1);
    lcd.print(" almacenada!  ");*/
    lcd.clear();
    //id = 0;
    first_read = false;
    id_ad = false;
    delay(1000);
    SaveUserToServer(id, nombre, siglas, correo, area);
    return true;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.setCursor(0,0);
    lcd.print("  Comunicación  ");
    lcd.setCursor(0,1);
    lcd.print("     ERROR!     ");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    lcd.setCursor(0,0);
    lcd.print("No se almacenó");
    lcd.setCursor(0,1);
    lcd.print("en el sensor");
    delay(1000);
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    lcd.setCursor(0,0);
    lcd.print("Error al guardar");
    lcd.setCursor(0,1);
    lcd.print("en memoria ");
    delay(1000);
    return p;
  } else {
    lcd.setCursor(0,0);
    lcd.print("     ERROR!     "); 
    lcd.setCursor(0,1);   
    lcd.print("    Desconocido    ");
    delay(1000);
    return p;
  }
}
