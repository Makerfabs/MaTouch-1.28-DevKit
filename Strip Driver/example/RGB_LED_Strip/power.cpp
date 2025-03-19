#include "power.h"

String voltage_str[3] = {"5V", "9V", "12V"};
int voltage_index = 0;
int voltage_flag = 0;

void set_voltage_extern()
{
    Serial.println("-------------------------------");
    Serial.println(voltage_index);
    Serial.println(voltage_str[voltage_index]);

    if (voltage_index == 0)
        USB_PD_POWER_5V();
    if (voltage_index == 1)
        USB_PD_POWER_9V();
    if (voltage_index == 2)
        USB_PD_POWER_12V();
}

void USB_PD_POWER_5V(void)
{
    digitalWrite(USB_PD_CFG1, HIGH);
    digitalWrite(USB_PD_CFG2, LOW);
    digitalWrite(USB_PD_CFG3, LOW);
}

void USB_PD_POWER_9V(void)
{
    digitalWrite(USB_PD_CFG1, LOW);
    digitalWrite(USB_PD_CFG2, LOW);
    digitalWrite(USB_PD_CFG3, LOW);
}

void USB_PD_POWER_12V(void)
{
    digitalWrite(USB_PD_CFG1, LOW);
    digitalWrite(USB_PD_CFG2, LOW);
    digitalWrite(USB_PD_CFG3, HIGH);
}

void USB_PD_POWER_15V(void)
{
    digitalWrite(USB_PD_CFG1, LOW);
    digitalWrite(USB_PD_CFG2, HIGH);
    digitalWrite(USB_PD_CFG3, HIGH);
}

void USB_PD_POWER_20V(void)
{
    digitalWrite(USB_PD_CFG1, LOW);
    digitalWrite(USB_PD_CFG2, HIGH);
    digitalWrite(USB_PD_CFG3, LOW);
}
