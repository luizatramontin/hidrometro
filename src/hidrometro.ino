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


//CONSTANTE A SEREM AJUSTADAS PARA CADA GRAVACAO
//********************************************************************
#define uuid_dispositivo "hidrometro_ect"  //"PATRICIO0002"
#define tempjson 60*10 //5 // tempo em segundos em que um json eh criado e colocado n fila
#define temppost 3*tempjson // tempo em segundos em que sao enviados os jsons na fila (com 80 carac cabem 231 jsons na fila)
#define temppostdebug 150 //O QUE EH ISSO?
#define tentativas 3 //numero de tentativas para envio de jsons antes de desistir
#define tamanhoFila 100// 1h para postar os erros
#define tempatualizahora 10*60*6*6 //tempo para atualzar a datahora via servidor
//*********************************************************************

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

#define termino "\0"
#define data "\"data_hora\":"
#define aspas " "
#define col "}"
#define fecha   "]"

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
/*
sSSs    sSSs  sdSS_SSSSSSbs   .S       S.    .S_sSSs
d%%SP   d%%SP  YSSS~S%SSSSSP  .SS       SS.  .SS~YS%%b
d%S'    d%S'         S%S       S%S       S%S  S%S   `S%b
S%|     S%S          S%S       S%S       S%S  S%S    S%S
S&S     S&S          S&S       S&S       S&S  S%S    d*S
Y&Ss    S&S_Ss       S&S       S&S       S&S  S&S   .S*S
`S&&S   S&S~SP       S&S       S&S       S&S  S&S_sdSSS
`S*S  S&S          S&S       S&S       S&S  S&S~YSSY
l*S  S*b          S*S       S*b       d*S  S*S
.S*P  S*S.         S*S       S*S.     .S*S  S*S
sSS*S    SSSbs       S*S        SSSbs_sdSSS   S*S
YSS'      YSSP       S*S         YSSP~YSSY    S*S
SP                       SP
Y                        Y

*/
ESP8266WebServer server(80);
void setup()
{

  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(0, hidro_leitura, CHANGE);//porta D8 do esp
  Serial.begin(115200);

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
  do{
    sincroHora = sincronizarHora();
  } while (!sincroHora);
  pushDebug(1, "Reiniciando");
  setupOTA(8266, uuid_dispositivo);// função para o OTA (porta,nome_dispositvo)
  ArduinoOTA.begin();

  // Atribuindo urls para funções
  server.on("/hora", HORAESP);
  // Iniciando servidor
  server.begin();

  erros_postar.attach(temppostdebug, actvate_post_debug);
  seta_hora.attach(tempatualizahora, actvate_seta_hora);

}

/*
 __        ______     ______   .______
|  |      /  __  \   /  __  \  |   _  \
|  |     |  |  |  | |  |  |  | |  |_)  |
|  |     |  |  |  | |  |  |  | |   ___/
|  `----.|  `--'  | |  `--'  | |  |
|_______| \______/   \______/  | _|

*/
void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  if (setInterrupt) {
    t_criar.attach(tempjson, actvate_flag_criar_json);
    delay(200);
    t_postar.attach(temppost, actvate_post_it);
    setInterrupt = false;
  }
  if (!sincroHora) {
    sincroHora = sincronizarHora();
  }
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
