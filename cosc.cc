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

    // int i = 0;
    // for (auto & element : projectionMatrix){    
    //     int product = 0;
    //     for (auto & projectionMatrixPoint : element){
    //         product += projectionMatrixPoint * point.at(i);
    //     }
    //     i += 1;
    //     projectedPoint.push_back(product);
    // }

    // return projectedPoint;
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
    
    int cubeScale = 5;
    int cubePOSX = 32;
    int cubePOSY = 32;
    float angle = 0;
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
    
    while (true) {
        vector<vector<float>> rotationZMatrix = {
            {cos(angle),-1*sin(angle), 0},
            {sin(angle), cos(angle), 0},
            {0,0,1}
        };
        vector<vector<float>> rotationXMatrix = {
            {1,0,0},
            {0, cos(angle), -1*sin(angle)},
            {0, sin(angle), cos(angle)}
        };
        vector<vector<float>> rotationYMatrix = {
            {cos(angle),0,sin(angle)},
            {0,1,0},
            {-1*sin(angle),0,cos(angle)}
        };
        
        Color color(255, 255, 0);
        rgb_matrix::DrawLine(canvas, 10, 10, 15, 15, color);
        for (auto & point : cubePoints){
            vector<float> rotatedZPoint = multiplyMatrices(rotationZMatrix,point);
            vector<float> rotatedXPoint = multiplyMatrices(rotationXMatrix,rotatedZPoint);
            vector<float> rotatedYPoint = multiplyMatrices(rotationYMatrix,rotatedXPoint);
            vector<float> projected = project2D(rotatedYPoint);
            float x = cubePOSX+projected.at(0)*cubeScale;
            float y = cubePOSY+projected.at(1)*cubeScale;
            canvas->SetPixel((int) x, (int) y, 255, 0, 0);
        }
        canvas->Clear();
        angle += 0.1;
        signal(SIGTERM, InterruptHandler);
        signal(SIGINT, InterruptHandler);
    }

    canvas->Clear();
    exit (0);
}
