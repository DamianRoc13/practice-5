#include <WiFi.h>
#include <WiFiClientSecure.h> 
#include <DHT.h>
#include <ESP32Servo.h>

const char* ssid = "Suda";
const char* password = "";

#define BOT_TOKEN "8176945771:AAGwzA0VlNBIlxVE6Zp2Da_3fZ4jDtwSTkg"
#define CHAT_ID "1848043793" 

const int trigPin1 = 13;  // Entrance
const int echoPin1 = 12;  
const int trigPin2 = 27;  // Exit
const int echoPin2 = 26;  

const int servoPin = 23;

const int buzzerPin = 33;
const int ledPin = 4;

#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClientSecure client;

int vehicleCount = 0;
int vehicleEntered = 0;
int vehicleExited = 0;

Servo myServo;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 20000; 

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando al WiFi...");
  }
  Serial.println("WiFi conexión establecida");

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  dht.begin();

  myServo.attach(servoPin);
  myServo.write(0); 

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
}

long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); 
  return (duration > 0) ? (duration / 2) / 29.1 : -1;  
}

void loop() {
  long distanciaEntrada = medirDistancia(trigPin1, echoPin1);
  long distanciaSalida = medirDistancia(trigPin2, echoPin2);

  if (distanciaEntrada > 0 && distanciaEntrada < 10) {
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

  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  if (temperatura > 28.0) {
    digitalWrite(buzzerPin, HIGH); 
    digitalWrite(ledPin, HIGH);    
    sendTemperatureAlert(temperatura);  
  } else {
    digitalWrite(buzzerPin, LOW);  
    digitalWrite(ledPin, LOW);     
  }

  // Temporizador para enviar actualizaciones
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendTelegramMessage(temperatura, humedad);
  }
}

// Función enviar mensajes a Telegram
void sendTelegramMessage(float temperatura, float humedad) {
  String message = "Número de vehículos en el parqueadero: " + String(vehicleCount) + "\n";
  message += "Entradas: " + String(vehicleEntered) + "\n";
  message += "Salidas: " + String(vehicleExited) + "\n";
  message += "Temperatura: " + String(temperatura) + " °C\n";
  message += "Humedad: " + String(humedad) + " %\n";

  enviarMensajeTelegram(message);
}

// Función alerta de temperatura alta
void sendTemperatureAlert(float temperatura) {
  String alertMessage = "⚠️ ¡ALERTA DE TEMPERATURA ALTA! ⚠️\n";
  alertMessage += "La temperatura actual es: " + String(temperatura) + " °C\n";
  alertMessage += "Por favor, tome las medidas necesarias.";
  enviarMensajeTelegram(alertMessage);
}

// Función enviar mensajes 
void enviarMensajeTelegram(String message) {
  Serial.println("Mensaje enviado a Telegram:");
  Serial.println(message);

  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + message;

  client.setInsecure();  // Desactiva la verificación del certificado SSL
  if (client.connect("api.telegram.org", 443)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "Connection: close\r\n\r\n");

    // Esperar y leer la respuesta del servidor
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);  // respuesta del servidor
    }
    Serial.println("Mensaje enviado correctamente!");
  } else {
    Serial.println("Fallo al conectarse con Telegram.");
  }
  client.stop(); 
}
