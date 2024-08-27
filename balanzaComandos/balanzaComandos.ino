#include "HX711.h"
#include <EEPROM.h>


// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 3;
const int LOADCELL_SCK_PIN = 2;

HX711 scale;

//Calibracion
const int MAX_CALIBRATION_POINTS = 10;
float calibrationWeights[MAX_CALIBRATION_POINTS];
float calibrationCounts[MAX_CALIBRATION_POINTS];
int calibrationPointIndex = 0;
bool calibrationInProgress = false;
int constante = 3500;


// Variables para el manejo de comandos
const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;
char command[numChars];
const char startMarker = '<';
const char endMarker = '>';
boolean continuousMode = false;
boolean intervalMode = false;
boolean requestMode = false;
boolean requestPreciseMode = false;
boolean requestPreciseModeStarted = false;
boolean stableMode = false;
boolean estable = false;
boolean recibioComando = false;
const int EEPROM_ADDR_CONST = 0;


unsigned long interval = 1000;  // Intervalo predeterminado de 1 segundo
unsigned long previousMillis = 0;
unsigned long preciseInterval = 0;
unsigned long preciseStartMillis = 0;
long sumPreciseData = 0;
int preciseDataCount = 0;
unsigned long stableStartMillis = 0;
const float stableThreshold = 1.0;      // Umbral de 1 gramo
const unsigned long stableTime = 3000;  // Tiempo de estabilización de 3 segundos
long stableSum = 0;
int stableCount = 0;
long lastStableReading = 0;
float tareValue = 0.0;  // Valor de la tara
float ultimoPromedio = 0;
bool zeroReported = false;

// Variables para las unidades
enum Units { REAL,
             COUNTS,
             BOTH };
Units currentUnit = REAL;

