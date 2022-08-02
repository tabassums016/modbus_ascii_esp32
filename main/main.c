/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 2048;

#define TXD_PIN (GPIO_NUM_13)
#define RXD_PIN (GPIO_NUM_12)
// char query[1][11]={{ 0x01, 0x0F, 0x00, 0x00, 0X00, 0X0A, 0X02, 0XFF, 0X03}};
// char query[13] = {0x3A, 0x30, 0x31, 0x30, 0X33, 0X30,  0X30, 0X31, 0X35, 0x30, 0x30, 0x30, 0x41};
char query[6] = {0x01, 0x03, 0x00, 0x15, 0x00, 0x14};
void sendData(char query[], int len);
void json_string_maker_float(int register_count, float slave_response[], int8_t query_index, char slave_id_array[], char func_code_array[]);

void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void string2hexString(char input[], char *ascii)
{

    int loop;
    int i;
    // char hex_str[(len*2)+1];
    char *output = malloc(50);
    // char ascii[1][17];
    int ascii_ix = 0;
    ascii[ascii_ix] = 58;

    char *ip = malloc(10);
    for (int ix = 0; ix < 7; ix++)
    {
        // printf("\nArray= %02X", input[ix]);
        sprintf(ip, "%02X", input[ix]);
        i = 0;
        loop = 0;
        while (ip[loop] != '\0')
        {
            sprintf((char *)(output), "%d", ip[loop]);
            loop += 1;
            i += 2;
            // printf("\nhex_str: %x\n", atoi(output));
            ascii_ix++;
            ascii[ascii_ix] = atoi(output);
        }
    }
    ascii_ix++;
    // ascii[0][ascii_ix++]=0x44;
    // ascii[0][ascii_ix++]=0x44;
    ascii[ascii_ix++] = 13;
    ascii[ascii_ix++] = 10;

  
}

char *ADD_LRC(char query[])
{
    /* bytes in message      */

    // static unsigned char LRC(auchMsg, usDataLen)
    char *query_LRC = malloc(50);
    unsigned char *auchMsg;   /* message to calculate  */
    unsigned short usDataLen; /* LRC upon quantity of  */
    unsigned char uchLRC = 0; /* LRC char initialized   */
    char ascii[17];
    for (int pos = 0; pos < 6; pos++) /* pass through message  */
    {
        uchLRC += query[pos];
    } /* buffer add buffer byte*/
      /* without carry         */
    query[6] = ((unsigned char)(-((uchLRC))));
   

    string2hexString(query, ascii);
    for (int i = 0; i < 17; i++)
    {
        query_LRC[i] = ascii[i];
        printf(" %02X", ascii[i]);
    }

    return query_LRC;
}

char *Add_CRC(char query[], int len) //++ Funtion to Add CRC Check for every query
{

    uint16_t crc = 0xFFFF;
    // int len=sizeof(query);
    printf("\naddcrc len=%d", len);
    char *query_crc = malloc(50);
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)query[pos]; // XOR byte into least sig. byte of crc

        for (int i = 8; i != 0; i--)
        { // Loop over each bit
            if ((crc & 0x0001) != 0)
            {              // If the LSB is set
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= 0xA001;
            }
            else           // Else LSB is not set
                crc >>= 1; // Just shift right
        }
    }
    // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
    query[len] = *((char *)&crc + 0);     //&crc+0; //*((char*)&crc+0);
    query[len + 1] = *((char *)&crc + 1); //&crc+1; //*((char*)&crc+1);
                                          // printf("\nbefore crc return=%s",query[5]);
    for (int i = 0; i < len + 2; i++)
    {
        query_crc[i] = query[i];
    }
    return query_crc;
}


void send_modbus_query(char query[], int len) //++ Function to send Modbus Queries
{
    char query_send[1][len];
    memset(query_send, 0, sizeof(query_send));
    printf("\n\nQyery sent:");
    for (int i = 0; i < len; i++)
    {
        query_send[0][i] = query[i];
        printf(" %02X", query[i]);
    }
    // const int len = sizeof(query_send[0]); //++ Sends Queries one by one
    const int txBytes = uart_write_bytes(UART_NUM_2, query_send[0], len);
    printf("\nSending query %d bytes\n", txBytes);
}
void ascii_to_hx(uint8_t *modbus_response)
{
    
    int index = 1;
    char hex_char[5];
    char temp[5];
    uint8_t ascii_conv[20 * 2];
    for (int i = 0; i < 20 * 2 +2; i++)
    {
        memset(temp, 0, sizeof(temp));
        memset(hex_char, 0, sizeof(hex_char));
        sprintf(hex_char, "%c", modbus_response[index]);
        sprintf(temp, "%c", modbus_response[index + 1]);
        strcat(hex_char, temp);
        // printf("\nhex char= %s", hex_char);
        int number = (int)strtol(hex_char, NULL, 16);
        // printf("\nHEX number %d: %02X", i, number);
        ascii_conv[i] = number;
        index += 2;
    }

    float response[20 / 2];
    index = 3;
    float mbdata = 0;

    for (int i = 0; i < 20 / 2; i++)
    {
        *((char *)&mbdata + 0) = ascii_conv[index + 3];
        *((char *)&mbdata + 1) = ascii_conv[index + 2];
        *((char *)&mbdata + 2) = ascii_conv[index + 1];
        *((char *)&mbdata + 3) = ascii_conv[index];

        index = index + 4;
        response[i] = mbdata;
        // printf("\nHEX number %d: %f", i, response[i]);
    }
     json_string_maker_float(20, response, 0, NULL, NULL);
    
}

void json_string_maker_float(int register_count, float slave_response[], int8_t query_index, char slave_id_array[], char func_code_array[])
{
    // clock_gettime(CLOCK_REALTIME, &start);
    char http_json[100];
    char mqtt_data[100];
  

    for (int i = 0; i < register_count / 2; i++)
    {
        
        char temp[5];
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%0.2f", slave_response[i]);
        strncat(http_json, temp, strlen(temp));
        strncat(mqtt_data, temp, strlen(temp));
        ESP_LOGI("FLOAT","\nfloat string %d: %s",i, temp);
        if (i != (register_count / 2) - 1)
        {
            strncat(http_json, ",", 2);
            strncat(mqtt_data, ",", 2);
        }
    }
    // strncat(http_json, "]}", 3);
     ESP_LOGD("TAG", "Modbus data= %s", http_json);

}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    // string2hexString(query);
    // int len = 0;
    char *final_query = malloc(50);
    // len = sizeof(query);
    // char send_arr[1][len];
    // for (int i = 0; i < len; i++)
    // {
    //     send_arr[0][i] = query[i];
    // }
    while (1)
    {
        final_query = ADD_LRC(query);
        send_modbus_query(final_query, 17);
        // string2hexString(query);
        // ADD_LRC(query,6);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        // ESP_LOGI(RX_TASK_TAG, "Reading Uart bytes................");
        const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);

            ascii_to_hx(data);
        }
    }
    free(data);
}

void app_main(void)
{
    init();
    xTaskCreate(rx_task, "uart_rx_task", 2048 * 2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 2048 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}
