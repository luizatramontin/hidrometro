void sincronizarHora() {
  while (year() == 1970) {
    String dataAtual = getHoraAtual();
    int ano = dataAtual.substring(0, 4).toInt();
    int mes = dataAtual.substring(5, 7).toInt();
    int dia = dataAtual.substring(8, 10).toInt();
    int h   = dataAtual.substring(11, 13).toInt();
    int m   = dataAtual.substring(14, 16).toInt();
    int s   = dataAtual.substring(17, 19).toInt();
    setTime(h, m, s, dia, mes, ano);
  } 
}
