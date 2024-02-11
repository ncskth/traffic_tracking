#include <metavision/sdk/driver/camera.h>
#include <metavision/sdk/base/events/event_cd.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <thread>
#include <cstdint>
#include <vector>
#include <utility>
#include <mutex>
#include <thread>
#include <filesystem>

#include "config.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


int main(void) {
    std::cout << "taking video snapshot" << std::endl;
    std::string snapshot_cmd = "ffmpeg -nostdin -y -f v4l2 -s 1920x1200 -i " VIDEO_CAMERA " -ss 0:0:1 -frames:v 1 -loglevel quiet " TMP_DIR VIDEO_SNAPSHOT_FILE_NAME;
    system(snapshot_cmd.c_str());

    uint8_t frame[480 * 720] = {};

    Metavision::Camera cam;
    cam = Metavision::Camera::from_first_available();
    std::cout << "taking event snapshot" << std::endl;
    cam.start();
    cam.cd().add_callback([&frame](const Metavision::EventCD *begin, const Metavision::EventCD *end) {
        for (const Metavision::EventCD *ev = begin; ev != end; ++ev) {
            if (ev->x >= 720 || ev->y >= 480) {
                continue;
            }
            frame[ev->x + ev->y * 720] = ev->p ? 255 : 128;
        }
    });
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(300ms);
    memset(frame, 0, sizeof(frame));
    std::this_thread::sleep_for(8ms);
    cam.stop();
    std::this_thread::sleep_for(100ms);
    stbi_write_bmp(TMP_DIR EVENT_SNAPSHOT_FILE_NAME, 720, 480, 1, frame);
}