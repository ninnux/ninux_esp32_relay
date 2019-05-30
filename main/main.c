/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "tcpip_adapter.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"

#include <esp_https_server.h>


#include "driver/gpio.h"
#define MAX_PORT 1
#define MIN_PORT 0

#define ZERO_PIN 23
#define ONE_PIN 17 

EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

//#include "esp32_web_basic_auth.h"
#include "ninux_esp32_relay_https.h"
#include "ninux_esp32_ota.h"
#include "ninux_esp32_mqtt.h"



void gpio_out(int ioport,int value){
    gpio_pad_select_gpio(ioport);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(ioport, GPIO_MODE_OUTPUT);
    gpio_set_level(ioport, value);
}

void do_action(char* portstr,char* action){
	int port;
	int i;
    	char mqtt_topic[512];
    	bzero(mqtt_topic,sizeof(mqtt_topic));
	if(strcmp(portstr,"all")==0){
		//for(i=MIN_PORT;i<MAX_PORT+1;i++){
		//	do_action((char*)i,action);
		//}
		if(strcmp(action,"on")==0){
			gpio_out(ZERO_PIN,0);
			gpio_out(ONE_PIN,0);
		}
		if(strcmp(action,"off")==0){
			gpio_out(ZERO_PIN,1);
			gpio_out(ONE_PIN,1);
		}
		if(strcmp(action,"reset")==0){
			gpio_out(ZERO_PIN,1);
			gpio_out(ONE_PIN,1);
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			gpio_out(ZERO_PIN,0);
			gpio_out(ONE_PIN,0);
		}
		
	}else{
        port=atoi(portstr);
        if(port <= MAX_PORT && port >= MIN_PORT ){
		if(port==0 && strcmp(action,"on")==0){
			gpio_out(ZERO_PIN,0);
		}
		if(port==0 && strcmp(action,"off")==0){
			gpio_out(ZERO_PIN,1);
		}
		if(port==0 && strcmp(action,"reset")==0){
			gpio_out(ZERO_PIN,1);
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			gpio_out(ZERO_PIN,0);
		}
		if(port==1 && strcmp(action,"on")==0){
			gpio_out(ONE_PIN,0);
		}
		if(port==1 && strcmp(action,"off")==0){
			gpio_out(ONE_PIN,1);
		}
		if(port==1 && strcmp(action,"reset")==0){
			gpio_out(ONE_PIN,1);
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			gpio_out(ONE_PIN,0);
		}
	}
	}
	//printf("controllo/feedback/testufficio/ports/%s",portstr);
	//sprintf(mqtt_topic,"controllo/feedback/testufficio/ports/%s",portstr);
        //ninux_mqtt_publish(mqtt_topic,action);
}

///* An HTTP GET handler */
static esp_err_t update_get_handler(httpd_req_t *req)
{
if(!esp32_web_basic_auth(req)){
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<h1>Hello Secure World2!</h1>", -1); // -1 = use strlen()
    ninux_esp32_ota();

}
else{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<h1>Culo2!</h1>", -1); // -1 = use strlen()
}
    return ESP_OK;
}

static esp_err_t port_management_handler(httpd_req_t *req)
{
if(!esp32_web_basic_auth(req)){
    char myuri[512];
    char http_message[512];
    char command[8];
    char *campo;
    char portstr[8];
    int port;
    bzero(http_message,sizeof(http_message));
    memcpy(myuri,req->uri,sizeof(myuri));
    campo=strtok(myuri,"/");
    printf("campo:%s\n",campo);
    if(strcmp(campo,"ports")==0){
        campo=strtok(NULL,"/");
    	printf("campo:%s\n",campo);
	memcpy(portstr,campo,strlen(campo));
        port=atoi(campo);
        if((port <= MAX_PORT && port >= MIN_PORT )|| strcmp(campo,"all")==0){
            campo=strtok(NULL,"/");
    	    printf("campo:%s\n",campo);
            if(strcmp(campo,"on")==0 || strcmp(campo,"off")==0 || strcmp(campo,"reset")==0){
                do_action(portstr,campo);
		printf("do_action %d %s\n",port,campo);
                httpd_resp_set_type(req, "text/html");
                sprintf(http_message,"<h1>%d %s</h1>",port,campo);
                httpd_resp_send(req, http_message, -1);
            }else{
                httpd_resp_set_type(req, "text/html");
                httpd_resp_send(req, "<h1>command error</h1>", -1); // -1 = use strlen()
            }

         }else{
                httpd_resp_set_type(req, "text/html");
                httpd_resp_send(req, "<h1>port unrecognized</h1>", -1); // -1 = use strlen()
         }
     }else{
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, "<h1>invalid path</h1>", -1); // -1 = use strlen()

     }
}
else{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<h1>Culo2!</h1>", -1); // -1 = use strlen()
}

    return ESP_OK;
}


