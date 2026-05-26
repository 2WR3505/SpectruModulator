#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define SAMPLE_RATE     44100
#define DURATION_SEC    50.0
#define MIN_FREQ        300.0
#define MAX_FREQ        3700.0
#define MAX_AMPLITUDE   0.65
#define OVERLAP         0.30     // 30% overlap - balanced

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_image> <output.wav> [duration_sec]\n", argv[0]);
        return 1;
    }

    const char *img_path = argv[1];
    const char *wav_path = argv[2];
    double duration = (argc > 3) ? atof(argv[3]) : DURATION_SEC;

    int width, height, channels;
    unsigned char *img = stbi_load(img_path, &width, &height, &channels, 1);
    if (!img) {
        fprintf(stderr, "Error loading image %s\n", img_path);
        return 1;
    }

    printf("Loaded: %dx%d pixels\n", width, height);

    SF_INFO sfinfo = {0};
    sfinfo.channels   = 2;
    sfinfo.samplerate = SAMPLE_RATE;
    sfinfo.format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE *file = sf_open(wav_path, SFM_WRITE, &sfinfo);
    if (!file) {
        fprintf(stderr, "Error: %s\n", sf_strerror(NULL));
        stbi_image_free(img);
        return 1;
    }

    long total_samples = (long)(duration * SAMPLE_RATE);
    long base_col_samples = total_samples / width;
    long overlap_samples = (long)(base_col_samples * OVERLAP);
    long step_samples = base_col_samples - overlap_samples;

    if (step_samples < 64) step_samples = 64;

    double *buffer = malloc(base_col_samples * sizeof(double));
    double *stereo = malloc(base_col_samples * 2 * sizeof(double));
    double *phases = calloc(height, sizeof(double));
    double *freqs  = malloc(height * sizeof(double));
    double *window = malloc(base_col_samples * sizeof(double));

    if (!buffer || !stereo || !phases || !freqs || !window) {
        fprintf(stderr, "Memory allocation failed\n");
        sf_close(file);
        stbi_image_free(img);
        return 1;
    }

    // Hann window
    for (long i = 0; i < base_col_samples; ++i) {
        window[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (base_col_samples - 1)));
    }

    double freq_step = (MAX_FREQ - MIN_FREQ) / (height > 1 ? height - 1 : 1);
    for (int row = 0; row < height; ++row) {
        freqs[row] = MIN_FREQ + row * freq_step;
    }

    printf("Generating FM Stereo audio | Duration: %.1fs | Overlap: %.0f%%\n", duration, OVERLAP*100);

    for (int col = 0; col < width; ++col) {
        memset(buffer, 0, base_col_samples * sizeof(double));

        for (int row = 0; row < height; ++row) {
            int brightness = img[col + row * width];
            if (brightness < 8) continue;

            double amp = (brightness / 255.0) * MAX_AMPLITUDE;
            double freq = freqs[row];
            double phase = phases[row];
            double phase_inc = 2.0 * M_PI * freq / SAMPLE_RATE;

            for (long s = 0; s < base_col_samples; ++s) {
                buffer[s] += amp * sin(phase);
                phase += phase_inc;
            }
            phases[row] = phase;
        }

        // Apply window
        for (long s = 0; s < base_col_samples; ++s) {
            buffer[s] *= window[s];
        }

        // Soft clip
        for (long s = 0; s < base_col_samples; ++s) {
            if (buffer[s] > 1.0)  buffer[s] = 1.0;
            if (buffer[s] < -1.0) buffer[s] = -1.0;
        }

        // To stereo
        for (long s = 0; s < base_col_samples; ++s) {
            stereo[2*s]   = buffer[s];
            stereo[2*s+1] = buffer[s];
        }

        sf_write_double(file, stereo, base_col_samples);
    }

    sf_close(file);
    free(buffer); free(stereo); free(phases); free(freqs); free(window);
    stbi_image_free(img);

    printf("✅ Done: %s\n", wav_path);
    return 0;
}
