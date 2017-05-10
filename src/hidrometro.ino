/*
   firmware correto mesmo sim agora vai e foi mesmo para o hidrometro da ect que vai ficar mesmo de verdade... Obrigado Senhor!!!
*/
#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
#include <Ticker.h>
#include <QueueList.h>
#include <stdlib.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>// comunica com o dns (ArduinoOTA)
#include <DNSServer.h> // habilita servidor dns no esp (WiFiManager)
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

QueueList<char*> filaPulsos;
QueueList<String> filaErros;
QueueList<String>filaErroConexao;

#define uuid_dispositivo "PATRICIO0001"
#define termino "\0"
#define data "\"data_hora\":"
#define aspas " "
#define col "}"
#define fecha   "]"
/*
#define tempjson 1// tempo em segundos
#define temppost 3// tempo em segundos //com 80 carac cabem 231 jsons na fila
#define temppostdebug 5 // 1h para postar os erros

*/
#define tempjson 1// tempo em segundos
#define temppost 3// tempo em segundos //com 80 carac cabem 231 jsons na fila
#define temppostdebug 5 // 1h para postar os erros

Ticker t_criar;
Ticker t_postar;
Ticker erros_postar;
Ticker seta_hora;

//para contagem de pulsos
volatile unsigned int contador = 0;
volatile unsigned long last_micros;
//filtro
boolean state = true; //false p baixo -> pulso
float debouncing_time = 100000;//equivale 100 milisegundos(esse tempo é em micro)
int cont=0;

char dateBuffer[12];
char horaBuffer[12];

//flag
volatile bool post_it = false;
volatile bool flag_criar_json = false;
volatile bool flag_n_criou = false;
volatile bool flag_n_postou = false;
bool setInterrupt = true;
bool desconectado;
bool enchendoFilaPulsos = true;
bool enchendoFilaDebug = true;
bool sincroHora = false;

//ip to string
String ipStr;
String statusesp;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(0, hidro_leitura, CHANGE);//porta D8 do esp
  Serial.begin(115200);

  erros_postar.attach(temppostdebug, actvate_post_debug);
  //seta_hora.attach(60 * 5 , actvate_seta_hora);

  WiFiManager wifiManager;
  wifiManager.autoConnect(uuid_dispositivo);

  Serial.println("conectado:");
  Serial.println(WiFi.SSID());

  desconectado = false;

  Serial.println(WiFi.localIP());

  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
  //Serial.println("Imprimindo IP: " + ipStr);
  sincronizarHora();
  pushDebug(1, "Reiniciando");
  /*
      BEGIN OTA
  */
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname( uuid_dispositivo );
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  /*
     END OTA OK
  */
}

void loop() {
  ArduinoOTA.handle();
  if (setInterrupt) {
    t_criar.attach(tempjson, actvate_flag_criar_json);
    delay(200);
    t_postar.attach(temppost, actvate_post_it);
    setInterrupt = false;
  }
  if (sincroHora) {
    sincroHora = false;
    sincronizarHora();
  }
  /*if (WiFi.status() != WL_CONNECTED && desconectado == false) {
    desconectado = true;
    pushDebug(4, "Desconectado" );
    }
    if (WiFi.status() == WL_CONNECTED &&  desconectado == true) {
    desconectado = false;
    pushDebug(4, "Conectado" );
    }*/

  digitalWrite(LED_BUILTIN, state);
  //  if (post_it)
  //  {
  //    postar();
  //    post_it = false;
  //  }
  if (flag_criar_json)
  {
    flag_criar_json = false;
    criaJson();
  }
  if (!enchendoFilaPulsos)
  {
    if (filaPulsos.count() > 0) {
      postar();
    }
    else {
      enchendoFilaPulsos = true;
    }
  }
  if (!enchendoFilaDebug)
  {
    if (filaErros.count() > 0) {
      postDebug();
    }
    else {
      enchendoFilaDebug = true;
    }
  }
}
