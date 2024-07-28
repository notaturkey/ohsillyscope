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


vector<vector<float>> multiplyMatrices(vector<vector<float>> A,vector<vector<float>> B) {
    int rowsA = A.size();
    int colsA = A[0].size();
    int rowsB = B.size();
    int colsB = B[0].size();

    // Ensure matrices can be multiplied
    if (colsA != rowsB) {
        throw invalid_argument("Number of columns in A must be equal to the number of rows in B.");
    }

    // Initialize result matrix with zeros
    vector<vector<float>> C(rowsA, vector<float>(colsB, 0));

    // Perform matrix multiplication
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            for (int k = 0; k < colsA; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return C;
}

vector<vector<float>> project2D (vector<vector<float>> matrix) {
    vector<float> projectedPoint;
    vector<vector<float>> projectionMatrix = {
        {1,0,0},
        {0,1,0},
        {0,0,0}
    };
    return multiplyMatrices(projectionMatrix, matrix);

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
    float angle = 0;
    vector<vector<float>> cubePoints = {
        {{1, 1, 1}},
        {{1, 1, -1}},
        {{1, -1, 1}},
        {{1, -1, -1}},
        {{-1, 1, -1}},
        {{-1, 1, 1}},
        {{-1, -1, 1}},
        {{-1, -1, -1}}
    };
    
    while (true) {
        vector<vector<float>> rotationZMatrix = {
            {cos(angle),-1*sin(angle), 0},
            {sin(angle), cos(angle), 0},
            {0,0,1}
        };

        

        for (auto & point : cubePoints){
            vector<vector<float>> projected = project2D(point);
            for (auto & element : projected){
                int x = (int) cubePOSX+element.at(0)*cubeScale;
                int y = (int) cubePOSY+element.at(1)*cubeScale;
                canvas->SetPixel(x, y, 255, 0, 0);
            }
        }
        angle += 0.01;

        signal(SIGTERM, InterruptHandler);
        signal(SIGINT, InterruptHandler);
    }

    canvas->Clear();
    exit (0);
}
