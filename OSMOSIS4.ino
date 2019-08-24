//PROGRAMA OSMOSIS 3.0 PARA CLIENTE KEEPEX  
//DESARROLLADOR: FRANCISCO REATEGUI

#include <SPI.h>
#include <Wire.h> //para la lectura de i2c
#include <Adafruit_ADS1015.h> //libreria de los ADC
#include "SHTSensor.h" //Sensor de temperatura y humedad
#include <Adafruit_GFX.h> //pantalla oled
#include <Adafruit_SSD1306.h> //pantalla oled
#include "PCF8574.h"//expansor I/O
#include <Adafruit_MCP4725.h> //DAC
#include <ArduinoJson.h>
//--------------define-----
#define OLED_RESET 4 //pantalla oled

Adafruit_MCP4725 dac;

Adafruit_SSD1306 display(OLED_RESET);//pantalla oled
SHTSensor sht; //sensor de temperatura y humedad

Adafruit_ADS1115 ads1(0x49); //presion #1, presion #2, presion #3, presion #4
Adafruit_ADS1115 ads2(0x4a); //presion estanque, temperaturaagua, tds
Adafruit_ADS1115 ads3(0x48); //vibracion, corriente, posicion valvula
Adafruit_ADS1115 ads4(0x4B); ///corriente 1, corriente 2,

//PCF8574 pcf8574out(0x26); //es de entrada...se debe cambiar el nombre out por in
PCF8574 pcf8574out(0x3E); //es de entrada...se debe cambiar el nombre out por in


PCF8574 pcf8574in(0x38); //direccion expansor input //ahora es el output//original es 20
//PCF8574 pcf8574in(0x20);

//PCF8574 pcf8574in2(0x24); //direccion expansor input
PCF8574 pcf8574in2(0x3C); //direccion expansor input


unsigned long timeElapsed;//EXPANSOR I/O

//---------------------------inicio declaracion de variables analogas
unsigned int presionuno; //ADS #1
unsigned int presiondos; //ADS #1
unsigned int presiontres; //ADS #1
unsigned int presioncuatro; //ADS #1

unsigned int presionestanque; //ADS #2
unsigned int temperaturaagua; //ADS #2
unsigned int tds; //ADS #2

unsigned int sensorvibracion; //ADS #3
unsigned int sensorvibracion2; //ADS #3
unsigned int posicionvalvula; //ADS #3

unsigned int sensorcorriente; //ADS #4
unsigned int sensorcorriente2; //ADS #4

float temperatura;
float humedad;

float caudalimetrouno;

//---------------------------fin declaracion de variables analogas


//-------------------------inicio declaracion de variables digitales------------------
//input, las variables necesarias por cada evento fisico son: estadodesistema, actual, estadoanterior, contador y cronometro.




byte puertatablero; // sensor efecto hall en puerta

byte estadofiltrozeolita = 0; //es la variable que utiliza el sistema, esta variable tiene mas restricciones para cambiar de estado
byte filtrozeolita; //input del filtro de zeolita
byte estadoanteriorfiltrozeolita = 0;
byte contadorfiltrozeolita = 0;
unsigned long cronometrofiltrozeolita;


byte estadonivelaltoestanque = 0;
byte nivelaltoestanque; //nivel alto del estanque
byte estadoanteriornivelaltoestanque = 0;
byte contadornivelaltoestanque = 0;
unsigned int cronometronivelaltoestanque;


byte estadonivelbajoestanque = 0;
byte nivelbajoestanque; //nivel bajo del estanque
byte estadoanteriornivelbajoestanque;
byte contadornivelbajoestanque = 0;
unsigned int cronometronivelbajoestanque;


byte estadopresostatobaja = 0; //es el estado que lee el sistema, puede que esta variable tenga mas restricciones para cambiar de estado
byte presostatobaja; //estado actual, estado fisico
byte estadoanteriorpresostatobaja = 0;
byte contadorpresostatobaja = 0;
unsigned long cronometropresostatobaja; //para detectar desde que se activa el presostato de baja


byte estadopresostatoalta = 0; //es el estado que lee el sistema, puede que esta variable tenga mas restricciones para cambiar de estado
byte presostatoalta; //estado actual, estado fisico
byte estadoanteriorpresostatoalta = 0;
byte contadorpresostatoalta = 0;
unsigned long cronometropresostatoalta; //para detectar desde que se activa el presostato de baja


//variable memflush
byte estadomemflush = 0;
unsigned long cronometromemflush;
unsigned long cuentaregresivamemflush = 60000;//este es el tiempo que durará el memflush en milisegundos (por definicion debe durar 4 minutos)

unsigned long cronometrozeolita; //esta variable muestra en pantalla la cantidad de segundos desde el inicio del lavado del filtro zeolita.


//---------------botones agregados---------
byte botondeservicio; //es el boton de servicio que puede servir para dar aviso a keepex...
byte botondepcb; //es un boton auxiliar para el funcionamiento de la placa, no para usuario.

//--------------se agregan guardamotores-------------

byte guardamotor1;
byte guardamotor2;



