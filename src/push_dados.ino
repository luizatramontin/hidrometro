
#include <Arduino.h>


void push_dados ()
{

  bool cont_impar = 0;
  int valor = 0;
  valor = contador - ultimo_contador;
  ultimo_contador = contador;


  if (valor % 2 != 0) {
    valor--;
    ultimo_contador--;
  }
  sprintf(dateBuffer, "%04u-%02u-%02u %02u:%02u:%02u", year(), month(), day(), hour(), minute(), second());
  dados_pulsos* d = new dados_pulsos(valor/2,dateBuffer);


  filaPulsos.push(d);
//  Serial.println(String(system_get_free_heap_size()));
  if(filaPulsos.count()>tamanhoFila && !filaPulsoCheia){
  filaPulsoCheia=true;
  String mensagemError = "tamanho da fila de pulsos: " + String(filaPulsos.count());
  pushDebug(6, mensagemError);
  }
  Serial.print("Pulsos: ");
  Serial.println(d->pulsos);
  Serial.print("Data_hora: ");
  Serial.println(d->data_hora);
  Serial.println(filaPulsos.count());

}
