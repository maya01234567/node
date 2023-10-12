//#pragma once
#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug
typedef enum
{
    PROVISITION_SMARTCONFIG = 0,
    PROVISITION_ACCESSPOIN = 1
} provision_type_t;

// Component header

// Public Headers

// Private Headers
void app_config(void);

#endif