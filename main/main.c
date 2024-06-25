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


/*DECLARACAO DE VARIAVEIS*/

static const int RX_BUF_SIZE = 1024;
#define LED_BUILTIN GPIO_NUM_2
#define WIFI_SSID      "Dyba_Bernardelli_2G"
#define WIFI_PASS      "123581321"
#define MAX_HTTP_RECV_BUFFER 1024
#define MAX_COMMAND_LENGTH 1024
#define ESP_INTR_FLAG_DEFAULT 0
#define DEBOUNCE_DELAY_MS 200
static bool http_get_task_running = false;
QueueHandle_t uart_queue_rx;
QueueHandle_t uart_queue_tx;
QueueHandle_t queue_atendimentos;
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
                                        //definicao do elevador 
char flagcabine, flagcorredorup, flagcorredordown;  //flags de eventos
char rx[13], tx[4], last;                           //variaveis de UART
char i, j;                                          //contadores genericos
char temp[16];                                      //vetor temporario
char n_btc_in, n_seq_up, n_seq_down;                //contadores de eventos
unsigned long timerled, timerelevador;              //timers
const char *alive_msg = "alive";
static const char *TAG = "Elevador";

/*DECLARACAO DE FUNCOES*/
void setup();
void zerar_rx(void);
void zerar_tx(void);
void init_uart();
void uart_rx_task(void *pvParameters);
void command_processor_task(void *pvParameters);
void uart_tx_task(void *pvParameters);
void atendimentos_task(void *pvParameters);
void led_blink_task(void *pvParameters);
//wifi e firebase
esp_err_t _http_event_handler(esp_http_client_event_t *evt);
void post_finished_to_firebase(const char* message, const char* campo);
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init();

void app_main() {
    esp_log_level_set("*", ESP_LOG_NONE); // Desativa todos os logs
    setup();
    uart_queue_rx = xQueueCreate(10, sizeof(char[13]));
    uart_queue_tx = xQueueCreate(10, sizeof(char[13]));
    queue_atendimentos = xQueueCreate(50, sizeof(int)); //50 elementos do tipo int
    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL);
    xTaskCreate(uart_tx_task, "uart_tx_task", 4096, NULL, 10, NULL);
    xTaskCreate(led_blink_task, "led_blink_task", 4096, NULL, 10, NULL);
    xTaskCreate(command_processor_task, "command_processor_task", 4096, NULL, 10, NULL);
    xTaskCreate(atendimentos_task, "command_processor_task", 4096, NULL, 10, NULL);
}

/*IMPLEMENTACAO DE FUNCOES*/

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

void zerar_rx(void) {
    memset(rx, 0, sizeof(rx));
}

