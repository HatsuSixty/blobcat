#include <stdio.h>
#include <string.h>

#include <raylib.h>

#include "ffmpeg.h"

#define BACKGROUND_COLOR GetColor(0x181818FF)

static Texture generate_blobcat_texture(Texture overlay, Texture underlay)
{
    RenderTexture2D final_texture = LoadRenderTexture(overlay.width, overlay.height);
    BeginTextureMode(final_texture);
    DrawTexturePro(underlay,
                   (Rectangle) { 0, 0, overlay.width, overlay.height },
                   (Rectangle) { 0, 0, overlay.width, overlay.height },
                   (Vector2) { 0, 0 }, 0.0f, WHITE);
    DrawTexturePro(overlay,
                   (Rectangle) { 0, 0, overlay.width, overlay.height },
                   (Rectangle) { 0, 0, overlay.width, overlay.height },
                   (Vector2) { 0, 0 }, 0.0f, WHITE);
    EndTextureMode();
    return final_texture.texture;
}

typedef struct {
    Camera3D camera;
    Model blobcat_model;
    float blobcat_model_rotation;
} AnimationContext;

static AnimationContext animation_context_create(const char* underlay)
{
    Image blobcat_image_overlay = LoadImage("./assets/textures/tiny_blobcat_overlay.png");
    Image blobcat_image_underlay = LoadImage(underlay);
    ImageFlipVertical(&blobcat_image_overlay);
    ImageFlipVertical(&blobcat_image_underlay);

    Model blobcat_model = LoadModel("./assets/models/tiny_blobcat.obj");
    Texture blobcat_texture_overlay = LoadTextureFromImage(blobcat_image_overlay);
    Texture blobcat_texture_underlay = LoadTextureFromImage(blobcat_image_underlay);
    Texture blobcat_texture = generate_blobcat_texture(blobcat_texture_overlay, blobcat_texture_underlay);
    blobcat_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = blobcat_texture;

    UnloadImage(blobcat_image_overlay);
    UnloadImage(blobcat_image_underlay);
    UnloadTexture(blobcat_texture_overlay);
    UnloadTexture(blobcat_texture_underlay);

    return (AnimationContext) {
        .camera = {
            .position = { 0.0f, 20.0f, 50.0f },
            .target = { 0.0f, 15.0f, 0.0f },
            .up = { 0.0f, 1.0f, 0.0f },
            .fovy = 45.0f,
            .projection = CAMERA_PERSPECTIVE,
        },
        .blobcat_model = blobcat_model,
        .blobcat_model_rotation = 0,
    };
}

