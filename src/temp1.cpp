#include <Arduino.h>

#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <PubSubClient.h>
#include "SoftwareSerial.h"
#include <Servo.h>

/************************** Configurando pines para el Arduino MEGA ***************************/

/************************************** Servomotor **************************************/
Servo motor;  // Creamos un objeto de tipo Servo
int pinMotor = 6;
int motorNoventa = 90; 
int motorCientoOchenta = 180; 
int tiempoEspera = 10000;
/************************************** Servomotor **************************************/

/************************************** PIR **************************************/
int inputPin = 4;               // Escoger el pin de entrada (Para el sensor PIR)
int pirState = LOW;             // Iniciamos asumiendo que ningún movimiento es detectado
int val = 0;                    // Variable para leer el estatus del pin
/************************************** PIR **************************************/

//Conexión a la red wifi: nombre de la red y contraseña
#define WIFI_AP ""
#define WIFI_PASSWORD ""

//Nombre o IP del servidor mosquitto
char server[50] = "192.168.0.8";

//Inicializamos el objeto de cliente esp
WiFiEspClient espClient;

//Iniciamos el objeto subscriptor del cliente 
//con el objeto del cliente
PubSubClient client(espClient);

//Conexión serial para el esp con una comunicación
//serial, pines 2: rx y 3: tx
SoftwareSerial soft(2, 3);

//Contador para el envio de datos
unsigned long lastSend;

int status = WL_IDLE_STATUS;

void setup() {
    //Inicializamos la comunicación serial para el log
    Serial.begin(9600);
    //Iniciamos la conexión a la red WiFi
    InitWiFi();
    //Colocamos la referencia del servidor y el puerto
    client.setServer( server, 1883 );
    lastSend = 0;

    /******************************** PIR *************************************/
    pinMode(inputPin, INPUT);     // Declara el sensor como un input
    /******************************** PIR *************************************/
    /******************************** SERVO ************************************/
    motor.attach(pinMotor);     // El pin al que estará conectado
    motor.write(motorNoventa); // Pone el servomotor en los grados que le especificamos
    pinMode(pinMotor, OUTPUT); // Establecemos que el pin será de salida
    /******************************** SERVO ************************************/
}

void loop() {
    //Validamos si el modulo WiFi aun esta conectado a la red
    status = WiFi.status();
    if(status != WL_CONNECTED) {
        //Si falla la conexión, reconectamos el modulo
        reconnectWifi();
    }

    //Validamos si esta la conexión del servidor
    if(!client.connected() ) {
        //Si falla reintentamos la conexión
        reconnectClient();
    }

    //Creamos un contador para enviar la data cada 2 segundos
    if(millis() - lastSend > 2000 ) {
        sendDataTopic();
        lastSend = millis();
    }

    client.loop();
}

void sendDataTopic()
{
    Serial.println("sendDataTopic");
    // Prepare a JSON payload string
    String payload = "-1";
    // pirState = LOW

    val = digitalRead(inputPin);  // Leer el valor de entrada
    if (val == HIGH) {
        if (pirState == LOW) {
            motor.write(motorCientoOchenta);
            delay(3000);
            motor.write(motorNoventa);
            pirState = HIGH;
            payload = "1";
        }
    } else {
        if (pirState == HIGH){
            // Se acaba de ir el movimiento
            payload = "0";
            pirState = LOW;
        }
    }

    // Send payload
    char attributes[100];
    payload.toCharArray( attributes, 100 );
    client.publish( "casa/comedor", attributes );
    Serial.println( attributes );
}

//Inicializamos la conexión a la red wifi
void InitWiFi()
{
    //Inicializamos el puerto serial
    soft.begin(9600);
    //Iniciamos la conexión wifi
    WiFi.init(&soft);
    //Verificamos si se pudo realizar la conexión al wifi
    //si obtenemos un error, lo mostramos por log y denememos el programa
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("El modulo WiFi no esta presente");
        while (true);
    }
    reconnectWifi();
}

void reconnectWifi() {
    Serial.println("Iniciar conección a la red WIFI");
    while(status != WL_CONNECTED) {
        Serial.print("Intentando conectarse a WPA SSID: ");
        Serial.println(WIFI_AP);
        //Conectar a red WPA/WPA2
        status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
        delay(500);
    }
    Serial.println("Conectado a la red WIFI");
}

void reconnectClient() {
    //Creamos un loop en donde intentamos hacer la conexión
    while(!client.connected()) {
        Serial.print("Conectando a: ");
        Serial.println(server);
        //Creamos una nueva cadena de conexión para el servidor
        //e intentamos realizar la conexión nueva
        //si requiere usuario y contraseña la enviamos connect(clientId, username, password)
        String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
        if(client.connect(clientId.c_str())) {
            Serial.println("[DONE]");
        } else {
            Serial.print( "[FAILED] [ rc = " );
            Serial.print( client.state() );
            Serial.println( " : retrying in 5 seconds]" );
            delay( 5000 );
        }
    }
}