//output
byte contactoralta;
byte contactorbaja;
byte electrovalvula;
byte clorador;
byte alarmavisual;


byte botononoff;
byte estadoanteriorbotononoff = 0;
byte contadorbotononoff = 0;

byte botonauxiliar;
byte estadoanteriorbotonauxiliar = 0;
byte contadorbotonauxiliar = 0;
byte iniciocambiofiltro = 0;
unsigned long cronometrocambiofiltro;

byte botonautomatico;



byte iniciodeplanta = 0; //el sistema sabrá cuando esté en el estado de encendido con esta variable, cuando el inicio termine vuelve a cero

unsigned long cronometroinicio; //para detectar desde que enciende la planta

unsigned long cronometrorpi = 0; //cronometro de la comunicacion con la raspberry, cada x segundo preguntará

unsigned long cronometropantallas = 0;
unsigned long cronometronextion = 0;


unsigned long tiemporespuesta = 0;//para la comunicacion con la rpi

unsigned int horalavado; //para el lavado de las membranas.

//-------------------------fin declaracion de variables digitales------------------

//------------inicio variables string-----------------
String reporte; //para ver los valores en el puerto comm del pc
//------------fin variables string-----------------
//----------------------inicio declaracion JSON-----------------
StaticJsonBuffer<500> jsonBuffer; //DECLARACION DE BUFFER 
JsonObject& data = jsonBuffer.createObject();

StaticJsonBuffer<500> jsonBuffer2; //segundo JSON 
JsonObject& question = jsonBuffer2.createObject(); //segundo JSON



//----------------------fin declaracion JSON-----------------

//------------------inicio declaracion electrovalvula----------
int rango1 = 880; //rango bajo 
byte direccionrango1 = 0; //direccion en la eeprom de la direccion
int rango2 = 910;
byte direccionrango2 = 10;
int valorvalvula = 3000; //original 1200
int retrovalorvalvula; //esta variable ya se lee antes y se llama posicion valvula
int precision = 5; //velocidad de valvula
unsigned long cronometrovalvula = 0;
int pidvalvula = 300;

int valorsensor; //NO SE SI SE OCUPA
int rebootvalvula = 1; //para empezar siempre desde la posicion de la valvula correcta


//------------------fin declaracion electrovalvula----------

