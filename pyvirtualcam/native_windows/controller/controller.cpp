#include <stdio.h>
#include "queue/share_queue.h"
#include "queue/share_queue_write.h"
#include "controller.h"

struct virtual_out_data {
	share_queue video_queue;
	int width = 0;
	int height = 0;
};

static struct virtual_out_data *virtual_out;
static struct virtual_out_data *virtual_out_1;
static bool output_running = false;
static bool output_running_1 = false;

static bool have_clockfreq = false;
static LARGE_INTEGER clock_freq;

static uint64_t get_timestamp_ns()
{
	LARGE_INTEGER current_time;
	double time_val;

	if (!have_clockfreq) {
		QueryPerformanceFrequency(&clock_freq);
		have_clockfreq = true;
	}

	QueryPerformanceCounter(&current_time);
	time_val = (double)current_time.QuadPart;
	time_val *= 1000000000.0;
	time_val /= (double)clock_freq.QuadPart;

	return (uint64_t)time_val;
}

virtual_out_data* create_out_data(int channel) {
	if (channel == 0) {
		virtual_out = (virtual_out_data*) malloc(sizeof(struct virtual_out_data));
		return (virtual_out_data*)virtual_out;
	} else {
		virtual_out_1 = (virtual_out_data*) malloc(sizeof(struct virtual_out_data));
		return (virtual_out_data*)virtual_out_1;
	}
}

bool virtual_output_start(int width, int height, double fps, int delay, int channel)
{
	bool start = false;

	virtual_out_data* out_data = create_out_data(channel);

	out_data->width = width;
	out_data->height = height;
	uint64_t interval = static_cast<int64_t>(1000000000 / fps);

	if (channel == 0) {
		start = shared_queue_create(&out_data->video_queue,
			ModeVideo, AV_PIX_FMT_RGBA, out_data->width, out_data->height,
			interval, delay + 10);
	} else {
		start = shared_queue_create(&out_data->video_queue,
			ModeVideo2, AV_PIX_FMT_RGBA, out_data->width, out_data->height,
			interval, delay + 10);
	}
	if (start) {
		if (channel == 0) {
			output_running = true;
		} else {
			output_running_1 = true;
		}
		shared_queue_set_delay(&out_data->video_queue, delay);
	} else {
		if (channel == 0) {
			output_running = false;
		} else {
			output_running_1 = false;
		}
		shared_queue_write_close(&out_data->video_queue);

		fprintf(stderr, "shared_queue_create() failed\n");
	}

	return start;
}



void virtual_output_stop()
{
    if (!virtual_out) {
        return;
	}	
	shared_queue_write_close(&virtual_out->video_queue);
    free(virtual_out);
	output_running = false;
}

// data is in RGBA format (packed RGBA 8:8:8:8, 32bpp, RGBARGBA...)
// TODO RGB24 would be better but not supported in obs-virtual-cam
void virtual_video(uint8_t **data, int channel)
{
	if (channel == 0) {
	if (!output_running)
		return;

    uint32_t linesize[1] = {virtual_out->width * 4};
	uint64_t timestamp = get_timestamp_ns();

	virtual_out_data *out_data = (virtual_out_data*)virtual_out;
    shared_queue_push_video(&out_data->video_queue, linesize, 
        out_data->width, out_data->height, data, timestamp);
	} else {
		if (!output_running_1)
			return;

		uint32_t linesize[1] = {virtual_out_1->width * 4};
		uint64_t timestamp = get_timestamp_ns();

		virtual_out_data *out_data = (virtual_out_data*)virtual_out_1;
		shared_queue_push_video(&out_data->video_queue, linesize,
		out_data->width, out_data->height, data, timestamp);
	}
}

bool virtual_output_is_running()
{
	return output_running;
}
