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

class dados_pulsos{
public:
  int pulsos;
  char  data_hora[26];
  dados_pulsos(int p, char dados[]);
};
dados_pulsos::dados_pulsos(int p, char dados[]){
  pulsos = p;
  strcpy(data_hora,dados);
}


QueueList<dados_pulsos*> filaPulsos;
QueueList<String> filaErros;
QueueList<String>filaErroConexao;

#define uuid_dispositivo "salarobotica_OTA"
#define termino "\0"
#define data "\"data_hora\":"
#define aspas " "
#define col "}"
#define fecha   "]"
#define tempjson 1// tempo em segundos
#define temppost 20// tempo em segundos //com 80 carac cabem 231 jsons na fila
#define temppostdebug 150
#define tentativas 3
#define tamanhoFila 100// 1h para postar os erros


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

char dateBuffer[26];
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
bool filaPulsoCheia=false;
bool filaDebugCheia=false;

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


  /*
  configuração do WiFiManager
  */
  WiFiManager wifiManager;
  wifiManager.autoConnect(uuid_dispositivo);

  Serial.println("conectado:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  desconectado = false;

  Serial.println(WiFi.localIP());

  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
  //Serial.println("Imprimindo IP: " + ipStr);
  sincronizarHoraSetup();
  pushDebug(1, "Reiniciando");

  setupOTA(8266, uuid_dispositivo);// função para o OTA (porta,nome_dispositvo)
  ArduinoOTA.begin();
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
if (flag_criar_json)
{
  flag_criar_json = false;
  push_dados();
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
if(filaPulsos.count()==0){
  filaPulsoCheia=false;
}
if(filaErros.count()==0){
  filaDebugCheia=false;
}
}
