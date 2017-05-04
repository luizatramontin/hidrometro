
#include <Time.h>
#include <TimeLib.h>
#include <Ticker.h>
#include<QueueList.h>
#include<stdlib.h>
#include<ESP8266HTTPClient.h>
#include<ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>


#include <Arduino.h>


QueueList<char*> filaPulsos;
QueueList<String> filaErros;
QueueList<String>filaErroConexao;

#define termino "\0"
#define data "\"data_hora\":"
#define aspas " "
#define col "}"
#define fecha   "]"
#define tempjson 60// tempo em segundos 
#define temppost 60*3// tempo em segundos //com 80 carac cabem 231 jsons na fila

//Timer t;
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

//para tempo
int ano, mes, dia, hora, minuto, seg;


/*
  char* ssid = "Robotica-IMD";
  char* password = "roboticawifi";
*/
char dateBuffer[12];
char horaBuffer[12];
//char uuid_dispositivo[38] = "3552df5ba4814cf6990aa521f1720f3a";
char uuid_dispositivo[100] = "salarobotica";

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


  erros_postar.attach(60 * 60, actvate_post_debug);
  //seta_hora.attach(60 * 5 , actvate_seta_hora);


  WiFiManager wifiManager;
  wifiManager.autoConnect("hidrometro", "senha1234");
  /*
    WiFi.begin(ssid, password);//
    Serial.println("conectando");
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
  */
  Serial.print("conectado");
  desconectado = false;

  Serial.println(WiFi.localIP());

  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
  //Serial.println("Imprimindo IP: " + ipStr);
  sincronizarHora();
  pushDebug(1, "Reiniciando");


}

void loop() {

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