void setup() {
  Serial.begin(9600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  //saveConstantToEEPROM(EEPROM_ADDR_CONST, 0);

  constante = readConstantFromEEPROM(EEPROM_ADDR_CONST);
  if (constante == 0) {
    constante = 3500;  // Valor por defecto si no hay constante guardada
  }
  Serial.print("Constante de calibración recuperada: ");
  Serial.println(constante);

  scale.set_scale();
  delay(2000);
  scale.tare();
  while (!scale.is_ready()) {
    delay(10);
  }
  Serial.println("HX711 inicializado correctamente.");
}


/*GUARDAR Y TRAER CONSTANTES*/
void saveConstantToEEPROM(int address, float constant) {
  EEPROM.put(address, constant);
}

float readConstantFromEEPROM(int address) {
  float constant;
  EEPROM.get(address, constant);
  return constant;
}




void leerBalanza() {
  long readingCounts = scale.read();
  float readingReal = scale.get_units(10);
  float readingWithTare = readingReal - tareValue;

  switch (currentUnit) {
    case REAL:
      Serial.print("Resultado: ");
      Serial.print(readingWithTare / constante);
      Serial.print(" g (Peso real: ");
      Serial.print(readingReal / constante);
      Serial.println(" g)");
      break;
    case COUNTS:
      Serial.print("Resultado: ");
      Serial.println(readingCounts);
      break;
    case BOTH:
      Serial.print("Resultado: ");
      Serial.print(readingWithTare / constante);
      Serial.print(" g ; ");
      Serial.print(readingCounts);
      Serial.print(" (Peso real: ");
      Serial.print(readingReal / constante);
      Serial.println(" g)");
      break;
  }
}

bool esEstable() {
  const int numReadings = 10;
  long readings[numReadings];
  long sum = 0;
  for (int i = 0; i < numReadings; i++) {
    readings[i] = scale.get_units(1) / constante;
    sum += readings[i];
    delay(100);
  }

  long average = sum / numReadings;
  for (int i = 0; i < numReadings; i++) {
    if (abs(readings[i] - average) > stableThreshold) {
      return false;
    }
  }
  return true;
}

void leerBalanzaConEstabilidad() {
  if (!recibioComando) {
    estable = esEstable();
  } else {
    float readingReal = scale.get_units(1);
    float readingWithTare = readingReal - tareValue;
    switch (currentUnit) {
      case REAL:
        Serial.print("Resultado: ");
        Serial.print(readingWithTare / constante);
        Serial.print(" g (Peso real: ");
        Serial.print(readingReal / constante);
        Serial.print(" g) ");
        if (estable) {
          Serial.println("estable");
        } else {
          Serial.println("no estable");
        }
        break;
      case COUNTS:
        long readingCounts = scale.read();
        Serial.print("Resultado: ");
        Serial.print(readingCounts);
        if (estable) {
          Serial.println(" estable");
        } else {
          Serial.println(" no estable");
        }
        break;
      case BOTH:
        long readingCountsBoth = scale.read();
        Serial.print("Resultado: ");
        Serial.print(readingWithTare / constante);
        Serial.print(" g ; ");
        Serial.print(readingCountsBoth);
        Serial.print(" (Peso real: ");
        Serial.print(readingReal / constante);
        Serial.print(" g) ");
        if (estable) {
          Serial.println("estable");
        } else {
          Serial.println("no estable");
        }
        break;
    }
    recibioComando = false;
  }
}

void leerBalanzaARequerimiento() {
  float average = (float)sumPreciseData / preciseDataCount;
  float readingWithTare = average - tareValue;

  switch (currentUnit) {
    case REAL:
      Serial.print("Resultado promedio: ");
      Serial.print(readingWithTare / constante);
      Serial.print(" g (Peso real: ");
      Serial.print(average / constante);
      Serial.print(" g) ");
      break;
    case COUNTS:
      long readingCounts = scale.read();
      Serial.print("Resultado: ");
      Serial.print(readingCounts);
      break;
    case BOTH:
      long readingCountsBoth = scale.read();
      Serial.print("Resultado: ");
      Serial.print(readingWithTare / constante);
      Serial.print(" g ; ");
      Serial.print(readingCountsBoth);
      Serial.print(" (Peso real: ");
      Serial.print(average / constante);
      Serial.print(" g) ");

      break;
  }
  sumPreciseData = 0;
  preciseDataCount = 0;
  requestPreciseModeStarted = false;
}

void get_command() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char rc;
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (recvInProgress) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        receivedChars[ndx] = '\0';  // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    } else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}
void calcularCalibracionLineal() {
  if (calibrationPointIndex < 2) {
    Serial.println("Error: Se requieren al menos 2 puntos de calibración.");
    return;
  }

  float sumWeights = 0, sumCounts = 0;
  for (int i = 0; i < calibrationPointIndex; i++) {
    sumWeights += calibrationWeights[i];
    sumCounts += calibrationCounts[i];
  }
  // Calculamos la constante como la media de cuentas/peso
  float promedioWeights = sumWeights / calibrationPointIndex;
  float promedioCounts = sumCounts / calibrationPointIndex;
  constante = promedioCounts / promedioWeights;

  Serial.print("Calibración completada. Nueva constante de calibración: ");
  Serial.println(constante);
  // Guardar la constante en la EEPROM
  saveConstantToEEPROM(EEPROM_ADDR_CONST, constante);
  Serial.println("Constante de calibración guardada en EEPROM.");
}

void process_command() {
  if (newData) {
    strcpy(command, receivedChars);
    newData = false;
    if (strncmp(command, "MODE:CONT", 9) == 0) {
      continuousMode = true;
      intervalMode = false;
      requestMode = false;
      requestPreciseMode = false;
      stableMode = false;
      //Serial.println("Modo continuo activado.");
      if (command[9] == '-') {
        intervalMode = true;
        interval = atoi(&command[10]);
        Serial.print("Intervalo establecido en ");
        Serial.print(interval);
        Serial.println(" milisegundos.");
      }
    } else if (strcmp(command, "MODE:REQ") == 0) {
      continuousMode = false;
      intervalMode = false;
      requestMode = true;
      requestPreciseMode = false;
      stableMode = false;
      Serial.println("Modo a requerimiento activado.");
    } else if (strncmp(command, "MODE:REQ-", 9) == 0) {
      continuousMode = false;
      intervalMode = false;
      requestMode = false;
      requestPreciseMode = true;
      stableMode = false;
      preciseInterval = atoi(&command[9]);
      sumPreciseData = 0;    // Initialize sum
      preciseDataCount = 0;  // Initialize count
      Serial.print("Modo a requerimiento con precision activado. Intervalo establecido en ");
      Serial.print(preciseInterval);
      Serial.println(" milisegundos.");
    } else if (strcmp(command, "MODE:STB") == 0) {
      continuousMode = false;
      intervalMode = false;
      requestMode = false;
      requestPreciseMode = false;
      stableMode = true;
      Serial.println("Modo estable activado.");
    } else if (strcmp(command, "PARAR") == 0) {
      continuousMode = false;
      intervalMode = false;
      requestMode = false;
      requestPreciseMode = false;
      stableMode = false;
      requestPreciseModeStarted = false;
      Serial.println("Modo continuo, continuo con intervalo, a requerimiento y estable desactivados.");
    } else if (strcmp(command, "M") == 0) {
      if (requestMode) {
        recibioComando = true;
        leerBalanzaConEstabilidad();
      } else if (requestPreciseMode) {
        requestPreciseModeStarted = true;
        preciseStartMillis = millis();  // Start timing
      }
    } else if (strncmp(command, "UNITS:REAL", 10) == 0) {
      currentUnit = REAL;
      Serial.println("Unidades establecidas a gramos.");
    } else if (strncmp(command, "UNITS:COUNTS", 12) == 0) {
      currentUnit = COUNTS;
      Serial.println("Unidades establecidas a cuentas.");
    } else if (strncmp(command, "UNITS:BOTH", 10) == 0) {
      currentUnit = BOTH;
      Serial.println("Unidades establecidas a ambos (gramos y cuentas).");
    } else if (strcmp(command, "TARE") == 0) {
      tareValue = scale.get_units(10);
      Serial.print("Balanza tarada. Valor de tara: ");
      Serial.print(tareValue / constante);
      Serial.print(" g. Peso real: ");
      Serial.print(scale.get_units(10) / constante);
      Serial.println(" g.");
    } else if (strncmp(command, "STARTCAL", 8) == 0) {
      calibrationInProgress = true;
      calibrationPointIndex = 0;
      continuousMode = false;
      intervalMode = false;
      requestMode = false;
      requestPreciseMode = false;
      stableMode = false;
      requestPreciseModeStarted = false;
      Serial.println("Inicio de secuencia de calibración. Coloque un peso en la balanza y envíe el peso conocido en gramos.");
    } else if (strncmp(command, "WEIGHT:", 7) == 0 && calibrationInProgress) {
      float knownWeight = atof(&command[7]);
      float currentReading = scale.get_units(10);
      if (calibrationPointIndex < MAX_CALIBRATION_POINTS) {
        calibrationWeights[calibrationPointIndex] = knownWeight;
        calibrationCounts[calibrationPointIndex] = currentReading;
        calibrationPointIndex++;
        Serial.println(calibrationPointIndex);
        Serial.print("Peso conocido: ");
        Serial.print(knownWeight);
        Serial.print(" g, Cuentas registradas: ");
        Serial.println(currentReading);
        Serial.println("Coloque otro peso y envíe el peso conocido en gramos o envíe <ENDCAL> para finalizar.");
      } else {
        Serial.println("Error: Se ha alcanzado el número máximo de puntos de calibración.");
      }
    } else if (strcmp(command, "ENDCAL") == 0 && calibrationInProgress) {
      calcularCalibracionLineal();
      calibrationInProgress = false;
      Serial.println("Secuencia de calibración finalizada.");
    } else {
      Serial.println("Comando inválido.");
    }
  }
}

void verificarEstabilidad() {
  if (scale.is_ready()) {
    long currentReading = scale.get_units(1) / constante;
    stableSum += currentReading;
    stableCount++;
    if (millis() - stableStartMillis >= stableTime) {
      float average = (float)stableSum / stableCount;
      long mismaLectura = scale.get_units(1) / constante;
      if (currentReading == 0 || currentReading == mismaLectura) {
        lastStableReading = 0;
      }
      if (lastStableReading == 0) {
        lastStableReading = average;
      }
      long variation = abs(currentReading - lastStableReading);
      if (variation <= stableThreshold) {
        if (ultimoPromedio == 0 || ultimoPromedio != average) {
          zeroReported = false;
          if (average == 0) {
            zeroReported = true;
          }
          if (!zeroReported) {
            Serial.print("Lectura estable: ");
            Serial.print(average);
            Serial.println(" g");
          }
          ultimoPromedio = average;
          lastStableReading = currentReading;
        }
      }
      stableSum = 0;
      stableCount = 0;
      stableStartMillis = millis();
    }
  }
}

void devolverDatos() {
  long readingCounts = scale.read();
  float readingReal = scale.get_units(10);
  float readingWithTare = readingReal - tareValue;
  Serial.println(readingReal / constante);
}
void loop() {
  get_command();
  process_command();

  if (continuousMode) {
    unsigned long currentMillis = millis();
    if (intervalMode) {
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        leerBalanza();
      }
    } else {
      devolverDatos();
    }
  }

  if (requestMode) {
    leerBalanzaConEstabilidad();
  }

  if (requestPreciseModeStarted) {
    unsigned long currentMillis = millis();
    if (currentMillis - preciseStartMillis < preciseInterval) {
      sumPreciseData += scale.get_units(1);
      preciseDataCount++;
    } else {
      leerBalanzaARequerimiento();
    }
  }

  if (stableMode) {
    verificarEstabilidad();
  }
}