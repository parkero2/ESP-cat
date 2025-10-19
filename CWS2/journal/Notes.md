# ESP32-WROVER-DEV
As per [camera_pins.h](../CameraWebServer/CameraWebServer/camera_pins.h), the following pins are configured for the camera on the ESP32-WROVER-DEV board:

```cpp
#if defined(CAMERA_MODEL_WROVER_KIT)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  21
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    19
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    5
#define Y2_GPIO_NUM    4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22
```
Leaving the remaining pins `33, 32, 12, 2, 14, 13` free for other uses.
This is limiting because the TB6612FNG motor controller requires 4 control pins and 2 PWM pins, totaling 6 pins leaving no more additional pins available for other functionalities on this specific board.

To work around this limitation, the following option has been implemented:
- Using a secondary board in a master/slave (_now referred to as controller/device_) setup.

## Master/Slave Setup
As the ESP32 will be handling all the wifi and webserver functionalities, it will act as the master/controller.
A secondary microcontroller (Arduino Nano) will be used as the slave/device to handle motor control and other GPIO functionalities.
### Connections
- **ESP32-WROVER-DEV (Master/Controller)**
  - Connect to the Arduino Nano via Software Serial on the following pins:
    - RX: GPIO 33
    - TX: GPIO 32
- **Arduino Nano (Slave/Device)**
  - Connect motor driver (TB6612FNG) to the following pins:
    - IN1: D2
    - IN2: D3
    - IN3: D4
    - IN4: D5
    - PWM A: D9
    - PWM B: D10
  - Connect Software Serial to the ESP32 on the following pins:
    - RX: D11
    - TX: D12

### Communication Protocol
- The Ardunino Nano will be programmed with [subController.ino](../CameraWebServer/CameraWebServer/subController.ino) to listen for commands from the ESP32.
- [subController.ino](../CameraWebServer/CameraWebServer/subController.ino) listens to serial commands in the format:
    - `pin:value\n`
    - Example: `D2:255\n` to set pin `D2` to `HIGH` (255).
- Each line is considered a separate command.
- The device/slave will not be sending any data back to the controller/master in this implementation.
- The device/slave will only execute commands received from the controller/master.
- The ESP32 will be the only controller in this configuration that should be aware of what pins are connected to the device.
- All pins on the device/slave are configured to be `OUTPUT`.

This configuration leaves 5 pins free on the ESP32-WROVER-DEV for other _input_ functionalities such as ultrasonic sensors trig while still allowing motor control via the Arduino Nano.

## Webserver Integration
The webserver runs exclusively on the ESP32-WROVER-DEV, handling camera streaming and user interface.

### Implementation
The original example code [CameraWebServer.ino](../CameraWebServer/CameraWebServer/CameraWebServer.ino) worked by taking a compressed html file and serving it via the ESP32's webserver, leaving the browser to decompress and render the page. This approach saves space and bandwidth which are both limited on the ESP32 platform.
To integrate motor control into a web interface, the following steps were taken:
1. **HTML Modification**: The original `index.html` file is stored in a compressed format in [camera_index.h](../CameraWebServer/CameraWebServer/camera_index.h).
    - Using a python script [html_processor.py](../CameraWebServer/CameraWebServer/html_processor.py), the original `index.html` was extracted and modified to include motor control buttons and JavaScript functions to send commands to the ESP32.
    - The following was added to the body of the HTML to add motor control buttons:
    ```html
        <div id="tankDriveControls">
            <!-- fwd, bck, rotate left, rotate right -->
            <button id="forwardBtn">⬆️</button><br>
            <button id="leftBtn">↪️</button>
            <button id="stopBtn">⏹️</button>
            <button id="rightBtn">↩️</button>
            <br>
            <button id="backwardBtn">⬇️</button>
        </div>
      ```
      - JavaScript functions were added to handle button clicks and send corresponding commands to the ESP32 via HTTP requests.
      ```javascript
      // TANK DRIVE FUNCTIONS //

      function drive_fwd() {
        fetchUrl(`${baseHost}/tank?dir=fw`, function (code, txt) {
          if (code != 200) {
            alert('Error[' + code + ']: ' + txt);
          }
        });
      }

      function drive_bck() {
        fetchUrl(`${baseHost}/tank?dir=bw`, function (code, txt) {
          if (code != 200) {
            alert('Error[' + code + ']: ' + txt);
          }
        });
      }

      function drive_left() {
        fetchUrl(`${baseHost}/tank?dir=lt`, function (code, txt) {
          if (code != 200) {
            alert('Error[' + code + ']: ' + txt);
          }
        });
      }

      function drive_right() {
        fetchUrl(`${baseHost}/tank?dir=rt`, function (code, txt) {
          if (code != 200) {
            alert('Error[' + code + ']: ' + txt);
          }
        });
      }

      function drive_stop() {
        fetchUrl(`${baseHost}/tank?dir=st`, function (code, txt) {
          if (code != 200) {
            alert('Error[' + code + ']: ' + txt);
          }
        });
      }

      // END OF TANK DRIVE FUNCTIONS // 
      // Attach tank drive button actions //

      const forwardBtn = document.getElementById('forwardBtn');
      forwardBtn.onclick = () => {
        drive_fwd();
      };

      const backwardBtn = document.getElementById('backwardBtn');
      backwardBtn.onclick = () => {
        drive_bck();
      };

      const leftBtn = document.getElementById('leftBtn');
      leftBtn.onclick = () => {
        drive_left();
      };

      const rightBtn = document.getElementById('rightBtn');
      rightBtn.onclick = () => {
        drive_right();
      };

      const stopBtn = document.getElementById('stopBtn');
      stopBtn.onclick = () => {
        drive_stop();
      };

      // END OF TANK DRIVE BUTTON ACTIONS //
      ```
      `${basehost}` is defined earlier in the script to point to the ESP32's IP address.
