#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "uart_t.h"
#include "app_config.h"
#include "json_generator.h"
#include "esp_http_client.h"



static const char *TAG = "uart_events";


#define WEB_SERVER "industrial.api.ubidots.com"
#define URL_POST "http://industrial.api.ubidots.com/api/v1.6/devices/ESP32_NODE?token=BBFF-AB1jEA8iDwvOAZBeEy2wy8xYbV0k7T"
#define HOST "/api/v1.6/devices/ESP32_NODE?token=BBFF-AB1jEA8iDwvOAZBeEy2wy8xYbV0k7T"

#define WEB_PORT "80"

#define EX_UART_NUM UART_NUM_2
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
char REQUEST[352];
json_gen_test_result_t result;

char data_uart[3];
char data_convert1[5]="....";
char data_convert2[5]="....";
int flag=0;
uint8_t node1[] = {0x00, 0x01, 0x17, 'N', 'O', '1'};
uint8_t node2[] = {0x00, 0x02, 0x17, 'N', 'O', '2'};
int check=0;

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}


static void post_rest_function(void)
{
    esp_http_client_config_t config_post = {
        .url = URL_POST,
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    
    esp_http_client_set_post_field(client, REQUEST, strlen(REQUEST));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void UART_PROCESS(char data[3], uint16_t length){
     strcpy(data_uart,data);
     if(flag==0){
     data_convert1[0]=data_uart[0];
     data_convert1[1]=data_uart[1];
     data_convert1[2]='.';
     data_convert1[3]=data_uart[2];
     json_gen_string_temp(&result, "temperature_1", data_convert1, REQUEST);
     printf("%s\n", data_convert1);
     post_rest_function();
     flag=1;
     check=1;
     } else if(flag ==1){
     data_convert2[0]=data_uart[0];
     data_convert2[1]=data_uart[1];
     data_convert2[2]='.';
     data_convert2[3]=data_uart[2];
     printf("%s\n", data_convert2);
     json_gen_string_temp(&result, "temperature_2", data_convert2,REQUEST);
     post_rest_function();
     flag=0;
     check=1;
     }
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_config();
    uart_init_cmd();
    uart_set_callback(UART_PROCESS);
    vTaskDelay(5000/portTICK_PERIOD_MS);
    while(1){
        uart_put((const char*)node1, sizeof(node1));
        vTaskDelay(1200/portTICK_PERIOD_MS);
        if(check==0){
            uart_put((const char*)node1, sizeof(node1));
        }else if(check==1){
        vTaskDelay(800/portTICK_PERIOD_MS);
        }
        vTaskDelay(800/portTICK_PERIOD_MS);
        if(check==0){
            printf("node 1 got out of sensor network\n");
        }
        check = 0;

        uart_put((const char*)node2, sizeof(node2));
        vTaskDelay(1200 / portTICK_PERIOD_MS);
        if(check==0){
           uart_put((const char*)node2, sizeof(node2));
        }else if(check==1){
        vTaskDelay(800 / portTICK_PERIOD_MS);
        }
        vTaskDelay(800 / portTICK_PERIOD_MS);
        if(check==0){
            printf("node 2 got out of sensor network\n");
        }
        check = 0;
    }
}
