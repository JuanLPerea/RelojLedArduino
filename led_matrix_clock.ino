#include <DHT.h>
#include <EEPROM.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <Fonts/TomThumb.h>

#define countof(a) (sizeof(a) / sizeof(a[0]))
#define DHTTYPE DHT11   // DHT 11

// Sensor de temperatura y humedad DHT11
const int DHTPin = A4;     // what digital pin we're connected to
DHT dht(DHTPin, DHTTYPE);


// Alimentar el módulo bluetooth a través del pin 12
// Poner a LOW si no estamos conectados y ha pasado un tiempo sin tocar ningún botón
const int bluetoothPIN = 9;

//Vcc - Vcc
//Gnd - Gnd
//Din - Mosi (Pin 11)
//Cs  - SS (Pin 10)
//Clk - Sck (Pin 13)

// Variables para la matriz roja y verde
const int NUM_ROWS = 8;
const int NUM_COLS = 8;

const int dataPin = 5;      // Row data line
const int clockPin = 4;     // Clock sequence
const int latchPin = 3;    // Latch Pin

// pin para controlar el Mosfet que activa la alarma
const int alarmaPin = 2;

// Variables para el control general del reloj
RtcDateTime dt;
float temporizadorAnimRYV = 0;
float temporizadorScroll = 0;
int posicionScroll = 0;
bool scrollOn;
int rowRYV = 0;
int val = 0;
int contraste = 0;
int apagarSeg = 0;
float tiempoApagar = 0;
int modoReloj = 1;
bool repetirMensaje = false;
bool activarAlarma = false;
String alarma = "";
bool tocarSonidoHoras = true;
float tiempoAlarma = 0;
float temporizadorAlarma = 0;
float temporizadorLeerTemperatura = 0;
float temporizadorDatosBluetooth = 0;
float temporizadorApagarBluetooth = 0;
float temporizadorAleatorio = 0;
bool estadoCampana = false;
bool bluetoothEncendido = true;
int modoAleatorioActual = 1;

int gdataset[13][8] = {{3,6,12,24,48,96,192,96} , {6,12,24,48,96,192,96,48} ,
{12,24,48,96,192,96,48,0} ,{24,48,96,192,96,48,0,12} ,
{48,96,192,96,48,0,12,6} ,{96,192,96,48,0,12,6,3} ,
  {192,96,48,0,12,6,3,6} , {96,48,0,12,6,3,6,12} ,
  {48,0,12,6,3,6,12,24} , {0,12,6,3,6,12,24,48} ,
  {12,6,3,6,12,24,48,96} , {6,3,6,12,24,48,96,192} , {3,6,12,24,48,96,192,128}};


int rdataset[13][8] = {{192,96,48,0,12,6,3,6} , {96,48,0,12,6,3,6,12} ,
  {48,0,12,6,3,6,12,24} , {0,12,6,3,6,12,24,48} ,
  {12,6,3,6,12,24,48,96} , {6,3,6,12,24,48,96,192} , 
  {3,6,12,24,48,96,192,96} , {6,12,24,48,96,192,96,48} ,
  {12,24,48,96,192,96,48,0} , {24,48,96,192,96,48,0,12} ,
  {48,96,192,96,48,0,12,6} , {96,192,96,48,0,12,6,3} ,{3,6,12,24,48,96,192,128}};
 
int posicionAnimacionRYV = 0;

// Módulo RTC
ThreeWire myWire(7,6,8); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

// Pines y variables para los 4 displays 8x8 rojos
// DIN 11
// CS 10
// CLK 13
const int pinCS = 10;
const int numberOfHorizontalDisplays = 4;
const int numberOfVerticalDisplays = 1;
const int pinButton1 = A0;
const int pinButton2 = A1;
const int pinButton3 = A2;
 
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

String relojBak = "--------";
int animacion=0;
int modo = 0;
float temporizador = 0;
float temporizadorAnimacion = 0;
bool fliphor1, fliphor2, flipmin1, flipmin2, flipseg1, flipseg2;
 
const int wait = 50; // In milliseconds
 
const int spacer = 1;
const int width = 5 + spacer; // The font width is 5 pixels
 
