# The Jetson Doorbell

This repository has everything you need to get started with your Jetson Doorbell.
The [Getting Started](/fitch22/JetsonDoorbell/docs/GettingStarted.pdf) guide provides details on how to hook it up to replace standard mechanical chime doorbells as well as general operation and troubleshooting.
Several enclosure designs are provided that you can 3D print as is, or modify as necessary for your application.

## Where to find Doorbell Sounds

Here are some links to web sites that have sounds that work well for doorbells:  
[Orange FREE Sounds](https://orangefreesounds.com/)  
[Mixkit](https://mixkit.co/free-sound-effects/doorbell/)  
[Pixabay](https://pixabay.com/sound-effects/search/doorbell/)  
[Zapsplat](https://www.zapsplat.com/)  
Note, several of the sounds you will find to download are in MP3 format and must be converted to WAVE format before using with Jetson Doorbell.
You can convert an MP3 to a WAVE with an online converter such as:  
https://online-audio-converter.com/  
https://cloudconvert.com/audio-converter  
Or you can use an application such as [Audacity](https://www.audacityteam.org/). Audacity can do much more than convert, you can also adjust the tonal balance of the tune which can help with small, less than ideal speakers.

## Building the Firmware

Jetson Doorbell uses an ESP32-S3-N4 module running at 240MHz.
The current version of firmware is 0.1.
It was built with ESP IDF 5.2.1 and uses Mongoose web server 7.13 using VSCODE and the ESP-IDF extension.
Information on how to install the ESP IDF as well as using VSCODE as a development environment can be found [here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/index.html).
Information on how to use the Mongoose web server can be found [here](https://mongoose.ws/).
