#include <WiFi.h>
#include <WiFiClientSecure.h>  // Usamos WiFiClientSecure para HTTPS
#include <DHT.h>
#include <ESP32Servo.h>

// WiFi configuration
const char* ssid = "Dr. Avila";
const char* password = "AviCam2piso";

// Telegram bot token and chat ID
#define BOT_TOKEN "8176945771:AAGwzA0VlNBIlxVE6Zp2Da_3fZ4jDtwSTkg"
#define CHAT_ID "1848043793"  // Replace with your chat ID

// Ultrasonic sensor pins
const int trigPin1 = 13;  // Sensor 1 (Entrance)
const int echoPin1 = 12;  // Sensor 1 (Entrance)
const int trigPin2 = 27;  // Sensor 2 (Exit)
const int echoPin2 = 26;  // Sensor 2 (Exit)

// Servo motor pin
const int servoPin = 23;

// Buzzer and air quality LED pins
const int buzzerPin = 33;
const int ledPin = 4;

// DHT sensor configuration
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Telegram client for secure connection
WiFiClientSecure client;

// Vehicle count variables
int vehicleCount = 0;
int vehicleEntered = 0;
int vehicleExited = 0;

// Servo motor object
Servo myServo;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 20000;  // 20 seconds

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando al WiFi...");
  }
  Serial.println("WiFi conexión establecida");

  // Configure ultrasonic sensor pins
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Start the DHT sensor
  dht.begin();

  // Attach the servo motor
  myServo.attach(servoPin);
  myServo.write(0);  // Initial servo position (closed)

  // Configure buzzer and air quality LED
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
}

// Function to measure distance with ultrasonic sensor
long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);  // Timeout to avoid blocking
  return (duration > 0) ? (duration / 2) / 29.1 : -1;  // Return -1 if no measurement
}

void loop() {
  // Leer distancia del sensor de entrada y salida
  long distanciaEntrada = medirDistancia(trigPin1, echoPin1);
  long distanciaSalida = medirDistancia(trigPin2, echoPin2);

  if (distanciaEntrada > 0 && distanciaEntrada < 10) { // Ajustar rango válido
    vehicleCount++;
    vehicleEntered++;
    myServo.write(90);
    delay(2000);
    myServo.write(0);
    Serial.println("Entrada de vehículo detectada.");
  }

  if (distanciaSalida > 0 && distanciaSalida < 10 && vehicleCount > 0) {
    vehicleCount--;
    vehicleExited++;
    myServo.write(90);
    delay(2000);
    myServo.write(0);
    Serial.println("Salida de vehículo detectada.");
  }

  // Leer temperatura y humedad
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Verificar si la temperatura excede 28 °C
  if (temperatura > 28.0) {
    digitalWrite(buzzerPin, HIGH);  // Encender el buzzer
    digitalWrite(ledPin, HIGH);    // Encender el LED
    sendTemperatureAlert(temperatura);  // Enviar alerta a Telegram
  } else {
    digitalWrite(buzzerPin, LOW);   // Apagar el buzzer
    digitalWrite(ledPin, LOW);     // Apagar el LED
  }

  // Temporizador para enviar actualizaciones regulares
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendTelegramMessage(temperatura, humedad);
  }
}

// Función para enviar mensajes regulares a Telegram
void sendTelegramMessage(float temperatura, float humedad) {
  String message = "Número de vehículos en el parqueadero: " + String(vehicleCount) + "\n";
  message += "Entradas: " + String(vehicleEntered) + "\n";
  message += "Salidas: " + String(vehicleExited) + "\n";
  message += "Temperatura: " + String(temperatura) + " °C\n";
  message += "Humedad: " + String(humedad) + " %\n";

  enviarMensajeTelegram(message);
}

// Función para enviar una alerta de temperatura alta a Telegram
void sendTemperatureAlert(float temperatura) {
  String alertMessage = "⚠️ ¡ALERTA DE TEMPERATURA ALTA! ⚠️\n";
  alertMessage += "La temperatura actual es: " + String(temperatura) + " °C\n";
  alertMessage += "Por favor, tome las medidas necesarias.";
  enviarMensajeTelegram(alertMessage);
}

// Función para enviar mensajes genéricos a Telegram
void enviarMensajeTelegram(String message) {
  Serial.println("Mensaje enviado a Telegram:");
  Serial.println(message);

  // Formar la URL para la solicitud de la API de Telegram
  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + message;

  // Conectar con el servidor de Telegram
  client.setInsecure();  // Desactiva la verificación del certificado SSL
  if (client.connect("api.telegram.org", 443)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "Connection: close\r\n\r\n");

    // Esperar y leer la respuesta del servidor
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);  // Imprimir la respuesta del servidor
    }
    Serial.println("Mensaje enviado correctamente!");
  } else {
    Serial.println("Fallo al conectarse con Telegram.");
  }
  client.stop();  // Cerrar la conexión
}