void setup() {

	//--------------------------------inicio setup expansor I/O --------------------
	Wire.begin();
	pinMode(LED_BUILTIN, OUTPUT);


	pcf8574out.pinMode(P0, OUTPUT); //se declaran los pines como salida
	pcf8574out.pinMode(P1, OUTPUT);
	pcf8574out.pinMode(P2, OUTPUT);
	pcf8574out.pinMode(P3, OUTPUT);
	pcf8574out.pinMode(P4, OUTPUT);
	pcf8574out.pinMode(P5, OUTPUT);
	pcf8574out.pinMode(P6, OUTPUT);
	pcf8574out.pinMode(P7, OUTPUT);
	pcf8574out.begin();              //inicializacion del integrado
	pcf8574out.digitalWrite(P0, HIGH); // inician todos los pines en low
	pcf8574out.digitalWrite(P1, HIGH);
	pcf8574out.digitalWrite(P2, HIGH);
	pcf8574out.digitalWrite(P3, HIGH);
	pcf8574out.digitalWrite(P4, HIGH);
	pcf8574out.digitalWrite(P5, HIGH);
	pcf8574out.digitalWrite(P6, HIGH);
	pcf8574out.digitalWrite(P7, HIGH);

	pcf8574in.pinMode(P0, OUTPUT);  //estos pines son de salida, pero deben mantenerse en alto
	pcf8574in.pinMode(P1, OUTPUT);  //por lo que se declaran en output, se ponen en alto, y luego se declaran como input
	pcf8574in.pinMode(P2, OUTPUT);
	pcf8574in.pinMode(P3, OUTPUT);
	pcf8574in.pinMode(P4, OUTPUT);
	pcf8574in.pinMode(P5, OUTPUT);
	pcf8574in.pinMode(P6, OUTPUT);
	pcf8574in.pinMode(P7, OUTPUT);
	pcf8574in.begin();             //inicializacion del integrado
	pcf8574in.digitalWrite(P0, HIGH); //se ponen todos los pines en alto
	pcf8574in.digitalWrite(P1, HIGH);
	pcf8574in.digitalWrite(P2, HIGH);
	pcf8574in.digitalWrite(P3, HIGH);
	pcf8574in.digitalWrite(P4, HIGH);
	pcf8574in.digitalWrite(P5, HIGH);
	pcf8574in.digitalWrite(P6, HIGH);
	pcf8574in.digitalWrite(P7, HIGH);

	pcf8574in.pinMode(P0, INPUT);  //finalmente quedan declarados como input, (ver mas detalle en datasheet, "quasi direccional")
	pcf8574in.pinMode(P1, INPUT);
	pcf8574in.pinMode(P2, INPUT);
	pcf8574in.pinMode(P3, INPUT);
	pcf8574in.pinMode(P4, INPUT);
	pcf8574in.pinMode(P5, INPUT);
	pcf8574in.pinMode(P6, INPUT);
	pcf8574in.pinMode(P7, INPUT);

	pcf8574in2.pinMode(P0, OUTPUT);  //estos pines son de salida, pero deben mantenerse en alto
	pcf8574in2.pinMode(P1, OUTPUT);  //por lo que se declaran en output, se ponen en alto, y luego se declaran como input
	pcf8574in2.pinMode(P2, OUTPUT);
	pcf8574in2.pinMode(P3, OUTPUT);
	pcf8574in2.pinMode(P4, OUTPUT);
	pcf8574in2.pinMode(P5, OUTPUT);
	pcf8574in2.pinMode(P6, OUTPUT);
	pcf8574in2.pinMode(P7, OUTPUT);
	pcf8574in2.begin();             //inicializacion del integrado
	pcf8574in2.digitalWrite(P0, HIGH); //se ponen todos los pines en alto
	pcf8574in2.digitalWrite(P1, HIGH);
	pcf8574in2.digitalWrite(P2, HIGH);
	pcf8574in2.digitalWrite(P3, HIGH);
	pcf8574in2.digitalWrite(P4, HIGH);
	pcf8574in2.digitalWrite(P5, HIGH);
	pcf8574in2.digitalWrite(P6, HIGH);
	pcf8574in2.digitalWrite(P7, HIGH);

	pcf8574in2.pinMode(P0, INPUT);  //finalmente quedan declarados como input, (ver mas detalle en datasheet, "quasi direccional")
	pcf8574in2.pinMode(P1, INPUT);
	pcf8574in2.pinMode(P2, INPUT);
	pcf8574in2.pinMode(P3, INPUT);
	pcf8574in2.pinMode(P4, INPUT);
	pcf8574in2.pinMode(P5, INPUT);
	pcf8574in2.pinMode(P6, INPUT);
	pcf8574in2.pinMode(P7, INPUT);



	   	  


	//--------------------------------fin setup expansor I/O --------------------
	Serial.begin(115200); //puerto comm del PC

	//solo se ejecuta la primera vez que se configura la pantalla nextion
	/*
	  Serial1.begin(9600);
	  delay(200);
	  Serial1.print("bauds=115200");
	  Serial1.write(0xff);  // We always have to send this three lines after each command sent to nextion.
	  Serial1.write(0xff);
	  Serial1.write(0xff);
	  Serial1.end();
	*/

	Serial1.begin(115200);

	//descripcion de ganancias del ADS, cada inicializacion de ads tiene su ganancia.
	// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit =  0.1875mV (default)
	ads1.setGain(GAIN_TWOTHIRDS);        // 1x gain   +/- 4.096V  1 bit =  0.125mV
	ads2.setGain(GAIN_TWOTHIRDS);        // 1x gain   +/- 4.096V  1 bit =  0.125mV
	ads3.setGain(GAIN_TWOTHIRDS);        // 1x gain   +/- 4.096V  1 bit =  0.125mV
	ads4.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit =  0.125mV


	// ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit =  0.0625mV
	// ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit =  0.03125mV
	// ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit =  0.015625mV
	// ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit =  0.0078125mV
	ads1.begin(); // inicialización del ads #1
	ads2.begin(); // inicialización del ads #2
	ads3.begin(); // inicialización del ads #3
	ads4.begin(); // inicialización del ads #4

	//---------------------------------------fin ads--------------------

	//-------------------inicio setup sensor de t y rh---------------
	if (sht.init()) {
		//Serial.print("init(): success\n");
	}
	else {
		//Serial.print("init(): failed\n");
	}
	sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // only supported by SHT3x
	//-------------------fin setup sensor de t y rh---------------

	//----------------------------inicio setup pantalla oled--------------
	display.begin(SSD1306_SWITCHCAPVCC, 0x3D); //inicio pantalla oled

	display.clearDisplay();

	// presentacion de pantalla de inicio, se puede eliminar sin problemas
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	display.println("LYTHIUM");
	display.setCursor(0, 20);
	display.println("2019");
	display.display();
	delay(1000);


	//----------------------------fin setup pantalla oled--------------



	//------------------botones auxiliares----------------------------
	pinMode(3, INPUT); //declaracion del boton onoff
	pinMode(2, INPUT); //declaracion del boton auxiliar
	pinMode(4, INPUT); //declaracion del boton auxiliar


	//-------------------inicio setup dac------------
	dac.begin(0x63); //original 60
	// dac.setVoltage(0, false); //4095 tengo el maximo

	//-------------------fin setup dac------------

}

