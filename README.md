## Smart Trash Can

EECS 221 IoT -- Term Project

Video demo: https://drive.google.com/file/d/1drDQBJRFs4NiDAgKd4q5NwcphHbRdqcq/view?usp=sharing

### Setup

**Controller #1** (ESP8266 NodeMCU) PINs (left is the PIN on the board):

- Ultrasonic distance sensor
  - VCC - VIN
  - D6 - Trig
  - D5 - Echo
  - GND - GND
- Load Sensor / HX711
  - D1 - DT 
  - D0 - SCK
  - 3V3 - VCC
- Software serial:
  - D3 - Controller #1 D4
  - D4 - Controller #1 D3
- Flex sensor:
  - A0 (with 3.3V in + 10KΩ resistor to measure voltage changes)

**Controller #2** (ESP8266 NodeMCU) PINs:

- Arducam Mini 2MP Plus

  - D0 - CS
  - D7 - MOSI
  - D6 - MISO
  - D5 - SCK
  - GND - GND
  - 3V3 - VCC
  - SDA - D2
  - SCL - D1

- Red LED

  - D8 - LED - 220Ω resistor - GND

- Software serial:

  - D3 - Controller #2 D4

  - D4 - Controller #2 D3

**Server** (Raspberry Pi 3b) runs the Flask web app (see `app/` folder).

### Arduino Code

Dependencies:

- [Arducam driver](https://github.com/ArduCAM/Arduino)
- [HX711 library](https://github.com/bogde/HX711)
- [EspSoftwareSerial library](https://github.com/plerup/espsoftwareserial/)

Choose `ArduCAM_ESP8266_UNO` board to use the camera driver.

### Web App Code

Dependencies:

- [Flask](http://flask.pocoo.org/)
- [Flask-SQLAlchemy](http://flask-sqlalchemy.pocoo.org/)
- [Flask-Migrate](https://github.com/miguelgrinberg/Flask-Migrate/)
- [google-cloud-vision](https://pypi.org/project/google-cloud-vision/)
- [pandas](https://pandas.pydata.org/)
- [matplotlib](https://matplotlib.org)

How to run:

```
$ export FLASK_APP=app
$ export FLASK_ENV=development
$ flask db init
$ flask db upgrade
$ flask db migrate
$ flask run --host=0.0.0.0
```
