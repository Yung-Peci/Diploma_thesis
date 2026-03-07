#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <time.h>

const byte rows = 4;
const byte columns = 4;
const byte laser = 25;
const byte sensor = 34;
const byte piezo = 26;
const int green_indicator = 32;
const int red_indicator = 33;

unsigned long previousMillis = 0;
const long interval = 150;

String lcdState = "";

LiquidCrystal_I2C lcd(0x20, 16, 2);

char layout[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[rows] = {23, 19, 18, 27}; 
byte columnPins[columns] = {17, 16, 4, 13};

Keypad keypada = Keypad(makeKeymap(layout), rowPins, columnPins, rows, columns);


String password = "";
String current_password = "123454";
String is_typed = "";

int cursor_position = 0;

enum State {
  ENTERING_PASSWORD,
  ARMED,
  TRIGGERED,
  DISARMED,
  HOME,
  SETTINGS,
  CHANGE_PASSWORD,
  DIAGNOSE,
  NEW_PASSWORD
};

State currentState = HOME;



const char* ssid = "Kalina Net";
const char* passwordd = "kalina0911";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;        // Change for your timezone
const int   daylightOffset_sec = 3600;   // Usually 3600 if DST active

void connectToWiFi();
void setupTime();
void printLocalTime();



void setup() {
  pinMode(laser, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(piezo, OUTPUT);
  pinMode(green_indicator, OUTPUT);
  pinMode(red_indicator, OUTPUT);
  Wire.begin(21, 22);
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Welcome!");
  lcd.setCursor(0,1);
  lcd.print("Loading...");
  connectToWiFi();
  setupTime();
  delay(1500);
  lcd.clear();
}



void loop() {
  char customKey = keypada.getKey();
  

  if (customKey == 'A' && currentState != TRIGGERED && currentState != ARMED){
    currentState = ENTERING_PASSWORD;
  }
  else if (customKey == 'B'  && currentState == TRIGGERED || customKey == 'B' && currentState == ARMED){
    currentState = DISARMED;
  }
  else if (customKey == 'C' && currentState == HOME){
    currentState = SETTINGS;
  }

  switch (currentState) {
    case ENTERING_PASSWORD:
      if (customKey){
        passwordMode("Attempt activate", customKey);
      }
      break;
    case DISARMED:
      turnOffSensor();
      break;
    case ARMED:
      monitorSensor();
      break;
    case TRIGGERED:
      alarm(piezo);
      break;
    case HOME:
      homeScreen();
      break;
    case SETTINGS:
      settingsMenu(customKey);
      break;
    case CHANGE_PASSWORD:
      passwordMode("Reset password", customKey);
      break;
    case NEW_PASSWORD:
      passwordMode("New password", customKey);
  }
}



void settingsMenu(char customKey){
  if(lcdState != "settings"){
    lcd.clear();
    lcdState = "settings";
  }
  String cursor = ">";

  if (customKey == '*' && cursor_position == 0){
    lcd.setCursor(0, cursor_position);
    lcd.print(" ");
    cursor_position = 1;
  }
  else if (customKey == '*' && cursor_position == 1){
    lcd.setCursor(0, cursor_position);
    lcd.print(" ");
    cursor_position = 0;
  }
  else if (customKey == '#' && cursor_position == 0){
    currentState = CHANGE_PASSWORD;
  }
  else if (customKey == '#' && cursor_position == 1){
    currentState = DIAGNOSE;
  }
  else if (customKey == 'B'){
    currentState = HOME;
    cursor_position = 0;
  }
    
  lcd.setCursor(0,cursor_position);
  lcd.print(cursor);
  lcd.setCursor(1,0);
  lcd.print("Change password");
  lcd.setCursor(1,1);
  lcd.print("Diagnostics");
}



// zamenqm attemptActivate i changePassword s universalna funkciq s argumenti
void passwordMode(String mode, char customKey){
  if (lcdState != "Enter password"){
    lcd.clear();
    lcd.setCursor(0, 0);
    if (mode == "Reset password"){
      lcd.print("Old password:");
    }
    else if (mode == "New password"){
      lcd.print("New password:");
    }
    else if (mode == "Attempt activate"){
      lcd.print("Enter password:");
    }
    lcdState = "Enter password";
  }

  if(customKey == '*'){
    lcd.setCursor(password.length()-1,1);
    lcd.print(" ");
    password.remove(password.length() - 1);
    
  }

  else if(customKey == '#' && password.length() == 6){
    lcd.clear();
    lcd.setCursor(0,0);
    if(password == current_password && mode == "Reset password"){
      password = "";
      lcdState = "";
      currentState = NEW_PASSWORD;
    }
    else if (password != current_password && mode == "Reset password"){
      lcd.print("Access denied!");
      delay(1500);
      currentState = SETTINGS;
    }
    else if(password == current_password && mode == "Attempt activate"){
      lcd.print("Access granted!");
      delay(2000);
      turnOnSensor();
      currentState = ARMED;
    }
    else if (password != current_password && mode == "Attempt activate"){
      lcd.print("Access denied!");
      delay(1500);
      currentState = HOME;
    }
    else if (mode == "New password"){
      lcd.print("Password changed");
      current_password = password;
      delay(1500);
      currentState = SETTINGS;
    }
    
    password = "";
    lcdState = "";
    return;
  }
  else if (customKey >= '0' && customKey <= '9' && password.length() < 6) {
    password += customKey;
    
    for (int i = 0; i < password.length(); i++){
      lcd.setCursor(i, 1);
      lcd.print("*");
    }
    Serial.println(password);
  }
  else if (customKey == 'B'){
    password = "";
    lcdState = "";
    if (mode == "Attempt activate"){
      currentState = HOME;
    }
    else {
      currentState = SETTINGS;
    }
    return;
  }
}



void homeScreen(){
  if (lcdState != "home"){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Home screen");
    lcdState = "home";
  }
  printLocalTime();
}



void turnOnSensor() {
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Leave laser area");

  for (int i = 5; i >= 0; i--){
    indicateGreen();
    lcd.setCursor(0,0);
    lcd.print("Turning on in: ");
    lcd.print(i);
    delay(500);
    turnOffIndicators();
    delay(500);
  }

  digitalWrite(laser, HIGH);
  if(lcdState != "armed"){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ARMED!");
      lcdState = "armed";
  }
  currentState = ARMED;
}



void monitorSensor(){
  int sensorValue = analogRead(sensor);
  
  if (sensorValue < 600){
    currentState = TRIGGERED;
  }
  else {
    indicateGreen();
  }
}



void turnOffSensor() {
  if(lcdState != "disarmed"){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("DISARMED!");
      lcdState = "disarmed";
  }
  currentState = HOME;
  noTone(piezo);
  digitalWrite(laser, LOW);
  turnOffIndicators();
  delay(2000);
}



void indicateGreen(){
  digitalWrite(green_indicator, HIGH);
  digitalWrite(red_indicator, LOW);
}



void turnOffIndicators(){
  digitalWrite(green_indicator, LOW);
  digitalWrite(red_indicator, LOW);
}



void alarm(int pin) {
  unsigned long currentMillis = millis();
  static bool state = false;

  digitalWrite(green_indicator, LOW);

  if (currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;

    digitalWrite(red_indicator, !digitalRead(red_indicator));

    state = !state;

    if(state) {
      tone(pin, 3000);
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("!TRESPASSING!");
    }
    else {
      tone(pin, 4243);
      lcd.clear();
    }
  }
}



void connectToWiFi() {
  WiFi.begin(ssid, passwordd);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
}

void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  lcd.setCursor(11,1);
  lcd.print(&timeinfo, "%H:%M:%S");
}
