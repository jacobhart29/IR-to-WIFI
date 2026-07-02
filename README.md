![Project Icon](icon.png)

# IR to WiFi

IR to WiFi lets you turn most IR devices (Air Conditioners, TVs, Fireplaces, and other IR devices) into WiFi devices.

## Setup

### 1. Clone the repository

```bash
git clone https://github.com/jacobhart29/IR-to-WIFI
```

### 2. Import the PlatformIO project

Open the project in PlatformIO and let the libraries install.

### 3. Configure your upload port

Go to the `platformio.ini` file and change the upload port and monitor port to match your ESP32's COM port.

### 4. Configure WiFi credentials and pinouts

Go to the `main.cpp` file and update:
- Your WiFi SSID and password
- The pinouts for your connected devices

Then upload the project to your ESP32.

### 5. Connect your devices

Wire up your IR devices to the ESP32 according to the pinouts you configured.

### 6. Get the device IP

Open the Serial Monitor to find your ESP32's IP address.

## Usage

Enter the IP address into a browser to access the following endpoints:

- **Record a signal:**
  ```
  http://IP/record
  ```

- **Send a signal:**
  ```
  http://IP/ir/send
  ```
  Use either a POST request or a query string with `name=YOUR_SIGNAL_NAME`.