void loop() {
	lecturasensores();
	detectarcambioestadopines();
	mostrarpantallas();
	comunicacionrpi();
	// delay(50);

	if (iniciocambiofiltro == 1)
	{
		iniciocambiofiltro = 0;
		unsigned long cronometroauxiliar = millis();
		while ((millis() - cronometroauxiliar) < 5000)
		{
			Serial1.print("t28.txt=");
			Serial1.print("\"");
			Serial1.print("TIENE DOS MINUTOS PARA CAMBIO FILTRO " + String((millis() - cronometroauxiliar) / 1000));
			//Serial1.print(String((millis()-cronometrozeolita)/1000));
			Serial1.print("\"");
			Serial1.write(0xff);
			Serial1.write(0xff);
			Serial1.write(0xff);
		}
		Serial1.print("t28.txt=");
		Serial1.print("\"");
		Serial1.print("TIEMPO TERMINADO");
		//Serial1.print(String((millis()-cronometrozeolita)/1000));
		Serial1.print("\"");
		Serial1.write(0xff);
		Serial1.write(0xff);
		Serial1.write(0xff);
		//AGREGAR QUE PUEDA SALIR DEL WHILE PRESIONANDO EL BOTON AUXILIAR

	}

	if (rebootvalvula == 1) //esta funcion lleva la valvula a una posicion segura antes de iniciar, SI LA VALVULA NO LLEGA A LA POSICION LA PLANTA NO PARTE
	{
		int division; //esta variable es solo para mostrar el numero de acercamiento al valor deseado
		dac.setVoltage(valorvalvula, false);
		if (posicionvalvula > valorvalvula)
		{
			division = posicionvalvula - valorvalvula;
			division = division * 100 / valorvalvula;
			if (division < 15)
			{
				rebootvalvula = 0;
			}
		}
		else
		{
			division = valorvalvula - posicionvalvula;
			division = division * 100 / valorvalvula;
			if (division < 15)
			{
				rebootvalvula = 0;
			}
		}
	}

	else if (estadofiltrozeolita == 1) //este es el estado más importante porque no depende del funcionamiento de la planta
	{
		enviarnextion("LAVADO FILTRO DE ZEOLITA", "t26.txt=");
		activador("contactorbaja", 0);
		activador("electrovalvula", 1);
		activador("contactoralta", 1);
		Serial1.print("t28.txt=");
		Serial1.print("\"");
		Serial1.print(String((millis() - cronometrozeolita) / 1000));
		Serial1.print("\"");
		Serial1.write(0xff);
		Serial1.write(0xff);
		Serial1.write(0xff);

	}

	else if (estadomemflush == 1 && (millis() - cronometromemflush < cuentaregresivamemflush)) //esta funciona se activará cuando el filtro de zeolita termine de su lavado
	{
		Serial1.print("t28.txt=");
		Serial1.print("\"");
		Serial1.print(String((cuentaregresivamemflush - (millis() - cronometromemflush)) / 1000));
		Serial1.print("\"");
		Serial1.write(0xff);
		Serial1.write(0xff);
		Serial1.write(0xff);
		dac.setVoltage(3000, false);//la valvulamotorizada se va a 0 incluir codigo. //antes era 2000, ojo.
		if (posicionvalvula >= 400) //20 MENOS POR SEGURIDAD
		{
			activador("contactorbaja", 0);
			activador("contactoralta", 0);
			activador("electrovalvula",0);
			enviarnextion("LAVADO DE MEMBRANAS EN PROCESO", "t26.txt=");
		}
		else
		{
			enviarnextion("POSICIONANDO VALVULA PARA LAVADO DE MEMBRANAS", "t26.txt=");
			activador("contactorbaja", 1);
			activador("electrovalvula", 1);
			activador("contactoralta", 1);
		}

	}
	else if (iniciodeplanta == 1 && botononoff == 1) //cuando la funcion "iniciodeplanta" está en 1, empieza la rutina de inicio.
	{

		if (millis() - cronometroinicio < 10000) //primer paso de inicio de la planta, bomba de baja y electrovalvula se activan por X segundos
		{
			dac.setVoltage(3000, false);//LA VALVULA SE ABRIRA EN EL INICIO DE LA PLANTA, DESPUES EMPIEZA LA REGULACION... (NUEVO)
			activador("contactorbaja", 0);
			activador("electrovalvula", 0);
		}
		else                                   //luego de eso se activa el contactor de la bomba de alta
		{
			activador("contactoralta", 0);
			activador("clorador", 0);
			iniciodeplanta = 0;
		}
		enviarnextion("INICIANDO CICLO DE PRODUCCION", "t26.txt=");
	}
	else if (iniciodeplanta == 0 && botononoff == 1)
	{
		if (estadopresostatobaja == 1 || estadopresostatoalta == 1 || estadonivelaltoestanque == 1)
		{
			activador("contactorbaja", 1);
			activador("electrovalvula",1);
			activador("contactoralta", 1);
			activador("clorador", 1);
			if (estadopresostatobaja == 1 || estadopresostatoalta == 1)
			{
				enviarnextion("FALLA PRESOSTATOS", "t26.txt="); //AQUI SEPARAR EL ESTADO DE ALTA Y BAJA PARA MOSTRAR EN PANTALLA, (ESTO ES UNA ALERTA ESTE ESTADO NO VA AQUI... SACAR DE INMEDIATO)
				//----nuevo-----
				delay(3000);
				enviarnextion("REINTENTANDO INICIAR", "t26.txt=");
				delay(3000);
				
				iniciodeplanta = 1;
				cronometroinicio = millis();
				presostatoalta = !pcf8574in.digitalRead(P4);
				presostatobaja = !pcf8574in.digitalRead(P5);
				//---fin nuevo----
			}
			else {
				enviarnextion("PLANTA STANDBY (ESTANQUE FULL)", "t26.txt=");
			}

		}

		else if (estadonivelaltoestanque == 0)
		{

			//  enviarnextion(String(valorvalvula), "t26.txt=");
			enviarnextion("PRODUCIENDO AGUA", "t26.txt=");
			dac.setVoltage(valorvalvula, false);
			//-----------------inicio regulacion de valvula---------
		   // enviarnextion("TEST", "t26.txt=");



			if (millis() - cronometrovalvula > pidvalvula) //con esto le doy rapidez al giro del motro cuando se esta acercando al valor deseado
			{
				cronometrovalvula = millis();
				if (presioncuatro >= rango1 && presioncuatro <= rango2)
				{
					dac.setVoltage(valorvalvula, false);

					// enviarnextion("VALVULA EN POSICION", "t26.txt=");
					//decir que la valvula está en posicion
				}
				else if (presioncuatro < rango1)
				{
					//  enviarnextion("PRESION MENOR", "t26.txt=");
					if (rango1 - presioncuatro > 100)
					{
						pidvalvula = 1; //mas velocidad
					}
					else pidvalvula = 500; //menos velocidad


					if (valorvalvula > 0 && valorvalvula <= 4000)
					{
						valorvalvula = valorvalvula - precision;
						dac.setVoltage(valorvalvula, false);
					}
				}
				else if (presioncuatro > rango2)
				{ //enviarnextion("PRESION MAYOR", "t26.txt=");
					if (presioncuatro - rango2 > 60)
					{
						pidvalvula = 1;
					}
					else pidvalvula = 500;

					if (valorvalvula >= 0 && valorvalvula < 4000) //valor original mayor es de 2000, ojo
					{
						valorvalvula = valorvalvula + precision;
						dac.setVoltage(valorvalvula, false);
					}
				}

				//-----------------fin regulacion de valvula-------------

			}

		}





	}

	else if (botononoff == 0)
	{
		dac.setVoltage(2000, false);//LA VALVULA SE ABRIRA EN EL INICIO DE LA PLANTA, DESPUES EMPIEZA LA REGULACION... (NUEVO)
		activador("contactorbaja", 1);
		activador("electrovalvula", 1);
		activador("contactoralta", 1);
		activador("clorador", 1);

		enviarnextion("PLANTA DETENIDA", "t26.txt=");
	}






}

