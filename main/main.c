#include <stdio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <intr_types.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <esp_vfs_dev.h>
#include <driver/usb_serial_jtag.h>

//servos
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEFT_WHEELS_IO 5
#define RIGHT_WHEELS_IO 2
#define LEDC_CHANNEL_LEFT LEDC_CHANNEL_0
#define LEDC_CHANNEL_RIGHT LEDC_CHANNEL_1
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT
#define LEDC_FREQ 50
#define LEDC_DUTY_MIN 307
#define LEDC_DUTY_STOPPED 614
#define LEDC_DUTY_MAX 922

//distance sensor
#define TRIGGER GPIO_NUM_11
#define ECHO GPIO_NUM_12
esp_timer_handle_t oneshotTimer;
int pulseWidth = 0;
int timeHigh;
int timeLow; 

//other constants
#define LOOP_DELAY 10
#define TIMEOUT 200

/**
 * defines an interrupt that will force the Trigger Pin low
 * @arg void 
 * @returns void 
 */
void IRAM_ATTR oneshotTimerHandler(void *arg){
    gpio_set_level(TRIGGER, 0);
}

/**
 * defines a function that will move the left wheels and right wheels forward
 * @arg void
 * @returns void 
 */
void carForward(void){
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_LEFT, LEDC_DUTY_MAX);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_LEFT);   
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT, LEDC_DUTY_MIN);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT);
}
/**
 * defines a function that will stop the car
 * @arg void
 * @returns void
 */
void carStop(void){
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_LEFT, LEDC_DUTY_STOPPED);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_LEFT);   
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT, LEDC_DUTY_STOPPED);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT);
}

/**
 * Defines a function that will turn the car left by utilizing a "tank turn" where the left wheel
 * moves backward while the right wheel moves forward
 * @arg void
 * @returns void
 */
void carLeft(void){
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_LEFT, LEDC_DUTY_MIN);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_LEFT);   
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT, LEDC_DUTY_MAX);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT);
}
/**
 * Defines a function that will turn the car left by utilizing a "tank turn" where the Right wheel
 * moves backward while the left wheel moves forward
 * @arg void
 * @returns void
 */
void carRight(void){
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_LEFT, LEDC_DUTY_MAX);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_LEFT);   
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT, LEDC_DUTY_MIN);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT);

}

/**
 * defines an interrupt that will assign the current distance from the sensor to a nearby object
 * by taking the pulse width and defining it by a conversion factor
 * 
 * @arg void
 * @returns void 
 */
void IRAM_ATTR distanceSensorHandler(void *arg){
    bool SensorLevel = gpio_get_level(ECHO);
    if (SensorLevel){
        timeHigh = esp_timer_get_time();
    }
    if (!SensorLevel){
        timeLow = esp_timer_get_time();
        pulseWidth = timeLow - timeHigh;
        pulseWidth /= 58;
    }
}

/**
 * Defines a function that will configure the LEDC timer, and both channels for the left and right 
 * wheels of the robot
 * @arg void
 * @returns void
 */
void ledcInit(void){
    ledc_timer_config_t ledcTimer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_channel_config_t ledcChannelLeft = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_LEFT,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEFT_WHEELS_IO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config_t ledcChannelRight = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RIGHT,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = RIGHT_WHEELS_IO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledcChannelLeft);
    ledc_channel_config(&ledcChannelRight);
    ledc_timer_config(&ledcTimer);
}
/**
 * defines a function that will initalize the reading of the keyboard input
 * @returns void
 * @arg void
 */
void serialCommunicationInit(void){
    usb_serial_jtag_driver_config_t usbConfig = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&usbConfig);
    esp_vfs_usb_serial_jtag_use_driver(); 
}

void app_main(void){
    ledcInit();
    serialCommunicationInit();
    int UserInput = 0;
    int buffer[1];
    int lastKey = 0;
    while(1){
        UserInput = usb_serial_jtag_read_bytes(buffer, 1, 0);
        int timeRun = esp_timer_get_time() / 1000; // checks to see when the last key press is in milliseconds
        if (UserInput > 0){
            if (buffer[0] == 'w' || buffer[0] == 'W'){
                carForward();
                lastKey = timeRun;
            }
            else if (buffer[0] == 'a' || buffer[0] == 'A'){
                carLeft();
                lastKey = timeRun;
            }
            else if (buffer[0] == 's' || buffer[0] == 'S'){
                carBackward();
                lastKey = timeRun;
            }
            else if (buffer[0] == 'd' || buffer[0] == 'D'){
                carRight();
                lastKey = timeRun;
            }
        }
        if (timeRun - lastKey > TIMEOUT){
            // if the time since the last key is > timeout then forcefully stop the car
            //NTS: once the distnace sensor is coded
            carStop();
        }
        vTaskDelay(LOOP_DELAY / portTICK_PERIOD_MS);
        }
    }

