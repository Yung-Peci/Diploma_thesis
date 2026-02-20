#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

const byte rows = 4;
const byte columns = 4;
const byte laser = 25;
const byte sensor = 34;
const byte piezo = 26;
const int arm_pin = 35;
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
bool enteringPassword = false;
bool armed = false;
bool triggered = false;



void setup() {
  pinMode(laser, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(piezo, OUTPUT);
  pinMode(arm_pin, INPUT);
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
  delay(1500);
  lcd.clear();
}



void loop() {
  char customKey = keypada.getKey();
  int arm_button = analogRead(arm_pin);
  

  if (customKey == 'A' && triggered != true && armed != true){
    enteringPassword = true;
  }
  else if (customKey == 'B' && triggered == true){
    turnOffSensor();
  }


  if (enteringPassword == true && customKey){
    attemptActivate(customKey);
  }


  if (armed){
    monitorSensor();
  }
  if (triggered){
    alarm(piezo);
  }
}



void attemptActivate(char customKey) {  
  if (is_typed != "Enter password"){
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("Enter password:");
    is_typed = "Enter password";
  }
	
  if(customKey == '*'){
    lcd.setCursor(password.length()-1,1);
    lcd.print(" ");
    password.remove(password.length() - 1);
    
  }
  else if(customKey == '#' && password.length() == 6){
    lcd.clear();
    lcd.setCursor(0,0);
    if(password == current_password){
      lcd.print("Access granted!");
      delay(2000);
      turnOnSensor();
    }
    else{
      lcd.print("Access denied!");
      delay(2000);
    }
    
    password = "";
    is_typed = "";
    enteringPassword = false;
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
}



void turnOnSensor() {
  digitalWrite(laser, HIGH);
  if(lcdState != "armed"){
      lcd.clear();
  }
  lcd.setCursor(0,0);
  lcd.print("ARMED!");
  lcdState = "armed";
  armed = true;
}



void monitorSensor(){
  int sensorValue = analogRead(sensor);
  
  if (sensorValue < 600){
    armed = false;
    triggered = true;
  }
  else {
    indicateGreen();
  }
}



void turnOffSensor() {
  if(lcdState != "disarmed"){
      lcd.clear();
  }
  lcd.setCursor(0,0);
  lcd.print("DISARMED!");
  lcdState = "disarmed";
  triggered = false;
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
