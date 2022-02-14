
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "psxcontroller.h"
#include "sdkconfig.h"

#pragma GCC optimize ("O0")

#define DELAY() asm("nop; nop; nop; nop;nop; nop; nop; nop;nop; nop; nop; nop;nop; nop; nop; nop;")

#if CONFIG_HW_JOYSTICK_ENABALE

void joystickDelay(void)
{
   DELAY();
   for (int delay = 0; delay < 100; delay++)
   {
   };
}

int digitalRead(uint8_t pin)
{
   if (pin < 32)
   {
      return (GPIO.in >> pin) & 0x1;
   }
   else
   {
      return (GPIO.in1.val >> (pin - 32)) & 0x1;
   }
   return 0;
}

void IRAM_ATTR digitalWrite(uint8_t pin, uint8_t val)
{
   if (val)
   {
      if (pin < 32)
      {
         GPIO.out_w1ts = ((uint32_t)1 << pin);
      }
      else if (pin < 34)
      {
         GPIO.out1_w1ts.val = ((uint32_t)1 << (pin - 32));
      }
   }
   else
   {
      if (pin < 32)
      {
         GPIO.out_w1tc = ((uint32_t)1 << pin);
      }
      else if (pin < 34)
      {
         GPIO.out1_w1tc.val = ((uint32_t)1 << (pin - 32));
      }
   }
}

int joystickReadInput()
{
   uint32_t clkPin = CONFIG_HW_JOYSTICK_CLK;
   uint32_t latchPin = CONFIG_HW_JOYSTICK_LATCH;
   uint32_t data1Pin = CONFIG_HW_JOYSTICK_DAT1;
   uint32_t data2Pin = CONFIG_HW_JOYSTICK_DAT2;
   uint16_t key1 = 0;
   uint16_t key2 = 0;
   digitalWrite(latchPin, 1);
   joystickDelay();
   digitalWrite(latchPin, 0);
   joystickDelay();
         if (digitalRead(data1Pin))
         key1 |= 1;
      if (digitalRead(data2Pin))
         key2 |= 1;

   digitalWrite(clkPin, 0);
   joystickDelay();
   for (int i = 0; i < 8; i++)
   {
      key1 <<= 1;
      key2 <<= 1;
      digitalWrite(clkPin, 1);
      joystickDelay();
      if (digitalRead(data1Pin))
         key1 |= 1;
      if (digitalRead(data2Pin))
         key2 |= 1;
      digitalWrite(clkPin, 0);
      joystickDelay();
   }
   digitalWrite(clkPin, 0);
   key1 >>= 1;
   key2 >>= 1;
   //printf("gpio in 0x%x in1 0x%x\r\n", GPIO.in, GPIO.in1.val);
   //if( key1 != 0xff)
   //   printf("read key1 0x%x key2 0x%x\r\n", key1, key2);
   return key1 | key2 << 8;
}

void joystickInit()
{
   printf("Joystick controller enabled.\n");
   gpio_config_t gpioconf[2] = {
       {.pin_bit_mask = (1ULL << CONFIG_HW_JOYSTICK_CLK) | (1ULL << CONFIG_HW_JOYSTICK_LATCH),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_PIN_INTR_DISABLE},
       {.pin_bit_mask = (1ULL << CONFIG_HW_JOYSTICK_DAT1) | (1ULL << CONFIG_HW_JOYSTICK_DAT2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_PIN_INTR_DISABLE}};
   gpio_config(&gpioconf[0]);
   gpio_config(&gpioconf[1]);
}
#else
int joystickReadInput()
{
   return 0xFFFF;
}

void joystickInit()
{
   printf("Joystick controller disabled in menuconfig; no input enabled.\n");
}
#endif
