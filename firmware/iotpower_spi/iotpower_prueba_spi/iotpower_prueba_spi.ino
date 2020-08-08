// Slave

/*
 * diagrama de estados del servidor
 * ================================
 * 
 * estado = 0 inicio de la comunicacion       (inicio STX     02)       02   06   r1
 * 
 * estado = 2 recepción de registros          (siguente registro)       12   r1   r2
 *                                            (siguente registro)       12   r2   rn
 *                                            (Line Feed \n   0A)       12   rn   00
 *                                            (suma de registros)       12   00   suma
 *                                            
 * estado = 3 comprabar la trama recibida     (Final  ETX     03)       03   suma 06(ACK)
 *
 * estado = 4 procesando y transmision de valores
 * 
 * estado = 5 Reset: Final del texto
 * 
 * estado = 6 espera entre comuniaciones
*/
 
#define DEBUG 0

#define DATOS 0x07

#include <SPI.h>

// char datos_matrix[] = {'h','o','l','a',':','3',0x0A};

char datos_matrix[]= "hola:3";

int8_t data_leng;

int8_t datos_pendientes;

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
  data_leng = sizeof(datos_matrix) / sizeof(datos_matrix[0]);
  
  if(c==0xB0){ // inicio  
    datos_pendientes = data_leng;
    SPDR = 0xA0 | data_leng;
    if(DEBUG) {
      Serial.print("bit recibido =  "); Serial.println(c,HEX);
      Serial.print("datos_pendientes =  "); Serial.println(datos_pendientes);
    }
  }

  if(c==0xB1){// transmision iniciada
    
    if(datos_pendientes > 0) {
      SPDR = datos_matrix[data_leng - datos_pendientes];  // 4-4=0, 4-3=1, 4-2=2, 4-1=3,
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
    datos_pendientes = data_leng;
    
    if(datos_pendientes > 0) {
      SPDR = datos_matrix[data_leng - datos_pendientes];  // 4-4=0, 4-3=1, 4-2=2, 4-1=3,
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
      SPDR = datos_matrix[data_leng - datos_pendientes];  // 4-4=0, 4-3=1, 4-2=2, 4-1=3,
      datos_pendientes = datos_pendientes-1;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("datos_pendientes =  "); Serial.println(datos_pendientes);
      }
    }
    else if(datos_pendientes == 0){
      uint8_t registros_suma  = sumar_registros();
      SPDR = registros_suma;
      datos_pendientes = -1;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("datos_pendientes:  ");Serial.println(datos_pendientes);
      }
    }
    else{
      SPDR = 0x15;
      datos_pendientes = -2;
      if(DEBUG) {
        Serial.print("bit recibido =  "); Serial.println(c,HEX);
        Serial.print("datos_pendientes:  ");Serial.println(datos_pendientes);
      }
    }  
  }


    
}  // end of interrupt service routine (ISR) SPI_STC_vect




void loop (void){

}  // end of loop

uint8_t sumar_registros(){
  uint8_t suma = 0;
  for(uint8_t n=0; n < data_leng; n++){
    suma = suma + datos_matrix[n];
  }
  Serial.print("suma: "); Serial.println(suma,HEX);
  return suma;
}



  
