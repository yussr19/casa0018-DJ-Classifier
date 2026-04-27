#define EIDSP_QUANTIZE_FILTERBANK 0
#include <DJ_classifier_casa0018_inferencing.h>
#include <PDM.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define LED_COUNT 16
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static signed short sampleBuffer[2048];
static bool debug_nn = false;

static void pdm_data_ready_inference_callback(void)
{
    int bytesAvailable = PDM.available();
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);
    if (inference.buf_ready == 0) {
        for(int i = 0; i < bytesRead>>1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];
            if(inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
                break;
            }
        }
    }
}

static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));
    if(inference.buffer == NULL) { return false; }
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;
    PDM.onReceive(&pdm_data_ready_inference_callback);
    PDM.setBufferSize(4096);
    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM!");
        free(inference.buffer);
        return false;
    }
    PDM.setGain(127);
    return true;
}

static bool microphone_inference_record(void)
{
    inference.buf_ready = 0;
    inference.buf_count = 0;
    while(inference.buf_ready == 0) { delay(10); }
    return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;
}

void setup()
{
    Serial.begin(115200);
    strip.begin();
    strip.setBrightness(80);
    strip.fill(strip.Color(255, 255, 255));
    strip.show();
    delay(1000);
    strip.clear();
    strip.show();
    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Could not allocate audio buffer\r\n");
        return;
    }
}

void loop()
{
    bool m = microphone_inference_record();
    if (!m) { return; }
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) { return; }
    float maxVal = 0;
    int maxIdx = 0;
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("%s: %.5f\n", result.classification[i].label, result.classification[i].value);
        if (result.classification[i].value > maxVal) {
            maxVal = result.classification[i].value;
            maxIdx = i;
        }
    }
    if (maxIdx == 0) {
        strip.fill(strip.Color(0, 0, 255));
    } else if (maxIdx == 2) {
        strip.fill(strip.Color(255, 0, 0));
    } else {
        strip.fill(strip.Color(0, 255, 0));
    }
    strip.show();
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
