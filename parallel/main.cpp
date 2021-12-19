#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <pthread.h>

using namespace std::chrono;
using namespace std;

#define NUM_THREADS_READ 5
#define NUM_THREADS_SMOOTH 8
#define NUM_THREADS_SEPIA 5
#define NUM_THREADS_MEAN 6

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

struct getPixsArgs
{
  int count, row, maxR, extra, end, id;
  char *fileReadBuffer;
};

void *getPixs(void *threadarg)
{
  struct getPixsArgs *args;
  args = (struct getPixsArgs *)threadarg;
  int count = args->count;

  for (int i = args->row; i < args->maxR; i++)
  {
    count += args->extra;
    for (int j = cols - 1; j >= 0; j--)
    {
      for (int k = 0; k < 3; k++)
      {
        pic[i][j][k] = (unsigned char)args->fileReadBuffer[args->end - count];
        count++;
      }
    }
  }
  pthread_exit(NULL);
}

void threading(int num, int end, void *(*func)(void *), char *buf = new char[0])
{
  pthread_t threads[num];
  struct getPixsArgs td[num + 1];
  int rc;
  int parts = rows / num;
  int start = 0;
  int extra = cols % 4;
  int i;
  for (i = 0; i < num; i++)
  {
    td[i].end = end;
    td[i].extra = extra;
    td[i].row = start;
    td[i].maxR = start + parts;
    start += parts;
    td[i].count = cols * i * parts * 3 + 1;
    td[i].fileReadBuffer = buf;
    rc = pthread_create(&threads[i], NULL, func, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
  }

  if (rows % num != 0)
  {
    pthread_t thread;

    td[i].end = end;
    td[i].extra = extra;
    td[i].row = start;
    td[i].maxR = start + (rows % num);
    start += parts;
    td[i].count = cols * i * parts * 3 + 1;
    td[i].fileReadBuffer = buf;
    rc = pthread_create(&thread, NULL, func, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
    pthread_join(thread, NULL);
  }
  for (int i = 0; i < num; ++i)
  {
    pthread_join(threads[i], NULL);
  }
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
  threading(NUM_THREADS_READ, end, getPixs, fileReadBuffer);
}
char *c;

void *setPixs(void *threadarg)
{
  struct getPixsArgs *args;
  args = (struct getPixsArgs *)threadarg;
  int count = args->count;

  for (int i = args->row; i < args->maxR; i++)
  {
    count += args->extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        c[args->end - count] = (unsigned char)out[i][j][k];
        count++;
      }
  }

  pthread_exit(NULL);
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
  c = fileBuffer;
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  threading(NUM_THREADS_READ, bufferSize, setPixs, c);
  write.write(c, bufferSize);
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

void *smooth(void *threadarg)
{
  struct getPixsArgs *args;
  args = (struct getPixsArgs *)threadarg;
  for (int i = args->row; i < args->maxR; i++)
    for (int j = 0; j < cols; j++)
      for (int k = 0; k < 3; k++)
        out[i][j][k] = mean_neighbers(i, j, k);

  pthread_exit(NULL);
}

void smoothing()
{
  threading(NUM_THREADS_SMOOTH, 0, smooth);
}

float mean_red = 0;
float mean_blue = 0;
float mean_green = 0;
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
      mean_red += pic[i][j][0];
      mean_green += pic[i][j][1];
      mean_blue += pic[i][j][2];
    }
}

void *mean(void *threadarg)
{
  struct getPixsArgs *args;
  args = (struct getPixsArgs *)threadarg;

  for (int i = args->row; i < args->maxR; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      out[i][j][0] = pic[i][j][0] * 0.4 + mean_red * 0.6;
      out[i][j][1] = pic[i][j][1] * 0.4 + mean_green * 0.6;
      out[i][j][2] = pic[i][j][2] * 0.4 + mean_blue * 0.6;
    }
  }
  pthread_exit(NULL);
}

void overall_mean()
{
  mean_red /= rows * cols;
  mean_green /= rows * cols;
  mean_blue /= rows * cols;
  threading(NUM_THREADS_MEAN, 0, mean);
}

void cross()
{
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
    {
      for (int k = 0; k <= 2; k++)
      {
        if (j == i || j == (cols - i))
        {
          out[i][j][k] = 255.0;
          if (i != 0)
          {
            out[i - 1][j][k] = 255.0;
          }
          if (i != rows - 1)
          {
            out[i + 1][j][k] = 255.0;
          }
        }
      }
    }
}

int main(int argc, char *argv[])
{
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }
  auto first = high_resolution_clock::now();
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);

  smoothing();

  sepia();

  overall_mean();

  cross();

  writeOutBmp24(fileBuffer, "output.bmp", bufferSize);

  auto last = high_resolution_clock::now();

  auto milliseconds = chrono::duration_cast<chrono::milliseconds>(last - first);
  cout << "Speedup: " << milliseconds.count() << endl;

  return 0;
}