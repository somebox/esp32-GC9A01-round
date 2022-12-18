# esp32-GC9A01-round

An example project showing two different clocks on two round TFT SPI displays, using ESP32 and the [eTFT_SPI](https://github.com/Bodmer/TFT_eSPI) lib. I built this to help learn how to make clocks with better and simpler code quality.

Some things demonstrated:
- Using the platformio.ini file to configure eTFT_SPI settings
- Switching between two different displays by toggling the CS pins of each
- Using native `time()`, `localtime_r()`, `configTime()`, `setEnv()` and timezone strings, to update via NTP without external libraries
- Connect to WiFi pattern with time sync, initialization, error handling and debug callbacks for WiFi events.
- Drawing both analog and digital clock faces using eTFT_SPI and `TFT_eSprite` primatives, as well as font handling.
- Storing fonts on an SPIFFs partition, updated by PlatformIO
- Track frame rate and timing in the loop

This project uses WiFiManager - which means a WIFI access point will be created for you to configure WiFI settings when you first flash a new board. After that, the settings will stay.

![IMG_2304](https://user-images.githubusercontent.com/7750/208321457-5206c8bf-f860-4d96-82de-4c69bd5c64a9.jpeg)

 
