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
#include <EEPROM.h>


//CONSTANTE A SEREM AJUSTADAS PARA CADA GRAVACAO
//********************************************************************
#define uuid_dispositivo "salarobotica_OTA"
#define tempjson 10*60//10min  tempo em segundos em que um json eh criado e colocado n fila
#define temppost 3*10*60 // 30min tempo em segundos em que sao enviados os jsons na fila (com 80 carac cabem 231 jsons na fila)
#define temppostdebug 100 // tempo em segundos em que sao enviados os jsons com os erros
#define temp_push_flash 5*60 // inserir o valor do contador na flash a cada X minutos
#define tentativas 3 //numero de tentativas para envio de jsons antes de desistir
#define tamanhoFila 250// 1h para postar os erros  ???????????????????????????????
#define tempatualizahora 6*60*60 //6h tempo para atualzar a datahora via servidor
#define sinchora true //usada para debugar a API se false desabilita todas as funções de ajustar hora automaticamente.
// ip do raspberry do lab para teste de segurança
// para usar o servidor ect usar o endereco a baixo no lugar dos numeros
// http://api.saiot.ect.ufrn.br
//https://10.7.226.85:81
#define GETDATAHORA "http://api.saiot.ect.ufrn.br/api/log/data-hora"
#define LOGCONTAGEM "http://api.saiot.ect.ufrn.br/api/log/hidrometro/"
#define LOGERRO "http://api.saiot.ect.ufrn.br/api/log/erro/"
//*********************************************************************
#define termino "\0"
#define data "\"data_hora\":"
#define aspas " "
#define col "}"
#define fecha "]"

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

//INTERRUPÇÕES
Ticker t_criar;
Ticker t_postar;
Ticker erros_postar;
Ticker push_flash;
Ticker seta_hora;

//para contagem de pulsos
volatile unsigned int contador = 0;
volatile unsigned long last_micros;
volatile unsigned int ultimo_contador = 0; //usado para gravar na memoria

//variaveis para uso da EEPROM
int addr = 0;

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

ESP8266WebServer server(80);
//ip to string
String ipStr;
String statusesp;
/*
                                              tttt
                                           ttt:::t
                                           t:::::t
                                           t:::::t
    ssssssssss       eeeeeeeeeeee    ttttttt:::::ttttttt    uuuuuu    uuuuuu ppppp   ppppppppp
  ss::::::::::s    ee::::::::::::ee  t:::::::::::::::::t    u::::u    u::::u p::::ppp:::::::::p
ss:::::::::::::s  e::::::eeeee:::::eet:::::::::::::::::t    u::::u    u::::u p:::::::::::::::::p
s::::::ssss:::::se::::::e     e:::::etttttt:::::::tttttt    u::::u    u::::u pp::::::ppppp::::::p
 s:::::s  ssssss e:::::::eeeee::::::e      t:::::t          u::::u    u::::u  p:::::p     p:::::p
   s::::::s      e:::::::::::::::::e       t:::::t          u::::u    u::::u  p:::::p     p:::::p
      s::::::s   e::::::eeeeeeeeeee        t:::::t          u::::u    u::::u  p:::::p     p:::::p
ssssss   s:::::s e:::::::e                 t:::::t    ttttttu:::::uuuu:::::u  p:::::p    p::::::p
s:::::ssss::::::se::::::::e                t::::::tttt:::::tu:::::::::::::::uup:::::ppppp:::::::p
s::::::::::::::s  e::::::::eeeeeeee        tt::::::::::::::t u:::::::::::::::up::::::::::::::::p
 s:::::::::::ss    ee:::::::::::::e          tt:::::::::::tt  uu::::::::uu:::up::::::::::::::pp
  sssssssssss        eeeeeeeeeeeeee            ttttttttttt      uuuuuuuu  uuuup::::::pppppppp
                                                                              p:::::p
                                                                              p:::::p
                                                                             p:::::::p
                                                                             p:::::::p
                                                                             p:::::::p
                                                                             ppppppppp*/
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D5,INPUT_PULLUP); //usado para gravação na memoria

  //interrupcao externa do contador de pulsos
  attachInterrupt(D3, hidro_leitura, CHANGE);

  //interrupcao por tempo para gravar na flash
  push_flash.attach(temp_push_flash, actvate_push_flash);

  Serial.begin(115200);

  //usado para gravação na memoria
  EEPROM.begin(10);
  if(digitalRead(D5) == LOW){
    EEPROM.put(addr,ultimo_contador);
    EEPROM.commit();
  }
  EEPROM.get(addr,contador);
  Serial.print("Valor inicial: ");
  Serial.println(contador);

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

  if (sinchora) {
    do{
      sincroHora = sincronizarHora();
      delay(10);//tempo necessário para aguardar o fechamento da conexão
    } while (!sincroHora);
  }
  pushDebug(1, "Reiniciando");
  setupOTA(8266, uuid_dispositivo);// função para o OTA (porta,nome_dispositvo)
  ArduinoOTA.begin();

  //CONFIGURAÇÃO DO SERVIDOR
  //********************************************************************
  // Atribuindo urls para funções
  server.on("/data-hora", dataHora);
  server.on("/data-hora/sinc", dataHoraSinc);
  server.on("/estado", estado);
  server.on("/reinicio", reinicio);
  server.on("/configuracao", configuracao);
  // Iniciando servidor
  server.begin();
  //********************************************************************

  erros_postar.attach(temppostdebug, actvate_post_debug);
  seta_hora.attach(tempatualizahora, actvate_seta_hora);


}

/*
lllllll
l:::::l
l:::::l
l:::::l
 l::::l    ooooooooooo      ooooooooooo   ppppp   ppppppppp
 l::::l  oo:::::::::::oo  oo:::::::::::oo p::::ppp:::::::::p
 l::::l o:::::::::::::::oo:::::::::::::::op:::::::::::::::::p
 l::::l o:::::ooooo:::::oo:::::ooooo:::::opp::::::ppppp::::::p
 l::::l o::::o     o::::oo::::o     o::::o p:::::p     p:::::p
 l::::l o::::o     o::::oo::::o     o::::o p:::::p     p:::::p
 l::::l o::::o     o::::oo::::o     o::::o p:::::p     p:::::p
 l::::l o::::o     o::::oo::::o     o::::o p:::::p    p::::::p
l::::::lo:::::ooooo:::::oo:::::ooooo:::::o p:::::ppppp:::::::p
l::::::lo:::::::::::::::oo:::::::::::::::o p::::::::::::::::p
l::::::l oo:::::::::::oo  oo:::::::::::oo  p::::::::::::::pp
llllllll   ooooooooooo      ooooooooooo    p::::::pppppppp
                                           p:::::p
                                           p:::::p
                                          p:::::::p
                                          p:::::::p
                                          p:::::::p
                                          ppppppppp*/
void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  if (setInterrupt) {
    t_criar.attach(tempjson, actvate_flag_criar_json);
    delay(200);
    t_postar.attach(temppost, actvate_post_it);
    setInterrupt = false;
  }
  if (sinchora ) {
    if (!sincroHora) {
      sincroHora = true;
      sincronizarHora();
    }
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
