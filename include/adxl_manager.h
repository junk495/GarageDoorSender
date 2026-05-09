#ifndef ADXL_MANAGER_H
#define ADXL_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "Esp32C3_DeepSleep.h"

// --- DEBUG-MODUS ---
#define DEBUG_MODE                  true

// --- KONFIGURATION ---
#define SENSOR_ID 12345
#define WAKEUP_PIN GPIO_NUM_2 // Wichtig: Muss an INT2 des ADXL angeschlossen sein

// --- GLOBALE VARIABLEN ---
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(SENSOR_ID);

// --- DIAGNOSEFUNKTION ---
inline void printADXLRegisters(uint8_t activityThreshold) {
  #ifdef DEBUG_MODE
    Serial.println("--- ADXL345 Register-Dump (Alle Achsen) ---");
    Serial.printf("DEVID:          0x%02X (sollte 0xE5)\n", accel.readRegister(ADXL345_REG_DEVID));
    Serial.printf("POWER_CTL:      0x%02X (sollte 0x0C)\n", accel.readRegister(ADXL345_REG_POWER_CTL));
    Serial.printf("INT_ENABLE:     0x%02X (sollte 0x10)\n", accel.readRegister(ADXL345_REG_INT_ENABLE));
    Serial.printf("INT_MAP:        0x%02X (sollte 0x10)\n", accel.readRegister(ADXL345_REG_INT_MAP));
    Serial.printf("ACT_INACT_CTL:  0x%02X (sollte 0x70)\n", accel.readRegister(ADXL345_REG_ACT_INACT_CTL));
    Serial.printf("THRESH_ACT:     0x%02X (ist %d)\n", accel.readRegister(ADXL345_REG_THRESH_ACT), activityThreshold);
    Serial.printf("DATA_FORMAT:    0x%02X (sollte 0x2B sein)\n", accel.readRegister(ADXL345_REG_DATA_FORMAT));
    Serial.println("----------------------------------------------------");
  #endif
}


// Konfiguriert den ADXL345 für den manuellen Low-Power Schlafmodus
inline void configureADXL345(uint8_t activityThreshold) {
  if (!accel.begin()) {
    #ifdef DEBUG_MODE
    Serial.println("ADXL345 nicht gefunden! Verkabelung prüfen.");
    #endif
    while (1) delay(10);
  }
  
  // Sensor in Standby versetzen für die Konfiguration
  accel.writeRegister(ADXL345_REG_POWER_CTL, 0x00);
  delay(5);

  accel.setRange(ADXL345_RANGE_16_G);
  accel.setDataRate(ADXL345_DATARATE_100_HZ);

  // 1. Den übergebenen Aktivitätsschwellenwert setzen.
  accel.writeRegister(ADXL345_REG_THRESH_ACT, activityThreshold);

  // 2. *** HIER IST DIE ÄNDERUNG ***
  //    Aktivitätserkennung auf allen drei Achsen (X, Y, Z) aktivieren.
  //    DC-gekoppelt (vergleicht mit einem festen Schwellenwert).
  //    Register ACT_INACT_CTL (0x27) -> 0x70 (01110000)
  uint8_t act_inact_ctl = 0;
  act_inact_ctl |= (1 << 6); // ACT_X enable
  act_inact_ctl |= (1 << 5); // ACT_Y enable
  act_inact_ctl |= (1 << 4); // ACT_Z enable
  accel.writeRegister(ADXL345_REG_ACT_INACT_CTL, act_inact_ctl);
  
  // 3. Nur den ACTIVITY-Interrupt aktivieren.
  accel.writeRegister(ADXL345_REG_INT_ENABLE, 0x10);
  
  // 4. Den ACTIVITY-Interrupt auf den INT2-Pin mappen.
  accel.writeRegister(ADXL345_REG_INT_MAP, 0x10);

  // 5. Setze die Interrupt-Polarität auf "active LOW".
  uint8_t data_format = accel.readRegister(ADXL345_REG_DATA_FORMAT);
  data_format |= 0b00100000; // Bit 5 = INT_INVERT -> active LOW
  accel.writeRegister(ADXL345_REG_DATA_FORMAT, data_format);
  
  // 6. Interrupt-Quelle löschen, um sofortigen Trigger zu vermeiden
  accel.readRegister(ADXL345_REG_INT_SOURCE);

  // 7. Power-Modus setzen (Measure + Sleep)
  accel.writeRegister(ADXL345_REG_POWER_CTL, 0x0C);

  #ifdef DEBUG_MODE
  Serial.println("ADXL345 für manuellen Schlafmodus (active LOW) konfiguriert.");
  printADXLRegisters(activityThreshold);
  #endif
}

// Initialisiert den Weck-Pin für den Tiefschlaf am ESP32
inline void setupADXLWakeup() {
  Esp32C3_DeepSleep::addWakeupPin(WAKEUP_PIN, false);
}

// Liest und löscht den Interrupt-Status des ADXL345
inline void clearADXLInterrupt() {
  // Das Lesen des INT_SOURCE Registers löscht alle anstehenden Interrupts.
  accel.readRegister(ADXL345_REG_INT_SOURCE);
}

#endif // ADXL_MANAGER_H
