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
//#include "protocol_examples_common.h"

#include "app_prov.h"

#include <esp_https_server.h>


#include "driver/gpio.h"
#define MAX_PORT 1
#define MIN_PORT 0

//#define ZERO_PIN 23
//#define ONE_PIN 17 
#define ZERO_PIN 16
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
			gpio_out(ZERO_PIN,1);
			gpio_out(ONE_PIN,1);
		}
		if(strcmp(action,"off")==0){
			gpio_out(ZERO_PIN,0);
			gpio_out(ONE_PIN,0);
		}
		if(strcmp(action,"reset")==0){
			gpio_out(ZERO_PIN,0);
			gpio_out(ONE_PIN,0);
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			gpio_out(ZERO_PIN,1);
			gpio_out(ONE_PIN,1);
		}
		
	}else{
        port=atoi(portstr);
        if(port <= MAX_PORT && port >= MIN_PORT ){
		if(port==0 && strcmp(action,"on")==0){
			gpio_out(ZERO_PIN,1);
		}
		if(port==0 && strcmp(action,"off")==0){
			gpio_out(ZERO_PIN,0);
		}
		if(port==0 && strcmp(action,"reset")==0){
			gpio_out(ZERO_PIN,0);
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			gpio_out(ZERO_PIN,1);
		}
		if(port==1 && strcmp(action,"on")==0){
			gpio_out(ONE_PIN,1);
		}
		if(port==1 && strcmp(action,"off")==0){
			gpio_out(ONE_PIN,0);
		}
		if(port==1 && strcmp(action,"reset")==0){
			gpio_out(ONE_PIN,0);
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			gpio_out(ONE_PIN,1);
		}
	}
	}
	printf("do_action alla fine... manca il publish\n");
	printf("controllo/feedback/ciabbatta0/ports/%s",portstr);
	sprintf(mqtt_topic,"controllo/feedback/ciabbatta0/ports/%s",portstr);
        ninux_mqtt_publish(mqtt_topic,action);
	printf("do_action alla fine... dopo il publish\n");
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
    //char mqtt_topic[512];
    //bzero(mqtt_topic,sizeof(mqtt_topic));
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
    //char mqtt_topic[512];
    bzero(action,sizeof(action));
    bzero(node_name,sizeof(node_name));
    bzero(portstr,sizeof(portstr));
    //bzero(mqtt_topic,sizeof(mqtt_topic));
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
    /* Invoke Provisioning event handler first */
    app_prov_event_handler(ctx, event);


    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        /* enable ipv6 */
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6");

        char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
        ESP_LOGI(TAG, "IPv6: %s", ip6);
    case SYSTEM_EVENT_AP_START:
        ESP_LOGI(TAG, "SoftAP started");
        break;
    case SYSTEM_EVENT_AP_STOP:
        ESP_LOGI(TAG, "SoftAP stopped");
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
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

static void wifi_init_sta()
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    /* Start wifi in station mode with credentials set during provisioning */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_start() );
}


void app_main()
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
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
    //tcpip_adapter_init();
    //initialise_wifi();
    
    /* Security version */
    int security = 0;
    /* Proof of possession */
    const protocomm_security_pop_t *pop = NULL;

#ifdef CONFIG_USE_SEC_1
    security = 1;
#endif

    /* Having proof of possession is optional */
#ifdef CONFIG_USE_POP
    const static protocomm_security_pop_t app_pop = {
        .data = (uint8_t *) CONFIG_POP,
        .len = (sizeof(CONFIG_POP)-1)
    };
    pop = &app_pop;
#endif

    /* Initialize networking stack */
    tcpip_adapter_init();

    /* Set our event handling */
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    /* Check if device is provisioned */
    bool provisioned;
    if (app_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        return;
    }

    if (provisioned == false) {
        /* If not provisioned, start provisioning via soft AP */
        ESP_LOGI(TAG, "Starting WiFi SoftAP provisioning");

        const char *ssid = NULL;

#ifdef CONFIG_SOFTAP_SSID
        ssid = CONFIG_SOFTAP_SSID;
#else
        uint8_t eth_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, eth_mac);

        char ssid_with_mac[33];
        snprintf(ssid_with_mac, sizeof(ssid_with_mac), "PROV_%02X%02X%02X",
                 eth_mac[3], eth_mac[4], eth_mac[5]);

        ssid = ssid_with_mac;
#endif

        app_prov_start_softap_provisioning(ssid, CONFIG_SOFTAP_PASS,
                                           security, pop);
    } else {
        /* Start WiFi station with credentials set during provisioning */
        ESP_LOGI(TAG, "Starting WiFi station");
        wifi_init_sta(NULL);

//////////
        esp_ota_mark_app_valid_cancel_rollback();


        httpd_uri_t * handler_array[2];
        handler_array[0]= &update;
        handler_array[1]= &port_management;
        
        handlers_struct_t* h=NULL;
	h=malloc(sizeof(httpd_uri_t));
        h->handlers=handler_array;
        h->elements=2;
	printf("ESP32 SDK version:%s, RAM left %d\n", system_get_sdk_version(), system_get_free_heap_size());
        //ninux_esp32_https(handler_array);
        ninux_esp32_https(h);
	printf("ESP32 SDK version:%s, RAM left %d\n", system_get_sdk_version(), system_get_free_heap_size());
        //esp_ota_mark_app_invalid_rollback_and_reboot();
        ninux_esp32_ota();
        //xTaskCreate(&ota_example_task, "ota_example", 8192, NULL, 5, NULL);
        //xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
        //ESP_LOGE(TAG, "SIMULAZIONE DI LOOP");
        //esp_restart();
        ninux_mqtt_set_topic("controllo/ciabbatta0/#");
        ninux_mqtt_init(mqtt_event_handler);
    }
}
