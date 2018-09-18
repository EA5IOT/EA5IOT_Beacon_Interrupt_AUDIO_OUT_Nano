/* EA5IOT - Beacon with Opera - Arduino Nano
    Copyright (C) 2018  EA5IOT

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <TimerOne.h>

#define Apagado LOW
#define Encendido HIGH

#define Punto 80                                                                      // Duración del punto, 80ms = 15 palabras por minuto
                                                                                      // 50 simbolos = 1 palabra, 750 simbolos = 15 palabras por minuto, 60 segundos / 750 = 80 ms
#define Raya Punto*3                                                                  // Duración de la raya en función del punto, una raya dura tres puntos
#define Retardo_Simbolo Punto                                                         // Espacio entre dos simbolos de una palabra, dura un punto
#define Retardo_Letra Punto*3                                                         // Espacio entre letras de una misma palabra, dura tres puntos                                                           
#define Retardo_Palabra Punto*7                                                       // Espacio entre palabras, dura siete puntos

int LED = 13;                                                                         // Este pin ya estaba implementado en el entorno Arduino -> D13

int PTT = 10;                                                                         // Pin para controlar las comunicacioens opticas y/o PTT
int AUDIO_OUT = 6;                                                                    // Pin de salida de audio (1500Hz)

char *Tabla_Letras[] = {"01", "1000", "1010", "100", "0", "0010", "110", "0000", "00", "0111", "101", "0100", "11",
                      // A       B       C      D     E      F      G       H      I     J       K       L      M       // CARACTER
                      // 65      66      67     68    69     70     71      72     73    74      75      76     77      // CODIGO ASCI

                        "10", "111", "0110", "1101", "010", "000", "1", "001", "0001", "011", "1001", "1011", "1100"};
                      // N      O      P       Q       R      S     T     U      V       W       X       Y       Z      // CARACTER
                      // 78     79     80      81      82     83    84    85     86      87      88      89      90     // CODIGO ASCI

char *Tabla_Numeros[] = {"11111", "01111", "00111", "00011", "00001", "00000", "10000", "11000", "11100", "11110"};
                      //    0        1        2        3        4        5        6        7        8        9          // CARACTER
                      //    48       49       50       51       52       53       54       55       56       57         // CODIGO ASCI

char *Tabla_Especiales[] = {"1111", "11011", "010101", "110011", "001100", "010010", "101011"};
                      //      CH       Ñ         .         ,        ?          "        !                               // CARACTER
                      //                         46        44       63         22       21                              // CODIGO ASCI

int Simbolo;                                                                          // Almacenará el retardo a aplicar para generar un punto o una raya
int Puntero;                                                                          // Almacenará el puntero de los simbolos Opera
long Retardo = 128000;                                                                // Retardo base para la transmisión de los 239 simbolos Opera
volatile bool Interrup;                                                               // Control de la activacion de la interrupcion Timer1
char* Codigo;                                                                         // Variable que contendrá los simbolos Opera

void setup()
{
  pinMode(LED, OUTPUT);                                                               // Set D13 as output
  digitalWrite(LED, Apagado);                                                         // Se apaga el LED

  pinMode(PTT, OUTPUT);                                                               // Set D10 as output
  pinMode(AUDIO_OUT, OUTPUT);                                                         // Set D6 as output

  Timer1.initialize(Retardo);
  Timer1.stop();
  Timer1.attachInterrupt(OpereaInt);                                                  // Se declara la rutina de interrupcion para el Timer1
  Interrup = false;
}

void loop()
{
  RF_Output_Enable(true);                                                             // Se activa la salida de RF
  delay(15000);                                                                       // Permanece transmitiendo 15 segundos
  RF_Output_Enable(false);                                                            // Se desactiva la transmisión
  delay(Retardo_Palabra);                                                             // Esperamos el retardo de una palabra para comenzar a transmitir
  Transmitir_Texto("VVV");                                                            // Se transmite en morse las letras VVV
  Transmitir_Texto("ED5YAF");                                                         // Se transmite en morse eL indicativo de la baliza
  Transmitir_Texto("IM98OL");                                                         // Se transmite en morse el locator de la baliza
  delay(2000);                                                                        // Pausamos 2 segundos
  Transmitir_Codigo_Opera("10110100110101010011001100101010101011001011001101010100110101001101010100110100110100110101001011001010110010110101010011001101001011010101001101010101010101010011010010110101010100110100110101010011010100110100110011010101001100110010101","05"); // Se transmite el indicativo en codificación Opera con velocidad 05 (239 bits)

  delay(8000);                                                                        // Pausamos 8 segundos
}

void Transmitir_Texto(char* texto)                                                    // Se transmite en morse el texto
{
  int i;
  char* codigo;                                                                       // Variable que contendrá el código morse del caracter del texto
  byte c;                                                                             // Variable que contendrá el valor ASCII del caracter del texto
  
  i = 0;

  while (texto[i] != '\0')                                                            // Recorremos todos los caracteres del texto
  {
    c = byte(texto[i]);                                                               // Se convierte a valor ASCII el caracter a transmitir
    if ((c >= 65) && (c <= 90))                                                       // ¿El caracter es una letra?
    {
      if ((c == 67) && (texto[i+1] == 'H'))                                           // ¿El caracter es el especial CH?
      {
        codigo = Tabla_Especiales[0];                                                 // Se ha detectado el caracter especial "CH"
        i++;                                                                          // Se incrementa en uno el puntero, ya que el siguiente caracter es una H y se envía a la vez que la C
      } else 
      {
        c -= 65;                                                                      // El caracter es una letra
        codigo = Tabla_Letras[c];                                                     // Se convierte el caracter del texto al código morse correspondiente
      };
    }
    else if ((c >= 48) && (c <= 57))                                                  // ¿El caracter es un número?
    {
      c -= 48;                                                                        // El caracter es un número
      codigo = Tabla_Numeros[c];                                                      // Se convierte el caracter del texto al código morse correspondiente
    }
    else                                                                              // Caracteres especiales menos el CH
    {
      if (c == 21) codigo = Tabla_Especiales[6];                                      // ¿El caracter es ! ?
      else if (c == 22) codigo = Tabla_Especiales[5];                                 // ¿El caracter es " ?
      else if (c == 63) codigo = Tabla_Especiales[4];                                 // ¿El caracter es ? ?
      else if (c == 44) codigo = Tabla_Especiales[3];                                 // ¿El caracter es , ?
      else if (c == 46) codigo = Tabla_Especiales[2];                                 // ¿El caracter es . ?
      else if (texto[i] == 'Ñ') codigo = Tabla_Especiales[1];                         // ¿El caracter es Ñ ?
      else codigo = "\0";                                                             // Si no es ningún código de las tablas no se transmite nada
    };

    Transmitir_Codigo_Morse(codigo);                                                  // Se transmite el codigo morse de la letra correspondiente
    
    i++;                                                                              // Se incrementa i para apuntar a la siguiente letra de la palabra    
    delay(Retardo_Letra);                                                             // Pausamos entre letras de una palabra
  };

  digitalWrite(LED, Encendido);                                                       //
  delay(Retardo_Palabra - Retardo_Letra);                                             // Pausamos entre palabras (Palabra - Letra, porque hemos aplicado el retardo de letra)
  digitalWrite(LED, Apagado);                                                         //
  
  return;
}

void Transmitir_Codigo_Morse(char* codigo)
{
  int ii;

  ii = 0;                                                                             // ii = 0 para que apunte al primer simbolo de la siguiente letra
  while (codigo[ii] != '\0')                                                          // Recorremos todos los simbolos morse del caracter del texto
  {
    if (codigo[ii] == '0') Simbolo = Punto;                                           // El simbolo a transmitir es un punto
    if (codigo[ii] == '1') Simbolo = Raya;                                            // El simbolo a transmitir es una raya
    ii++;
    
    RF_Output_Enable(true);                                                           // Se activa la salida de RF            
    delay(Simbolo);                                                                   // Mantenemos activa la salida de RF para generar un punto o una raya            
    RF_Output_Enable(false);                                                          // Desactivamos la salida de RF
    
    delay(Retardo_Simbolo);                                                           // Pausamos entre simbolos de una letra      
  };

  return;
}

void Transmitir_Codigo_Opera(char* codigo, char* velocidad)
{
  long retardo;

  retardo = Retardo;                                                                  // Se calcula el retardo de bit en microsegundos para velocidad 05

  if (velocidad == "1") retardo *= 2;                                                 // Se comprueba a que velocidad hay que transmitir el mensaje
  else if (velocidad == "2") retardo *= 4;                                            // Se comprueba a que velocidad hay que transmitir el mensaje
  else if (velocidad == "4") retardo *= 8;                                            // Se comprueba a que velocidad hay que transmitir el mensaje
  else if (velocidad == "8") retardo *= 16;                                           // Se comprueba a que velocidad hay que transmitir el mensaje
  else if (velocidad == "32") retardo *= 64;                                          // Se comprueba a que velocidad hay que transmitir el mensaje
  else if (velocidad == "65") retardo *= 128;
  else if (velocidad == "2H") retardo *= 256;

  Timer1.setPeriod(retardo);                                                          // Se reprograma el Timer1 con el retardo especificado

  Interrup = true;                                                                    // Se habilitan las interrupciones
  Puntero = 0;                                                                        // Se inicializa el puntero para transmitir los simbolos Opera
  Codigo = codigo;                                                                    // Se carga el código Opera a transmitir

  Timer1.start();
  while(Interrup) {};                                                                 // Esperamos a que se termine el envío de simbolos Opera
  Timer1.stop();

  RF_Output_Enable(false);                                                            // Se desactiva la salida de RF, por si el ultimo simbolo de opera ha sido un uno

  return;
}

void OpereaInt(void)
{
  bool activacion;

  if (Codigo[Puntero] != '\0')                                                        // Recorremos todos los simbolos del mensaje Opera
  {
    if (Codigo[Puntero] == '0') activacion = false;                                   // El simbolo a transmitir requiere apagar el transmisor
    if (Codigo[Puntero] == '1') activacion = true;                                    // El simbolo a transmitir requiere encender el transmisor
    Puntero++;

    RF_Output_Enable(activacion);                                                     // Se activa o desactiva la salida de RF       
  } else 
  {
    Interrup = false;                                                                 // Si se ha terminado de enviar los simbolos desactivamos las interrupciones                                      
  };

  return;
}

void RF_Output_Enable(bool enable)                                                    // Activamos o desactivamos la salida de RF por software
{
  if (!enable)
  {
    digitalWrite(LED, Apagado);
    noTone(AUDIO_OUT);                                                                // Se desactiva la salida de audio
    digitalWrite(PTT, LOW);                                                           // Se desactiva el led de potencia
  }
  else 
  {
    digitalWrite(LED, Encendido);
    tone(AUDIO_OUT, 1500);                                                            // Se activa la salida de audio
    digitalWrite(PTT, HIGH);                                                          // Se activa el led de potencia
  };

  return;
}