httpd_uri_t update = {
    .uri       = "/update",
    .method    = HTTP_GET,
    .handler   = update_get_handler
};

httpd_uri_t port_management = {
    .uri       = "/ports/*",
    .method    = HTTP_GET,
    .handler   = port_management_handler
};




static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char mytopic[512];
    char *campo;
	int port;
    char node_name[32];
    char portstr[8];
    char action[16];
    bzero(action,sizeof(action));
    bzero(node_name,sizeof(node_name));
    bzero(portstr,sizeof(portstr));
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG2, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT);
            msg_id = esp_mqtt_client_subscribe(client,input_mqtt_topic , 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG2, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG2, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG2, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG2, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG2, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG2, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
	    sprintf(mytopic,"%.*s",event->topic_len, event->topic);
	    sprintf(action,"%.*s",event->data_len, event->data);

	    //sscanf(mytopic,"controllo/%s/ports/%s",node_name,portstr); 
            //port=atoi(portstr);
	    //printf("node:%s\n",node_name);
	    //printf("port:%d\n",port);
	    //printf("action:%s\n",action);
            //if((port <= MAX_PORT && port >= MIN_PORT )|| strcmp(portstr,"all")==0){
	    //    printf("condizione OK\n");
	    //    do_action(portstr,action);
	    //}
	    //memcpy(mytopic,event->topic,event->topic_len);
            campo=strtok(mytopic,"/");
            printf("campo:%s\n",campo);
            campo=strtok(NULL,"/");//nome del nodo
            printf("campo:%s\n",campo);
            campo=strtok(NULL,"/");//salta il nome del nodo
            if(strcmp(campo,"ports")==0){
                campo=strtok(NULL,"/");
            	printf("porta:%s\n",campo);
	        memcpy(portstr,campo,strlen(campo));
               port=atoi(campo);
                if((port <= MAX_PORT && port >= MIN_PORT )|| strcmp(campo,"all")==0){
                    //campo=strtok(NULL,"/");
            	    printf("action:%s\n",action);
                    if(strcmp(action,"on")==0 || strcmp(action,"off")==0 || strcmp(action,"reset")==0){
	        	//sprintf(action,"%s",event->data);
                        do_action(portstr,action);
	               	printf("do_action %d %s\n",port,action);
                    }else{
                        printf("command error");
                    }

                 }else{
                        printf("port unrecognized");
                 }
             }else{
                printf("invalid path");
             }
	    printf("do_action fine\n");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG2, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG2, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        // This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. 
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}


void app_main()
{
    static httpd_handle_t server = NULL;

    //ESP_ERROR_CHECK(nvs_flash_init());
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    tcpip_adapter_init();
    initialise_wifi();
    esp_ota_mark_app_valid_cancel_rollback();


    httpd_uri_t * handler_array[2];
    handler_array[0]= &update;
    handler_array[1]= &port_management;

    ninux_esp32_https(handler_array);
    //esp_ota_mark_app_invalid_rollback_and_reboot();
    ninux_esp32_ota();
    //xTaskCreate(&ota_example_task, "ota_example", 8192, NULL, 5, NULL);
    //xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
    //ESP_LOGE(TAG, "SIMULAZIONE DI LOOP");
    //esp_restart();
    ninux_mqtt_init(mqtt_event_handler);
    ninux_mqtt_subscribe_topic("controllo/testufficio/#");
}
