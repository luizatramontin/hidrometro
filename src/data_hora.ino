#include <Arduino.h>
String getHoraAtual() {
  HTTPClient http;
  http.begin(GETDATAHORA);
  //http.addHeader("Content-Type", "application/json");
  int httpCode = http.GET(); //Retorna o código http, caso não conecte irá retornar -1
  String payload = http.getString();
  Serial.println("getHoraAtual: ");
  Serial.println(payload);
  http.end();
  if (httpCode != 200) {
    return "0";
  }
  return payload;
}
bool sincronizarHora() {
  int ano , mes, dia, h, m, s;
  String dataAtual = getHoraAtual();
  Serial.println("sincronizarHora: ");
  Serial.println(dataAtual);
  if (dataAtual == "0"){
    return false;
  }
  ano = dataAtual.substring(1, 5).toInt();
  mes = dataAtual.substring(6, 8).toInt();
  dia = dataAtual.substring(9, 11).toInt();
  h   = dataAtual.substring(12, 14).toInt();
  m   = dataAtual.substring(15, 17).toInt();
  s   = dataAtual.substring(18, 20).toInt();
  setTime(h, m, s, dia, mes, ano);
  return true;
}
