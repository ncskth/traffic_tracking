this repository contains a recording software and a dashboard for controlling said recording software.


## Recorder

### build

You need to install an old version of OpenEB (3.12) to run the software with a gen1 camera. Recent versions is will also work (but only with new cameras)

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

## Interface
### running
I recommend setting up a virtual environment
```cd interface
python -m venv .venv
source .venv/bin/activate
```
`source .venv/bin/activate` you will always need to run this before starting the dashboard

install dependencies
```
pip install -r requirements.txt
```

Running the dashboard is just `python dashboard.py`