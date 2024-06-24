
#include <stdio.h>
#include <string.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_netif.h"

static const int RX_BUF_SIZE = 1024;
#define LED_BUILTIN GPIO_NUM_2

#define WIFI_SSID      "Dyba_Bernardelli_2G"
#define WIFI_PASS      "123581321"
#define MAX_HTTP_RECV_BUFFER 1024
#define MAX_COMMAND_LENGTH 1024
#define ESP_INTR_FLAG_DEFAULT 0
#define DEBOUNCE_DELAY_MS 200

static bool http_get_task_running = false;

QueueHandle_t uart_queue;

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI("TAG", "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI("TAG", "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI("TAG", "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI("TAG", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            // Check for chunked encoding is set to false
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE("TAG", "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI("TAG", "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is not chunked
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI("TAG", "HTTP_EVENT_REDIRECT");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI("TAG", "HTTP_EVENT_DISCONNECTED");
            if (output_buffer != NULL) {
                // Free memory of output buffer
                free(output_buffer);
                output_buffer = NULL;
                output_len = 0;
            }
            break;
    }
    return ESP_OK;
}

void post_finished_to_firebase(const char* message, const char* campo) {
    const char* url = "https://elevador-c3c39-default-rtdb.firebaseio.com/.json";
    char post_data[256];
    snprintf(post_data, sizeof(post_data), "{\"%s\": \"%s\"}", campo, message);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PATCH,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE("TAG", "Failed to initialize HTTP client");
        return;
    }

    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGI("TAG", "Successfully posted status to Firebase.");
        } else {
            ESP_LOGE("TAG", "HTTP POST failed with status code: %d", status_code);
        }
    } else {
        ESP_LOGE("TAG", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// Handler para eventos do Wi-Fi
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI("TAG", "Tentando reconectar ao AP...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI("TAG", "Conectado com sucesso!");
    }
}

// Configuração e inicialização do Wi-Fi
void wifi_init() {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &event_handler,
                                        NULL,
                                        &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}

struct elevador {
    char andar;
    char estado;
    char prox;
    char btc_up[15];
    char btc_down[15];
    char btc_in[16];
    char seq_up[16];
    char seq_down[16];
    char n_btc_in;
    char n_seq_up;
    char n_seq_down;
    char porta;
} esquerdo;

// Flags de eventos
char flagcabine, flagcorredorup, flagcorredordown;

// Variáveis de UART
char rx[13], tx[4], last;

// Contadores genéricos
char i, j;

// Vetor temporário
char temp[16];

// Contadores de eventos
char n_btc_in, n_seq_up, n_seq_down;

// Temporizadores de software
unsigned long timerled, timerelevador;

const char *alive_msg = "alive";
static const char *TAG = "Elevador";

void zerar_serial(void) {
    memset(rx, 0, sizeof(rx));
    memset(tx, 0, sizeof(tx));
}

void init_uart() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    ESP_LOGI(TAG, "UART initialized");
}

void setup() {
    flagcabine = 0;
    flagcorredorup = 0;
    flagcorredordown = 0;
    n_btc_in = 0;
    n_seq_up = 0;
    n_seq_down = 0;

    esquerdo.andar = 0;
    esquerdo.estado = 'P';
    esquerdo.prox = 0;
    esquerdo.n_btc_in = 0;
    esquerdo.n_seq_up = 0;
    esquerdo.n_seq_down = 0;
    memset(esquerdo.btc_in, 0, sizeof(esquerdo.btc_in));
    memset(esquerdo.btc_down, 0, sizeof(esquerdo.btc_down));
    memset(esquerdo.btc_up, 0, sizeof(esquerdo.btc_up));
    memset(esquerdo.seq_up, 0, sizeof(esquerdo.seq_up));
    memset(esquerdo.seq_down, 0, sizeof(esquerdo.seq_down));

    init_uart();

    gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
    timerled = esp_timer_get_time() / 1000;
    timerelevador = esp_timer_get_time() / 1000;
    
    wifi_init();

    ESP_LOGI(TAG, "Setup completed");
}

void uart_rx_task(void *pvParameters) {
   
    int index = 0;

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, (uint8_t *)&rx[index], 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            post_finished_to_firebase("entrei rx_task", "RX Task");
            post_finished_to_firebase(rx, "RX Task");
            if (rx[index] == 0x0A) { //0x0A = \n
                rx[index + 1] = '\0';
                xQueueSend(uart_queue, &rx, portMAX_DELAY);
                zerar_serial();
                post_finished_to_firebase("Enviei pra fila", "RX Task");
                index = 0;
            } else {
                index++;
                if (index >= 13) {
                    index = 0;
                }
            }
            
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void command_processor_task(void *pvParameters) {
    char rx[13];
    while (1) {
        if (xQueueReceive(uart_queue, &rx, portMAX_DELAY)) {
             post_finished_to_firebase("Recebi nova fila", "RX Process Task");
            char field_name[5];
            /*for (int i = 0; i < 13; i++) {
                snprintf(field_name, sizeof(field_name), "rx%d", i);
                post_finished_to_firebase(&rx[i], field_name);
            }*/
           post_finished_to_firebase(rx, "Rx dentro do command processor");
           
            if (rx[12] == 'r') {
                post_finished_to_firebase(rx, "Entrei em R com mensagem:");

                if (strncmp(rx, "initialized", 11) == 0) {
                    post_finished_to_firebase("initialized", "Entrei em initialized");
                    vTaskDelay(pdMS_TO_TICKS(10000));
                    const char *reset_msg = "er\r";
                    uart_write_bytes(UART_NUM_0, reset_msg, strlen(reset_msg));
                    post_finished_to_firebase("er", "Escrevi UART");
                    esquerdo.andar = 0;
                    esquerdo.estado = 'P';
                    esquerdo.prox = 0;
                    esquerdo.n_btc_in = 0;
                    esquerdo.n_seq_up = 0;
                    esquerdo.n_seq_down = 0;
                    memset(esquerdo.btc_in, 0, sizeof(esquerdo.btc_in));
                    memset(esquerdo.btc_down, 0, sizeof(esquerdo.btc_down));
                    memset(esquerdo.btc_up, 0, sizeof(esquerdo.btc_up));
                    memset(esquerdo.seq_up, 0, sizeof(esquerdo.seq_up));
                    memset(esquerdo.seq_down, 0, sizeof(esquerdo.seq_down));
                    zerar_serial();
                }

                zerar_serial();
            }
        }
    }
}

void uart_tx_task(void *pvParameters) {
    int contador = 0;
    while (1) {
        contador++;
        if (contador > 30) {
            uart_write_bytes(UART_NUM_0, alive_msg, strlen(alive_msg));
            contador = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void led_blink_task(void *pvParameters) {
    unsigned long timerled = esp_timer_get_time() / 1000;
    while (1) {
        unsigned long current_time = esp_timer_get_time() / 1000;
        if ((current_time - timerled) >= 200) {
            gpio_set_level(LED_BUILTIN, !gpio_get_level(LED_BUILTIN));
            timerled = current_time;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main() {
    setup();
    uart_queue = xQueueCreate(10, sizeof(char[13]));
    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL);
    xTaskCreate(uart_tx_task, "uart_tx_task", 4096, NULL, 10, NULL);
    xTaskCreate(led_blink_task, "led_blink_task", 4096, NULL, 10, NULL);
    xTaskCreate(command_processor_task, "command_processor_task", 4096, NULL, 10, NULL);
}
