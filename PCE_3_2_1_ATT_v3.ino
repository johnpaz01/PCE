#include <SFE_BMP180.h>
#include <Wire.h>
#include <EEPROM.h>
#include <MPU6050.h>

SFE_BMP180 pressure;
MPU6050 mpu;

#define Skib 12
#define Verde 7
#define MAX_ALTITUDE_ADDRESS 0

int mem_pos = 0;
double baseline;
double maxAltitude = -999;
bool armado = true;
int cont = 0;
int jump = 10;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(Skib, OUTPUT);
  pinMode(Verde, OUTPUT);

  Serial.println("Preparando...");
  digitalWrite(Verde, HIGH);
  delay(1000);
  digitalWrite(Verde, LOW);

  // Inicializa BMP180
  if (pressure.begin()) {
    Serial.println("BMP180 init success");
    for (int i = 0; i < 33; i++) {
      digitalWrite(Verde, HIGH);
      delay(70);
      digitalWrite(Verde, LOW);
      delay(70);
      Serial.println("Acendi! e Desliguei");
    }
  } else {
    Serial.println("BMP180 init fail (disconnected?)");
    digitalWrite(Verde, HIGH);
    while (1);
  }

  // Inicializa MPU6050
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
    for (int i = 0; i < 33; i++) {
      digitalWrite(Verde, HIGH);
      delay(70);
      digitalWrite(Verde, LOW);
      delay(70);
    }
  } else {
    Serial.println("MPU6050 connection failed");
    digitalWrite(Verde, HIGH);
    while (1);
  }

  // Obtém a pressão de referência
  baseline = getPressure();
  if (baseline != -1) {
    Serial.print("baseline pressure: ");
    Serial.print(baseline);
    Serial.println(" mb");
  } else {
    Serial.println("Erro ao obter a pressão de referência");
  }
  Serial.println("indo pro loop:");
  digitalWrite(Verde, LOW);

  maxAltitude = 0.0;
}

void loop() {
  int16_t ax, ay, az;
  double a, P;
  
  P = getPressure();
  if (P != -1) {
    a = pressure.altitude(P, baseline);
  } else {
    a = NAN;
    Serial.println("Erro ao obter a pressão atual");
  }

  mpu.getAcceleration(&ax, &ay, &az);

  // Convertendo para g's (gravidade terrestre)
  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  digitalWrite(Verde, HIGH);
  Serial.print("relative altitude: ");
  if (a >= 0.0) Serial.print(" ");
  Serial.print("Aceleracao em X: "); Serial.print(ax_g); Serial.println(" g");
  Serial.print("Aceleracao em Y: "); Serial.print(ay_g); Serial.println(" g");
  Serial.print("Aceleracao em Z: "); Serial.print(az_g); Serial.println(" g");
  Serial.println();
  if (!isnan(a)) {
    Serial.print(a, 1);
    Serial.print(" meters, ");
    if (a >= 0.0) Serial.print(" ");
    Serial.print(a * 3.28084, 0);
    Serial.println(" feet");
  } else {
    Serial.println("nan meters, nan feet");
  }

  if (a > 3.0 && (ax_g >= 1.0 || ay_g >= 1.0 || az_g >= 1.0)) {
    cont++;
    if (cont % jump == 0 && mem_pos <= 1000) {
      mem_pos += sizeof(a);
      saveAltitudeToEEPROM(a);
      if (a > maxAltitude) {
        maxAltitude = a;
        saveMaxAltitudeToEEPROM(maxAltitude);
      }
    }
  }

  // Verifica se o foguete está em queda e se a aceleração é maior que 1.2 g em qualquer eixo
  if ((a <= maxAltitude - 5.0) && armado && (ax_g >= 1.0 || ay_g >= 1.0 || az_g >= 1.0)) {
    Serial.print("Aceleracao em X: "); Serial.print(ax_g); Serial.println(" g");
    Serial.print("Aceleracao em Y: "); Serial.print(ay_g); Serial.println(" g");
    Serial.print("Aceleracao em Z: "); Serial.print(az_g); Serial.println(" g");
    Serial.println();
    Serial.println("Consegui: ");
    Serial.print("A: ");
    Serial.println(a);
    Serial.print("Max altitude: ");
    Serial.println(maxAltitude);
    digitalWrite(Verde, HIGH);
    ativaSkip();
    armado = false;
    digitalWrite(Verde, HIGH);
    delay(1000);
    digitalWrite(Verde, LOW);
    delay(2000);
    digitalWrite(Verde, HIGH);
  }
}

double getPressure() {
  char status;
  double T, P;
  
  status = pressure.startTemperature();
  if (status != 0) {
    delay(status);
    status = pressure.getTemperature(T);
    if (status != 0) {
      status = pressure.startPressure(3);
      if (status != 0) {
        delay(status);
        status = pressure.getPressure(P, T);
        if (status != 0) {
          return (P);
        } else {
          Serial.println("error retrieving pressure measurement");
        }
      } else {
        Serial.println("error starting pressure measurement");
      }
    } else {
      Serial.println("error retrieving temperature measurement");
    }
  } else {
    Serial.println("error starting temperature measurement");
  }
  return -1; // Adicione um valor de retorno padrão em caso de falha
}

void ativaSkip() {
  digitalWrite(Skib, HIGH);
  delay(2000);
  digitalWrite(Skib, LOW);
}

void saveMaxAltitudeToEEPROM(double maxAltitude) {
  byte* p = (byte*)(void*)&maxAltitude;
  for (int i = 0; i < sizeof(maxAltitude); i++) {
    EEPROM.write(MAX_ALTITUDE_ADDRESS + i, *p++);
  }
}

void saveAltitudeToEEPROM(double Altitude) {
  byte* p = (byte*)(void*)&Altitude;
  for (int i = 0; i < sizeof(Altitude); i++) {
    EEPROM.write(mem_pos + i, *p++);
  }
}
