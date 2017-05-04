void sincronizarHora() {
  while (year() == 1970) {
    String dataAtual = getHoraAtual();
    int ano = dataAtual.substring(0, 4).toInt();
    int mes = dataAtual.substring(5, 7).toInt();
    int dia = dataAtual.substring(8, 10).toInt();
    int hora = dataAtual.substring(11, 13).toInt();
    int minu = dataAtual.substring(14, 16).toInt();
    int seg = dataAtual.substring(17, 19).toInt();
    sethoraatual(hora, minu, seg, dia, mes, ano);
  }
}
void sethoraatual(int h, int m, int s, int d, int mes, int a)
{
  setTime(h, m, s, d, mes, a);
}
