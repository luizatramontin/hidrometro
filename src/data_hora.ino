#include <Arduino.h>
String getHoraAtual() {
  HTTPClient http;
  http.begin("http://api.saiot.ect.ufrn.br/api/log/data-hora");
  //http.addHeader("Content-Type", "application/json");
  int httpCode = http.GET(); //Retorna o código http, caso não conecte irá retornar -1
  String payload = http.getString();
  Serial.println(payload);
  http.end();
  if (httpCode != 200) {
    return "0";
  }
  return payload;
}
void sincronizarHora() {
  String dataAtual = getHoraAtual();
  Serial.println(dataAtual);
  int ano = dataAtual.substring(0, 4).toInt();
  int mes = dataAtual.substring(5, 7).toInt();
  int dia = dataAtual.substring(8, 10).toInt();
  int h   = dataAtual.substring(11, 13).toInt();
  int m   = dataAtual.substring(14, 16).toInt();
  int s   = dataAtual.substring(17, 19).toInt();
  if ( (ano <= year()) && (mes <= month()) && (dia <= day()) && (h <= hour()) && (m < minute()) ){
    setTime(h, m, s, dia, mes, ano);
  }
}
void sincronizarHoraSetup() {
  int ano = 0;
  int mes = 0;
  int dia = 0;
  int h   = 0;
  int m   = 0;
  int s   = 0;
  do {
    String dataAtual = getHoraAtual();
    Serial.println(dataAtual);
    ano = dataAtual.substring(1, 5).toInt();
    mes = dataAtual.substring(6, 8).toInt();
    dia = dataAtual.substring(9, 11).toInt();
    h   = dataAtual.substring(12, 14).toInt();
    m   = dataAtual.substring(15, 17).toInt();
    s   = dataAtual.substring(18, 20).toInt();
  }
  while ((ano <= year()) && (mes <= month()) && (dia <= day()) && (h <= hour()) && (m < minute()));
  setTime(h, m, s, dia, mes, ano);
}
