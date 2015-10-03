
#include <EEPROMVar.h>
#include <EEPROMex.h>
#include <Wire.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <VoltageReference.h>

VoltageReference vRef;

#define ONE_WIRE_BUS 2 //One Wire PIN für Dallas

int tempReadValue = 0;
float cpuVoltage = 0.0;
int currentVoltagePin = A1; //CurrentVoltagePin
float vin = 0.0;// CurrentVoltage
float r1 = 100000.0; // r1 CurrentVoltage
float r2 = 10000.0;// r2 CurrentVoltage
float vout = 0.0; //  CurrentVoltage


int batterieVoltagePin = A3; //BatterieVoltage
float batterievin = 0.0;// BatterieVoltage
float batterier1 = 100000.0; // BatterieVoltage
float batterier2 = 10000.0;// BatterieVoltage
float batterievout = 0.0;// BatterieVoltage


int currentMaxVoltagePin = A2;// Max Load Voltage
float vinMax = 0.0;  // MaxVoltage
float voutMax = 0.0; //CurrentVoltage
float r1Max = 100000.0; // MaxVoltage
float r2Max = 1.0; //MaxVoltage

//Display
#define PIN_SCE   12
#define PIN_RESET 4
#define PIN_DC    3 
#define PIN_SDIN  5 
#define PIN_SCLK  8  
#define LCD_COMMAND 0 
#define LCD_DATA  1
int BACKGROUND_PIN = 11;

int capacityAdress = 255;
int capacityRead = 0;
int milliamps = 0; //Durchgeflossener gelesener analoger Wert
int currencyMilliampsReadPin = A0;// Max Load Voltage
int enableDisableLoadingPIN = 7;//Pin für die aktivierungfunktion zum Laden
int saveVoltagePIN = 13;//Pin für die Kapazitätseinstellung
int saveVoltagePINReadValue = 0; //Wert für Speichern der Kapazität
int currentCapacity = 0; //Aktuelle Kapazität
bool readyToWrite = false;

Adafruit_PCD8544 display = Adafruit_PCD8544(PIN_SCLK, PIN_SDIN, PIN_DC, PIN_SCE, PIN_RESET);
bool loading = false; //Wird geladen?

bool readyToStart = false; //Prüft ob die aktuelle Sonnenleistung ausreicht
int readableCounts = 0; //Zählt die mind. Anzahl der Sekunden durch, wie lange das System mit der Sonnenleistung auskommen kann
float currency = 0.0f;  //Aktueller Stromfluss
float insertCurrency = 0; //Übertragener Strm
long start;
long dauer;
int countLoadingIsDisabled = 1; //Zählt die Dauer vom AUS Zustand, bis die Hintergrundbeleuchtung angeht

void writeCapacityIntoVariable(float maxValue);