void setup() {

  Serial.begin(9600);
        // Actualizamos el temporizador para que no se apague el bluetooth
      bluetoothEncendido = true;
      temporizadorApagarBluetooth = millis() + 600000;
      
      // Recuperar los ajustes de la EEPROM
      recuperarEEPROM();

   // Controlar la alimentación del módulo bluetooth mediante un transistor conectado al pin 12
   pinMode(bluetoothPIN, OUTPUT);
  

    // Inicializar sensor de temperatura
    dht.begin();

    // Alarma 
    pinMode(alarmaPin, OUTPUT);

    // Inicialización matriz Roja y Verde
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(latchPin, OUTPUT);

    // Botones
    pinMode(pinButton1 , INPUT_PULLUP);
    pinMode(pinButton2 , INPUT_PULLUP);
    pinMode(pinButton3 , INPUT_PULLUP);
    
    // RTC
    Rtc.Begin();

    // LED MATRIX
   matrix.setIntensity(contraste); // Use a value between 0 and 15 for brightness
 
   // Adjust to your own needs
   matrix.setPosition(0, 0, 0); // The first display is at <0, 0>
   matrix.setPosition(1, 1, 0); // The second display is at <1, 0>
   matrix.setPosition(2, 2, 0); // The third display is at <2, 0>
   matrix.setPosition(3, 3, 0); // And the last display is at <3, 0>
 
   matrix.setRotation(0, 1);    // Display is position upside down
   matrix.setRotation(1, 1);    // Display is position upside down
   matrix.setRotation(2, 1);    // Display is position upside down
   matrix.setRotation(3, 1);    // Display is position upside down

}
 
void loop() {

// Controlar en encendido del bluetooth
if (bluetoothEncendido) {
   digitalWrite(bluetoothPIN, HIGH);
   bluetoothEncendido = false;
 //  Serial.println("Bluetooth encendido");
}

// Si ha pasado un tiempo sin usar el bluetooth, lo apagamos
if (temporizadorApagarBluetooth < millis()) {
    digitalWrite(bluetoothPIN, LOW);
    bluetoothEncendido = false;
}


// Si tenemos activada la pantalla mostramos el reloj
if (tiempoApagar > millis() || apagarSeg == 0) {
  animacionRojaVerde();

  if (scrollOn) {
    scrollLeft(relojBak);
  } else {
    
    switch (modoReloj) {
      case 1:
      // Reloj en modo normal
        relojFlip();
      break;
      case 2:
      // Reloj en modo mini
        relojFlip();
      break;
      case 3:
      // Si tenemos seleccionado el modo aleatorio,alternar cada 30 segundos entre las distintas visualizaciones: Modo normal, modo mini, fecha, temperatura
     
        if (temporizadorAleatorio < millis()) {
          temporizadorAleatorio = millis() + 30000;
          modoAleatorioActual++;
          if (modoAleatorioActual == 5) modoAleatorioActual = 1;

        }

        
          switch (modoAleatorioActual) {
            case 1:
              modo = 0;
              relojFlip();
            break;
            case 2:
              modo = 1;
              relojFlip();
            break;
            case 3:
              relojBak = actualizarFecha();
              scrollOn = true;
            break;
            case 4:
               // Leer la temperatura del sensor
               leerTemperatura();
            break;
          }
      
      break;
      case 4:
      // Mostrar la fecha
        relojBak = actualizarFecha();
        scrollOn = true;
      break;
      
    }
    
  }
} else {
  // Si no, apagamos la matriz de leds
  matrix.fillScreen(LOW);
  matrix.write();
}


  // Comprobar la alarma
  String horaActual = actualizarReloj();
  String alarmaSinDias = alarma.substring(0,8);

  
  if (tiempoAlarma < millis() && activarAlarma) apagarAlarma();
  if (horaActual.equals(alarmaSinDias) && !activarAlarma) {
     alarmaActivada();
     tiempoAlarma = millis() + 60000;
     temporizadorAlarma = millis() + 1000;
  }
  if (activarAlarma) {
     if (temporizadorAlarma < millis()) {
         temporizadorAlarma = millis() + 1000;
      if (estadoCampana == false) {
        estadoCampana = true;
         digitalWrite(alarmaPin , HIGH);
     //    Serial.println("Alarma high");
      } else {
        estadoCampana = false;
        digitalWrite(alarmaPin , LOW);
     //   Serial.println("Alarma low");
      }
     }   
  } 


  // Tocar sonido cada hora en punto ------------------------------------------------------------------
  if ( tocarSonidoHoras && horaActual.substring(3,9) == "00:00") {
     digitalWrite(alarmaPin , HIGH);
  } 

  // Encender durante 1 segundo el electroiman
  if (horaActual.substring(3,9) == "00 01") {
     digitalWrite(alarmaPin , LOW);
   } 
    


  // Leer botones
  int boton1 = digitalRead(pinButton1);
  int boton2 = digitalRead(pinButton2);
  int boton3 = digitalRead(pinButton3);

  // Usamos un temporizador para evitar que detecte varias pulsaciones muy rápidas de los botones
  if (temporizador < millis()) {
  // Botón 1 ver alarma  ...............................................................................
  if (boton1 == 0) {
    // Apagar la alarma si estuviera funcionando
    apagarAlarma();

    temporizador = millis() + 1000;
    // encender la pantalla si estaba apagada
    tiempoApagar = millis() + (apagarSeg * 1000);

    if (alarma.equals("")) {
      relojBak = "No hay alarma";   
    } else {
      relojBak = "Alarma: " + alarma;
    }
   
   scrollOn = true;

     // Si tocamos un botón encendemos el bluetooth y lo programamos para apagarse en 10 minutos (600000 milisegundos)
    bluetoothEncendido = true;
    temporizadorApagarBluetooth = millis() + 600000;

 
  }

  // Botón 2 mostrar fecha actual ...............................................................................
  if (boton2 == 0 ) {
    // Apagar la alarma si estuviera funcionando
    apagarAlarma();
    
  temporizador = millis() + 100;
      // encender la pantalla si estaba apagada
    tiempoApagar = millis() + (apagarSeg * 1000);
  

   relojBak = actualizarFecha();
   scrollOn = true;

  // Si tocamos un botón encendemos el bluetooth y lo programamos para apagarse en 10 minutos (600000 milisegundos)
    bluetoothEncendido = true;
    temporizadorApagarBluetooth = millis() + 600000;
  }


  // Botón 3 mostrar temperatura ...............................................................................
  if (boton3 == 0) {
    // Apagar la alarma si estuviera funcionando
    apagarAlarma();
    
    temporizador = millis() + 1000;
      // encender la pantalla si estaba apagada
    tiempoApagar = millis() + (apagarSeg * 1000);

      // Leer la temperatura del sensor
      leerTemperatura();


  // Si tocamos un botón encendemos el bluetooth y lo programamos para apagarse en 10 minutos (600000 milisegundos)
    bluetoothEncendido = true;
    temporizadorApagarBluetooth = millis() + 600000;

   
    
  }

  
  }

  

  escucharBluetooth();

  
}