void zerar_tx(void) {
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

void uart_rx_task(void *pvParameters) {
   
    int index = 0;
    char rx_char[2]; // Buffer para armazenar o caractere recebido como string

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, (uint8_t *)&rx[index], 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            //post_finished_to_firebase("entrei rx_task", "RX Task");
            //post_finished_to_firebase(rx, "RX Task valor");
            
            // Converte o caractere recebido para string e envia ao Firebase
            //snprintf(rx_char, sizeof(rx_char), "%c", rx[index]);


            //post_finished_to_firebase(rx_char, "RX Task valor caracter");
           

            if (rx[index] == '\n') { //0x0A = \n
                rx[index + 1] = '\0';
                xQueueSend(uart_queue_rx, &rx, portMAX_DELAY);
                zerar_rx();
                post_finished_to_firebase("Enviei pra fila", "RX Task fila");
                index = 0;
            } else {
                index++;
                if (index >= 13) {
                    index = 0;
                    zerar_rx();
                }
            }
            
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void uart_tx_task(void *pvParameters) {
    char tx_task[13];
    while (1) {

       if (xQueueReceive(uart_queue_tx, &tx_task, portMAX_DELAY)) {
            uart_write_bytes(UART_NUM_0, tx_task, strlen(tx_task));
            post_finished_to_firebase(tx_task, "Escrevi UART");
            memset(tx_task, 0, sizeof(tx_task));
            zerar_tx();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void command_processor_task(void *pvParameters) {
    char rx_processor[13];
    while (1) {
        if (xQueueReceive(uart_queue_rx, &rx_processor, portMAX_DELAY)) {
             post_finished_to_firebase("Recebi nova fila", "RX Process Task");
            char field_name[5];
            /*for (int i = 0; i < 13; i++) {
                snprintf(field_name, sizeof(field_name), "rx%d", i);
                post_finished_to_firebase(&rx[i], field_name);
            }*/
           post_finished_to_firebase(rx_processor, "Rx dentro do command processor");
           
           //comando de inicialização
            if (strncmp(rx_processor, "initialized", 11) == 0) {
                    post_finished_to_firebase("initialized", "Entrei em initialized");
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    tx[0]='e';tx[1]='r';tx[2]='\r';
                    xQueueSend(uart_queue_tx, &tx, portMAX_DELAY);
                     post_finished_to_firebase("Enviei pra fila", "Enviar fila tx");
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
                    zerar_tx();

                }

        //porta fechada
        if (strncmp(rx_processor, "eF", 2) == 0) {
             esquerdo.estado = 'F';
        }

        //porta aberta
                //porta fechada
        if (strncmp(rx_processor, "eA", 2) == 0) {
             esquerdo.estado = 'A';
        }

        //atendimentos solicitados dentro do elevador
        if (strncmp(rx_processor, "eI", 2) == 0) {
            char command_temp[30];
            char caracter_andar_destino = 'z';
            char destino_char[2]; // Buffer para armazenar o caractere recebido como string

            caracter_andar_destino = rx_processor[2];
            snprintf(destino_char, sizeof(destino_char), "%c", caracter_andar_destino);
            post_finished_to_firebase(destino_char, "caracter destino");
            
            //Processa e manda para fila de atendimentos
            int new_request = -1;

            switch(caracter_andar_destino){
                case 'a':
                new_request = 0;
                break;
                
                case 'b':
                new_request = 1;
                break; 

                case 'c':
                new_request = 2;
                break;  

                case 'd':
                new_request = 3;
                break;  

                case 'e':
                new_request = 4;
                break;  

                case 'f':
                new_request = 5;
                break;  

                case 'g':
                new_request = 6;
                break;  

                case 'h':
                new_request = 7;
                break;  

                case 'i':
                new_request = 8;
                break;  

                case 'j':
                new_request = 9;
                break;  

                case 'k':
                new_request = 10;
                break;  

                case 'l':
                new_request = 11;
                break;  

                case 'm':
                new_request = 12;
                break;  

                case 'n':
                new_request = 13;
                break;  

                case 'o':
                new_request = 14;
                break;  

                case 'p':
                new_request = 15;
                break;                
            }
        if(new_request!= -1){
            xQueueSend(queue_atendimentos, &new_request, portMAX_DELAY);
            post_finished_to_firebase("Enviei fila atendimentos", "Atendimentos");
        }
        }
        memset(rx_processor, 0, sizeof(rx_processor));
              
            
        }
         zerar_tx();
    }
}

//Atendimentos são os andares.
void atendimentos_task(void *pvParameters) {
    int atendimento_task;
    int delay = 0;

    while (1) {

       if (xQueueReceive(queue_atendimentos, &atendimento_task, portMAX_DELAY)) {
        //Fecha a porta e vai até o andar solicitado
          post_finished_to_firebase("Entrei fila atendimentos", "Atendimentos");
            if(esquerdo.estado == 'A'){
                //manda fechar a porta
                tx[0]='e';tx[1]='f';tx[2]='\r';
                xQueueSend(uart_queue_tx, &tx, portMAX_DELAY);
            }
            //Espera porta fechar
            while(esquerdo.estado=='A'){vTaskDelay(pdMS_TO_TICKS(10));} 
           
           //Manda subir
            tx[0]='e';tx[1]='s';tx[2]='\r';
            xQueueSend(uart_queue_tx, &tx, portMAX_DELAY);
            esquerdo.estado = 'S';

            //Espera x milisegundos
            delay = atendimento_task*5000;
            vTaskDelay(pdMS_TO_TICKS(delay));

            //Manda parar
            tx[0]='e';tx[1]='p';tx[2]='\r';
            xQueueSend(uart_queue_tx, &tx, portMAX_DELAY);
            esquerdo.estado = 'P';

            //Manda abrir porta
            tx[0]='e';tx[1]='a';tx[2]='\r';
            xQueueSend(uart_queue_tx, &tx, portMAX_DELAY);
            esquerdo.estado = 'P';




            zerar_tx();
  
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

/*COISAS DE FIREBASE E WIFI*/
//manipulador de eventos
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
            if (!esp_http_client_is_chunked_response(evt->client)) {
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
//solicitacao de post
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
//handler para eventos do Wi-Fi
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
//configuracao e inicializacao do wifi
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
