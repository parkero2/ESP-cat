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