void activador(String palabra, bool valor) {

	if (palabra == "contactoralta") {
		pcf8574out.digitalWrite(P0, valor);
		contactoralta = valor;
		return;
	}

	if (palabra == "electrovalvula") {
		pcf8574out.digitalWrite(P2, valor);//anterior P2
		electrovalvula = valor;
		return;
	}

	if (palabra == "contactorbaja") {
		pcf8574out.digitalWrite(P1, valor);
		contactorbaja = valor;
		return;
	}

	if (palabra == "clorador") {
		pcf8574out.digitalWrite(P5, valor);
		clorador = valor;
		return;
	}



}

void comunicacionrpi(void) {
	if (millis() - cronometrorpi > 5000)
	{
		cronometrorpi = millis();
		question["puerta"] = "que hora es";
		question.printTo(Serial);
	}

	//-----------------------codigo nuevo ----------------------------------------------
	tiemporespuesta = millis();
	do {

		if (Serial.available())
		{
			StaticJsonBuffer<600> jsonBafer;
			JsonObject& roots = jsonBafer.parseObject(Serial);
			if (!roots.success())
			{
				return;
			}

			//--------------------fin declaracion--------------------------------------------------

		}
		//-------------------------fin codigo nuevo------------------------------------------------

	} while (millis() - tiemporespuesta < 300);

}

