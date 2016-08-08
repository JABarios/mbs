// memboost, firmware v 02
// juan Barios, agosto 2016

#include "ads1298.h"
#include <stdlib.h>     /* strtoul */
#include "adsCMD.h"
#include "util.h"
#include <SPI.h>  // include the SPI library:

#include "firmware_mbs_01.h"
#include "version.h"
//#include "serial_parser.h"

long ultima_lectura[8];
long contador_muestras=0;

#define TABLE_SIZE 48
#define TWO_PI (3.14159 * 2)
float samples [TABLE_SIZE];
float phaseIncrement = TWO_PI/TABLE_SIZE;

void crea_seno(void){
  float currentPhase = 0.0;
  int i;
  for (i = 0; i < TABLE_SIZE; i ++){
      samples[i] = 100*sin(currentPhase)+100;
      currentPhase += phaseIncrement;
      }
}      

      

void inicia_hw() {
  using namespace ADS1298;

  //al empezar, reset
   crea_seno();
   delay(800); //desde Power up, esperar 1 segundo para mover nada
   digitalWrite(kPIN_RESET, LOW);
   delay(100);
   digitalWrite(kPIN_RESET, HIGH);
   delay(240); //deberia bastar, tiempo en datasheet es 240 ms


  pinMode(IPIN_CS, OUTPUT);
  pinMode(PIN_START, OUTPUT);
  pinMode(IPIN_DRDY, INPUT);
  pinMode(kPIN_LED, OUTPUT);
  pinMode(kPIN_RESET, OUTPUT);
  pinMode(kPIN_CLKSEL, OUTPUT);
  
  digitalWrite(PIN_START, LOW);
  digitalWrite(IPIN_CS, HIGH);
  digitalWrite(kPIN_CLKSEL, HIGH); // el reloj sea el interno
    
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);

  adc_send_command(SDATAC); // dejamos el modo READ para emitir comandos
  delay(10);

  // Determine model number and number of channels available
  gIDval = adc_rreg(ID); //lower 5 bits of register 0 reveal chip type
  switch (gIDval & B00011111 ) { //least significant bits reports channels
          case  B10000: //16
            gMaxChan = 4; //ads1294
            break;
          case B10001: //17
            gMaxChan = 6; //ads1296
            break; 
          case B10010: //18
            gMaxChan = 8; //ads1298
            break;
          case B11110: //30
            gMaxChan = 8; //ads1299
            break;
          default: 
            gMaxChan = 0;
  }

}

void ads_setupSimple() {
   using namespace ADS1298;
   adc_wreg(GPIO, char(0));
   adc_wreg(CONFIG1, LOW_POWR_250_SPS);
   adc_wreg(CONFIG2, INT_TEST_4HZ_2X);  // generate internal test signals
   adc_wreg(CONFIG3,char(PD_REFBUF | CONFIG3_const)); //PD_REFBUF used for test signal, activa la referencia interna
   #define MISC1 0x15
//   adc_wreg(MISC1,0x20); //set SRB1  
     //con srb1 activado, todos los canales P se referencian a srb1.
   delay(150);
   for (int i = 1; i <= gMaxChan; i++){
        if (gtestSignal)
           adc_wreg(char(CHnSET + i), char(TEST_SIGNAL | GAIN_12X ));
        else   
           adc_wreg(char(CHnSET + i), char(ELECTRODE_INPUT | GAIN_12X )); //report this channel with x12 gain
    } 
   
    detectActiveChannels(); 
	
	//start streaming data
    isRDATAC = true;
    adc_send_command(RDATAC); 
    adc_send_command(START); 
              
}

void detectActiveChannels() {  //actualiza gActiveChan y gNumActiveChan
  using namespace ADS1298; 
  gNumActiveChan = 0;
  for (int i = 1; i <= gMaxChan; i++) {
     delayMicroseconds(1); 
     int chSet = adc_rreg(CHnSET + i);
     gActiveChan[i] = ((chSet & 7) != SHORTED);
     if ( (chSet & 7) != SHORTED) gNumActiveChan ++;   
  }
}

