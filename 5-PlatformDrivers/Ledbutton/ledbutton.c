#include <linux/module.h>
#include <asm-generic/errno.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>


MODULE_DESCRIPTION("Ledbutton Kernel Module (Platform Driver) - FDI-UCM");
MODULE_AUTHOR("Juan Carlos Saez");
MODULE_LICENSE("GPL");

#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0

#define MANUAL_DEBOUNCE

#define NR_GPIO_LEDS  3

/* Array to hold gpio descriptors */
struct gpio_desc* gpio_descriptors[NR_GPIO_LEDS];

struct gpio_desc* desc_button = NULL;
static int gpio_button_irqn = -1;
static int led_state = ALL_LEDS_ON;


/* Set led state to that specified by mask */
static inline int set_pi_leds(unsigned int mask) {
  int i;
  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_set_value(gpio_descriptors[i], (mask >> i) & 0x1 );
  return 0;
}

/* Interrupt handler for button **/
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
#ifdef MANUAL_DEBOUNCE
  static unsigned long last_interrupt = 0;
  unsigned long diff = jiffies - last_interrupt;
  if (diff < 20)
    return IRQ_HANDLED;

  last_interrupt = jiffies;
#endif

  led_state = ~led_state & ALL_LEDS_ON;
  set_pi_leds(led_state);
  return IRQ_HANDLED;
}


/**
 *  Invoked when the kernel module is loaded in the kernel and
 *     a match was found in the Device Tree.
 */
int ledbutton_probe_device(struct platform_device* pdev) {
  int i, j;
  int err = 0;
  unsigned char gpio_out_ok = 0;
  unsigned int initial_led_state = 1;

  for (i = 0; i < NR_GPIO_LEDS; i++) {
    /* Retrieve LED GPIO Descriptor, configure it as output and set its value to 0 */
    if (!(gpio_descriptors[i] = gpiod_get_index(&pdev->dev, "led", i, GPIOD_OUT_LOW))) {
      dev_err(&pdev->dev, "LED GPIO%d not found in the device tree\n", i);
      err = -EINVAL;
      goto err_handle;
    }
  }

  /* Retrieve Button GPIO Descriptor and configure it as input  */
  if (!(desc_button = gpiod_get(&pdev->dev, "button", GPIOD_IN))) {
    dev_err(&pdev->dev, "Button GPIO not found in device tree\n");
    err = -EINVAL;
    goto err_handle;
  }

  gpio_out_ok = 1;

  /*
  ** The lines below are commented because gpiod_set_debounce is not supported
  ** in the Raspberry pi. Debounce is handled manually in this driver.
  */
#ifndef MANUAL_DEBOUNCE
  //Debounce the button with a delay of 200ms
  if (gpiod_set_debounce(desc_button, 200) < 0) {
    dev_err(&pdev->dev, "ERROR: gpio_set_debounce - %d\n", GPIO_BUTTON);
    goto err_handle;
  }
#endif

  //Get the IRQ number for our GPIO
  gpio_button_irqn = gpiod_to_irq(desc_button);

  if (request_irq(gpio_button_irqn,             //IRQ number
                  gpio_irq_handler,   //IRQ handler
                  IRQF_TRIGGER_RISING,        //Handler will be called in raising edge
                  "ledbutton",               //used to identify the device name using this IRQ
                  NULL)) {                    //device id for shared IRQ
    dev_err(&pdev->dev, "cannot register IRQ ");
    goto err_handle;
  }

  /* Read initial state of LEDs (All OFF or All ON) from Device Tree */
  if (of_property_read_u32(pdev->dev.of_node, "initial-led-state", &initial_led_state)) {
    dev_warn(&pdev->dev, "Device tree property not found\n");
  }

  if (initial_led_state == 1)
    led_state = ALL_LEDS_ON;
  else
    led_state = ALL_LEDS_OFF;

  set_pi_leds(led_state);

  dev_info(&pdev->dev, "Ledbutton platform device was initialized correctly\n");
  return 0;
err_handle:
  for (j = 0; j < i; j++)
    gpiod_put(gpio_descriptors[j]);

  if (gpio_out_ok)
    gpiod_put(desc_button);

  return err;

}

/* Invoked when the kernel module is removed */
int ledbutton_remove_device(struct platform_device* pdev)
{
  int i;
  set_pi_leds(ALL_LEDS_OFF);
  free_irq(gpio_button_irqn, NULL);

  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_put(gpio_descriptors[i]);

  gpiod_put(desc_button);

  dev_info(&pdev->dev, "Ledbutton platform device was removed\n");

  return 0;
}


/* Table of compatible devices */
static struct of_device_id device_match[] = {
  {.compatible = "custom,ledbutton"},
  {}
};

MODULE_DEVICE_TABLE(of, device_match);


/* Initialization of platform driver structure */
static struct platform_driver my_platform_driver = {
  .probe = ledbutton_probe_device,
  .remove = ledbutton_remove_device,
  .driver = {
    .name = "ledbutton",
    .of_match_table = device_match,
    .owner = THIS_MODULE,
  }
};


static int __init ledbutton_init(void)
{
  return platform_driver_register(&my_platform_driver);
}


static void __exit ledbutton_exit(void)
{
  platform_driver_unregister(&my_platform_driver);
}

module_init(ledbutton_init);
module_exit(ledbutton_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Modleds");