void detectarcambioestadopines(void)
{   
	//rutina para detectar el cambio-----------------------------------BOTON ON OFF----
	if (estadoanteriorbotononoff != botononoff) {
		contadorbotononoff++;
		int validarpar = contadorbotononoff % 2;
		if (validarpar == 1)
		{
			iniciodeplanta = 1;
			cronometroinicio = millis();
		}
		estadoanteriorbotononoff = botononoff;
	}


	//-------------------------------------------------------------boton auxiliar------
	if (estadoanteriorbotonauxiliar != botonauxiliar) {
		contadorbotonauxiliar++;
		int validarpar = contadorbotonauxiliar % 2;
		if (validarpar == 1)
		{
			cronometrocambiofiltro = millis();
		}
		estadoanteriorbotonauxiliar = botonauxiliar;
	}
	else {
		if (millis() - cronometrocambiofiltro > 3000 && botonauxiliar == 1) //aqui detecto si presionaron el boton por mas de tres segundo
		{
			iniciocambiofiltro = 1;
		}
		else iniciocambiofiltro = 0;
	}


	//--------------------------------------------------------------------------------


	  //rutina para detectar el cambio y un tiempo en ese estado--------------------PRESOSTATO BAJA
	if (estadoanteriorpresostatobaja != presostatobaja) {
		contadorpresostatobaja++;
		int validarpar = contadorpresostatobaja % 2;
		if (validarpar != 1)
		{
			cronometropresostatobaja = millis();
		}

		estadoanteriorpresostatobaja = presostatobaja;


	}
	else {
		if (millis() - cronometropresostatobaja > 4000 && presostatobaja == 0) //si el presostato de baja se mantiene 4 segundos en bajo, recien en ese momento se cambia el estado.
		{
			estadopresostatobaja = 1;
			//detecto e intento 3 veces activar la planta
		}
		else estadopresostatobaja = 0;
	}
	//fin rutina para detectar el cambio y un tiempo en ese estado------------------------------------------------

	//--------------------------------------------------------------------------------PRESOSTATO ALTA
	if (estadoanteriorpresostatoalta != presostatoalta) {
		contadorpresostatoalta++;
		int validarpar = contadorpresostatoalta % 2;
		if (validarpar != 1)
		{
			cronometropresostatoalta = millis();
		}

		estadoanteriorpresostatoalta = presostatoalta;
	}
	else {
		if (millis() - cronometropresostatoalta > 4000 && presostatoalta == 0) //si el presostato de baja se mantiene 4 segundos en bajo, recien en ese momento se cambia el estado.
		{
			estadopresostatoalta = 1;
			//detecto e intento 3 veces activarla
		}
		else estadopresostatoalta = 0;
	}

	//-------------------------------------------------------------------      ----------


	//--------------------------------------------------------------------------------FILTRO ZEOLITA
	if (estadoanteriorfiltrozeolita != filtrozeolita) {
		contadorfiltrozeolita++;
		int validarpar = contadorfiltrozeolita % 2;
		if (validarpar == 1) //cuando se quiere detectar de cero a uno se pone "==1", cuando se quiere dectectar el cambio de uno a cero se pone"!=1"
		{
			cronometrofiltrozeolita = millis();
			cronometrozeolita = millis();
		}
		else
		{
			iniciodeplanta = 1; //esta instruccion se va a activar cuando el filtro de zeolita termine su lavado, y solo se activará una sola vez (por cada lavado)
			cronometroinicio = millis(); //despues del lavado de filtro de zeolita es importante y necesario volver a partir, por eso el contador de inicio de la planta vuelve a cero.
		    estadomemflush = 1;          //ESTA LINEA SE COMENTA PARA QUE NO INICIE EL LAVADO DE MEMBRANAS DESPUES DEL FILTRO DE ZEOLITA
		    cronometromemflush = millis(); //despues del lavado de zeolita viene el lavado de membranas //ESTA LINEA SE COMENTA PARA QUE NO INICIE EL LAVADO DE MEMBRANAS DESPUES DEL FILTRO DE ZEOLITA
		}
		estadoanteriorfiltrozeolita = filtrozeolita;
	}
	else {
		if (millis() - cronometrofiltrozeolita > 2000 && filtrozeolita == 1) //si el filtro de zeolita se mantiene X segundos en bajo, recien en ese momento se cambia el estado para evitar rebotes.
		{
			estadofiltrozeolita = 1;

		}
		else estadofiltrozeolita = 0;
	}

	//----------------------------------------------------------------------------------


	//--------------------------------------------------------------------------------NIVEL ALTO ESTANQUE--------
	if (estadoanteriornivelaltoestanque != nivelaltoestanque) {
		contadornivelaltoestanque++;
		int validarpar = contadornivelaltoestanque % 2;
		if (validarpar == 1) //cuando se quiere detectar de cero a uno se pone "==1", cuando se quiere dectectar el cambio de uno a cero se pone"!=1"
		{
			cronometronivelaltoestanque = millis();
		}

		else
		{
			iniciodeplanta = 1; //esta instruccion se va a activar cuando el filtro de zeolita termine su lavado, y solo se activará una sola vez (por cada lavado) 
			cronometroinicio = millis(); //despues del lavado de filtro de zeolita es importante y necesario volver a partir, por eso el contador de inicio de la planta vuelve a cero.
			cronometronivelaltoestanque = millis(); //despues del lavado de zeolita viene el lavado de membranas
		}

		estadoanteriornivelaltoestanque = nivelaltoestanque;
	}
	else {
		if (millis() - cronometronivelaltoestanque > 4000 && nivelaltoestanque == 1) //si
		{
			estadonivelaltoestanque = 1;
		}
		else if (millis() - cronometronivelaltoestanque > 4000 && nivelaltoestanque == 0)
		{
			estadonivelaltoestanque = 0;


		}


		//  else estadonivelaltoestanque=0; //quizas esto lo saque porque ya está en la condicional anterior
	}

	//----------------------------------------------------------------------------------












}

void enviarnextion(String frase, String casilla)
{
	//esta rutina envia informacion a la pantalla nextion cada 2.5 segundos, el tiempo puede variar, la idea es no saturar el puerto de comunicacion innecesariamente.
	if (millis() - cronometronextion > 2500)
	{
		Serial1.print(casilla);
		Serial1.print("\"");
		Serial1.print(frase);
		Serial1.print("\"");
		Serial1.write(0xff);
		Serial1.write(0xff);
		Serial1.write(0xff);
		cronometronextion = millis();
	}

}