void mensaje_inicio(){
   WiredSerial.println("");
   WiredSerial.print(F("Memboost v "));
   WiredSerial.println(build_version);   
   WiredSerial.print(F("build "));
   WiredSerial.println(build_fecha);   
   WiredSerial.println(F("(c) Scignals Technologies"));
   // si hay 8 canales, es q esta vivo...
   
   WiredSerial.print(F("canales activos:"));
   WiredSerial.println(gMaxChan);
   WiredSerial.println(F("Comandos:"));
   WiredSerial.println(F("hlp -- ayuda "));
   WiredSerial.println(F("sim -- señal simulada on/off")); 
   WiredSerial.println(F("prt -- protocolos")); 
   
}

void f_proto(int arg_cnt, char **args) {
    if(arg_cnt > 0) {
      modo_salida = atoi(args[0]);
    }
}



void inicia_serial_pc(){
  gLetra=new char[80];  
  WiredSerial.begin(SERIAL_SPEED); 
  while (WiredSerial.read() >= 0) {} ;
  HC06.begin(SERIAL_SPEED_BT); //use native port on Due
  while (HC06.read() >= 0) {} ;
  delay(200);  // Catch Due reset problem

//  cmdInit(&WiredSerial);
//  cmdAdd("test",f_test);
//  cmdAdd("proto",f_proto);


//   cmdAdd("hello", f_test);
  
}


void setup(){
  inicia_serial_pc();
  inicia_hw();
  ads_setupSimple();

  if(!gtestCONTINUO){
      mensaje_inicio();
      while(1);
      //nos quedamos colgados para terminar
  }
}

void imprime_linea( boolean modo){
   for (int i = 1; i < numSerialBytes; i+=3)
   {
    long numero = to_Int32(serialBytes+i);
     if(modo) {
                  WiredSerial.print(numero,HEX);
//                  WiredSerial.print(SEPARADOR_SERIAL );
                  HC06.print(numero,HEX);
//                  HC06.print(SEPARADOR_SERIAL );
     } else {
                  WiredSerial.print(numero);
                  HC06.print(numero);
                  if(i<numSerialBytes-3){
                      WiredSerial.print(SEPARADOR_SERIAL );
                      HC06.print(SEPARADOR_SERIAL );
                  }
                  
            }      
    }
//añadimos un ; al final 
    WiredSerial.println(FINLINEA);
    HC06.println(FINLINEA);
}


void imprime_openEEG_p2(boolean modo){
   int ind;
   int indj=0;
   static int count = -1;
   unsigned long ul;
   dos_int dosnum;

   ind=0;
   txBuf[ind++]=0xA5;
   txBuf[ind++]=0x5A;
   txBuf[ind++]=2;
   txBuf[ind++]=indice_paquete++;
      
   for (int i = 1; i < numSerialBytes-6; i+=3)
   {
     long numero = to_Int32(serialBytes+i) ;
     numero >>= 13; 
     //LSB - HSB
     txBuf[ind++] = (byte) (numero & 0x000000FF)  ;     
     txBuf[ind++] = (byte) ((numero & 0x0000FF00) >> 8)  ;
   }
   byte switches = 0x07;
   count++; if (count >= 18) count=0;
   if (count >= 9) {
      switches = 0x0F;
   } 
   txBuf[ind]=switches; 
   ind=0;
   if(modo){    
     for(int m=0;m<4;m++){
        WiredSerial.print(txBuf[m]);
        WiredSerial.print(SEPARADOR_SERIAL );
        ind++;
     }
     for(int m=0;m<6;m++){
        int vv=((int)txBuf[ind++] << 8);
        vv += (int)txBuf[ind++] ;
        WiredSerial.print(vv );
        WiredSerial.print(SEPARADOR_SERIAL );
     }
     WiredSerial.print(txBuf[ind]  );
//añadimos un ; al final 
    WiredSerial.println(";");
    HC06.println(";");

     } else {
     WiredSerial.write(txBuf,17);
     }
}