- This was then recompressed back into a byte array and updated in [camera_index.h](../CameraWebServer/CameraWebServer/camera_index.h) using the same python script.

2. **ESP32 Code Modification**: The main ESP32 code [app_httpd.cpp](../CameraWebServer/CameraWebServer/app_httpd.cpp) was modified to handle the new tank drive commands.
    - First external dependancies from [CameraWebServer.ino](../CameraWebServer/CameraWebServer/CameraWebServer.ino) were imported into [app_httpd.cpp](../CameraWebServer/CameraWebServer/app_httpd.cpp) to allow motor control functions to be called:
    ```cpp
    // File: app_httpd.cpp ~line 25

    // External motor control functions from CameraWebServer.ino
    extern void fwd();
    extern void bck();
    extern void lft();
    extern void rgt();
    extern void setSpeed(int l, int r);
    extern const int LF, LB, RF, RB, LSP_PWM, RSP_PWM;
    ```
    - A new route `/tank` was added to the webserver to listen for tank drive commands.
    ```cpp
    // File: app_httpd.cpp ~line 183
    /* START OF TANK DRIVE MODIFCATIONS */

    // Define a handler for `/tank` URI
    static esp_err_t tank_handler(httpd_req_t *req)
    {

    char buf[100];            // Buffer to store the received data
    size_t buf_len;           // Length of the buffer
    char direction[16] = {0}; // Variable to store direction. FWD, BCK, LFT, RGT, STP

    int speedL, speedR = 128; // Variables to store speed values. Defaulting to 128 (50% speed)

    // Extract the query parameters from the URL (?dir=<string>&speedL=<?int>&speedR=<?int>)
    // `speedL` and `speedR` are optional parameters. Default to 128 if not provided.
    // Allocate memory to suit length of query string +1 for null terminator

    buf_len = httpd_req_get_url_query_len(req) + 1;

    // More than the single null terminator
    if (buf_len > 1)
    {
        // Check for query string and copy to buffer
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
        // Check for value of expected key `dir`
        // Params: query from, key, value out, size of value
        if (httpd_query_key_value(buf, "dir", direction, sizeof(direction)) == ESP_OK)
        {

            // Expected values of `direction`: fw, bw, lt, rt, st
            // strcmp compares strings (case sensitive). Returns 0 for identical
            if (strcmp(direction, "fw") == 0)
            {
            fwd();
            }
            else if (strcmp(direction, "bw") == 0)
            {
            bck();
            }
            else if (strcmp(direction, "lt") == 0)
            {
            lft();
            }
            else if (strcmp(direction, "rt") == 0)
            {
            rgt();
            }
            else if (strcmp(direction, "st") == 0)
            {
            // Stop. Set speed to 0
            setSpeed(0, 0);
            }
            else
            {
            // All other cases are invalid query parameters
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "dir param invalid");
            return ESP_FAIL;
            }
            // End of ?dir query extraction
        }
        else
        {
            // missing ?dir parameter
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing dir param");
            return ESP_FAIL;
        }
        }
    }
    else
    {
        // Missing any query param. Not a valid GET request
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing params. Not a valid endpoint.");
        return ESP_FAIL;
    }

    // Successful response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    const char *resp_str = "{\"status\":\"ok\"";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
    }

    /* END OF TANK DRIVE MODIFCATIONS */
    ```
    - The handler extracts the `dir` query parameter to determine the direction command and calls the corresponding motor control functions (`fwd()`, `bck()`, `lft()`, `rgt()`, or stops the motors).
    - The new handler is defined in the webserver initialization function:
    ```cpp
    // File: app_httpd.cpp ~line 905
        // DECLARE /tank handler
        // uri : the route (/tank)
        // method : method used to handle the request (GET)
        // handler : the function handling the request (tank_handler)
        httpd_uri_t tank_uri = {
            .uri = "/tank",
            .method = HTTP_GET,
            .handler = tank_handler,
            .user_ctx = NULL
        };
    ```
    - The handler is then registered with the webserver instance:
    ```cpp
    // File: app_httpd.cpp ~line 1077
        // Register the /tank route handler
        // Takes params server instance, pointer to uri handler struct
        httpd_register_uri_handler(camera_httpd, &tank_uri);
    ```
    - These steps tell the webserver to listen for requests to `/tank` and route them to the `tank_handler` function for processing - not doing this would result in a 404 Not Found error when trying to access the endpoint.