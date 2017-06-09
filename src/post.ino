#include <Arduino.h>


/*
funções usadas para postar e gerenciar os posts
*/
void postar() {
  // Serial.println("Entrou no postar");
  char json[150];
  char big_json[200];
  if (filaPulsos.count() <= 0 || desconectado || cont>tentativas){
    cont=0;
    return;
  }
  dados_pulsos* d = filaPulsos.peek();

  sprintf(big_json, "[");
  sprintf(json, "{\"serial\":\"%s\", \"pulso\":%d,", uuid_dispositivo, d->pulsos);
  strcat(json, data);
  strcat(json, "\"");
  strcat(json, d->data_hora);
  strcat(json, aspas);
  strcat(json, "\"");
  strcat(json, col);
  strcat(big_json, json);
  strcat(big_json, termino);
  strcat(big_json, fecha);
  Serial.println(big_json);
  HTTPClient http;
  http.begin(LOGCONTAGEM);

  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(big_json);//Retorna o código http, caso não conecte irá retornar -1
  String payload = http.getString();
  // Serial.print(httpCode);
  // Serial.println(payload);
  http.end();
  HttpCode(httpCode);
}

void HttpCode(int httpCode) {
  dados_pulsos* d = filaPulsos.peek();
  if (httpCode == 200) {
    Serial.println(filaPulsos.count());
    contador = contador -  d->pulsos * 2 ;
    ultimo_contador = ultimo_contador -  d->pulsos * 2 ;
    tiraFila();
    //postar();
    cont=0;
  }
  else {
    String msgErro = "HTTP_CODE: " + String(httpCode);
    pushDebug(3, msgErro);
    cont++;
    if(cont>tentativas){
      enchendoFilaPulsos = true;
      enchendoFilaDebug = true;
      //  msgError = "Excedeu a quantidade maxima de tentativas";
      //  pushDebug(3, msgErro);
    }
  }
}

void pushDebug(int code_debug, String msg) {
  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);
  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
  //Serial.print("Entrou no Debug");
  String jsonDebug;
  sprintf(dateBuffer, "%04u-%02u-%02u", year(), month(), day());
  sprintf(horaBuffer, "%02u:%02u:%02u",  hour(), minute(), second());
  jsonDebug = "[{\"cod_erro\": " + String(code_debug) + " ,\"serial\": \"" + uuid_dispositivo + "\", \"mensagem\":" + "\"" + msg + "\"" + ", " + data + "\"" + String(dateBuffer) + " " +  String(horaBuffer) + "\"" + ", " + "\"ip\":" + "\"" + ipStr + "\"" +  ", " + "\"sinal_wifi\":" + rssi + "}]";
  filaErros.push(jsonDebug);
  if(filaErros.count()>tamanhoFila && !filaDebugCheia){
    filaDebugCheia = true;
    String mensagemdeErro = "tamanho da fila de Erros: " + String(filaErros.count());
    pushDebug(6, mensagemdeErro);
  }
  Serial.println(jsonDebug);
  Serial.print(filaErros.count());
  //  Serial.print(" - ");
  //  Serial.println(filaErros.pop());
  postDebug();
}

void postDebug() {
  if (filaErros.count() <= 0 || desconectado || cont>tentativas){
    cont=0;
    return;
  }

  HTTPClient http;
  http.begin(LOGERRO);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(filaErros.peek());//Retorna o código http, caso não conecte irá retornar -1
  String payload = http.getString();
  // Serial.print(httpCode);
  // Serial.println(payload);
  http.end();
  if (httpCode == 200) {
    filaErros.pop();
  }
}
