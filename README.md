**General OverView:**

For this project, we wanted to build upon the driver's Ed car system that we had worked on in previous projects. For this project,
  we designed a way for student drivers to practice their skills before getting behind the wheel of a real vehicle. We wanted a car model that could actually drive. 
  This allows them to practice their driving skills in a safe and low-risk environment. We wanted to implement safety features that would be beneficial to a new driver. 
  This would allow new drivers to practice with real safety features found in modern vehicles. We choose to use continuous servo motors to drive and steer the car, 
  and an ultrasonic distance sensor as our safety sensor. We then implemented a system that would reverse the car away from any potential hazards that the sensor detected. 

**Implementation:**

Necessary Hardware
* ESP-32-S3 MCU
* 5V buzzer
* 2 Yellow LEDS
* 4 FITEC Continuous Servo Motors
* Micro-USB to USBC cable
* 6V Battery Pack
* Adafruit Ultrasonic Distance Sensor
  
  To start on the hardware, we first wired all four servo motors in parallel with the 6V battery pack in order to maintain the voltage
needed to drive each of the servo motors. We then independently wired two of the servo motors to one ADC channel, and then the other two servos to their own LEDC channel,
so we can have control over each side of the car (for the tank turning feature). The two servos on the left were wired together, and the two on the right were wired together. We then wired another LEDC channel to the headlights at the front of the car, so that we would be able to control the headlights with keyboard inputs.
Then, to interface the keyboard with the ESP32s3, we needed to read the bytes coming from the keyboard consistently. We coded different inputs to drive the car forward,
backwards, and to turn left and right. If there was a period of time without a reading, then we would default to stopping the car.
For the ultrasonic sensor, we designed a safety subsystem that would first scan to see if any objects were nearing the front of the car,
and if nothing was detected, the system allowed for more inputs. However, if something is detected and the system decides that it is “too close,” then the car is
put into reverse for a short period of time until any objects are at a safe distance away. 

**Potential Changes (maybe in the future):**

* Allowing for a 360-degree vision system by mounting ultrasonic sensors on each side of the car
* Constructing a 3D printed frame
* ESP-NOW protocol to have wireless communication between the MCU and the car
* Mounting a camera to the front of the car
* Implementing a CNN that would read information from the camera, for self-driving capability (using TensorFlow Lite, and a Micro SD card for adjustments)
Using a positional servo motor as an axle for more “car-like” steering.