void escribirXcaracteres(String cadenaHora){
  int posicion = 1; 
  matrix.fillScreen(LOW);
  for (int n=0 ; n< cadenaHora.length() ; n++) {
    matrix.drawChar(posicion,0, cadenaHora[n], HIGH, LOW, 1, 1);
    matrix.write();
      posicion += 6;  
  }
}

void imprimirCadena(String cadenaHora) {
  matrix.setFont(&TomThumb);
  matrix.fillScreen(LOW);
  matrix.setCursor(0,5);
  matrix.print(cadenaHora);
  matrix.write();
}

void relojFlip() {

//// Serial.println (modo);

     matrix.setFont();
     int ajusteVerticalPicoPixel = 0;
     int ajusteHorizontal = 6;
     int posicionInicio = 1;

  String reloj = actualizarReloj();

  if (modo == 1) {
     matrix.setFont(&TomThumb);  
     ajusteVerticalPicoPixel = 6;
     ajusteHorizontal = 4;
     posicionInicio = 0;
  } 
  
  matrix.fillScreen(LOW);

  // Mirar si ha cambiado algún dígito del reloj y si es así lanzar la animación
  if (reloj[0] != relojBak[0]) {
    fliphor1 = true;
  }
  if (reloj[1] != relojBak[1]) {
    fliphor2 = true;
  }
  if (reloj[3] != relojBak[3]) {
    flipmin1 = true;
  }
  if (reloj[4] != relojBak[4]) {
    flipmin2 = true;
  }

  // Segundos en el modo PicoPixel
  
  if (reloj[6] != relojBak[6]) {
    flipseg1 = true;
  }
  if (reloj[7] != relojBak[7]) {
    flipseg2 = true;
  }
 
 // // Serial.print (reloj[6]);
 // // Serial.println (relojBak[6]);

 // incrementar la posición 'y' si hay alguna animación
 if (temporizadorAnimacion < millis()) {
 if (fliphor1 || fliphor2 || flipmin1 || flipmin2 || flipseg1 || flipseg2) {
      temporizadorAnimacion = millis() + 100;     // Velocidad de la animación de los números
      animacion++;
      if (animacion == 9) {
        animacion = 0 + ajusteVerticalPicoPixel;
        fliphor1 = false;
        fliphor2 = false;
        flipmin1 = false;
        flipmin2 = false;
        flipseg1 = false;
        flipseg2 = false;
        relojBak = reloj;
      }
    }
 }
   // Imprimir los 2 puntos
   if (modo == 0) {
    matrix.drawChar(posicionInicio + 1 + (ajusteHorizontal*2) - 1, 0 + ajusteVerticalPicoPixel, reloj[2], HIGH, LOW, 1);
    matrix.drawChar(posicionInicio + 1 + (ajusteHorizontal*5) - 1, 0 + ajusteVerticalPicoPixel, reloj[2], HIGH, LOW, 1); 
   } else {
    matrix.drawChar(posicionInicio + 1 + (ajusteHorizontal*2), 0 + ajusteVerticalPicoPixel, reloj[2], HIGH, LOW, 1);
    matrix.drawChar(posicionInicio + 1 + (ajusteHorizontal*5), 0 + ajusteVerticalPicoPixel, reloj[2], HIGH, LOW, 1); 
   }
   

 // Animar los números
    if (fliphor1) {
    // deslizar los dígitos hacia abajo
      matrix.drawChar(posicionInicio  , animacion, relojBak[0], HIGH, LOW, 1);
      matrix.drawChar(posicionInicio , animacion-8, reloj[0], HIGH, LOW, 1);
  } else {
      matrix.drawChar(posicionInicio, 0 + ajusteVerticalPicoPixel, reloj[0], HIGH, LOW, 1);
  }

  if (fliphor2) {
    // deslizar los dígitos hacia abajo
      matrix.drawChar(posicionInicio + ajusteHorizontal, animacion, relojBak[1], HIGH, LOW, 1);
      matrix.drawChar(posicionInicio + ajusteHorizontal, animacion-8, reloj[1], HIGH, LOW, 1);
  } else {
      matrix.drawChar(posicionInicio + ajusteHorizontal, 0+ ajusteVerticalPicoPixel, reloj[1], HIGH, LOW, 1);
  }

  if (flipmin1) {
    // deslizar los dígitos hacia abajo
      matrix.drawChar(posicionInicio+ (ajusteHorizontal*3), animacion, relojBak[3], HIGH, LOW, 1);
      matrix.drawChar(posicionInicio+ (ajusteHorizontal*3), animacion-8, reloj[3], HIGH, LOW, 1);
  } else {
      matrix.drawChar(posicionInicio+(ajusteHorizontal*3), 0+ ajusteVerticalPicoPixel, reloj[3], HIGH, LOW, 1);
  }

  if (flipmin2) {
    // deslizar los dígitos hacia abajo
      matrix.drawChar(posicionInicio + (ajusteHorizontal*4), animacion, relojBak[4], HIGH, LOW, 1);
      matrix.drawChar(posicionInicio + (ajusteHorizontal*4), animacion-8, reloj[4], HIGH, LOW, 1);
  } else {
      matrix.drawChar(posicionInicio + (ajusteHorizontal*4), 0+ ajusteVerticalPicoPixel, reloj[4], HIGH, LOW, 1);
  }

  if (flipseg1) {
    // deslizar los dígitos hacia abajo
      matrix.drawChar(posicionInicio+ (ajusteHorizontal*6), animacion, relojBak[6], HIGH, LOW, 1);
      matrix.drawChar(posicionInicio+ (ajusteHorizontal*6), animacion-8, reloj[6], HIGH, LOW, 1);
  } else {
      matrix.drawChar(posicionInicio+(ajusteHorizontal*6), 0+ ajusteVerticalPicoPixel, reloj[6], HIGH, LOW, 1);
  }

  if (flipseg2) {
    // deslizar los dígitos hacia abajo
      matrix.drawChar(posicionInicio + (ajusteHorizontal*7), animacion, relojBak[7], HIGH, LOW, 1);
      matrix.drawChar(posicionInicio + (ajusteHorizontal*7), animacion-8, reloj[7], HIGH, LOW, 1);
  } else {
      matrix.drawChar(posicionInicio + (ajusteHorizontal*7), 0+ ajusteVerticalPicoPixel, reloj[7], HIGH, LOW, 1);
  }


  
   // Pasar los datos a la matriz
   matrix.write();
  // delay(100);
}


