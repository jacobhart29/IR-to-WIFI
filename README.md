![Project Icon](icon.png)

``IR to WiFi lets you turn most IR devices (Air Conditioners, TV's, Fireplaces, and other IR devices) into WiFi devices.``

``To use first clone the repository``
```git clone https://github.com/jacobhart29/IR-to-WIFI```

``Import the platform io project and let the libs install``

``Goto the platformio.ini file and change upload port and monitor port to your ESP32 COM Port.``

``Goto the main.cpp file change your ssid and password and pinouts of the devices in the file and then upload to your ESP32``

``Connect the devices to the ESP32``

``Get the IP from the Serial Monitor and then enter it into a browser and goto http://IP/record to record signals and then use http://IP/ir/send with either a post request or query with name = YOUR SIGNAL NAME``

