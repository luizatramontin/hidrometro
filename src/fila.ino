#include <Arduino.h>

/*
 * acho que aqui vcaberia a função 
 */
void tiraFila() {
 // Serial.println("Entrou no tirarFila");
  //Serial.println(f.pop());
  free(filaPulsos.pop());
}
