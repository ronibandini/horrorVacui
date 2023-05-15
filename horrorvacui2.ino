/* 
Horror Vacui
Ejemplo de Machine Learning
UCA Rosario Mayo 2023
Roni Bandini

Oled screen https://www.dfrobot.com/product-2019.html SDA y SCL to pins A4 and A5, VCC and GND
*/

#define EIDSP_QUANTIZE_FILTERBANK   0
#include <PDM.h>
#include <Horror_Vacui_inferencing.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>


U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  

/** Audio buffers, pointers and selectors */
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static signed short sampleBuffer[2048];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

int myCounter=0;
float lastInference=0;
float eeeInference=0;
float ambientInference=0;
float eeeLimit=0.95;
float ambientLimit=0.2;

void setup()
{


  
    Serial.begin(115200);  
    
    u8g2.begin();    
    u8g2.enableUTF8Print();    
    u8g2.setFont(u8g2_font_unifont_t_chinese2);   
    u8g2.setFontDirection(0);  
    u8g2.firstPage();
 
    do {
      u8g2.clear();
      u8g2.setCursor(/* x=*/0, /* y=*/15);    
      u8g2.print("Horror Vacui");
      u8g2.setCursor(/* x=*/0, /* y=*/30); 
      u8g2.print("@RoniBandini");      
    } while ( u8g2.nextPage() );
    
    delay(2000);
    
    Serial.println("Horror Vacui");
    Serial.println("Roni Bandini Mayo 2023");

    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }
}


void loop()
{
    ei_printf("Inferencing in 2 seconds...\n");
   Serial.println("Counter: " + String(myCounter));

   do {
    u8g2.clear();
    u8g2.setCursor(/* x=*/0, /* y=*/15);    
    u8g2.print("Horror Vacui");
    u8g2.setCursor(/* x=*/0, /* y=*/30); 
    u8g2.print("Esperando...");
    u8g2.setCursor(0, 30);  
    } while ( u8g2.nextPage() );

    delay(2000);

    ei_printf("Recording...\n");

    do {
    u8g2.clear();
    u8g2.setCursor(/* x=*/0, /* y=*/15);    
    u8g2.print("Eee: "+String(lastInference)+"%");
    u8g2.setCursor(/* x=*/0, /* y=*/30); 
    u8g2.print("Contador: "+String(myCounter));
    u8g2.setCursor(0, 30);  
    } while ( u8g2.nextPage() );

    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    ei_printf("Recording done\n");

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);

        lastInference=(result.classification[ix].value)*100;
        
        
        if (result.classification[ix].label=="eee"){
            eeeInference=result.classification[ix].value;
            
        }

         if (result.classification[ix].label=="ambiente"){
            ambientInference=result.classification[ix].value;
            
        }
          
    }

    if (eeeInference>eeeLimit and ambientInference<ambientLimit){
      ei_printf("Sumando contador");
      Serial.println(String(eeeInference));
      Serial.println(String(ambientInference));
      myCounter++;  

      do {
      u8g2.clear();
      u8g2.setCursor(/* x=*/0, /* y=*/15);    
      u8g2.print("Horror Vacui");
      u8g2.setCursor(/* x=*/0, /* y=*/30); 
      u8g2.print("DETECTADO");
      u8g2.setCursor(0, 30);  
      } while ( u8g2.nextPage() );
    
      delay(3000);  
      }

    
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
}

/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */
static void pdm_data_ready_inference_callback(void)
{
    int bytesAvailable = PDM.available();

    // read into the sample buffer
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

    if(inference.buffer == NULL) {
        return false;
    }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

    // configure the data receive callback
    PDM.onReceive(&pdm_data_ready_inference_callback);

    PDM.setBufferSize(4096);


    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM!");
        microphone_inference_end();

        return false;
    }

    // set the gain, defaults to 20
    PDM.setGain(127);

    return true;
}


static bool microphone_inference_record(void)
{
    inference.buf_ready = 0;
    inference.buf_count = 0;

    while(inference.buf_ready == 0) {
        delay(10);
    }

    return true;
}


static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);

    return 0;
}


static void microphone_inference_end(void)
{
    PDM.end();
    free(inference.buffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