void setup() {

	vRef.begin();
	insertCurrency = 0;
	
	Serial.begin(9600);
	display.begin();
	display.setContrast(60);
	display.clearDisplay();    //clears the screen and buffer
	pinMode(BACKGROUND_PIN, OUTPUT);
	digitalWrite(BACKGROUND_PIN, HIGH);
	pinMode(currentVoltagePin, INPUT);
	pinMode(batterieVoltagePin, INPUT);
	pinMode(currentMaxVoltagePin, INPUT);
	pinMode(currencyMilliampsReadPin, INPUT);
	pinMode(enableDisableLoadingPIN, OUTPUT);
	pinMode(saveVoltagePIN, OUTPUT);
	digitalWrite(enableDisableLoadingPIN, HIGH);
	digitalWrite(saveVoltagePIN, HIGH);
	loading = true;
	readableCounts = 0;

}
void loop() {




	if (readyToStart)
	{
		capacityRead = EEPROM.readLong(capacityAdress);
		//Interne CPU Spannung
		digitalWrite(enableDisableLoadingPIN, HIGH);
		saveVoltagePINReadValue = digitalRead(saveVoltagePIN);

		if (saveVoltagePINReadValue == 1)
		{
			start = millis();
			display.clearDisplay();
			setTemperature();
			delay(1000);
			display.clearDisplay();
			display.setTextSize(1);

			cpuVoltage = readVcc();
			cpuVoltage = cpuVoltage / 1000.00;

			//VCC Messung
			setCurrentVoltage();
			//Ladeschlussspannungsmessung
			setMaxLoadVoltage();
			//Batteriespannung messen
			setBatterieVoltage();
			countCurrency();

			set_text(0, 30, "IL: " + String((int)insertCurrency) + " mAh", BLACK);
			CheckBackround();
			delay(1000);
			dauer = millis() - start;
			countInsertedCurrency();
			Serial.println(dauer);
			display.clearDisplay();

		}
		else
		{
			readyToWrite = true;
			writeCapacityIntoVariable();

			delay(100);
			display.clearDisplay();

		}
		if (readyToWrite && saveVoltagePINReadValue == 1)
		{
			readyToWrite = false;
			EEPROM.writeLong(capacityAdress, currentCapacity);
			setup();
		}

	}
	else
	{
		if (readableCounts >= 10)
		{
			readyToStart = true;
		}
		
		readableCounts = readableCounts + 1;
		delay(1000);
		
	}

}
void countCurrency()
{

	tempReadValue = analogRead(currencyMilliampsReadPin);
	Serial.println("Aktuelle Stromspannung" + String(tempReadValue));
	if (1024 - (tempReadValue * 2) <= 0)
	{
		tempReadValue = 512;
	}
	double temp_cpuVoltage = readVcc();
	double currencyMilliampsReadValue = (tempReadValue * (temp_cpuVoltage / 1024.0));

	currency = (((temp_cpuVoltage) / 2.0) - currencyMilliampsReadValue) / 66 * 1000;
	Serial.println("currency:" + String(currency));
	if (loading)
	{
		countLoadingIsDisabled = 1;
		set_text(0, 40, "I:" + String(currency) + " mA.", BLACK);
	}
	else
	{
		countLoadingIsDisabled = countLoadingIsDisabled + 1;
		set_text(0, 40, "Laden aus.", BLACK);
	}
}
//Ein und ausschalten der Hintergrundbeleuchtung
void CheckBackround()
{

	if (countLoadingIsDisabled > 50)
	{
		digitalWrite(BACKGROUND_PIN, LOW);

	}
	//Wenn die Batteriespannung groeser als die Solarzellenspannung ist, dann schalte die Hintergrundbeleuchtung ein
	if (insertCurrency >= capacityRead)
	{
		digitalWrite(enableDisableLoadingPIN, LOW);
		loading = false;
	
		return;
	}
	if (vin < batterievin || currency <= 0)
	{
		digitalWrite(enableDisableLoadingPIN, HIGH);
		loading = false;

	}
	else
	{
		digitalWrite(enableDisableLoadingPIN, HIGH);
		digitalWrite(BACKGROUND_PIN, HIGH);
	
		loading = true;
	}

}
//Setze Batteriespannung
float setBatterieVoltage()
{
	tempReadValue = analogRead(batterieVoltagePin);
	batterievout = tempReadValue * (cpuVoltage / 1024.0);
	batterievin = batterievout / (r2 / (r1 + r2));
	set_text(0, 0, "Batt.:" + String(batterievin) + " V.", BLACK);

	return batterievin;
}
//Setze Solarzellenspannung
float setCurrentVoltage()
{
	tempReadValue = analogRead(currentVoltagePin);
	vout = tempReadValue * (cpuVoltage / 1024.0);
	vin = vout / (r2 / (r1 + r2));
	set_text(0, 10, "VCC:" + String(vin) + " V.", BLACK);
	return vin;

}
//Setze Max. Batteriekapazität
float setMaxLoadVoltage()
{
	set_text(0, 20, "Cp:" + String(capacityRead) + " mAh", BLACK);
}
//Schreibt die aktuell gelesene Kapazität in die Variable
void writeCapacityIntoVariable()
{

	tempReadValue = analogRead(currentMaxVoltagePin);
	voutMax = tempReadValue * (cpuVoltage / 1024.0);
	r2Max = (r1Max*voutMax) / (vin - voutMax);

	vinMax = (8912.00*r2Max) / 10000.00;
	currentCapacity = vinMax;
	set_text(0, 20, "Cp:" + String(currentCapacity) + " mAh", BLACK);
}
//Setze Displaytext
void set_text(int x, int y, String text, int color) {

	display.setTextColor(color);  //Textfarbe setzen, also Schwarz oder Weiss
	display.setCursor(x, y);      // Startpunkt-Position des Textes
	display.println(text);       // Textzeile ausgeben
	display.display();            //Display aktualisieren
}
//Lese interne CPU Spannung

