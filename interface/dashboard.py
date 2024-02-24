from flask import Flask, render_template, send_file
import random
import psutil
import subprocess
import time
import serial
import threading

import config

class Dashboard:
    def __init__(self):
        self.app = Flask(__name__)
        self.process = subprocess.Popen(['../recorder/build/snapshot'])
        self.recording = False
        self.volt1 = 3
        self.volt2 = 1
        self.git_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
        self.register_routes()
        try:
            self.ser = serial.Serial(
                port=config.SERIAL_PORT,
                baudrate=115200
            )
            serial_reader_t = threading.Thread(target=self.serial_reader_thread)
            monitor_t = threading.Thread(target=self.monitor_thread)
            serial_reader_t.start()
            monitor_t.start()
        except:
            print("Couldn't open serial")


    def serial_reader_thread(self):
        buf = bytes()
        while True:
            buf = bytes()
            while True:
                char = self.ser.read(1)
                buf += char
                if char == b"\n":
                    break
            buf.strip()
            args = buf.split(b" ")
            if args[0] == b"adc1":
                self.volt1 = float(args[1]) * config.VOLT1_GAIN

            if args[0] == b"adc2":
                self.volt2 = float(args[1]) * config.VOLT2_GAIN

            time.sleep(0.1)

    def monitor_thread(self):
        ping = False
        while True:
            if ping:
                if self.recording and self.process.poll() is not None:
                    self.ser.write(b"rgb 255 0 0\n")
                elif self.recording:
                    self.ser.write(b"rgb 0 255 255\n")
                else:
                    self.ser.write(b"rgb 0 255 0\n")
                ping = False
            else:
                self.ser.write(b"rgb 0 0 0\n")
                ping = True

            time.sleep(1)

    def register_routes(self):
        self.app.route("/")(self.index)
        self.app.route("/telem/event_snapshot.bmp")(self.get_event_snapshot)
        self.app.route("/telem/video_snapshot.jpg")(self.get_video_snapshot)
        self.app.route("/telem/total_voltage")(self.get_total_voltage)
        self.app.route("/telem/battery1_voltage")(self.get_battery1_voltage)
        self.app.route("/telem/battery2_voltage")(self.get_battery2_voltage)
        self.app.route("/telem/disk_usage")(self.get_disk_usage)
        self.app.route("/telem/is_recording")(self.get_is_recording)
        self.app.route("/telem/has_error")(self.get_error)
        self.app.route("/control/start_recording")(self.start_recording)
        self.app.route("/control/stop_recording")(self.stop_recording)
        self.app.route("/control/update_snapshot")(self.update_snapshot)

    def index(self):
        return render_template("index.html", config=config, version=self.git_hash)

    def get_is_recording(self):
        print("get is recording")
        return str(self.recording)

    def start_recording(self):
        print("update snapshot")
        if self.process.poll() is not None:
            self.process = subprocess.Popen(['../recorder/build/recorder', config.RECORD_DIR])
            self.recording = True
        return "ok"

    def stop_recording(self):
        print("stop streaming")
        self.recording = False
        if self.process.poll() is None:
            self.process.terminate()
        return "ok"

    def get_event_snapshot(self):
        print("get event snapshot")
        try:
            return send_file(config.TMP_DIR + config.EVENT_SNAPSHOT_FILE_NAME)
        except:
            return send_file("img/dog.jpg")

    def get_video_snapshot(self):
        print("get video snapshot")
        try:
            return send_file(config.TMP_DIR + config.VIDEO_SNAPSHOT_FILE_NAME)
        except:
            return send_file("img/dog.jpg")

    def get_battery1_voltage(self):
        print("get battery1 voltage")
        return f"{self.volt2 - self.volt1:.2f}"

    def get_battery2_voltage(self):
        print("get battery2 voltage")
        return f"{self.volt1:.2f}"

    def get_total_voltage(self):
        print("get total voltage")
        return f"{self.volt2:.2f}"

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

    def get_error(self):
        print("get error")
        return str(self.recording and self.process.poll() is not None)

    def run(self):
        self.app.run(host="0.0.0.0", port=config.PORT, debug=config.DEBUG)

if __name__ == "__main__":
    dashboard = Dashboard()
    dashboard.run()