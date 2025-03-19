#include "local_store.h"

// ESP32 写NVS   DEBUG LOG
// 验证过，一般不会失败
void write_nvs(const char *key, const char *value)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    DEBUG_PRINTF("Opening NVS handle... ");
    nvs_handle my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        DEBUG_PRINTF("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        printf("Done\n");

        DEBUG_PRINTF("Updating %s:%s in NVS ... ", key, value);
        err = nvs_set_str(my_handle, key, value);
        DEBUG_PRINTF((err != ESP_OK) ? "Failed!\n" : "Done\n");

        DEBUG_PRINTF("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        DEBUG_PRINTF((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(my_handle);
    }
}

// ESP32 读NVS   DEBUG LOG
// 验证过，一般不会失败
int read_nvs(const char *key, char *value)
{
    char saved_value[NVS_DATA_LENGTH];
    size_t save_value_length = NVS_DATA_LENGTH;
    int check_status = 0;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    DEBUG_PRINTF("Opening NVS handle... \n");
    nvs_handle my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        DEBUG_PRINTF("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        DEBUG_PRINTF("Done\n");
        DEBUG_PRINTF("Reading %s from NVS ... \n", key);

        err = nvs_get_str(my_handle, key, saved_value, &save_value_length);
        switch (err)
        {
        case ESP_OK:
            DEBUG_PRINTF("Done\n");
            DEBUG_PRINTF("Value: %s\n", saved_value);
            DEBUG_PRINTF("Value length= %d\n", save_value_length);
            strcpy(value, saved_value);
            check_status++;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            DEBUG_PRINTF("The value is not initialized yet!\n");
            break;
        default:
            DEBUG_PRINTF("Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    }

    if (check_status == 1)
        return SUCCESS;
    else
        return ERROR;
}
