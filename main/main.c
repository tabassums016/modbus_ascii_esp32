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

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_13)
#define RXD_PIN (GPIO_NUM_12)
// char query[1][11]={{ 0x01, 0x0F, 0x00, 0x00, 0X00, 0X0A, 0X02, 0XFF, 0X03}};
// char query[13] = {0x3A, 0x30, 0x31, 0x30, 0X33, 0X30,  0X30, 0X31, 0X35, 0x30, 0x30, 0x30, 0x41};
char query[6] = {0x01, 0x03, 0x00, 0x15, 0x00, 0x0A};
void sendData(char query[], int len);

void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_7_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void string2hexString(char input[]) // char* output)
{

    int loop;
    int i;
    // char hex_str[(len*2)+1];
    char *output = malloc(50);
    char ascii[1][17];
    int ascii_ix=0;
    ascii[0][ascii_ix]=58;

    char *ip = malloc(10);
    for (int ix = 0; ix < 6; ix++)
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
            ascii[0][ascii_ix]=atoi(output);
        }
    }
    ascii_ix++;
    ascii[0][ascii_ix++]=0x44;
    ascii[0][ascii_ix++]=0x44;
    ascii[0][ascii_ix++]=13;
    ascii[0][ascii_ix++]=10;

     for (int i = 0; i < 17; i++)
    {
        // query_send[0][i] = query[i];
        printf(" %02X", ascii[0][i]);
    }
    sendData(ascii[0], 17);


    // insert NULL at the end of the output string
    //  output[i++] = '\0';
}

char *ADD_LRC(char query[], int len)
{
    /* bytes in message      */

    // static unsigned char LRC(auchMsg, usDataLen)
    char *query_LRC = malloc(50);
    unsigned char *auchMsg;             /* message to calculate  */
    unsigned short usDataLen;           /* LRC upon quantity of  */
    unsigned char uchLRC = 0;           /* LRC char initialized   */
    for (int pos = 0; pos < len; pos++) /* pass through message  */
    {
        uchLRC += query[pos];
    } /* buffer add buffer byte*/
      /* without carry         */
    query[len] = ((unsigned char)(-((uchLRC))));
    printf("\nLRC= %X", query[len]);

    //   query[len+1]= 0X0D;
    //    query[len+2]= 0X0A;
    for (int i = 0; i < len + 3; i++)
    {
        query_LRC[i] = query[i];
    }
    /* return twos complemen */
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

// int sendData(const char* logName)
// {
//     const int len = sizeof(query[0]);
//     const int txBytes = uart_write_bytes(UART_NUM_1, query[0], len);
//     printf("\n\nQyery sent:");
//     for (int i = 0; i < 11; i++)
//     {

//         printf(" %x", query[0][i]);
//     }
//     printf("\n");
//     ESP_LOGI(logName, "Wrote %d bytes", txBytes);
//     return txBytes;
// }
void sendData(char query[], int len) //++ Function to send Modbus Queries
{

    printf("\n\nQyery sent:");
    for (int i = 0; i < len; i++)
    {
        // query_send[0][i] = query[i];
        printf(" %x", query[i]);
    }
    const int txBytes = uart_write_bytes(UART_NUM_1, query, len);
    printf("\nSending query %d bytes", txBytes);
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    string2hexString(query);
    int len = 0;
    char *final_query = malloc(50);
    len = sizeof(query);
    char send_arr[1][len];
    for (int i = 0; i < len; i++)
    {
        send_arr[0][i] = query[i];
    }
    while (1)
    {
        // final_query = ADD_LRC(send_arr[0], len);
        // sendData(final_query, len + 3);
        string2hexString(query);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

void app_main(void)
{
    init();
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}
