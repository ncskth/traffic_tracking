#include <thread>
#include <chrono>
#include <metavision/sdk/driver/camera.h>
#include <metavision/sdk/base/events/event_cd.h>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <unistd.h>
#include <iomanip>
#include <ctime>
#include <stdbool.h>
#include <sys/stat.h>
#include <filesystem>
#include <csignal>


#include <sys/types.h>
#include <sys/wait.h>

#define FRAME_WIDTH 720
#define FRAME_HEIGHT 480

uint8_t frame[FRAME_WIDTH * FRAME_HEIGHT];
volatile bool terminate = false;
pid_t ffmpeg_pid;



void write_bmp(const char* filename, const unsigned char* buffer, int width, int height) {
    // BMP file header and info header
    unsigned char bmpFileHeader[14] = {'B','M', 0,0,0,0, 0,0,0,0, 54,0,0,0};
    unsigned char bmpInfoHeader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,8,0};
    int fileSize = 54 + width * height; // File size: header (54 bytes) + pixel data
    bmpFileHeader[ 2] = (unsigned char)(fileSize      );
    bmpFileHeader[ 3] = (unsigned char)(fileSize >>  8);
    bmpFileHeader[ 4] = (unsigned char)(fileSize >> 16);
    bmpFileHeader[ 5] = (unsigned char)(fileSize >> 24);
    bmpInfoHeader[ 4] = (unsigned char)(width      );
    bmpInfoHeader[ 5] = (unsigned char)(width >>  8);
    bmpInfoHeader[ 6] = (unsigned char)(width >> 16);
    bmpInfoHeader[ 7] = (unsigned char)(width >> 24);
    bmpInfoHeader[ 8] = (unsigned char)(height      );
    bmpInfoHeader[ 9] = (unsigned char)(height >>  8);
    bmpInfoHeader[10] = (unsigned char)(height >> 16);
    bmpInfoHeader[11] = (unsigned char)(height >> 24);

    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (file) {
        // Write headers
        file.write(reinterpret_cast<char*>(bmpFileHeader), sizeof(bmpFileHeader));
        file.write(reinterpret_cast<char*>(bmpInfoHeader), sizeof(bmpInfoHeader));

        // Write pixel data
        // BMP files store data bottom-to-top so we write from the last row to the first
        for (int y = height - 1; y >= 0; y--) {
            file.write(reinterpret_cast<const char*>(buffer + y * width), width);

            // BMP row size must be a multiple of 4 bytes
            // Add padding bytes if necessary
            int padding = (4 - (width % 4)) % 4;
            for (int pad = 0; pad < padding; ++pad) {
                unsigned char zero = 0;
                file.write(reinterpret_cast<char*>(&zero), 1);
            }
        }

        file.close();
    }
}

void frame_saver_thread() {
    while (true) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(300ms);
        memset(frame, 0, sizeof(frame));
        std::this_thread::sleep_for(10ms);
        write_bmp("/tmp/event_snapshot.bmp", frame, FRAME_WIDTH, FRAME_HEIGHT);
        if (terminate) {
            return;
        }
    }
}

void event_cb(const Metavision::EventCD *begin, const Metavision::EventCD *end) {
    for (const Metavision::EventCD *ev = begin; ev != end; ++ev) {
        frame[ev->x + ev->y * FRAME_WIDTH] = ev->p ? 255 : 128;
    }
}

void signal_handler(int signum) {
    terminate = true;
    kill(ffmpeg_pid, SIGTERM);
}

bool file_exists(const std::string& filePath) {
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
}

void main_process(std::string output_dir, bool *ready, bool record) {
    signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);
    std::cout << "readying event camera" << std::endl;
    Metavision::Camera cam; // create the camera
    cam = Metavision::Camera::from_first_available();
    cam.start();
    cam.cd().add_callback(event_cb);
    std::thread frame_saver_t(frame_saver_thread);
    *ready = true;


    std::cout << "waiting for ffmpeg file to be created" << std::endl;
    while (not file_exists(output_dir + "/video.mp4")) {}
    std::cout << "starting event recording" << std::endl;
    cam.start_recording(output_dir + "events.raw");
    while (terminate == false) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
    int status;
    waitpid(ffmpeg_pid, &status, 0);
}

void child_process(std::string(output_dir), bool *ready, bool record) {
    // take a snapshot
    std::cout << "taking video snapshot" << std::endl;
    std::string snapshot_cmd = "ffmpeg -nostdin -y -f v4l2 -s 1920x1200 -i /dev/video2 -ss 0:0:1 -frames:v 1 -loglevel quiet /tmp/video_snapshot.jpg";
    system(snapshot_cmd.c_str());

    std::cout << "waiting for event camera" << std::endl;

    while (not *ready) {}

    std::cout << "starting video recording" << std::endl;
    std::string record_cmd = "ffmpeg -f v4l2 -framerate 90 -video_size 1920x1200 -input_format mjpeg -i /dev/video2 -c:v copy -y -nostdin -loglevel quiet " + output_dir + "/video.mp4";
    system(record_cmd.c_str());
    return;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Provide an output directory and whether to record\n");
        return -1;
    }

    bool record = false;
    if (std::string(argv[2]) == "true") {
        record = true;
    }

    std::string output_dir;
    std::ostringstream oss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    oss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
    std::string date = oss.str();
    output_dir = std::string(argv[1]) + "/" + date + "_recordings/";
    std::cout << output_dir << std::endl;
    std::filesystem::create_directory(output_dir);

    ffmpeg_pid = fork();

    bool *ready = (bool*) malloc(sizeof(bool));;

    if (ffmpeg_pid == 0) {
        child_process(output_dir, ready, record);
    } else if (ffmpeg_pid > 0) {
        main_process(output_dir, ready, record);
    } else {
        std::cout << "fork failed" << std::endl;
    }
}
