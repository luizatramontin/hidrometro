#include <Arduino.h>


void criaJson()
{
  bool cont_impar = 0;
  int valor;

  valor=contador;
  contador = 0;

  if (valor % 2 != 0) {
    cont_impar = true;
    valor--;
  }

   if (cont_impar) {
    contador ++;
  }
  char json[200];
  char *Jsaum = (char *)malloc(150);
  //Serial.println(contador);
  sprintf(Jsaum, "[");
  sprintf(json, "{\"serial\":\"%s\", \"pulso\":%d,", uuid_dispositivo, valor/2);
  strcat(json, data);
  strcat(json, "\"");
  sprintf(dateBuffer, "%04u-%02u-%02u", year(), month(), day());
  strcat(json, dateBuffer);
  sprintf(horaBuffer, "%02u:%02u:%02u", hour(), minute(), second());
  strcat(json, aspas);
  strcat(json, horaBuffer);
  strcat(json, "\"");
  strcat(json, col);
  strcat(Jsaum, json);
  strcat(Jsaum, termino);
  strcat(Jsaum, fecha);
  filaPulsos.push(Jsaum);
  if(filaPulsos.count()>tamanhoFila){
  String mensagemError = "tamanho da fila de pulsos: " + String(filaPulsos.count());
  pushDebug(6, mensagemError);
  }
  Serial.print(Jsaum);
  Serial.println(filaPulsos.count());

}
