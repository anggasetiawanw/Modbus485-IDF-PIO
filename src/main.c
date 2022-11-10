
#include "shared.h"
#include "mbcontroller.h"

mb_parameter_descriptor_t device_parameters[] = {
    // CID, Name, Units, Modbus addr, register type, Modbus Reg Start Addr, Modbus Reg read length,
    // Instance offset (NA), Instance type, Instance length (bytes), Options (NA), Permissions
    {CID_VOLTAGE, STR("VOLTAGE"), STR("Volts"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_START_VOLTAGE, 6,
     0, PARAM_TYPE_U16, 12, OPTS(0, 9999, 1), PAR_PERMS_READ_WRITE_TRIGGER},
    {CID_CURRENT, STR("CURRENT"), STR("Amp"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_START_CURRENT, 3,
     0, PARAM_TYPE_U16, 6, OPTS(0, 9999, 1), PAR_PERMS_READ_WRITE_TRIGGER},
    {CID_POWER, STR("POWER"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_START_POWER, 16,
     0, PARAM_TYPE_U16, 32, OPTS(0, 9999, 1), PAR_PERMS_READ_WRITE_TRIGGER},
    {CID_FREQ, STR("FREQUENCY"), STR("Hz"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_START_FREQ, 1,
     0, PARAM_TYPE_U16, 2, OPTS(4500, 6500, 1), PAR_PERMS_READ_WRITE_TRIGGER},
    {CID_THD_V, STR("THD Voltage"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_START_THDV, 3,
     0, PARAM_TYPE_U16, 6, OPTS(4500, 6500, 1), PAR_PERMS_READ_WRITE_TRIGGER},
    {CID_THD_A, STR("THD Current"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_START_THDI, 3,
     0, PARAM_TYPE_U16, 6, OPTS(4500, 6500, 1), PAR_PERMS_READ_WRITE_TRIGGER},
    {CID_DP_1, STR("Decimal Point 1"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_DEC_POINT_1, 1,
     0, PARAM_TYPE_U16, 2, OPTS(4500, 6500, 1), PAR_PERMS_READ_WRITE_TRIGGER},
    {CID_DP_2, STR("Decimal Point 2"), STR("--"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, REG_DEC_POINT_2, 1,
     0, PARAM_TYPE_U16, 2, OPTS(4500, 6500, 1), PAR_PERMS_READ_WRITE_TRIGGER},
};

uint16_t num_device_parameters = (sizeof(device_parameters) / sizeof(device_parameters[0]));

// Modbus master initialization
static void master_operation_func(void *arg)
{
    esp_err_t err = ESP_OK;
    const mb_parameter_descriptor_t *param_descriptor = NULL;
    uint8_t temp_data[32] = {0}; // temporary buffer to hold maximum CID size
    uint8_t type = 0;

    ESP_LOGI(TAG, "Start modbus test...");
    for (uint16_t cid = 0; (err != ESP_ERR_NOT_FOUND) && cid < MAX_CID; cid++)
    {
        err = mbc_master_get_cid_info(cid, &param_descriptor);
        if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL))
        {
            err = mbc_master_get_parameter(param_descriptor->cid, (char *)param_descriptor->param_key, (uint8_t *)temp_data, &type);
            if (err == ESP_OK)
            {
                ESP_LOGI(MODBUS_TAG, "Characteristic #%d %s (%s) value = (0x%08x) read successful.",
                         param_descriptor->cid,
                         (char *)param_descriptor->param_key,
                         (char *)param_descriptor->param_units,
                         *(uint32_t *)temp_data);
            }
            else
            {
                ESP_LOGE(MODBUS_TAG, "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
                         param_descriptor->cid,
                         (char *)param_descriptor->param_key,
                         (int)err,
                         (char *)esp_err_to_name(err));
            }
        }
        else
        {
            ESP_LOGE(TAG, "Could not get information for characteristic %d.", cid);
        }
        vTaskDelay(POLL_TIMEOUT_TICS); // timeout between polls
    }
}

static esp_err_t master_init(void)
{
    mb_communication_info_t comm = {
        .port = MB_PORT_NUM,
        .mode = MB_MODE_RTU,
        .baudrate = MB_DEV_SPEED,
        .parity = MB_PARITY_NONE};
    void *master_handler = NULL;
    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
    MB_RETURN_ON_FALSE((master_handler != NULL), ESP_ERR_INVALID_STATE, MODBUS_TAG,
                       "mb controller initialization fail.");
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, MODBUS_TAG,
                       "mb controller initialization fail, returns(0x%x).",
                       (uint32_t)err);
    err = mbc_master_setup((void *)&comm);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, MODBUS_TAG,
                       "mb controller setup fail, returns(0x%x).",
                       (uint32_t)err);
    err = uart_set_pin(MB_PORT_NUM, U2TXD, U2RXD,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    err = mbc_master_start();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, MODBUS_TAG,
                       "mb controller start fail, returns(0x%x).",
                       (uint32_t)err);
    err = uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, MODBUS_TAG,
                       "mb serial set mode failure, uart_set_mode() returned (0x%x).", (uint32_t)err);
    vTaskDelay(5);
    err = mbc_master_set_descriptor(&device_parameters[0], num_device_parameters);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, MODBUS_TAG,
                       "mb controller set descriptor fail, returns(0x%x).",
                       (uint32_t)err);
    ESP_LOGI(MODBUS_TAG, "Modbus master stack initialized...");

    return err;
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    vTaskDelay(1000);
    ESP_LOGI(TAG, "MODBUS INITIALIZE");
    ESP_ERROR_CHECK(master_init());
    vTaskDelay(10);
    ESP_LOGI(TAG, "TEST MODBUS POLL");
    master_operation_func(NULL);
}