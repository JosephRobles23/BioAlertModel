#include <Arduino.h>
#include "SPIFFS.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Configuración del micrófono MAX4466
const int sampleWindow = 50;  // Ventana de muestreo en ms
const int AMP_PIN = 34;       // Pin del micrófono (ADC1_CHANNEL6)

// Configuración de TensorFlow Lite
constexpr int kTensorArenaSize = 2 * 1024; // Memoria para tensores
uint8_t tensor_arena[kTensorArenaSize];
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

void setup() {
    Serial.begin(115200);

    // Inicializar SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Error al montar el sistema de archivos SPIFFS");
        while (true);
    }

    // Leer el modelo desde SPIFFS
    File modelFile = SPIFFS.open("/modelo_esp32.tflite", "r");
    if (!modelFile) {
        Serial.println("Error al abrir el archivo del modelo");
        while (true);
    }

    // Cargar el modelo en memoria
    size_t modelSize = modelFile.size();
    uint8_t* modelData = (uint8_t*)malloc(modelSize);
    if (!modelData) {
        Serial.println("Error al asignar memoria para el modelo");
        while (true);
    }
    modelFile.read(modelData, modelSize);
    modelFile.close();

    // Configurar el modelo TensorFlow Lite
    model = tflite::GetModel(modelData);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("Versión del modelo incompatible con TensorFlow Lite Micro.");
        while (true);
    }

    // Resolver las operaciones del modelo
    static tflite::AllOpsResolver resolver;

    // Configurar el intérprete
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // Asignar tensores
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("Error al asignar tensores");
        while (true);
    }

    // Obtener tensores de entrada y salida
    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("Modelo cargado correctamente. Listo para inferencias.");
}

void loop() {
    unsigned long startMillis = millis();
    unsigned int signalMax = 0;
    unsigned int signalMin = 4095;
    unsigned int sample;

    // Leer datos del micrófono durante la ventana de muestreo
    while (millis() - startMillis < sampleWindow) {
        sample = analogRead(AMP_PIN);
        if (sample < 4095) {  // Filtrar lecturas espurias
            if (sample > signalMax) {
                signalMax = sample;
            }
            if (sample < signalMin) {
                signalMin = sample;
            }
        }
    }

    // Calcular la amplitud pico a pico
    unsigned int peakToPeak = signalMax - signalMin;

    // Normalizar la entrada para el modelo (entre 0 y 1)
    float normalizedValue = (float)peakToPeak / 4095.0;

    // Alimentar el modelo con la entrada
    input->data.f[0] = normalizedValue;

    // Ejecutar la inferencia
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("Error al ejecutar la inferencia");
        return;
    }

    // Leer el resultado de la inferencia
    float output0 = output->data.f[0];  // Primera salida del modelo
    float output1 = output->data.f[1];  // Segunda salida del modelo

    // Mostrar resultados
    Serial.print("Salida 0: ");
    Serial.println(output0);
    Serial.print("Salida 1: ");
    Serial.println(output1);

    // Evaluar el resultado y tomar decisiones
    if (output0 > output1) {
        Serial.println("Predicción: No es una motosierra");
    } else {
        Serial.println("Predicción: Es una motosierra");
    }

    delay(100);  // Esperar antes del próximo ciclo
}
