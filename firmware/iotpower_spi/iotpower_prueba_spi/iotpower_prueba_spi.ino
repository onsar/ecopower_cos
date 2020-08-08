// Slave


 
#define DEBUG 1

#define DATOS 0x07

#include <SPI.h>

char datos_matrix[] = {'h','o','l','a',':','3',0x0A};
uint8_t datos_pendientes;

uint32_t t_last_tx;

void setup (void)
{
  Serial.begin(9600);

  pinMode(MISO, OUTPUT);

  // turn on SPI in slave mode
  SPCR |= bit(SPE);

  // turn on interrupts
  SPCR |= bit(SPIE);

  SPDR = 0x00;
  datos_pendientes = 0;

}  // end of setup


// SPI interrupt routine
ISR (SPI_STC_vect){
  byte c = SPDR;
  
  if(c==0xB0){ // inicio  
    datos_pendientes = DATOS;
    SPDR = 0xA0 | DATOS;
    if(DEBUG) {
      Serial.print("bit recibido =  "); Serial.println(c,HEX);
      Serial.print("datos_pendientes =  "); Serial.println(datos_pendientes);
    }
  }

  if(c==0xB1){// transmision iniciada
    
    if(datos_pendientes > 0) {
      SPDR = datos_matrix[DATOS - datos_pendientes];  // 4-4=0, 4-3=1, 4-2=2, 4-1=3,
      datos_pendientes = datos_pendientes-1;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("datos_pendientes =  "); Serial.println(datos_pendientes);
      }
    }
    else{
      uint8_t registros_suma  = sumar_registros();
      SPDR = registros_suma;
      datos_pendientes = 0;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("registros_suma:  ");Serial.println(registros_suma,HEX);
      }
    }  
  }

  if(c==0xB8){ // reset   
    SPDR = 0x55;
    datos_pendientes = 0;
    if(DEBUG) {
      Serial.print("bit recibido =  "); Serial.println(c,HEX);
      Serial.println(" ***reset  ");
    }
  }


  if(c==0x02){ // inicio  
    datos_pendientes = DATOS;
    
    if(datos_pendientes > 0) {
      SPDR = datos_matrix[DATOS - datos_pendientes];  // 4-4=0, 4-3=1, 4-2=2, 4-1=3,
      datos_pendientes = datos_pendientes-1;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("datos_pendientes =  "); Serial.println(datos_pendientes);
      }
    }

    else{
      uint8_t registros_suma  = sumar_registros();
      SPDR = registros_suma;
      datos_pendientes = 0;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("registros_suma:  ");Serial.println(registros_suma,HEX);
      }
    }
  }

  if(c==0x03){ // reset   
    SPDR = 0x06;
    datos_pendientes = 0;
    if(DEBUG) {
      Serial.print("bit recibido =  "); Serial.println(c,HEX);
      Serial.println(" ***reset  ");
    }
  }

  if(c==0x12){// transmision iniciada
    
    if(datos_pendientes > 0) {
      SPDR = datos_matrix[DATOS - datos_pendientes];  // 4-4=0, 4-3=1, 4-2=2, 4-1=3,
      datos_pendientes = datos_pendientes-1;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("datos_pendientes =  "); Serial.println(datos_pendientes);
      }
    }
    else{
      uint8_t registros_suma  = sumar_registros();
      SPDR = registros_suma;
      datos_pendientes = 0;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("registros_suma:  ");Serial.println(registros_suma,HEX);
      }
    }  
  }


    
}  // end of interrupt service routine (ISR) SPI_STC_vect




void loop (void){

}  // end of loop

uint8_t sumar_registros(){
  uint8_t suma = 0;
  for(uint8_t n=0; n < DATOS; n++){
    suma = suma + datos_matrix[n];
  }
  Serial.print("suma: "); Serial.println(suma,HEX);
  return suma;
}



  
