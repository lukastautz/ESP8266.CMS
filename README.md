# ESP8266.CMS
ESP8266.CMS is a Content Management System running on an ESP8266.
<hr>

## Installation
Change the WLAN-name, WLAN-password, blog name and admin ip in `config.h`. Compile `main.cpp` and `config.h` (recommended is with PlatformIO and `platformio.ini` (Do not forget to change `upload_port` directive)). Then upload the firmware on the ESP8266.
<hr>

## LEDs
Blue LED on startup means that the ESP8266 is connecting with the WLAN network. After connecting finished, blue LED should be out. If a visitor requests the site, blue LED should be on for the time the data is read/send.
<hr>

## Favicons
If you want to change the favicon, you can change it by setting `FAVICON_ICO` in `config.h` to an urlencoded favicon.ico.
<hr>

## Benchmark
I've made this Benchmark with <a href="https://github.com/JoeDog/siege">siege</a> with 5 concurrent users and 30min test. Results are on every benchmark differnet, because of many factors like Wifi connection speed, settings and other. Results below:
- 35100 requests
- 0 failed requests
- 260 ms average response time
- 60 ms shortest reponse time
- 610 ms longest response time
<hr>

Copyright (C) 2022 Lukas Tautz