void scrollLeft(String tape) {
// Seleccionar fuente grande
matrix.setFont();

      if (temporizadorScroll < millis()) {
        if (posicionScroll < width * tape.length() + matrix.width() - 1 - spacer) {
          temporizadorScroll = millis() + wait;
          posicionScroll++;  
        } else {
          if (!repetirMensaje) {
          scrollOn = false;  
          }
          posicionScroll = 0;
          
        }
      }

    // for (int i = 0; i < width * tape.length() + matrix.width() - 1 - spacer; i++) {
 
      matrix.fillScreen(LOW);
 
      int letter = posicionScroll / width;
      int x = (matrix.width() - 1) - posicionScroll % width;
      int y = (matrix.height() - 8) / 2; // center the text vertically
 
      while (x + width - spacer >= 0 && letter >= 0) {
         if (letter < tape.length()) {
            matrix.drawChar(x, y, tape[letter], HIGH, LOW, 1);
         }
 
         letter--;
         x -= width;
      }
      matrix.write(); // Send bitmap to display
 
    //  delay(wait);
  // }
}

String actualizarReloj(){
    dt = Rtc.GetDateTime();

    String hora = (String) (dt.Hour());
    if (hora.length()==1) hora = "0" + hora;
    String minuto = (String) (dt.Minute());
    if (minuto.length()==1) minuto = "0" + minuto;
    String segundo = (String) (dt.Second());
    if (segundo.length()==1) segundo = "0" + segundo;

    String separador = " ";
    
    if (dt.Second()%2 == 0) {
      separador = ":";
    } else {
      separador = " ";
    }

    String datestring = hora + separador + minuto + separador + segundo;

  //// Serial.println(datestring);
 // delay(100);
  return datestring;
}

