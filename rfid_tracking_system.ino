/*
   Projeto de Controle de Acesso com Arduino
   Autor: [Seu Nome]
   Data: [Data]

   Descrição:
   Este código é parte de um projeto de controle de acesso que utiliza um Arduino, um módulo RFID (MFRC522) e um sensor de movimento PIR.
   Ele permite a autenticação de cartões RFID e o envio de dados para uma API web para controle de acesso.

   Componentes necessários:
   - Arduino
   - Módulo RFID (MFRC522)
   - Sensor de Movimento PIR
   - Acesso à Internet via Wi-Fi

   Bibliotecas requeridas:
   - WiFi.h
   - HTTPClient.h
   - SPI.h
   - MFRC522.h

   Pinagem:
   - RST_PIN: Pino de redefinição do módulo RFID
   - SS_PIN: Pino de seleção do módulo RFID
   - PIR_SENSOR_PIN: Pino do sensor de movimento
   - MAX_OBJECTS: Limite de objetos permitidos
   - MOTION_TIMEOUT: Tempo em milissegundos para considerar que o objeto foi removido

   Configuração:
   - Configure as constantes 'ssid', 'password', 'apiEndpoint' e 'apiToken' com as informações corretas.

   Funcionamento:
   - O código verifica a presença de movimento no sensor PIR.
   - Se houver movimento, ele lê um cartão RFID usando o módulo MFRC522.
   - O ID do cartão RFID é enviado para uma API web definida em 'apiEndpoint' com autenticação por token.
   - O código rastreia o número de objetos autenticados e respeita o limite definido em 'MAX_OBJECTS'.
   - Ele também detecta quando um objeto é removido (sem movimento por 'MOTION_TIMEOUT' milissegundos).

   Notas:
   - Certifique-se de instalar as bibliotecas necessárias usando o Gerenciador de Bibliotecas da Arduino IDE.
   - Defina as informações de rede Wi-Fi e API corretamente.
   - Este código é uma base para um sistema de controle de acesso e pode ser estendido conforme necessário.

   Referências:
   - Biblioteca MFRC522: [URL da Biblioteca]
   - Documentação da Arduino: [URL da Documentação]

   Aviso:
   Este código é fornecido apenas como exemplo e não deve ser usado em aplicações críticas de segurança sem avaliação adequada.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9
#define SS_PIN          10
#define PIR_SENSOR_PIN  11 // Pino do sensor de movimento
#define MAX_OBJECTS     10 // Limite de objetos permitidos
#define MOTION_TIMEOUT  30000 // Tempo em milissegundos para considerar que o objeto foi removido (30 segundos neste exemplo)

MFRC522 mfrc522(SS_PIN, RST_PIN);
int objectCount = 0;
unsigned long lastMotionTime = 0;

const char *ssid = "NOME_DA_SUA_REDE_WIFI";
const char *password = "SENHA_DA_SUA_REDE_WIFI";
const char *apiEndpoint = "URL_DA_SUA_API_ENDPOINT";
const char *apiToken = "SEU_TOKEN_DE_AUTENTICACAO";

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(PIR_SENSOR_PIN, INPUT);

  // Conectar ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }

  Serial.println("Conectado ao WiFi");
}

void loop() {
  // Verifica se há movimento (objeto foi colocado)
  if (digitalRead(PIR_SENSOR_PIN) == HIGH) {
    lastMotionTime = millis(); // Atualiza o tempo do último movimento
    if (objectCount < MAX_OBJECTS) {
      // Se há espaço para mais objetos, leia o cartão RFID
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        String content = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
          content.concat(String(mfrc522.uid.uidByte[i], HEX));
        }

        // Enviar ID para a API
        sendToAPI(content);

        mfrc522.PICC_HaltA();
        objectCount++;
      }
    } else {
      // Se o limite de objetos for atingido, envie uma mensagem
      Serial.println("Limite de objetos atingido. Não é possível adicionar mais objetos.");
      delay(5000); // Aguarde 5 segundos para evitar múltiplas mensagens
    }
  } else {
    // Verifica se o objeto foi removido (não há movimento por um período de tempo)
    if (millis() - lastMotionTime > MOTION_TIMEOUT) {
      // Objeto foi removido
      if (objectCount > 0) {
        objectCount--;
        Serial.println("Objeto removido. Total de objetos: " + String(objectCount));
      }
      delay(5000); // Aguarde 5 segundos para evitar múltiplas mensagens
    }
  }
}

void sendToAPI(String content) {
  // Enviar dados para a API
  HTTPClient http;
  http.begin(apiEndpoint);

  // Configurar cabeçalhos da requisição
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(apiToken));

  // Montar o corpo da requisição JSON
  String jsonBody = "{\"rfid_id\":\"" + content + "\"}";

  // Enviar requisição para a API
  int httpCode = http.POST(jsonBody);

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Resposta da API: " + payload);
  } else {
    Serial.println("Falha na requisição HTTP");
  }

  http.end();