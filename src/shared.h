#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

static const char *TAG = "MAIN";
static const char *MODBUS_TAG = "MODBUS";
// MODBUS Initialize
#define MB_PORT_NUM UART_NUM_2 // Number of UART port used for Modbus connection
#define MB_DEV_SPEED 9600      // The communication speed of the UART
#define U2RXD 16
#define U2TXD 17
#define MAX_CID 8
#define MAX_RETRY 30
// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS (500)
#define UPDATE_CIDS_TIMEOUT_TICS (UPDATE_CIDS_TIMEOUT_MS / portTICK_PERIOD_MS)

// Timeout between polls
#define POLL_TIMEOUT_MS (1)
#define POLL_TIMEOUT_TICS (POLL_TIMEOUT_MS / portTICK_PERIOD_MS)
// Enumeration of modbus device addresses accessed by master

#define STR(fieldname) ((const char *)(fieldname))
// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val)                   \
    {                                                      \
        .opt1 = min_val, .opt2 = max_val, .opt3 = step_val \
    }

enum
{
    MB_DEVICE_ADDR1 = 0x01 // Only one slave device used for the test (add other slave addresses here)
};

// Modbus AMC Addresses Accessed by Master
enum
{
    REG_DEC_POINT_1 = 0x0023,
    REG_DEC_POINT_2 = 0x0024,
    REG_START_VOLTAGE = 0x0025,
    REG_START_CURRENT = 0x002B,
    REG_START_POWER = 0x002E,
    REG_START_FREQ = 0x003E,
    REG_START_THDV = 0x0400,
    REG_START_THDI = 0x0403

};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum
{
    CID_VOLTAGE = 0,
    CID_CURRENT,
    CID_POWER,
    CID_FREQ,
    CID_THD_V,
    CID_THD_A,
    CID_DP_1,
    CID_DP_2,
};