String actualizarFechaCorta() {
   dt = Rtc.GetDateTime();
   String dia = (String) (dt.Day());
   String mes = (String) (dt.Month());
   String agno = (String) (dt.Year());

   String datestring =  dia + " " + mes + " " + agno;

   return datestring;
   
}

String actualizarFecha(){
    dt = Rtc.GetDateTime();

    String dia = (String) (dt.Day());
    String mes = "";

    switch (dt.Month()) {
      case 1:
      mes = "Enero";
      break;
      case 2:
      mes = "Febrero";
      break;
      case 3:
      mes = "Marzo";
      break;
      case 4:
      mes = "Abril";
      break;
      case 5:
      mes = "Mayo";
      break;
      case 6:
      mes = "Junio";
      break;
      case 7:
      mes = "Julio";
      break;
      case 8:
      mes = "Agosto";
      break;
      case 9:
      mes = "Septiembre";
      break;
      case 10:
      mes = "Octubre";
      break;
      case 11:
      mes = "Noviembre";
      break;
      case 12:
      mes = "Diciembre";
      break;      
    }
    String agno = (String) (dt.Year());

    String diasemana = "";

    switch (dt.DayOfWeek()) {
      case 0:
      diasemana = "Domingo";
      break;
            case 1:
      diasemana = "Lunes";
      break;
            case 2:
      diasemana = "Martes";
      break;
            case 3:
      diasemana = "Miercoles";
      break;
            case 4:
      diasemana = "Jueves";
      break;
            case 5:
      diasemana = "Viernes";
      break;
            case 6:
      diasemana = "Sabado";
      break;
    }

    String separador = "/";

    String datestring = diasemana + " " + dia + " de " + mes + " de " + agno + " ";

  //// Serial.println(datestring);
 // delay(100);
  return datestring;
}

