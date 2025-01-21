#ifndef FFMPEG_H_
#define FFMPEG_H_

#include <stddef.h>

typedef struct FFMPEG FFMPEG;

typedef enum {
    FFMPEG_FORMAT_MP4,
    FFMPEG_FORMAT_GIF,
} FFMPEGFormat;

FFMPEG* ffmpeg_start_rendering(size_t width, size_t height, size_t fps, FFMPEGFormat format);
void ffmpeg_send_frame(FFMPEG* ffmpeg, void* data, size_t width, size_t height);
void ffmpeg_send_frame_flipped(FFMPEG* ffmpeg, void* data, size_t width, size_t height);
void ffmpeg_end_rendering(FFMPEG* ffmpeg);

#endif // FFMPEG_H_
