# ESP Cat
An ESP32-based robot cat with camera and motor control via a web interface.

## Functionality
### Web Server & Routes Diagram

```mermaid
flowchart TD
    User["User's Browser"]
    Server["ESP32 Web Server"]
    Camera["ESP32-CAM Module"]

    User -- "HTTP GET /" --> Server
    Server -- "Serves index.html" --> User

    User -- "HTTP GET /video" --> Server
    Server -- "Captures image
    (GET /video)" --> Camera
    Camera -- "JPEG image" --> Server
    Server -- "Sends JPEG image" --> User
```

- **/**: Loads the web interface (index.html) from the ESP32.
- **/video**: Captures a photo from the camera and returns it as a JPEG image.
- _Note: Not a true video feed, REST API for single images_

## Motor Control Diagram

```mermaid
    flowchart TD
        User["User's Browser"]
        Server["ESP32 Web Server"]
        Motors["Motor Driver (TB6612FNG)"]

        User -- "HTTP GET /speed?l=[int]&r=[int]" --> Server
        Server -- "Sets motor speeds" --> Motors
```
- **/speed?l=[int]&r=[int]**: Sets the speed of the left (l) and right (r) motors. Speed values range from -255 to 255.
- **TB6612FNG** is used in a tank drive configuration.
## HTML Build Process Diagram

```mermaid
flowchart TD
    A["index.html (Source HTML)"]
    B["html-minify.py"]
    C["index-min.html (Minified HTML)"]
    D["headerfy.py"]
    E["index-min.h (C Header for ESP32)"]

    A -- "Minify HTML, CSS, JS" --> B
    B -- "Output minified HTML" --> C
    C -- "Convert to C header" --> D
    D -- "Output header file" --> E
```

- **html-minify.py**: Minifies HTML, CSS, and JS from `index.html` to produce `index-min.html`.
- **headerfy.py**: Converts `index-min.html` into a C header file (`index-min.h`) for embedding in ESP32 firmware.