void animacionRojaVerde() {
    if (temporizadorAnimRYV < millis()) {
    temporizadorAnimRYV = millis() + 200;
        val++;
        if (val == 12) {val=0;}
    }

    
    for (int row = 0 ; row < NUM_ROWS ; row++) {
        int rcur = rdataset[val][row];
        int gcur = gdataset[val][row];

        shiftOut(dataPin, clockPin, MSBFIRST, 255 - rcur);
        shiftOut(dataPin, clockPin, MSBFIRST, 255 - gcur);
        shiftOut(dataPin, clockPin, MSBFIRST, B00000001 << row);
        digitalWrite(latchPin, HIGH);
        digitalWrite(latchPin, LOW);

      //  delayMicroseconds(100);
        
        shiftOut(dataPin, clockPin, MSBFIRST, 255);
        shiftOut(dataPin, clockPin, MSBFIRST, 255);
        shiftOut(dataPin, clockPin, MSBFIRST, B00000001 << row);
        digitalWrite(latchPin, HIGH);
        digitalWrite(latchPin, LOW);
    }
}



  void escucharBluetooth() {
     //si existe información pendiente
     String data = "";
     String comando = "";
     
   if ( Serial.available())
   {
      data = Serial.readStringUntil('\n');
      comando = data.substring(0, 3);
      
      // Actualizamos el temporizador para que no se apague el bluetooth
      bluetoothEncendido = true;
      temporizadorApagarBluetooth = millis() + 600000;

      // Serial.println("Recibido: " + comando);
   }

   

   if (comando.equals("#H#")) { 
         // Se ha recibido el comando para poner en hora
         // Serial.println("datos: " + data);
      
          repetirMensaje = false;
         int horasSet = 0;
         int minutosSet = 0;
         int segundosSet = 0;
         int diaSet = 0;
         int mesSet = 0;
         int agnoSet = 0;
         int diaSemanaSet = 0;

          int numeroDato=0;
          String cadenaDato = "";

          for (int n=0 ; n < data.length() ; n++) {
            char caracter = data[n];
            if (caracter == '#') {

                switch (numeroDato) {
                  case 2:
                   horasSet = cadenaDato.toInt();
                  break;
                  case 3:
                   minutosSet = cadenaDato.toInt();
                  break;
                  case 4:
                   segundosSet = cadenaDato.toInt();
                  break;
                  case 5:
                   diaSet = cadenaDato.toInt();
                  break;
                  case 6:
                   mesSet = cadenaDato.toInt();
                  break;
                  case 7:
                   agnoSet = cadenaDato.toInt();
                  break;
                  case 8:
                   diaSemanaSet = cadenaDato.toInt();
                  break;
                }
              
              numeroDato++;
              cadenaDato = "";
            } else {
              cadenaDato.concat(caracter);
            }
          }

          ponerEnHora(horasSet, minutosSet, segundosSet, diaSet, mesSet, agnoSet, diaSemanaSet);
          relojBak = "Reloj Actualizado";
          scrollOn = true;
         
         
   } else {

    // Se ha recibido la orden de ajustar el brillo...
    if (comando.equals("#B#")){
      repetirMensaje = false;
      contraste = (data.substring(3,5)).toInt();
      matrix.setIntensity(contraste); // Use a value between 0 and 15 for brightness
      relojBak = "Brillo ajustado a " + data.substring(3,5) ;
      scrollOn = true;
    } else {
        if (comando.equals("#A#")) {
          // Se ha recibido la orden de apagar la pantalla cuando pasen 'x' segundos
          apagarSeg = (data.substring(3,5)).toInt();
          tiempoApagar = millis() + (apagarSeg * 1000);
          if (apagarSeg > 0) {
          relojBak = "Apagar a los " + data.substring(3,5) + " seg.";  
          } else {
          relojBak = "No apagar nunca la pantalla";  
          }
          
          scrollOn = true;
        } else {
          if (comando.equals("#M#")) {
            repetirMensaje = false;
            // Cambiar el modo en que visualizamos el reloj

            int comandoModo = (data.substring(3,5)).toInt();

            switch (comandoModo){
              case 1:
                 modo = 0;
                 modoReloj = 1;
                 relojBak = "Modo normal";
                 scrollOn = true;
              break;
              case 2:
                 modo = 1;
                 modoReloj = 2;
                 relojBak = "Modo mini";
                 scrollOn = true;
              break;
              case 3:
                 modoReloj = 3;
                 relojBak = "Modo aleatorio";
                 scrollOn = true;
                 temporizadorAleatorio = millis() + 30000;
              break;
              case 4:
                 modoReloj = 4;
                 relojBak = "Modo fecha";
                 scrollOn = true;
              break;
            }
 
            
          } else {
            if (comando.equals("#t#")) {
              repetirMensaje = false;
              // Recibido mensaje para mostrar por pantalla
                 repetirMensaje = false;
                 relojBak = data.substring(3, data.length()) ;
                 scrollOn = true;
                           
            } else {
                if (comando.equals("#T#")) {
                 repetirMensaje = true;
                 relojBak = data.substring(3, data.length()) ;
                 scrollOn = true;
              } else {
                 // Recibido comando para ajustar la alarma
                 if (comando.equals("#S#")) {
                  repetirMensaje = false;
                 alarma = data.substring(3, data.length());
                 
                 // Serial.println(alarma);
                 // Serial.println(alarma.substring(0,8));
                 
                 relojBak = "Alarma activada" ;
                 scrollOn = true;
                 } else {
                  // Recibido comando para desactivar la alarma
                    if (comando.equals("#s#")) {
                      repetirMensaje = false;
                    alarma = "";
                    relojBak = "Alarma desactivada" ;
                    scrollOn = true;                    
                  } else {
                    // Recibido comando para tocar sonido en las horas
                      if (comando.equals("#C#")) {
                        repetirMensaje = false;
                          tocarSonidoHoras = true;   
                          relojBak = "Tocar horas" ;
                          scrollOn = true;
                      } else {
                        if (comando.equals("#c#")) {
                          repetirMensaje = false;
                          tocarSonidoHoras = false;
                          relojBak = "Sin sonido horas" ;
                          scrollOn = true;
                        } else {
                          // Recibido comando para las animaciones ...

                          if (comando.equals("#X#")) {
                            repetirMensaje = false;

                            // Hacer un blucle que vaya recibiendo los bytes y asignandolos a la variable que ya tenemos para la animación
                            // en total serán 192 bytes (96 bytes por color, 8 bytes cada pantalla)
                            // No saldrá del bucle hasta que complete la recepción de los datos

                       //     Serial.println("Recibido: " + comando);
                            Serial.flush();

                            for (int cr = 0; cr < 13 ; cr++) {
                              for (int cc = 0; cc < 8 ; cc++) {
                                rdataset[cr][cc] = 0;
                                gdataset[cr][cc] = 0;
                              }
                            }

                            imprimirCadena("Rec.Datos");
                                            
                               int posicionFila = 0;
                               int posicionColumna = 0;
                               temporizadorDatosBluetooth = millis() + 30000;

                               do {
                               if (Serial.available()){
                                  int datoRecibido = Serial.read();
                               //   Serial.print(datoRecibido);
                               //   Serial.print(",");

                                    if (posicionFila < 12) {
                                    rdataset[posicionFila][posicionColumna] = datoRecibido;  
                                    } else {
                                    gdataset[posicionFila - 12][posicionColumna] = datoRecibido;  
                                    }
                                  
                                    posicionColumna++;
                                    if (posicionColumna == 8) {
                                    posicionColumna = 0;
                                    posicionFila++;
                                  }
                               }
                            
                                  
                              } while (millis() < temporizadorDatosBluetooth);
                          } else {
                            Serial.flush();
                          }

                       // Fin recibir ordenes por bluetooth ---------------------------------------------------------------------------------------------------------------------------------
                     }
                   }
                 }
               }
             }
           } 
         }
       }
     }
   }

     // Guardamos los ajustes en la EEPROM
     guardarEEPROM();

  }


  
  void ponerEnHora(int horasSet,int minutosSet,int segundosSet,int diaSet,int mesSet,int agnoSet,int diaSemanaSet) {
    RtcDateTime compiled = RtcDateTime(agnoSet, mesSet, diaSet, horasSet, minutosSet, segundosSet);
    Rtc.SetDateTime(compiled);
  }

