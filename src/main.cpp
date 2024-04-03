#include <Arduino.h>
#include <BluetoothSerial.h>
#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include <Adafruit_BMP085.h> // Biblioteca para el sensor BMP180

BluetoothSerial SerialBT;

// Definición de nombres para los pines
const int MQ138_Cetonas = 36;
const int MQ4_Metano = 39;
const int MQ3_Alcohol = 34;
const int MG811_CO2 = 35;
const int MQ135_Acetona = 32;
const int MQ137_Amoniaco = 33;

const int ledAzul = 2; // Pin del LED azul en el ESP32
const int pinPWM = 23; // Pin para la salida PWM

const int numSamples = 50; // Número de muestras para el promedio
const int pins[] = {MQ138_Cetonas, MQ4_Metano, MQ3_Alcohol, MG811_CO2, MQ135_Acetona, MQ137_Amoniaco}; // Pines a medir
const int numPins = sizeof(pins) / sizeof(pins[0]); // Número de pines a medir
const int intervaloMedicion = 2000; // Intervalo de medición en milisegundos
const float yo = 0.09317;
const float xc = 3.12095;
const float w1 = 0.43486;
const float C = 1.17273;

Adafruit_BMP085 bmp; // Objeto para el sensor BMP180

bool loopActivado = false; // Variable booleana para controlar el loop

void setup() {
  Serial.begin(9600); // Inicia la comunicación serial para debug
  SerialBT.begin("ESP32_BT"); // Inicia el puerto serie Bluetooth

  pinMode(ledAzul, OUTPUT); // Configura el pin del LED azul como salida
  pinMode(pinPWM, OUTPUT); // Configura el pin para la salida PWM

  digitalWrite(ledAzul, LOW); // Apaga inicialmente el LED azul
  analogWrite(pinPWM, 0); // Inicializa la salida PWM con un duty cycle de 0

  delay(1000); // Espera a que se establezca la conexión Bluetooth

  digitalWrite(ledAzul, HIGH); // Enciende el LED azul cuando se conecta a Bluetooth

  // Intenta inicializar el sensor BMP180 varias veces
  int intentos = 0;
  while (intentos < 5) { // Intenta 5 veces
    if (bmp.begin()) {
      SerialBT.println("Sensor BMP180 inicializado correctamente.");
      break; // Sale del bucle si se inicializa correctamente
    } else {
      SerialBT.println("Error al inicializar el sensor BMP180. Reintentando...");
      delay(500); // Espera 500 ms antes de intentarlo de nuevo
      intentos++;
    }
  }

  // Si no se pudo inicializar después de 5 intentos, imprime un mensaje de error y detiene el programa
  if (intentos >= 5) {
    SerialBT.println("No se pudo inicializar el sensor BMP180 después de varios intentos. Verifica la conexión.");
    }
}

void loop() {
  if (loopActivado) {
    // Activa la salida PWM durante el mismo tiempo que el intervalo de mediciones
    analogWrite(pinPWM, 255); // Establece el duty cycle al máximo
    delay(intervaloMedicion); // Espera el intervalo de mediciones
    analogWrite(pinPWM, 0); // Apaga la salida PWM

    // Realiza el resto de tus mediciones aquí...
    float sum[numPins] = {0}; // Arreglo para almacenar la suma de las tensiones de cada pin

    for (int j = 0; j < numSamples; j++) {
      for (int i = 0; i < numPins; i++) {
        int valorADC = analogRead(pins[i]); // Lee el valor analógico del pin
        float logADC = log10(valorADC);
        float coth_value = 1.0 / (tanh((logADC - xc) / w1)); // Calcula la aproximación de coth
        float logtension = yo + (C * (coth_value - (w1 / (logADC - xc)))); // Calcula el logaritmo base 10 de la tensión
        float tension = pow(10, logtension); // Convierte el valor a voltaje
        sum[i] += tension; // Agrega la tensión al total del pin i
      }
      delay(20); // Espera 20 milisegundos entre cada muestra
    }

    // Calcula el promedio de tensiones para cada pin
    for (int i = 0; i < numPins; i++) {
      float average_tension = sum[i] / numSamples; // Calcula el promedio de tensiones del pin i
      String mensaje = "Sensor ";
      switch(pins[i]) {
        case MQ138_Cetonas:
          mensaje += "MQ138 (Alcohol): ";
          break;
        case MQ4_Metano:
          mensaje += "MQ4 (Metano): ";
          break;
        case MQ3_Alcohol:
          mensaje += "MQ3 (Cetonas): ";
          break;
        case MG811_CO2:
          mensaje += "MG811 (Dióxido de Carbono): ";
          break;
        case MQ135_Acetona:
          mensaje += "MQ135 (Acetona): ";
          break;
        case MQ137_Amoniaco:
          mensaje += "MQ137 (Amoniaco): ";
          break;
        default:
          mensaje += "Pin no definido: ";
      }
      mensaje += String(average_tension) + "V\n";
      SerialBT.print(mensaje);
    }

    SerialBT.println("-----------------------");
  }

  // Espera un tiempo antes de revisar si se recibió un comando
  delay(100);

  // Verifica si se recibió un comando
  if (SerialBT.available()) {
    String comando = SerialBT.readStringUntil('\n'); // Lee el comando recibido por Bluetooth
    comando.trim(); // Elimina los espacios en blanco al principio y al final del comando

    // Verifica si el comando es "play" para activar el loop
    if (comando.equals("play")) {
      loopActivado = true; // Activa el loop
    } else if (comando.equals("stop")) { // Verifica si el comando es "stop" para detener el loop
      loopActivado = false; // Detiene el loop
    }
  }
}
