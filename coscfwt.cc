#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "led-matrix.h"
#include "graphics.h"
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <utility>
#include <chrono>
#include <thread>
#include <fftw3.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using namespace std;

#define FFT_SIZE 1024
#define AUDIO_BUFFER_SIZE (FFT_SIZE * 2)  // 2x for stereo

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

vector<float> multiplyMatrices(const vector<vector<float>>& A, const vector<float>& B) {
    vector<float> C(A.size(), 0);
    for (size_t i = 0; i < A.size(); ++i)
        for (size_t j = 0; j < B.size(); ++j)
            C[i] += A[i][j] * B[j];
    return C;
}

vector<float> project2D(vector<float> vertex) {
    vector<vector<float>> projectionMatrix = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 0}
    };
    return multiplyMatrices(projectionMatrix, vertex);
}

float ScaleFromFFT(fftw_complex* out, int fft_size, float sample_rate, float& cubeScale) {
    double bin_width = sample_rate / fft_size;
    int low_freq_bin_limit = 4; // Covers ~43–172 Hz
    double low_freq_energy = 0;

    for (int i = 1; i <= low_freq_bin_limit; ++i) {
        double mag = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
        low_freq_energy += mag;
    }

    // Debug output (optional)
    // std::cout << "Low frequency energy: " << low_freq_energy << std::endl;

    float threshold = 0.3;
    if (low_freq_energy > threshold) {
        cubeScale = 15;
    } else {
        cubeScale -= 0.5;
        if (cubeScale < 10) cubeScale = 10;
    }

    return cubeScale;
}

int main(int argc, char* argv[]) {
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "adafruit-hat";
    defaults.rows = 64;
    defaults.cols = 64;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    Canvas* canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);
    if (canvas == NULL) return 1;

    rgb_matrix::Color color(0, 0, 255);
    float cubeScale = 10;
    int cubePOSX = 32, cubePOSY = 32;
    float anglex = 0, angley = 0, anglez = 0;
    vector<vector<float>> cubePoints = {
        {1, 1, 1}, {1, 1, -1}, {1, -1, 1}, {1, -1, -1},
        {-1, 1, -1}, {-1, 1, 1}, {-1, -1, 1}, {-1, -1, -1}
    };

    // Audio
    short buf[AUDIO_BUFFER_SIZE];
    double fft_input[FFT_SIZE];
    fftw_complex fft_output[FFT_SIZE / 2 + 1];
    fftw_plan plan = fftw_plan_dft_r2c_1d(FFT_SIZE, fft_input, fft_output, FFTW_ESTIMATE);

    snd_pcm_t* capture_handle;
    snd_pcm_hw_params_t* hw_params;
    unsigned int rate = 44100;

    int err;
    if ((err = snd_pcm_open(&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0 ||
        (err = snd_pcm_hw_params_malloc(&hw_params)) < 0 ||
        (err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0 ||
        (err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0 ||
        (err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0 ||
        (err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0 ||
        (err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 2)) < 0 ||
        (err = snd_pcm_hw_params(capture_handle, hw_params)) < 0 ||
        (err = snd_pcm_prepare(capture_handle)) < 0) {
        fprintf(stderr, "Audio init error: %s\n", snd_strerror(err));
        exit(1);
    }
    snd_pcm_hw_params_free(hw_params);

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    while (!interrupt_received) {
        if ((err = snd_pcm_readi(capture_handle, buf, AUDIO_BUFFER_SIZE / 2)) != AUDIO_BUFFER_SIZE / 2) {
            fprintf(stderr, "Audio read failed (%s)\n", snd_strerror(err));
            continue;
        }

        // Convert stereo to mono and normalize
        for (int i = 0; i < AUDIO_BUFFER_SIZE; i += 2) {
            double mono = (buf[i] + buf[i + 1]) / 2.0 / 32768.0;

            // Apply Hanning window
            int index = i / 2;
            double window = 0.5 * (1 - cos(2 * M_PI * index / (FFT_SIZE - 1)));
            fft_input[index] = mono * window;
        }

        fftw_execute(plan);
        cubeScale = ScaleFromFFT(fft_output, FFT_SIZE, rate, cubeScale);

        // Rotation matrices
        vector<vector<float>> rotationZ = {
            {cos(anglez), -sin(anglez), 0},
            {sin(anglez), cos(anglez), 0},
            {0, 0, 1}
        };
        vector<vector<float>> rotationX = {
            {1, 0, 0},
            {0, cos(anglex), -sin(anglex)},
            {0, sin(anglex), cos(anglex)}
        };
        vector<vector<float>> rotationY = {
            {cos(angley), 0, sin(angley)},
            {0, 1, 0},
            {-sin(angley), 0, cos(angley)}
        };

        vector<vector<int>> rotatedPoints;
        for (auto& point : cubePoints) {
            auto rotated = multiplyMatrices(rotationZ, point);
            rotated = multiplyMatrices(rotationX, rotated);
            rotated = multiplyMatrices(rotationY, rotated);
            auto projected = project2D(rotated);
            float x = cubePOSX + projected[0] * cubeScale;
            float y = cubePOSY + projected[1] * cubeScale;
            rotatedPoints.push_back({(int)x, (int)y});
        }

        canvas->Clear();
        // First pane
        rgb_matrix::DrawLine(canvas, rotatedPoints[0][0], rotatedPoints[0][1], rotatedPoints[1][0], rotatedPoints[1][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[1][0], rotatedPoints[1][1], rotatedPoints[3][0], rotatedPoints[3][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[3][0], rotatedPoints[3][1], rotatedPoints[2][0], rotatedPoints[2][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[2][0], rotatedPoints[2][1], rotatedPoints[0][0], rotatedPoints[0][1], color);
        // Second pane
        rgb_matrix::DrawLine(canvas, rotatedPoints[4][0], rotatedPoints[4][1], rotatedPoints[5][0], rotatedPoints[5][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[5][0], rotatedPoints[5][1], rotatedPoints[6][0], rotatedPoints[6][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[6][0], rotatedPoints[6][1], rotatedPoints[7][0], rotatedPoints[7][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[7][0], rotatedPoints[7][1], rotatedPoints[4][0], rotatedPoints[4][1], color);
        // Connecting lines
        rgb_matrix::DrawLine(canvas, rotatedPoints[4][0], rotatedPoints[4][1], rotatedPoints[1][0], rotatedPoints[1][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[5][0], rotatedPoints[5][1], rotatedPoints[0][0], rotatedPoints[0][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[6][0], rotatedPoints[6][1], rotatedPoints[2][0], rotatedPoints[2][1], color);
        rgb_matrix::DrawLine(canvas, rotatedPoints[7][0], rotatedPoints[7][1], rotatedPoints[3][0], rotatedPoints[3][1], color);

        anglex += 0.01;
        angley += 0.004;
        anglez += 0.007;

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    canvas->Clear();
    fftw_destroy_plan(plan);
    snd_pcm_close(capture_handle);
    return 0;
}
