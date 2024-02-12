from flask import Flask, render_template, send_file
import random
import psutil
import subprocess

import config

class Dashboard:
    def __init__(self):
        self.app = Flask(__name__)
        self.process = subprocess.Popen(['../recorder/build/snapshot'])
        self.register_routes()

    def register_routes(self):
        self.app.route("/")(self.index)
        self.app.route("/telem/event_snapshot.bmp")(self.get_event_snapshot)
        self.app.route("/telem/video_snapshot.jpg")(self.get_video_snapshot)
        self.app.route("/telem/battery_voltage")(self.get_battery_voltage)
        self.app.route("/telem/disk_usage")(self.get_disk_usage)
        self.app.route("/telem/is_recording")(self.get_is_recording)
        self.app.route("/control/start_recording")(self.start_recording)
        self.app.route("/control/stop_recording")(self.stop_recording)
        self.app.route("/control/update_snapshot")(self.update_snapshot)

    def index(self):
        return render_template("index.html", data=self)

    def get_is_recording(self):
        print("get is recording")
        return str(self.process.poll() is None)

    def start_recording(self):
        print("update snapshot")
        if self.process.poll() is not None:
            self.process = subprocess.Popen(['../recorder/build/recorder', config.RECORD_DIR])
        return "ok"

    def stop_recording(self):
        print("stop streaming")
        if self.process.poll() is None:
            self.process.terminate()
        return "ok"

    def get_event_snapshot(self):
        print("get event snapshot")
        return send_file(config.TMP_DIR + config.EVENT_SNAPSHOT_FILE_NAME)

    def get_video_snapshot(self):
        print("get video snapshot")
        return send_file(config.TMP_DIR + config.VIDEO_SNAPSHOT_FILE_NAME)

    def get_battery_voltage(self):
        print("get battery voltage")
        self.battery_voltage = round(random.uniform(13.0, 14.0), 2)
        return str(self.battery_voltage)

    def get_disk_usage(self):
        print("get disk usage")
        disk_usage = psutil.disk_usage(config.RECORD_DIR)
        free_space_gb = disk_usage.free / (1024 ** 3)  # Convert bytes to gigabytes
        return f"{free_space_gb:.2f}"

    def update_snapshot(self):
        print("update snapshot")
        if self.process.poll() is not None:
            self.process = subprocess.Popen(['../recorder/build/snapshot'])
        return "ok"


    def run(self):
        self.app.run(host="0.0.0.0", port=8080, debug=True)

if __name__ == "__main__":
    dashboard = Dashboard()
    dashboard.run()