static void animation_context_destroy(AnimationContext* context)
{
    UnloadTexture(context->blobcat_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    UnloadModel(context->blobcat_model);
}

static bool animation_context_update(AnimationContext* context, float dt, Color background_color)
{
    ClearBackground(background_color);

    BeginMode3D(context->camera);
    {
        const float blobcat_model_scale = 30.0f;
        DrawModelEx(context->blobcat_model,
                    (Vector3) { 0, 4, 0 },
                    (Vector3) { 0, 1, 0 }, context->blobcat_model_rotation,
                    (Vector3) {
                        blobcat_model_scale,
                        blobcat_model_scale,
                        blobcat_model_scale,
                    },
                    WHITE);
    }
    EndMode3D();

    context->blobcat_model_rotation += 150.f * dt;
    bool animation_finished = context->blobcat_model_rotation >= 360.f;
    if (animation_finished)
        context->blobcat_model_rotation = 0;

    return animation_finished;
}

static void usage(FILE* stream, const char* program_name)
{
    fprintf(stream, "Usage: %s [FLAGS] <UNDERLAY> <FORMAT>\n", program_name);
    fprintf(stream, "    FLAGS:\n");
    fprintf(stream, "        --help, -h  Prints this help and exits.\n");
    fprintf(stream, "\n");
    fprintf(stream, "    UNDERLAY: The underlay texture that's going to be used to render the\n");
    fprintf(stream, "              blobcat model.\n");
    fprintf(stream, "\n");
    fprintf(stream, "    FORMAT: The format to generate the video in when rendering. The available\n");
    fprintf(stream, "            formats are:\n");
    fprintf(stream, "                mp4  Generates the video in MP4 format.\n");
    fprintf(stream, "                gif  Generates the video in GIF format.\n");
}

int main(int argc, const char** argv)
{
    const char* program_name = argv[0];

    if (argc < 2) {
        fprintf(stderr, "ERROR: input file not provided\n");
        usage(stderr, program_name);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage(stdout, program_name);
        return 0;
    }

    const char* blobcat_underlay_texture = argv[1];

    if (argc < 3) {
        fprintf(stderr, "ERROR: file format not specified\n");
        usage(stderr, program_name);
        return 1;
    }

    FFMPEGFormat video_format;
    if (strcmp(argv[2], "mp4") == 0)
        video_format = FFMPEG_FORMAT_MP4;
    else if (strcmp(argv[2], "gif") == 0)
        video_format = FFMPEG_FORMAT_GIF;
    else {
        fprintf(stderr, "ERROR: invalid file format: `%s`\n", argv[2]);
        usage(stderr, program_name);
        return 1;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 480, "Blobcat");

    AnimationContext animation_context = animation_context_create(blobcat_underlay_texture);

    FFMPEG* rendering_ffmpeg = NULL;
    const float rendering_fps = 25;
    const float rendering_width = (float)((int)(GetScreenWidth() / 2.f));
    const float rendering_height = rendering_width;
    AnimationContext rendering_animation_context;

    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsKeyPressed(KEY_R)) {
            rendering_animation_context = animation_context_create(blobcat_underlay_texture);
            rendering_ffmpeg = ffmpeg_start_rendering((size_t)rendering_width,
                                                      (size_t)rendering_height,
                                                      (size_t)rendering_fps,
                                                      video_format);
            SetTraceLogLevel(LOG_WARNING);
        }

        if (rendering_ffmpeg) {
            RenderTexture frame_texture = LoadRenderTexture(rendering_width, rendering_height);
            BeginTextureMode(frame_texture);
            bool animation_finished = animation_context_update(&rendering_animation_context,
                                                               1 / rendering_fps, BLANK);
            EndTextureMode();

            if (animation_finished) {
                SetTraceLogLevel(LOG_INFO);
                ffmpeg_end_rendering(rendering_ffmpeg);
                rendering_ffmpeg = NULL;
                animation_context_destroy(&rendering_animation_context);
            } else {
                Image frame_image = LoadImageFromTexture(frame_texture.texture);
                UnloadRenderTexture(frame_texture);

                ffmpeg_send_frame_flipped(rendering_ffmpeg,
                                          frame_image.data,
                                          frame_image.width,
                                          frame_image.height);
                UnloadImage(frame_image);
            }
        }

        animation_context_update(&animation_context, GetFrameTime(),
                                 BACKGROUND_COLOR);

        if (rendering_ffmpeg) {
            const char* rendering_text = "Rendering...";
            const float rendering_text_height = 20;
            const float rendering_text_width = MeasureText(rendering_text, rendering_text_height);
            const float rendering_text_padding = 10;
            DrawText(rendering_text,
                     GetScreenWidth() - rendering_text_width - rendering_text_padding,
                     rendering_text_padding,
                     rendering_text_height,
                     RED);
        } else {
            const char* instruction_text = "Press R to start rendering!";
            const float instruction_text_height = 20;
            const float instruction_text_width = MeasureText(instruction_text, instruction_text_height);
            const float instruction_text_padding = 10;
            DrawText(instruction_text,
                     GetScreenWidth() - instruction_text_width - instruction_text_padding,
                     GetScreenHeight() - instruction_text_height - instruction_text_padding,
                     instruction_text_height,
                     WHITE);
        }

        DrawFPS(10, 10);
        EndDrawing();
    }
    animation_context_destroy(&animation_context);

    if (rendering_ffmpeg)
        ffmpeg_end_rendering(rendering_ffmpeg);

    CloseWindow();

    return 0;
}
