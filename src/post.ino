#include <Arduino.h>


/*
  funções usadas para postar e gerenciar os posts
*/
void postar() {
  // Serial.println("Entrou no postar");
  if (filaPulsos.count() <= 0 || desconectado || cont>tentativas){
    cont=0;
    return;
  }
  HTTPClient http;
  //http.begin("http://domotica-node.herokuapp.com/api/log/");
  //http.begin("http://10.7.220.210:3000/api/log/hidrometro/");
  http.begin("http://api.saiot.ect.ufrn.br/api/log/hidrometro/");
  //http.begin("http://10.6.1.145/api/log/hidrometro/");
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(filaPulsos.peek());//Retorna o código http, caso não conecte irá retornar -1
  String payload = http.getString();
  // Serial.print(httpCode);
  // Serial.println(payload);
  http.end();
  HttpCode(httpCode);
}

void HttpCode(int httpCode) {
  if (httpCode == 200) {
    Serial.println(filaPulsos.count());
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
    //  pushDebug(3, msgError);
    }
  }
}

void pushDebug(int code_debug, String msg) {

  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
  //Serial.print("Entrou no Debug");
  String jsonDebug;
  sprintf(dateBuffer, "%04u-%02u-%02u", year(), month(), day());
  sprintf(horaBuffer, "%02u:%02u:%02u",  hour(), minute(), second());
  jsonDebug = "[{\"cod_erro\": " + String(code_debug) + " ,\"serial\": \"" + uuid_dispositivo + "\", \"mensagem\":" + "\"" + msg + "\"" + ", " + data + "\"" + String(dateBuffer) + " " +  String(horaBuffer) + "\"" + ", " + "\"ip\":" + "\"" + ipStr + "\"" + "}]";
  filaErros.push(jsonDebug);
  if(filaErros.count()>tamanhoFila){
  String mensagemdeErro = "tamanho da fila de Erros: " + String(filaErros.count());
  pushDebug(3, mensagemdeErro);
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
  http.begin("http://api.saiot.ect.ufrn.br/api/log/erro/");
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

String getHoraAtual() {
  HTTPClient http;
  http.begin("http://api.saiot.ect.ufrn.br/api/log/data-hora");
  //http.addHeader("Content-Type", "application/json");

  int httpCode = http.GET(); //Retorna o código http, caso não conecte irá retornar -1
  String payload = http.getString();

  // Serial.print(httpCode);
  Serial.println(payload);
  http.end();
  if (httpCode != 200) {
    return "0";
  }
  return payload;
}
