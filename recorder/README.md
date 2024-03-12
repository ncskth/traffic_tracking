## Recorder

### General
The program uses gstreamer and OpenEB to record data from the camera and event camera respectively. The events are recorded using the built in functionality in openeb (`camera.record()`). Gstreamer uses an enormous pipeline to take raw video from the camera at 1920x1200 90fps and then save it without re-encoding. Gstreamer is completely incomprehensible so it was mostly written by chatGPT.

Every time gstreamer records a frame it triggers an interrupt. In the interrupt callback the current event-camera timstestamp is recorded to a csv file.


### Build

You need to install an old version of OpenEB (3.12) to run the software with a gen1 camera. Recent versions also work (but only with newer cameras)

You need Gstreamer.
```sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good gstreamer1.0-plugins-bad```

Then to build
```
cd recorder
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```
The binaries will then be available in build as `snapshot` and `recorder`
