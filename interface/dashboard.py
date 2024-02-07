from flask import Flask, render_template, send_file
import random

class Dashboard:
    def __init__(self):
        self.app = Flask(__name__)
        self.is_recording = True

        self.register_routes()

    def register_routes(self):
        self.app.route("/")(self.index)
        self.app.route("/telem/event_snapshot.jpg")(self.get_event_snapshot)
        self.app.route("/telem/video_snapshot.jpg")(self.get_video_snapshot)
        self.app.route("/telem/battery_voltage")(self.get_battery_voltage)
        self.app.route("/telem/disk_usage")(self.get_disk_usage)
        self.app.route("/control/start_recording")(self.start_recording)
        self.app.route("/control/stop_recording")(self.stop_recording)

    def index(self):
        return render_template("index.html", data=self)

    def start_recording(self):
        print("started streaming")

    def stop_recording(self):
        print("stop streaming")

    def get_event_snapshot(self):
        print("get event snapshot")
        return send_file("img/test1.jpg")

    def get_video_snapshot(self):
        print("get video snapshot")
        return send_file("img/test2.jpg")

    def get_battery_voltage(self):
        print("get battery voltage")
        self.battery_voltage = round(random.uniform(13.0, 14.0), 2)
        return str(self.battery_voltage)

    def get_disk_usage(self):
        print("get disk usage")
        self.disk_usage = round(random.uniform(48.0, 74.0), 2)
        return str(self.disk_usage)

    def run(self):
        self.app.run(host="0.0.0.0", port=8080, debug=True)

if __name__ == "__main__":
    dashboard = Dashboard()
    dashboard.run()