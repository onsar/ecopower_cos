// master

/*
 * 
 * PROBLEMA: El esp12 recibe los datos por miso dependiendo del voltage de mosi
 * 
 * 
 * 
 * pasar de uint_16 a int a String en procesado de datos
 * 2 registros sin signo solo llegan a: -32,768 to 32,767
 * para alguna instalación puede ser insuficiente
 * 
 */



/*
 * Descubrimiento de esclavos
 * ===============================

 * estado = 0 inicio de la comunicacion       (sensores?)               B0   55   An  
 * estado = 1 confirmacion esclavo preparado  (final de comunicacion)   B8   An   55  
 * estado = 2 comprobacion, final de comunicacion                       B8   55   55
 * estado = 3 espera entre comuniaciones                                                            
 */


 /*
 * diagrama de estados del servidor
 * ===============================

 * estado = 0 inicio de la comunicacion       (sensores?)               B0   55   An  
 * estado = 1 confirmacion, cliente preparado (siguente registro)       B1   An   r1  
 * estado = 2 recepción de registros          (siguente registro)       B1   r1   r2
 *                                            (siguente registro)       B1   r2   r3
 *                                            (siguente registro)       B1   r3   rn
 *                                            (suma de registros)       B1   rn   suma
 * estado = 3 comprabar la trama recibida     (final de comunicacion)   B8   suma 55
 *                                                              
 * estado = 4 procesando y transmision de valores
 *            primer valor el de menor peso           registros_recibidos[0]
 *            segundo valor recibido el de mas peso   registros_recibidos[1]
 * Estado = 5 Reinicio y espera entre comuniaciones                     B8   ??   55
 */

#define DEBUG 1

#include <SPI.h>


// Datos de los esclavos
typedef struct { 
  int pin_seleccion;
  int numero_sensores;
  char nombres_sesores[7][4]; // 3 + el final de string
} esclavo;

esclavo es1= {D0,0,"p0","p1","p2","p3","p4","p5","v0"};
esclavo es2= {D1,0,"p10","p11","p12","p13","p14","p15","p16"};
esclavo es3= {D2,0,"p20","p21","p22","p23","p24","p25","p26"};
esclavo es4= {D8,0,"p30","p31","p32","p33","p34","p35","p36"};

esclavo* esclavos_[] = {&es1, &es2, &es3, &es4};

// control de los esclavos
const int max_esclavos = 4;
int numero_esclavos;              // esclavos reales conectados
int posicion_esclavo;             // posicion del esclavo seleccionado

// control de los registros
char registros_recibidos[14];     // matriz para almacenar los registros recibidos
uint8_t registros_esclavo;        // numero de registros del dispositivo slave (recibido)
uint8_t registros_pendientes;     // registros pendientes de recibir desde el esclavo
uint8_t registros_suma;           // suma de los registros recibidos para comprobar

// Cotrol del estado
uint8_t estado;

//Control de la división de tiempos
uint32_t t_last_tx;               // tiempo de la ultima transmision de datos

void setup() {
  Serial.begin(9600);
  Serial.println ("Starting");
  spi_setup();
}

void loop() {

  spi_loop();

} // fin loop

uint8_t readRegister(uint8_t b, int e) { // b=byte a transmitir e= esclavo
  
  uint8_t result = 0;
  // digitalWrite(esclavos_[e]->pin_seleccion, LOW);
  // delay(10);
  result = SPI.transfer(b); // (unsigned int)
  // delay(10);
  // digitalWrite(esclavos_[e]->pin_seleccion, HIGH);
  // delay(5);
  return (result);
}

void transmision(String this_string){
  Serial.println("valor a transmitir: ");
  Serial.println(this_string);
}

void reset_seleccion_esclavo(){
  for(int n= 0; n < max_esclavos; n++){
    digitalWrite(esclavos_[n]->pin_seleccion, HIGH); 
  }   
}

void spi_setup(){
  
  SPI.begin();

  SPI.setClockDivider(SPI_CLOCK_DIV8);  // 2 MHz

  // pines de selección (SS) a 5V
  for(int n= 0; n < max_esclavos; n++){
    pinMode(esclavos_[n]->pin_seleccion, OUTPUT);
  } 
  reset_seleccion_esclavo();  
  delay(1000);  
  
  
  // DETECTAR LOS ESCLAVOS CONECTADOS
  
  numero_esclavos = 0;
  
  for(int n= 0; n < max_esclavos; n++){  
    uint8_t registro_leido = 0x00; 
    reset_seleccion_esclavo();
    digitalWrite(esclavos_[n]->pin_seleccion, LOW); 
    delay(50);

    registro_leido = readRegister(0xB0, n);
    if(DEBUG) {
      Serial.print("inicio descubrimiento, B0 registro: "); 
      Serial.println(registro_leido, HEX);
    }
    delay(10);
    registro_leido = readRegister(0xB8, n);
    if(((registro_leido & 0xF0) == 0xA0)&&((registro_leido & 0x0F) >= 2)){
      esclavos_[n]->numero_sensores = registro_leido & 0x0F; 
      if(DEBUG){
        Serial.print("esclavo preparado, B8 registro: "); 
        Serial.println(registro_leido, HEX);
      }
    }
    else{
      if(DEBUG){
        Serial.print("esclavo NO-preparado, registro_leido: "); 
        Serial.println(registro_leido, HEX);
      }
      break;
    }
    delay(10);
    registro_leido = readRegister(0xB8, n);
    if(DEBUG){
      Serial.print("reset esclavo, B8 registro_leido: "); 
      Serial.println(registro_leido, HEX);
    }
    if (registro_leido == 0x55){
      numero_esclavos = n+1;  
    }
    else{
      break; 
    }
  }

  Serial.print("ESCALVOS DETECTADOS: ");  Serial.println(numero_esclavos);

  // CONFIGURACION DE LAS VARIABLES INICIALES

  posicion_esclavo = numero_esclavos; // Se iniciará con el esclavo 0
  registros_esclavo = 0; 
  t_last_tx=0;
  estado = 5;   
}



