#include <Arduino.h>


void hidro_leitura() {

  // if (abs(micros() - last_micros) >= debouncing_time && state ^ digitalRead(D3) ) {
  if (state ^ digitalRead(D3)) {
    if (abs(micros() - last_micros) >= debouncing_time ) {
      state = !state;
      contador++;
      Serial.print("Contador");
      Serial.println(contador);
      last_micros = micros();
    }
    else {
      Serial.println("Filtro de tempo");
    }
  }
  else {
    Serial.println("Filtro de interferencia");
  }


}


void actvate_flag_criar_json()
{
  flag_criar_json = true;
}

void actvate_post_it()
{
  //  post_it = true;
  if (WiFi.status() == WL_CONNECTED) {
    enchendoFilaPulsos = false;
    if (desconectado) {
      desconectado = false;
      pushDebug(4, "Reconectado" );
    }
  } else {
    if (desconectado == false) {
      pushDebug(5, "Desconectado" );
      desconectado = true;
    }
  }

}
void actvate_post_debug()
{
  if (WiFi.status() == WL_CONNECTED) {
    enchendoFilaDebug = false;
    if (desconectado) {
      desconectado = false;
      pushDebug(4, "Reconectado" );
    }
  } else {
    if (desconectado == false) {
      pushDebug(5, "Desconectado" );
      desconectado = true;
    }
  }
}
void actvate_seta_hora() {
  sincroHora = true;
}