/*
void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    // Serial.print(datestring);
}
*/


void alarmaActivada() {

     switch (dt.DayOfWeek()) {
            case 0:
            // Serial.print("0");
       if (alarma.indexOf("D") != -1) {
          activarAlarma = true;
          }
      break;
            case 1:
            // Serial.print("1");
         if (alarma.indexOf("L") != -1) {
          activarAlarma = true;
       }
      break;
            case 2:
            // Serial.print("2");
       if (alarma.indexOf("M") != -1) {
          activarAlarma = true;
       }
      break;
            case 3:
            // Serial.print("3");
       if (alarma.indexOf("X") != -1) {
          activarAlarma = true;
       }
      break;
            case 4:
            // Serial.print("4");
       if (alarma.indexOf("J") != -1) {
          activarAlarma = true;
       }
      break;
            case 5:
            // Serial.print("5");
       if (alarma.indexOf("V") != -1) {
          activarAlarma = true;
       }
      break;
            case 6:
            // Serial.print("6");
       if (alarma.indexOf("S") != -1) {
          activarAlarma = true;
       }
      break;
      default:
      // Serial.print("Alarma no encontrada");
      break;
      
    }

     relojBak = "Alarma" ;
     scrollOn = true;
}


void apagarAlarma() {
   digitalWrite(alarmaPin, LOW);
   activarAlarma = false;
//// Serial.println("Apagar alarma"); 
}

void leerTemperatura() {

 //if (temporizadorLeerTemperatura < millis()) {
    
   // temporizadorLeerTemperatura = millis() + 1000;

    
    // Reading temperature or humidity takes about 250 milliseconds!
   int h = dht.readHumidity();
   int t = dht.readTemperature();
 
   if (isnan(h) || isnan(t)) {
     //  Serial.println("Failed to read from DHT sensor!");
      return;
   }
 
  
// }


     relojBak = "Temperatura: " + (String)t + " C   Humedad: " + (String)h + "%";
     scrollOn = true;
   
}


