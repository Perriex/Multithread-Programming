#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>

using namespace std::chrono;
using namespace std;

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
#pragma once

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

float pic[2500][2500][3] = {0};
float out[2500][2500][3] = {0};

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "File" << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // fileReadBuffer[end - count] is the red value
          pic[i][j][k] = (unsigned char)fileReadBuffer[end - count];
          break;
        case 1:
          // fileReadBuffer[end - count] is the green value
          pic[i][j][k] = (unsigned char)fileReadBuffer[end - count];
          break;
        case 2:
          // fileReadBuffer[end - count] is the blue value
          pic[i][j][k] = (unsigned char)fileReadBuffer[end - count];
          break;
          // go to the next position in the buffer
        }
        count++;
      }
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // write red value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = (unsigned char)out[i][j][k];
          break;
        case 1:
          // write green value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = (unsigned char)out[i][j][k];
          break;
        case 2:
          // write blue value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = (unsigned char)out[i][j][k];
          break;
          // go to the next position in the buffer
        }
        count++;
      }
  }
  write.write(fileBuffer, bufferSize);
}

short mean_neighbers(int i, int j, int k)
{
  if (i + 1 >= rows || j + 1 >= cols || i - 1 < 0 || j - 1 < 0)
    return pic[i][j][k];

  float sum = 0.0;
  for (int p = -1; p <= 1; p++)
    for (int q = -1; q <= 1; q++)
      sum += pic[i - p][j - q][k] / 9;

  return sum;
}

void smoothing()
{
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
    {
      out[i][j][0] = mean_neighbers(i, j, 0);
      out[i][j][1] = mean_neighbers(i, j, 1);
      out[i][j][2] = mean_neighbers(i, j, 2);
    }
}

void sepia()
{
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
    {
      pic[i][j][0] = (0.393 * out[i][j][0]) + (0.769 * out[i][j][1]) + (0.189 * out[i][j][2]);
      if (pic[i][j][0] >= 255)
        pic[i][j][0] = 255;

      pic[i][j][1] = (0.349 * out[i][j][0]) + (0.686 * out[i][j][1]) + (0.168 * out[i][j][2]);
      if (pic[i][j][1] >= 255)
        pic[i][j][1] = 255;

      pic[i][j][2] = (0.272 * out[i][j][0]) + (0.534 * out[i][j][1]) + (0.131 * out[i][j][2]);
      if (pic[i][j][2] >= 255)
        pic[i][j][2] = 255;
    }
}

void overall_mean()
{
  float mean_red = 0, mean_green = 0, mean_blue = 0;
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
    {
      mean_red += pic[i][j][0];
      mean_green += pic[i][j][1];
      mean_blue += pic[i][j][2];
    }
  mean_red /= rows * cols;
  mean_green /= rows * cols;
  mean_blue /= rows * cols;
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
    {
      out[i][j][0] = pic[i][j][0] * 0.4 + mean_red * 0.6;
      out[i][j][1] = pic[i][j][1] * 0.4 + mean_green * 0.6;
      out[i][j][2] = pic[i][j][2] * 0.4 + mean_blue * 0.6;
    }
}

void cross()
{
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
    {
      if (j == i || j == (cols - i))
      {
        out[i][j][0] = 255.0;
        out[i][j][1] = 255.0;
        out[i][j][2] = 255.0;
        if (i != 0)
        {
          out[i - 1][j][0] = 255.0;
          out[i - 1][j][1] = 255.0;
          out[i - 1][j][2] = 255.0;
        }
        if (i != rows - 1)
        {
          out[i + 1][j][0] = 255.0;
          out[i + 1][j][1] = 255.0;
          out[i + 1][j][2] = 255.0;
        }
      }
    }
}

int main(int argc, char *argv[])
{
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
  auto first = high_resolution_clock::now();
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }

  // read input file
  auto start = high_resolution_clock::now();
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << "read picture:" << duration.count() << endl;

  // apply filters
  start = high_resolution_clock::now();
  smoothing();
  stop = high_resolution_clock::now();
  duration = duration_cast<microseconds>(stop - start);
  cout << "smoothing: " << duration.count() << endl;

  start = high_resolution_clock::now();
  sepia();
  stop = high_resolution_clock::now();
  duration = duration_cast<microseconds>(stop - start);
  cout << "sepia: " << duration.count() << endl;

  start = high_resolution_clock::now();
  overall_mean();
  stop = high_resolution_clock::now();
  duration = duration_cast<microseconds>(stop - start);
  cout << "mean: " << duration.count() << endl;

  start = high_resolution_clock::now();
  cross();
  stop = high_resolution_clock::now();
  duration = duration_cast<microseconds>(stop - start);
  cout << "cross: " << duration.count() << endl;
  // write output file
  start = high_resolution_clock::now();
  writeOutBmp24(fileBuffer, "output.bmp", bufferSize);
  stop = high_resolution_clock::now();
  duration = duration_cast<microseconds>(stop - start);
  cout << "write pic:" << duration.count() << endl;
  auto last = high_resolution_clock::now();

  auto milliseconds = chrono::duration_cast<chrono::milliseconds>(last - first);
  cout << "overall: " << milliseconds.count() << endl;

  return 0;
}