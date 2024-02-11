#include <gst/gst.h>
#include <gst/app/gstappsink.h>
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


#define FRAME_WIDTH 720
#define FRAME_HEIGHT 480

std::string tmp_dir = TMP_DIR;


bool terminate_saver = false;
bool terminate_camera = false;
bool terminate = false;


uint32_t event_count;
uint8_t event_frame[FRAME_WIDTH * FRAME_HEIGHT];
bool video_frame_available = false;
uint8_t video_frame[300000];
uint64_t video_frame_size;
std::vector<std::pair<uint64_t, uint64_t>> timestamps;
std::mutex timestamps_mutex;

Metavision::Camera cam;

uint16_t frame_count = 0;

void saver_thread(std::string output_dir) {
    std::string video_snapshot_path = tmp_dir + VIDEO_SNAPSHOT_FILE_NAME;
    std::string event_snapshot_path = tmp_dir + EVENT_SNAPSHOT_FILE_NAME;
    std::string timestamps_path = output_dir + "/timestamps.csv";

    std::ofstream timestamps_file (timestamps_path);
    timestamps_file << "event_time,frame_time\n";
    while (true) {

        //1 Hz update rate is sufficient, logging of raw events and camera video is done elsewhere
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(700ms);
        video_frame_available = false;
        std::this_thread::sleep_for(80ms);
        memset(event_frame, 0, sizeof(event_frame));
        std::this_thread::sleep_for(20ms);

        // write video frame to file
        std::ofstream video_snapshot_file (video_snapshot_path);
        video_snapshot_file.write((const char *) video_frame, video_frame_size);

        // write event frame to file
        stbi_write_bmp(event_snapshot_path.c_str(), FRAME_WIDTH, FRAME_HEIGHT, 1, event_frame);

        // write timestamps to file
        std::unique_lock<std::mutex> lock(timestamps_mutex);
        std::vector<std::pair<uint64_t, uint64_t>> timestamps_copy (timestamps);
        timestamps.clear();
        lock.unlock();
        for (auto v : timestamps_copy) {
            timestamps_file << v.first << "," << v.second << "\n";
        }
        std::cout << "handled " << timestamps_copy.size() << "frames " << "and " << event_count / 1000000 << "M events" << std::endl;
        event_count = 0;
    }
}

// called every time a gstreamer frame is passed
static void on_identity_handoff(GstElement *identity, GstBuffer *buffer, GstPad *pad, gpointer user_data) {
    frame_count++;
    uint64_t frame_time = GST_BUFFER_PTS(buffer) / 1000; // nano to micro
    uint64_t event_time = cam.get_last_timestamp(); // micro

    std::unique_lock<std::mutex> lock(timestamps_mutex);
    timestamps.push_back({event_time, frame_time});
    lock.unlock();
    if (video_frame_available == false) {
        GstMapInfo map;
        // Map the buffer so we can access its data
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            if (sizeof(video_frame) < map.size) {
                std::cout << "video frame too small " << map.size << std::endl;
            }
            memcpy(video_frame, map.data, map.size);
            video_frame_size = map.size;
            gst_buffer_unmap(buffer, &map);
        } else {
            std::cerr << "Could not map buffer for reading." << std::endl;
        }
    }
}

int gstreamer_thread(std::string output_dir) {
    GstElement *pipeline, *source, *filter, *identity, *muxer, *sink;
    GMainLoop *loop;
    GstCaps *caps;

    gst_init(NULL, NULL);

    // Create elements
    source = gst_element_factory_make("v4l2src", "source");
    filter = gst_element_factory_make("capsfilter", "filter");
    identity = gst_element_factory_make("identity", "identity");
    muxer = gst_element_factory_make("avimux", "muxer");
    sink = gst_element_factory_make("filesink", "sink");

    if (!source || !filter || !identity || !muxer || !sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    g_object_set(G_OBJECT(source), "device", VIDEO_CAMERA, NULL);
    g_object_set(G_OBJECT(sink), "location", (output_dir + "/video.avi").c_str(), NULL);

    // Set the caps for the filter
    caps = gst_caps_new_simple("image/jpeg",
                               "width", G_TYPE_INT, 1920,
                               "height", G_TYPE_INT, 1200,
                               "framerate", GST_TYPE_FRACTION, 90, 1,
                               NULL);
    g_object_set(G_OBJECT(filter), "caps", caps, NULL);
    gst_caps_unref(caps);

    g_object_set(G_OBJECT(identity), "signal-handoffs", TRUE, NULL);
    g_signal_connect(identity, "handoff", G_CALLBACK(on_identity_handoff), NULL);

    pipeline = gst_pipeline_new("capture-pipeline");

    gst_bin_add_many(GST_BIN(pipeline), source, filter, identity, muxer, sink, NULL);
    if (!gst_element_link_many(source, filter, identity, muxer, sink, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

void event_cb(const Metavision::EventCD *begin, const Metavision::EventCD *end) {
    for (const Metavision::EventCD *ev = begin; ev != end; ++ev) {
        if (ev->x >= FRAME_WIDTH || ev->y >= FRAME_HEIGHT) {
            continue;
        }
        event_frame[ev->x + ev->y * FRAME_WIDTH] = ev->p ? 255 : 128;
        event_count++;
    }
}

int event_camera_thread(std::string output_dir) {
    std::string events_output_path = output_dir + "/events.raw";

    cam = Metavision::Camera::from_first_available();
    cam.start();
    cam.cd().add_callback(event_cb);
    cam.start_recording(events_output_path);
    while (true) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
    }
}

int main(int argc, char** argv) {
    using namespace std::chrono_literals;
    if (argc != 2) {
        printf("Provide an output directory\n");
        return -1;
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

    std::thread saver_t(saver_thread, output_dir);
    std::thread event_camera_t(event_camera_thread, output_dir);
    std::this_thread::sleep_for(5s); // give event camera time to get ready
    std::thread gstreamer_t(gstreamer_thread, output_dir);

    while (true) {
        std::this_thread::sleep_for(1000ms);
    }
}