//   open_the_device();
//   set_the_parameters_of_the_device();
//   while (!done) {
//        /* one or both of these */
//        receive_audio_data_from_the_device();
//    deliver_audio_data_to_the_device();
//   }
//   close the device

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
        
#include "led-matrix.h"

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

//{1, -1, 1}
vector<int> project2D (vector<int> point) {
    vector<int> projectedPoint;
    vector<vector<int>> projectionMatrix = {
        {1,0,0},
        {0,1,0}
    };

    int i = 0;
    for (auto & element : projectionMatrix){    
        int product = 0;
        for (auto & projectionMatrixPoint : element){
            product += projectionMatrixPoint * point.at(i);
        }
        i += 1;
        projectedPoint.push_back(product);
    }

    return projectedPoint;
}

int main (int argc, char *argv[])
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
    
    int cubeScale = 5;
    int cubePOSX = 32;
    int cubePOSY = 32;
    vector<vector<int>> cubePoints = {
        {1, 1, 1},
        {1, 1, -1},
        {1, -1, 1},
        {1, -1, -1},
        {-1, 1, -1},
        {-1, 1, 1},
        {-1, -1, 1},
        {-1, -1, -1}
    };



    

    while (true) {
        for (auto & element : cubePoints){
            canvas->SetPixel(cubePOSX+project2D(element).at(0), cubePOSY+project2D(element).at(1), 255, 0, 0);
        }
        signal(SIGTERM, InterruptHandler);
        signal(SIGINT, InterruptHandler);
    }

    canvas->Clear();
    return 0;
}
