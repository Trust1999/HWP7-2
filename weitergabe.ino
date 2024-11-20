// Definition der Pins
const int inputPins[] = {2, 3, 4, 5}; // Eingabepins
const int outputPins[] = {6, 7, 8, 9}; // Ausgabepins

void setup() {
  // Konfiguration der Eingabepins
  for (int i = 0; i < 4; i++) {
    pinMode(inputPins[i], INPUT);
  }
  
  // Konfiguration der Ausgabepins
  for (int i = 0; i < 4; i++) {
    pinMode(outputPins[i], OUTPUT);
  }
}

void loop() {
  // Lesen der Eingabepins und Setzen der Ausgabepins
  for (int i = 0; i < 4; i++) {
    int value = digitalRead(inputPins[i]); // Signal einlesen
    digitalWrite(outputPins[i], value);   // Signal ausgeben
  }
}