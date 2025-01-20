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

static void animation_context_update(AnimationContext* context, float dt)
{
    ClearBackground(BACKGROUND_COLOR);

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

    context->blobcat_model_rotation += 70.f * dt;
    if (context->blobcat_model_rotation >= 360.f)
        context->blobcat_model_rotation = 0;
}

static void usage(FILE* stream, const char* program_name)
{
    fprintf(stream, "Usage: %s [FLAGS] <UNDERLAY>\n", program_name);
    fprintf(stream, "    FLAGS:\n");
    fprintf(stream, "        --help, -h  Prints this help and exits.\n");
    fprintf(stream, "\n");
    fprintf(stream, "    UNDERLAY: The underlay texture that's going to be used to render the\n");
    fprintf(stream, "              blobcat model.\n");
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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 480, "Blobcat");

    FFMPEG* rendering_ffmpeg = NULL;
    const float rendering_fps = 60;
    float rendering_width = GetScreenWidth();
    float rendering_height = GetScreenHeight();

    AnimationContext context = animation_context_create(blobcat_underlay_texture);
    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsKeyPressed(KEY_R)) {
            rendering_width = GetScreenWidth();
            rendering_height = GetScreenHeight();
            rendering_ffmpeg = ffmpeg_start_rendering((size_t)rendering_width,
                                                      (size_t)rendering_height,
                                                      (size_t)rendering_fps);
            SetTraceLogLevel(LOG_WARNING);
        }

        if (IsKeyPressed(KEY_S) && rendering_ffmpeg) {
            ffmpeg_end_rendering(rendering_ffmpeg);
            rendering_ffmpeg = NULL;
            SetTraceLogLevel(LOG_INFO);
        }

        if (rendering_ffmpeg) {
            RenderTexture frame_texture = LoadRenderTexture(rendering_width, rendering_height);
            BeginTextureMode(frame_texture);
            animation_context_update(&context, 1 / rendering_fps);
            EndTextureMode();

            Image frame_image = LoadImageFromTexture(frame_texture.texture);
            UnloadRenderTexture(frame_texture);

            ffmpeg_send_frame_flipped(rendering_ffmpeg,
                                      frame_image.data,
                                      frame_image.width,
                                      frame_image.height);
            UnloadImage(frame_image);
        }

        animation_context_update(&context, GetFrameTime());

        const char* instruction_text = rendering_ffmpeg
            ? "Press S to stop rendering!"
            : "Press R to start rendering!";
        const float instruction_text_height = 20;
        const float instruction_text_width = MeasureText(instruction_text, instruction_text_height);
        const float instruction_text_padding = 10;
        DrawText(instruction_text,
                 GetScreenWidth() - instruction_text_width - instruction_text_padding,
                 GetScreenHeight() - instruction_text_height - instruction_text_padding,
                 instruction_text_height,
                 WHITE);

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
        }

        DrawFPS(10, 10);
        EndDrawing();
    }
    animation_context_destroy(&context);

    if (rendering_ffmpeg)
        ffmpeg_end_rendering(rendering_ffmpeg);

    CloseWindow();

    return 0;
}