void imprime_openBCI_V3(int modo_bci_protocolo){
   int ind;
   int indj=0;
   static int count = -1;
   unsigned long ul;
   dos_int dosnum;

   ind=2;
   txBuf[0]=0xA0;
   txBuf[1]=indice_paquete++;
   for (int i = 1; i < numSerialBytes; i+=3)
   {
     txBuf[ind++] = serialBytes[i];
     txBuf[ind++] = serialBytes[i+1];
     txBuf[ind++] = serialBytes[i+2];
   }
      //los acelerometros, no los tenemos...
     txBuf[ind++] = 0;
     txBuf[ind++] = 0;
     txBuf[ind++] = 0;
     txBuf[ind++] = 0;
     txBuf[ind++] = 0;
     txBuf[ind++] = 0;
     txBuf[ind]=0xC0; 

   ind=0;
   if(modo_bci_protocolo==1){    
     for(int m=0;m<2;m++){
        WiredSerial.print(txBuf[ind]);
        WiredSerial.print(SEPARADOR_SERIAL );
        ind++;
     }
     for(int m=0;m<10;m++){
        long vv = to_Int32(txBuf+ind);
        WiredSerial.print(vv );
        if(m<9)WiredSerial.print(SEPARADOR_SERIAL );
        ind+=3;
     }
//añadimos un ; al final 
    WiredSerial.println(";");
    HC06.println(";");

  } else {
    if(modo_bci_protocolo==2){
        WiredSerial.write(txBuf,33);
    } else {    
         char letras[5];
         for(int jj=0;jj<33;jj++){
          sprintf(letras, "%02X",txBuf[jj]);
          WiredSerial.print(letras);
         }
         WiredSerial.println("");
    }
  }
}


void lee_datos_ads1299(void) { 
// hardware puro, 
// lee el ads y lo pone en serialBytes[]--numSerialBytes
    int i = 0;
    int jj=0;
    byte muestra[3];
    numSerialBytes = 1 + (3 * gNumActiveChan); //8-bits header plus 24-bits per ACTIVE channel

      
// cs a 0, empezamos a leer el ads1299	  
      digitalWrite(IPIN_CS, LOW);
      serialBytes[i++] =SPI.transfer(0); //get 1st byte of header
      SPI.transfer(0); //skip 2nd byte of header
      SPI.transfer(0); //skip 3rd byte of header
      for (int ch = 1; ch <= gMaxChan; ch++) {
        if( !gSimuladaSignal ){
              serialBytes[i++] = SPI.transfer(0);
              serialBytes[i++] = SPI.transfer(0);
              serialBytes[i++] = SPI.transfer(0);
              long vlast= to_Int32(serialBytes+i-3);
              long diff= ultima_lectura[ch] - vlast;
              if (abs(diff)>250000  ){
                 to_3bytes(0,serialBytes+i-3);
              } else {
                 ultima_lectura[ch]=vlast;
              }
              
        } else {      
              // señal seno, creada al inicio 
              jj=contador_muestras%TABLE_SIZE;
              to_3bytes(samples[jj]*100,muestra);
              if(ch!=166){
                  serialBytes[i++] = muestra[0];
                  serialBytes[i++] = muestra[1];
                  serialBytes[i++] = muestra[2];
              } else {
                  serialBytes[i++] = 0;
                  serialBytes[i++] = 0;
                  serialBytes[i++] = 0;
              }
        }
      }
// cs a 1, terminamos de leer el ads1299    
      delayMicroseconds(1);
      digitalWrite(IPIN_CS, HIGH);
}


void leeSerial(){
    if(WiredSerial.available()==0)return;
    
     String texto;
     texto=WiredSerial.readString();
      if(texto=="sim"){
        gSimuladaSignal=!gSimuladaSignal;
        return;
      }
      if(texto=="hlp"){
       mensaje_inicio();
       while(WiredSerial.available()==0);
       return;
      }
      if(texto=="prt"){
         //modo_salida++;
         if(++modo_salida>5)modo_salida=1;
         return;
      }
}

void loop()
{

  leeSerial();
    
  if(gtestCONTINUO && isRDATAC && digitalRead(IPIN_DRDY) == LOW ){
     lee_datos_ads1299();
     contador_muestras++;

     switch(modo_salida){
      case 1: 
         imprime_linea(true);
         break;
      case 2:
         imprime_linea(false);
         break;
      case 3:   
         imprime_openEEG_p2(true);
         break;
      case 4:   
         imprime_openEEG_p2(false);
         break;
      case 5:   
         imprime_openBCI_V3(1);
         break;
      case 6:   
         imprime_openBCI_V3(2);
         break;
      case 7:   
         imprime_openBCI_V3(3);
         break;
         
     }
  }   
}




