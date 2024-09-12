# This is my highschool project to detect drowning using machine learning model on edge device

The software is meant to be run on a ESP32 with BLE (Bluetooth Low Energy) 

To use the software you first have to download the machine learning library called project-1-arduino-1.0.5.zip and include it in the main program. 

Extension PlatformIO should be used on VScode to upload the code.

Platform.ini should be modified as following:

[env:seeed_xiao_esp32c3]
platform = espressif32
board = seeed_xiao_esp32c3
framework = arduino
build_flags = -D PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED
monitor_speed = 115200
board_build.partitions = no_ota.csv
lib_deps= C:\users\admin\Documents\Library_Machine_Learning\project-1-arduino-1.0.4\minh_quang2304-project-1_inferencing
          https://github.com/jrowberg/i2cdevlib.git
          https://github.com/ElectronicCats/mpu6050.git
          jrowberg/I2Cdevlib-BMP085 @ ^1.0.0

Seeed ESP32C3 board must be used with accelerometer via I2C.

# References:
https://www.hackster.io/mjrobot/exploring-machine-learning-with-the-new-xiao-esp32s3-6463e5

https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/

https://www.hackster.io/HackingSTEM/stream-data-from-arduino-into-excel-f1bede
