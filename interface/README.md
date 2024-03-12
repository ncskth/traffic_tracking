### interface
The interface runs using Python and Flask, a small web framework. It makes it possible to map specific HTTP endpoints to python functions. You can also generate dynamic webpages from HTML templates but in this project almost everything is static.

To update or change the web page you need to edit `templates/index.hmtl` to your liking. If the change is purely cosmetic then that's probably enough but for anything more you will also need to edit `dashboard.py`. There you can set how and when different python functions are called. Usually you write your function and then register it to a specific endpoint using `register_routes`.

Most relevant things such as ports and paths can be configured in `config.py`. Make sure to check `config.h` in `../recorder/` as well

### running
I recommend setting up a virtual environment
```
python -m venv .venv
source .venv/bin/activate
```

`source .venv/bin/activate` this activates the virtual environment. You will need to do it every single time you open a new shell and want to run the application.

Install dependencies (in the virtual environment)
```
pip install -r requirements.txt
```

Running the dashboard is just `python dashboard.py`

### Starting at boot
There is a systemd service file in the root of this repository. Copy it to `/usr/lib/systemd/system/` and then run `systemctl enable traffic_dashboard` to make systemd manage it automatically.