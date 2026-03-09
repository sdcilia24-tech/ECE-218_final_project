#include <stdio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <esp_intr_types.h>
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

//Headlights
#define HEADLIGHTS_IO 17
#define HEADLIGHT_CHANNEL LEDC_CHANNEL_2
#define HEADLIGHTS_MAX 8191
#define BRIGHTNESS 1024


//distance sensor
#define TRIGGER 11
#define ECHOPIN 12
#define ALARM 7
#define distanceThreshold 10
esp_timer_handle_t oneshotTimer;
volatile int pulseWidth = 0;
volatile int timeHigh = 0;
volatile int timeLow = 0; 
#define CONVFACTOR 58

//other constants
#define LOOP_DELAY 25
#define TIMEOUT 550
volatile int errorCheck = 0;

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
 * defines a function that will move the left wheels and right wheels in reverse
 * @arg void
 * @returns void 
 */
void carBackward(void){
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_LEFT, LEDC_DUTY_MIN);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_LEFT);   
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RIGHT, LEDC_DUTY_MAX);
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
 * @arg void
 * @returns void 
 */
void IRAM_ATTR distanceSensorHandler(void *arg){
    bool SensorLevel = gpio_get_level(ECHOPIN);
    if (SensorLevel){
        timeHigh = esp_timer_get_time();
    }
    if (!SensorLevel){
        timeLow = esp_timer_get_time();
        pulseWidth = timeLow - timeHigh;
        pulseWidth /= CONVFACTOR;
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
        .duty = LEDC_DUTY_STOPPED,
        .hpoint = 0
    };
    ledc_channel_config_t ledcChannelRight = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RIGHT,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = RIGHT_WHEELS_IO,
        .duty = LEDC_DUTY_STOPPED,
        .hpoint = 0
    };
    ledc_channel_config_t ledcHeadlights = {
        .speed_mode = LEDC_MODE,
        .channel = HEADLIGHT_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = HEADLIGHTS_IO,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledcHeadlights);
    ledc_channel_config(&ledcChannelLeft);
    ledc_channel_config(&ledcChannelRight);
    ledc_timer_config(&ledcTimer);
}

/**
 * defines a function that will initalize both the trigger and the echo pin for the 
 * Adafruit ultrasonic sensor that will trigger via ISR.
 * @arg void
 * @returns void
 */
void distanceSensorInit(){
    gpio_reset_pin(TRIGGER);
    gpio_set_direction(TRIGGER, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIGGER, 0);

    gpio_reset_pin(ECHOPIN);
    gpio_set_direction(ECHOPIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(ECHOPIN, GPIO_INTR_ANYEDGE);
    gpio_intr_enable(ECHOPIN);
    gpio_install_isr_service(0);

    gpio_isr_handler_add(ECHOPIN, &distanceSensorHandler, NULL);

    const esp_timer_create_args_t oneshot_timer_args = {
        .callback = &oneshotTimerHandler,
        .name = "one-shot"
    };
    esp_timer_create(&oneshot_timer_args, &oneshotTimer);
}

/**
 * defines a function that will compare the distance between the ultrasonic sensor
 * and the threhold, and if there are ten consecutive readings below the threshold,
 * stop, sound an alarm, and put the car in reverse until the car is out of range of teh object
 * @arg void
 * @returns void
 */
void distanceCheck(void){
    if (pulseWidth < distanceThreshold){
            errorCheck++;
        }
    else{
        errorCheck = 0;
    }
    if (errorCheck >= 10){
        carStop();
        gpio_set_level(ALARM, 1);
        while(pulseWidth <= distanceThreshold + 5){
            esp_timer_start_once(oneshotTimer, 10);
            gpio_set_level(TRIGGER, 1);
            carBackward();
            vTaskDelay(LOOP_DELAY / portTICK_PERIOD_MS);
        }
        carStop();
        gpio_set_level(ALARM, 0);
        errorCheck = 0;
    }
}

void app_main(void){
    gpio_reset_pin(ALARM);
    gpio_set_direction(ALARM, GPIO_MODE_OUTPUT);
    gpio_set_level(ALARM, 0);
    ledcInit();
    distanceSensorInit();
    usb_serial_jtag_driver_config_t usbConfig = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&usbConfig);
    esp_vfs_usb_serial_jtag_use_driver();
    char input;
    int lastKey = 0;
    int headlightLevel = 0;
    while(1){
        int currentData = usb_serial_jtag_read_bytes(&input, 1, 0);
        uint64_t timeRun = esp_timer_get_time() / 1000; // checks to see when the last key press is in milliseconds
        esp_timer_start_once(oneshotTimer, 10);
        gpio_set_level(TRIGGER, 1);
        distanceCheck();
        if (currentData > 0){
            lastKey = timeRun;
            if (input == 'c' || input == 'C'){
                if (headlightLevel < 8191){
                    headlightLevel += BRIGHTNESS;
                }
                ledc_set_duty(LEDC_MODE, HEADLIGHT_CHANNEL, headlightLevel);
                ledc_update_duty(LEDC_MODE, HEADLIGHT_CHANNEL);
            }
            if (input == 'v' || input == 'V'){
                if (headlightLevel > 0){
                    headlightLevel -= BRIGHTNESS;
                }
                ledc_set_duty(LEDC_MODE, HEADLIGHT_CHANNEL, headlightLevel);
                ledc_update_duty(LEDC_MODE, HEADLIGHT_CHANNEL);
            }
            if (input == 'w' || input == 'W'){
                carForward();
            }
            else if (input == 'a' || input == 'A'){
                carLeft();
            }
            else if (input == 's' || input == 'S'){
                carBackward();
            }
            else if (input == 'd' || input == 'D'){
                carRight();
            }
            else{
                continue;
            }
        }
        if (timeRun - lastKey > TIMEOUT){
            carStop();
        }
        vTaskDelay(LOOP_DELAY / portTICK_PERIOD_MS);
        }
    }