void guardarEEPROM () {

  struct MyObject {
      int contrasteBAK;
      int apagarSegundosBAK;
      int modoRelojBAK;
      int horaAlarmaBAK;
      int minutoAlarmaBAK;
      int lAlarmaBAK;
      int mAlarmaBAK;
      int xAlarmaBAK;
      int jAlarmaBAK;
      int vAlarmaBAK;
      int sAlarmaBAK;
      int dAlarmaBAK;
  };

int lAlarma = 0;
int mAlarma = 0;
int xAlarma = 0;
int jAlarma = 0;
int vAlarma = 0;
int sAlarma = 0;
int dAlarma = 0;

if (alarma.indexOf("L") != -1) lAlarma = 1;
if (alarma.indexOf("M") != -1) mAlarma = 1;
if (alarma.indexOf("X") != -1) xAlarma = 1;
if (alarma.indexOf("J") != -1) jAlarma = 1;
if (alarma.indexOf("V") != -1) vAlarma = 1;
if (alarma.indexOf("S") != -1) sAlarma = 1;
if (alarma.indexOf("D") != -1) dAlarma = 1;

int horaAlarma = (alarma.substring(0,3)).toInt();
int minutoAlarma = (alarma.substring(3,5)).toInt();

    //Data to store.
  MyObject customVar = {
    contraste,
    apagarSeg,
    modoReloj,
    horaAlarma,
    minutoAlarma,
    lAlarma,
    mAlarma,
    xAlarma,
    jAlarma,
    vAlarma,
    sAlarma,
    dAlarma
  };

  EEPROM.put(0, customVar);

}

void recuperarEEPROM() {

   struct MyObject {
      int contrasteBAK;
      int apagarSegundosBAK;
      int modoRelojBAK;
      int horaAlarmaBAK;
      int minutoAlarmaBAK;
      int lAlarmaBAK;
      int mAlarmaBAK;
      int xAlarmaBAK;
      int jAlarmaBAK;
      int vAlarmaBAK;
      int sAlarmaBAK;
      int dAlarmaBAK;
  };

   MyObject customVar; //Variable to store custom object read from EEPROM.
   EEPROM.get(0, customVar);

   contraste = customVar.contrasteBAK;
   apagarSeg = customVar.apagarSegundosBAK;
   modoReloj = customVar.modoRelojBAK;
   alarma = "";
   String diasAlarma = "";
  
   if (customVar.lAlarmaBAK == 1) diasAlarma += "L";
   if (customVar.mAlarmaBAK == 1) diasAlarma += "M";
   if (customVar.xAlarmaBAK == 1) diasAlarma += "X";
   if (customVar.jAlarmaBAK == 1) diasAlarma += "J";
   if (customVar.vAlarmaBAK == 1) diasAlarma += "V";
   if (customVar.sAlarmaBAK == 1) diasAlarma += "S";
   if (customVar.dAlarmaBAK == 1) diasAlarma += "D";

   if (customVar.horaAlarmaBAK < 10) alarma.concat("0"); 
   alarma.concat(customVar.horaAlarmaBAK); 
   alarma.concat(":");
   if (customVar.minutoAlarmaBAK < 10) alarma.concat("0");
   alarma.concat(customVar.minutoAlarmaBAK);
   alarma.concat(":00");
   alarma.concat(diasAlarma);
   if (alarma.equals("00:00:00")) alarma = "";
   
}

/*
 void seleccionarAnimacion (int animacion) {
  switch (animacion) {
    case 0:
     int gdataset0[13][8] = {{3,6,12,24,48,96,192,96} , {6,12,24,48,96,192,96,48} ,
      {12,24,48,96,192,96,48,0} ,{24,48,96,192,96,48,0,12} ,
      {48,96,192,96,48,0,12,6} ,{96,192,96,48,0,12,6,3} ,
      {192,96,48,0,12,6,3,6} , {96,48,0,12,6,3,6,12} ,
      {48,0,12,6,3,6,12,24} , {0,12,6,3,6,12,24,48} ,
      {12,6,3,6,12,24,48,96} , {6,3,6,12,24,48,96,192} , {3,6,12,24,48,96,192,128}};


    int rdataset0[13][8] = {{192,96,48,0,12,6,3,6} , {96,48,0,12,6,3,6,12} ,
      {48,0,12,6,3,6,12,24} , {0,12,6,3,6,12,24,48} ,
      {12,6,3,6,12,24,48,96} , {6,3,6,12,24,48,96,192} ,  
      {3,6,12,24,48,96,192,96} , {6,12,24,48,96,192,96,48} ,
      {12,24,48,96,192,96,48,0} , {24,48,96,192,96,48,0,12} ,
      {48,96,192,96,48,0,12,6} , {96,192,96,48,0,12,6,3} ,{3,6,12,24,48,96,192,128}};
    break;


    
  }
 }
*/
