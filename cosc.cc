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

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using namespace std;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

float SetCubeScale(short buf[]){
    // audio wave has minimum amplitude of -32,768 and a maximum value of 32,767
    bool isClipping = false;
    int cubeScale = 10;
    for (int i=0; i<128; i++){
        if(buf[i] < -32767 || buf[i] > 32766){
            isClipping = true;
        }
    }

    if(isClipping){
        cubeScale = 15;
    }
    return cubeScale;
}

vector<float> multiplyMatrices(const vector<vector<float>>& A, const vector<float>& B) {
    vector<float> C(A.size(), 0); // Initialize result vector with zeros

    for (size_t i = 0; i < A.size(); ++i) {
        for (size_t j = 0; j < B.size(); ++j) {
            C[i] += A[i][j] * B[j];
        }
    }

    return C;
}

vector<float> project2D (vector<float> vertex) {
    vector<float> projectedPoint;
    vector<vector<float>> projectionMatrix = {
        {1,0,0},
        {0,1,0},
        {0,0,0}
    };
    return multiplyMatrices(projectionMatrix, vertex);
}

main (int argc, char *argv[])
{
    // matrix is hardcoded for 64x64 unless someone wants to fix that
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "adafruit-hat";  // or e.g. ""
    defaults.rows = 64;
    defaults.cols = 64;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    Canvas *canvas = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);
    if (canvas == NULL)
        return 1;
    
    // cube setup
    rgb_matrix::Color color(0, 0, 255);
    float cubeScale = 10;
    int cubePOSX = 32;
    int cubePOSY = 32;
    float anglex = 0;
    float angley = 0;
    float anglez = 0;
    vector<vector<float>> cubePoints = {
        {1, 1, 1},
        {1, 1, -1},
        {1, -1, 1},
        {1, -1, -1},
        {-1, 1, -1},
        {-1, 1, 1},
        {-1, -1, 1},
        {-1, -1, -1}
    };

    // sound stuff
    int i;
    int err;
    short buf[128];
    snd_pcm_t *capture_handle;
    snd_pcm_hw_params_t *hw_params;
    unsigned int dumb = 10;
    unsigned int* test = &dumb;
    if ((err = snd_pcm_open (&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n", 
                argv[1],
                snd_strerror (err));
        exit (1);
    }
        
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        exit (1);
    }
                
    if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, test, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
                snd_strerror (err));
        exit (1);
    }

    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (capture_handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        exit (1);
    }
    
    while (true) {
        // read interface into buffer
        if ((err = snd_pcm_readi (capture_handle, buf, 128)) != 128) {
            fprintf (stderr, "read from audio interface failed (%s)\n",
                    snd_strerror (err));
            exit (1);
        }
        
                
        // calulate position cube and render as 2D
        vector<vector<float>> rotationZMatrix = {
            {cos(anglez),-1*sin(anglez), 0},
            {sin(anglez), cos(anglez), 0},
            {0,0,1}
        };
        vector<vector<float>> rotationXMatrix = {
            {1,0,0},
            {0, cos(anglex), -1*sin(anglex)},
            {0, sin(anglex), cos(anglex)}
        };
        vector<vector<float>> rotationYMatrix = {
            {cos(angley),0,sin(angley)},
            {0,1,0},
            {-1*sin(angley),0,cos(angley)}
        };
        
    
        vector<vector<int>> rotatedPoints={};
        for (auto & point : cubePoints){	
            vector<float> rotatedZPoint = multiplyMatrices(rotationZMatrix,point);
            vector<float> rotatedXPoint = multiplyMatrices(rotationXMatrix,rotatedZPoint);
            vector<float> rotatedYPoint = multiplyMatrices(rotationYMatrix,rotatedXPoint);
            vector<float> projected = project2D(rotatedYPoint);
            float x = cubePOSX+projected.at(0)*cubeScale;
            float y = cubePOSY+projected.at(1)*cubeScale;
            canvas->SetPixel((int) x, (int) y, 0, 0, 255);
            rotatedPoints.push_back({(int) x, (int) y});
        }

        // draw cube
	canvas->Clear();
        // first pane
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(0).at(0), rotatedPoints.at(0).at(1), rotatedPoints.at(1).at(0), rotatedPoints.at(1).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(1).at(0), rotatedPoints.at(1).at(1), rotatedPoints.at(3).at(0), rotatedPoints.at(3).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(3).at(0), rotatedPoints.at(3).at(1), rotatedPoints.at(2).at(0), rotatedPoints.at(2).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(2).at(0), rotatedPoints.at(2).at(1), rotatedPoints.at(0).at(0), rotatedPoints.at(0).at(1), color);
	    
        // second pane
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(4).at(0), rotatedPoints.at(4).at(1), rotatedPoints.at(5).at(0), rotatedPoints.at(5).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(5).at(0), rotatedPoints.at(5).at(1), rotatedPoints.at(6).at(0), rotatedPoints.at(6).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(6).at(0), rotatedPoints.at(6).at(1), rotatedPoints.at(7).at(0), rotatedPoints.at(7).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(7).at(0), rotatedPoints.at(7).at(1), rotatedPoints.at(4).at(0), rotatedPoints.at(4).at(1), color);

        // last 4 lines
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(4).at(0), rotatedPoints.at(4).at(1), rotatedPoints.at(1).at(0), rotatedPoints.at(1).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(5).at(0), rotatedPoints.at(5).at(1), rotatedPoints.at(0).at(0), rotatedPoints.at(0).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(6).at(0), rotatedPoints.at(6).at(1), rotatedPoints.at(2).at(0), rotatedPoints.at(2).at(1), color);
        rgb_matrix::DrawLine(canvas, rotatedPoints.at(7).at(0), rotatedPoints.at(7).at(1), rotatedPoints.at(3).at(0), rotatedPoints.at(3).at(1), color);
        
        anglex += 0.01;
	angley += 0.04;
	anglez += 0.07;
        if (cubeScale > 10){
            cubeScale -= 0.1;
        }
	else{
	    // set scale based off buffer
            cubeScale = SetCubeScale(buf);
	}

        signal(SIGTERM, InterruptHandler);
        signal(SIGINT, InterruptHandler);
    }

    canvas->Clear();
    exit (0);
}
