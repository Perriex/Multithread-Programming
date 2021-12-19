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
#define NUM_THREADS_SMOOTH 5
#define NUM_THREADS_SEPIA 5
#define NUM_THREADS_MEAN 5

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
      pic[i][j][0] = (unsigned char)args->fileReadBuffer[args->end - count];
      count++;
      pic[i][j][1] = (unsigned char)args->fileReadBuffer[args->end - count];
      count++;
      pic[i][j][2] = (unsigned char)args->fileReadBuffer[args->end - count];
      count++;
    }
  }
  pthread_exit(NULL);
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
  pthread_t threads[NUM_THREADS_READ];
  struct getPixsArgs td[NUM_THREADS_READ + 1];
  int rc;
  int parts = rows / NUM_THREADS_READ;
  int start = 0;
  int extra = cols % 4;
  int i;
  for (i = 0; i < NUM_THREADS_READ; i++)
  {
    td[i].end = end;
    td[i].extra = extra;
    td[i].row = start;
    td[i].maxR = start + parts;
    start += parts;
    td[i].count = cols * i * parts * 3 + 1;
    td[i].fileReadBuffer = fileReadBuffer;
    rc = pthread_create(&threads[i], NULL, getPixs, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
  }

  if (rows % NUM_THREADS_READ != 0)
  {
    pthread_t thread;

    td[i].end = end;
    td[i].extra = extra;
    td[i].row = start;
    td[i].maxR = start + (rows % NUM_THREADS_READ);
    start += parts;
    td[i].count = cols * i * parts * 3 + 1;
    td[i].fileReadBuffer = fileReadBuffer;
    rc = pthread_create(&thread, NULL, getPixs, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
    pthread_join(thread, NULL);
  }
  for (int i = 0; i < NUM_THREADS_READ; ++i)
  {
    pthread_join(threads[i], NULL);
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
      sum += pic[i - p][j - q][k];

  return sum;
}

void *smooth(void *threadarg)
{
  struct getPixsArgs *args;
  args = (struct getPixsArgs *)threadarg;
  for (int i = args->row; i < args->maxR; i++)
    for (int j = 0; j < cols; j++)
      for (int k = 0; k < 3; k++)
        out[i][j][k] = mean_neighbers(i, j, k) / 9;

  pthread_exit(NULL);
}

void smoothing()
{
  pthread_t threads[NUM_THREADS_SMOOTH];
  struct getPixsArgs td[NUM_THREADS_SMOOTH + 1];
  int rc;
  int parts = rows / (NUM_THREADS_SMOOTH);
  int start = 0;
  int i;
  for (i = 0; i < NUM_THREADS_SMOOTH; i++)
  {
    td[i].row = start;
    td[i].maxR = start + parts;
    start += parts;
    rc = pthread_create(&threads[i], NULL, smooth, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
  }

  if (rows % (NUM_THREADS_SMOOTH) != 0)
  {
    pthread_t thread;

    td[i].row = start;
    td[i].maxR = start + (rows % (NUM_THREADS_SMOOTH));
    start += parts;
    td[i].count = cols * i * parts * 3 + 1;
    rc = pthread_create(&thread, NULL, getPixs, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
    pthread_join(thread, NULL);
  }
  for (int i = 0; i < NUM_THREADS_SMOOTH; ++i)
  {
    pthread_join(threads[i], NULL);
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

float mean_red = 0;
float mean_blue = 0;
float mean_green = 0;

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

  pthread_t threads[NUM_THREADS_MEAN];
  struct getPixsArgs td[NUM_THREADS_MEAN + 1];
  int rc;
  int parts = rows / (NUM_THREADS_MEAN);
  int start = 0;
  int i;
  for (i = 0; i < NUM_THREADS_MEAN; i++)
  {
    td[i].row = start;
    td[i].maxR = start + parts;
    start += parts;
    rc = pthread_create(&threads[i], NULL, mean, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
  }

  if (rows % (NUM_THREADS_MEAN) != 0)
  {
    pthread_t thread;

    td[i].row = start;
    td[i].maxR = start + (rows % (NUM_THREADS_MEAN));
    start += parts;
    td[i].count = cols * i * parts * 3 + 1;
    rc = pthread_create(&thread, NULL, getPixs, (void *)&td[i]);
    if (rc)
    {
      cout << "Error:unable to create thread," << rc << endl;
      exit(-1);
    }
    pthread_join(thread, NULL);
  }
  for (int i = 0; i < NUM_THREADS_MEAN; ++i)
  {
    pthread_join(threads[i], NULL);
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