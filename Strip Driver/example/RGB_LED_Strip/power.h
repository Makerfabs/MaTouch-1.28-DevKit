#include "pin_config.h"
#include <Arduino.h>

extern int voltage_index;
extern int voltage_flag;

void set_voltage_extern();
void USB_PD_POWER_5V(void);
void USB_PD_POWER_9V(void);
void USB_PD_POWER_12V(void);
void USB_PD_POWER_15V(void);
void USB_PD_POWER_20V(void);