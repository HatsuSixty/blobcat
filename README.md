# Blobcat

Render low poly spinning blobcat.

## Quick Start

To build, run:
```
$ ./build.sh
```

To run the compiled executable, you will need to provide an underlay texture for the blobcat model. Sample textures are provided under `./assets/textures/`:
```
$ ./main ./assets/textures/tiny_blobcat_trans_underlay.png
```

Press `R` to start rendering the animation into a video, and press `S` to stop rendering. The generated video will be available at `./output.mp4`.

## Credits

- `./assets/models/tiny_blobcat.obj` - OBJ file rendered by [green](https://mk.absturztau.be/@green) from the `.blend` file available at <https://gitlab.com/renere/spinny_blobs/>;
- `./src/ffmpeg.c` & `./src/ffmpeg.h` - <https://github.com/tsoding/rendering-video-in-c-with-ffmpeg>.