void lecturasensores(void)
{
	presionuno = map(ads1.readADC_SingleEnded(0), 4681, 23300, 0, 50);
	presiondos = map(ads1.readADC_SingleEnded(1), 4681, 23300, 0, 50);
	presiontres = map(ads1.readADC_SingleEnded(2), 4681, 23300, 0, 50);
	presioncuatro = map(ads1.readADC_SingleEnded(3), 4681, 23300, 0, 1450); //1450 original //33600 original
   // presionuno    = ads1.readADC_SingleEnded(0);
   // presiondos    = ads1.readADC_SingleEnded(1);
   // presiontres   = ads1.readADC_SingleEnded(2);
	//presioncuatro = ads1.readADC_SingleEnded(3); //1450 original //33600 original


	presionestanque = map(ads2.readADC_SingleEnded(0), 4899, 23679, 0, 10000);
	temperaturaagua = map(ads2.readADC_SingleEnded(1), 7500, 9000, 10, 20);
	tds = map(ads2.readADC_SingleEnded(2), 6280, 17200, 0, 400);


	// presionestanque    = ads2.readADC_SingleEnded(0);
	// temperaturaagua    = ads2.readADC_SingleEnded(1);
	 //tds                = ads2.readADC_SingleEnded(2);

	sensorvibracion = map(ads3.readADC_SingleEnded(0), 4700, 23300, 0, 30); //ADS #37
	sensorvibracion2 = map(ads3.readADC_SingleEnded(1), 4700, 23300, 0, 30);
	posicionvalvula = map(ads3.readADC_SingleEnded(2), 4899, 23300, 0, 2000); //ADS #3 valor antiguo 17536


   // sensorvibracion    = ads3.readADC_SingleEnded(0); //ADS #37
   // sensorvibracion2    = ads3.readADC_SingleEnded(1);
	//posicionvalvula    = ads3.readADC_SingleEnded(2); //ADS #3 valor antiguo 17536

   // posicionvalvula    = map(ads3.readADC_SingleEnded(1),618,24391,0,2000); //ADS #3 valor antiguo 17536

	sensorcorriente = ads4.readADC_SingleEnded(0); //ADS #4
	sensorcorriente2 = ads4.readADC_SingleEnded(1); //ADS #4

	Wire.requestFrom(9, 1);
	caudalimetrouno = (Wire.read() / 10.0);

	// sensorcorriente    = map(ads3.readADC_SingleEnded(3),460,30000,0,30); //ADS #3

	sht.readSample();

	temperatura = sht.getTemperature();
	humedad = sht.getHumidity();


	puertatablero = !pcf8574in.digitalRead(P0); //la entradas de este conjunto son negadas porque el pcf8574 mantiene sus estados en pullup
	filtrozeolita = !pcf8574in.digitalRead(P1);
	nivelaltoestanque = !pcf8574in.digitalRead(P2);
	nivelbajoestanque = !pcf8574in.digitalRead(P3);
	presostatoalta = !pcf8574in.digitalRead(P4);
	presostatobaja = !pcf8574in.digitalRead(P5);
	botondeservicio = !pcf8574in.digitalRead(P6);
	botondepcb = !pcf8574in.digitalRead(P7);

	guardamotor1 = !pcf8574in2.digitalRead(P0);
	guardamotor2 = !pcf8574in2.digitalRead(P1);





	botononoff = digitalRead(3);
	botonauxiliar = digitalRead(2);
	botonautomatico = digitalRead(4);
	/*
	 reporte = " /presion 1: "+String(presionuno) +" /presion 2: "+String(presiondos)+
					  " /presion 3: "+String(presiontres)+" /presion 4: "+String(presioncuatro)+
					  " /presion estanque: "+String(presionestanque)+" /t° agua: "+String(temperaturaagua)+
					  " /tds: "+String(tds)+" /Sensor Vibracion: "+String(sensorvibracion)+
					  " /sensor corriente: "+String(sensorcorriente)+" /t° ambiente: "+String(temperatura)+
					  " /rh ambiente: "+String(humedad);

	*/
	data["presion1"] = presionuno;
	data["presion2"] = presiondos;
	data["presion3"] = presiontres;
	data["presion4"] = presioncuatro;
	data["estanque"] = presionestanque;
	data["tempagua"] = temperaturaagua;
	data["tds"] = tds;
	data["caudalimetrouno"] = caudalimetrouno;

	data["tempambiente"] = temperatura; //LO SAQUE PORQUE ME DABA ERROR EN EL JSON 
	data["humedad"] = humedad;

	data["puerta"] = puertatablero;
	data["zeolita"] = filtrozeolita;
	data["nivelalto"] = nivelaltoestanque;
	data["nivelbajo"] = nivelbajoestanque;
	data["presostatoalta"] = presostatoalta;
	data["presostatobaja"] = presostatobaja;

	data["onoff"] = botononoff;
	data["auxiliar"] = botonauxiliar;

	return;

}
void mostrarpantallas(void)
{//  Serial.println(reporte);
	if (millis() - cronometropantallas > 1500) //original 2000
	{
		cronometropantallas = millis();
		data.printTo(Serial);
		//  Serial.println(); //en la rpi puedo especificar que el mensaje empieza con "{" y termina co "}", asi evito el ponerle el println al final

		//------------------inicio imprimir en lcd nextion---------
		Serial1.print("n0.val=");  // Sensor presion uno
		Serial1.print(presionuno);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n1.val=");  // Sensor presion dos
		Serial1.print(presiondos);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n2.val=");  // Sensor presion tres
		Serial1.print(presiontres);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n3.val=");  // Sensor presion cuatro
		Serial1.print(presioncuatro);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n4.val=");  // Sensor presion estanque
		Serial1.print(presionestanque);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n5.val=");  //  temperatura del agua
		Serial1.print(temperaturaagua);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n6.val=");  // tds del agua
		Serial1.print(tds);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n7.val=");  // tds del agua
		Serial1.print(sensorvibracion);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n8.val=");  // tds del agua
		Serial1.print(sensorcorriente);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n9.val=");  // tds del agua
		//Serial1.print(posicionvalvula);  // ORIGINAL, VOLVER A SU ESTADO NORMAL
		Serial1.print(map(ads3.readADC_SingleEnded(2), 4899, 23300, 0, 100));
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n10.val=");  // temperatura ambiente
		Serial1.print(temperatura);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n11.val=");  // temperatura ambiente
		Serial1.print(humedad);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("t31.txt=");  // temperatura ambiente
		Serial1.print("\"");
		Serial1.print(caudalimetrouno);  // 
		Serial1.print("\"");
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n13.val=");  // temperatura ambiente
		Serial1.print(puertatablero);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n14.val=");  // temperatura ambiente
		Serial1.print(filtrozeolita);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n15.val=");  // temperatura ambiente
		Serial1.print(nivelaltoestanque);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n16.val=");  // temperatura ambiente
		Serial1.print(nivelbajoestanque);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n17.val=");  // temperatura ambiente
		Serial1.print(presostatoalta);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n18.val=");  // temperatura ambiente
		Serial1.print(presostatobaja);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n19.val=");  // temperatura ambiente
		Serial1.print(botononoff);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n20.val=");  // temperatura ambiente
		Serial1.print(botonauxiliar);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n21.val=");  // temperatura ambiente
		Serial1.print(contactoralta);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n22.val=");  // temperatura ambiente
		Serial1.print(contactorbaja);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n23.val=");  // temperatura ambiente
		Serial1.print(electrovalvula);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n24.val=");  // temperatura ambiente
		Serial1.print(clorador);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

		Serial1.print("n25.val=");  // temperatura ambiente
		Serial1.print(alarmavisual);  // 
		Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);


		display.clearDisplay();

		// text display tests
		display.setTextSize(0);
		display.setTextColor(WHITE);
		display.setCursor(0, 0);
		display.print("1:");
		display.println(presionuno);

		display.setCursor(0, 9);
		display.print("2:");
		display.println(presiondos);
		display.setCursor(0, 18);

		display.print("3:");
		display.println(presiontres);
		display.setCursor(0, 27);


		display.print("4:");
		display.println(presioncuatro);
		display.setCursor(0, 36);

		display.print("A:");
		display.println(presionestanque);
		display.setCursor(0, 45);

		display.print("B:");
		display.println(temperaturaagua);
		display.setCursor(0, 54);

		display.print("C:");
		display.println(tds);
		display.setCursor(0, 63);


		display.setCursor(44, 0);
		display.print("X:");
		display.println(sensorvibracion);

		display.setCursor(44, 9);
		display.print("Y:");
		display.println(sensorvibracion2);


		display.setCursor(44, 18);
		display.print("Z:");
		display.println(posicionvalvula);


		display.setCursor(44, 27);
		display.print("S:");
		display.println(sensorcorriente);


		display.setCursor(44, 36);
		display.print("T:");
		display.println(sensorcorriente2);

		display.setCursor(44, 45);

		display.print("I:");
		display.print(puertatablero);
		display.print(filtrozeolita);
		display.print(nivelaltoestanque);
		display.print(nivelbajoestanque);
		display.print(presostatobaja);
		display.print(presostatoalta);
		display.print(botondeservicio);
		display.print(botondepcb);
		display.print("'");
		display.print(guardamotor1);
		display.print(guardamotor2);


		display.setCursor(44, 54);
		display.print("O:");
		display.print(contactoralta);
		display.print(contactorbaja);
		display.print(electrovalvula);
		display.print(clorador);
		display.print(alarmavisual);
		display.print("'");
		display.print(botononoff);
		display.print(botonauxiliar);
		display.print(botonautomatico);



		display.setCursor(88, 0);
		display.print("T:");
		display.println(temperatura, 1);

		display.setCursor(88, 9);
		display.print("H:");
		display.println(humedad, 1);

		display.setCursor(88, 18);
		display.print("v:");
		display.println(valorvalvula, 1);

		display.setCursor(88, 27);
		display.print("F:");
		display.println(caudalimetrouno);



		// display.print(contadorpresostatobaja);
		display.display();

		//-------------------fin imprimir en lcd nextion-----------
	}
}
