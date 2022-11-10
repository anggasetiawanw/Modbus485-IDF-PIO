# Modbus485-IDF-PIO
Halo, Selamat Datang di reponya angga. Pada repo ini saya membuat example menggunakan freemodbus atau ESP-Modbus yang di sediakan oleh espressfnya itu sendiri.

## Referensi

[ESP_MODBUS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/modbus.html)

## Library
Library yang di gunakan di simpan pada direktori `components/freemodbus`. Serta harus di input pada extra components pada dir `src/CMakeList.txt`
```bash
set(EXCLUDE_COMPONENTS components/)
```
dan tambahkan pada `main.c`
```c
#include "mbcontroller.h"
```