void spi_loop(){
  
  // EJECUCION EN BASE A TIEMPOS
  uint32_t current_time= millis();  
  if (current_time < t_last_tx) t_last_tx=0;         // para el desbordamiento de millis()
  if (current_time - t_last_tx > 10000){             // inicio de la lectura del esclavo
    t_last_tx = current_time;

    // Un esclavo cada vez
    posicion_esclavo++;                              
    if ( posicion_esclavo > (numero_esclavos-1)) {posicion_esclavo=0;} 
    reset_seleccion_esclavo();
    digitalWrite(esclavos_[posicion_esclavo]->pin_seleccion, LOW); 
    delay(10);

    // inicio de los estados
    estado = 0;

    // configuracion inicial de los registros
    registros_esclavo = 0;
    for(int n=0; n<14; n++){registros_recibidos[n] = 0x00;}   
    registros_pendientes = 14;
    registros_suma = 0;

    if(DEBUG){
      Serial.print(F("******inicio comunicaciones estado=0 sgs: "));
      Serial.println(millis() / 1000);
      Serial.print("freeHeap: "); Serial.println(ESP.getFreeHeap()); 
      Serial.print("esclavo: ");  Serial.println(posicion_esclavo);  
    } 
  }

  if(estado == 0){  //inicio de la comunicacion
    uint8_t registro_leido = 0x00;
    registro_leido = readRegister(0x02, posicion_esclavo); 
    registros_esclavo =14;
    
    estado = 2;  
    if(DEBUG) {Serial.print(registro_leido, HEX);Serial.print("  Estado 0 -> Estado "); Serial.println(estado);}
    // delay(10);
    return;
  } 
  
  if(estado == 2){  //recepcion de registros
    
    if(registros_pendientes > 0){  
      uint8_t registro_leido = readRegister(0x12, posicion_esclavo);
      uint8_t registro_orden = registros_esclavo - registros_pendientes;
      if(registro_leido == 0x0A){
          estado = 3;
      }
      else estado = 2;
      registros_recibidos[registro_orden] = registro_leido;    
      registros_pendientes = registros_pendientes-1;
      registros_suma = registros_suma + registro_leido;
      
      if(DEBUG){
        Serial.print("iniciada, registros_recibidos[]: "); Serial.print(registro_orden);
        Serial.print("  registro: "); Serial.println(registro_leido,HEX);
        Serial.print("registros_suma: "); Serial.println(registros_suma,HEX);
      }
    } 
    else {
      estado = 5;
      if(DEBUG) {Serial.print("Estado 2 -> Estado "); Serial.println(estado);}
    }
    return;
  }

  if(estado == 3){  //comprabar la trama recibida con final de trama: registros_suma
    
    uint8_t registro_leido = readRegister(0x03, posicion_esclavo);
    if(registro_leido == registros_suma){
      estado = 4;
      if(DEBUG) {
        Serial.print("Estado 3 -> Estado: "); Serial.print(estado);Serial.print("  trama correcta ");
        Serial.print("  registro_leido: ");Serial.println((registro_leido), HEX);     
      }      
    }
    else{
      estado=5;
      if(DEBUG) {
        Serial.print("Estado 3 -> Estado: "); Serial.print(estado); Serial.print("trama IN-correcta ");
        Serial.print("  registro_leido: ");Serial.println((registro_leido), HEX);     
      }       
    }
    return;
  }

  if(estado == 4){  //procesando y transmision de valores
    estado=5;
    if(DEBUG) {
      Serial.print("Estado 4 -> Estado: "); Serial.println(estado); 
      Serial.print("cadena: ");Serial.println(registros_recibidos);     
    }   
    return; 

  } 
  if(estado == 5){  //procesando y transmision de valores
    estado=6;
    uint8_t registro_leido = readRegister(0x03, posicion_esclavo);
    if(DEBUG) {
      Serial.println("Estado: "); Serial.print(estado);   
    } 
  }
 
}//fin spi_loop 