float readVcc() {
	return vRef.readVcc();
}
//Zaehle den uebertragenen Strom
void countInsertedCurrency()
{
	if (loading)
	{
		int multiplikator = dauer / 1000;
		insertCurrency += ((currency / 60) / 60)*multiplikator;

	}
	//for (int i = 0; i < 84; i++)
	//{
	//	set_text(i, 30, " ", BLACK);
	//}
	display.display();
	set_text(0, 30, "IL: " + String((int)insertCurrency) + " mAh", BLACK);

}
void setTemperature()
{
	OneWire ds(ONE_WIRE_BUS);
	DallasTemperature sensors(&ds);
	sensors.begin();
	byte i;
	byte present = 0;
	byte data[12];
	byte addr[8];
	String resultString;
	int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
	if (!ds.search(addr)) {
		ds.reset_search();
		return;
	}
	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1); // start conversion, with parasite power on at the end
	delay(1000);     // maybe 750ms is enough, maybe not
					 // we might do a ds.depower() here, but the reset will take care of it.
	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad
	for (i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
	}

	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;  // test most sig bit
	if (SignBit) // negative
	{
		TReading = (TReading ^ 0xffff) + 1; // 2's comp
	}
	Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

	Whole = Tc_100 / 100;  // separate off the whole and fractional portions
	Fract = Tc_100 % 100;

	if (SignBit) // If its negative
	{
		resultString += "-";
		//Serial.print("-");
	}

	resultString += Whole;
	//Serial.print(Whole);
	resultString += ".";
	//Serial.print(".");
	if (Fract < 10)
	{
		resultString += "0";
		//Serial.print("0");
	}
	//Serial.print(Fract);
	resultString += Fract;
	if (resultString.toInt() < 0)
	{
		display.setTextSize(1);
		set_text(0, 22, "-", BLACK);
	}
	display.setTextSize(3);
	set_text(10, 15, String(resultString.toInt()) + " C ", BLACK);
	display.setTextSize(2);
	set_text(53, 10, "o", BLACK);
	resultString = "";
}

///DEBUG

//Serial.println("Aktuelle currencyMilliampsReadValue:" + String(currencyMilliampsReadValue));
//Serial.println(currencyMilliampsReadValue,10);
//Serial.println("(((temp_cpuVoltage) / 2.0) - currencyMilliampsReadValue) :" + String((((temp_cpuVoltage) / 2.0) - currencyMilliampsReadValue)));

//Serial.println("(((temp_cpuVoltage) / 2.0) - currencyMilliampsReadValue) :" + String((((temp_cpuVoltage) / 2.0) - currencyMilliampsReadValue) * 1000) );



//float readVcc() {
//   //Read 1.1V reference against AVcc
//   //set the reference to Vcc and the measurement to the internal 1.1V reference
//  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
//    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
//  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
//     ADMUX = _BV(MUX5) | _BV(MUX0) ;
//  #else
//    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
//  #endif  
// 
//  delay(2); // Wait for Vref to settle
//  ADCSRA |= _BV(ADSC); // Start conversion
//  while (bit_is_set(ADCSRA,ADSC));  //measuring
// 
//  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
//  uint8_t high = ADCH; // unlocks both
// 
//  long result = (high<<8) | low;
// 
//  result = 1125300L / result;  //Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
//  return result;  //Vcc in millivolts
//}


// Serial.println("setCurrentVolatage" + String(tempReadValue));


//Serial.println("setMaxLoadVoltage" + String(tempReadValue));	

/*Serial.println("vin" + String(vin));
Serial.println("batterievin" + String(batterievin));
Serial.println("currency" + String(currency));*/


//temp_cpuVoltage = temp_cpuVoltage;
//Serial.println(temp_cpuVoltage, 10);
//Serial.println("Aktuelle CPU Spannung" + String(temp_cpuVoltage));
//Serial.println("Aktuelle tempReadValue Spannung" + String(tempReadValue));

//Serial.println("saveVoltagePINReadValue:" + String(saveVoltagePINReadValue));