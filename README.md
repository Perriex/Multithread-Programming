# Multithread Program

First, your program reads the input image and stores the values of its three color channels in its memory. After extracting this information, the program will apply the desired filters step by step.

- picture preprocess 

The numeric value of each pixel of the image in RGB mode (the values of the three color channels red, green, and blue) must be stored in the custom data structure.

- Smoothing filter

To apply this filter, you must set the value of each color channel per pixel equal to the average value of your same channel and that pixel neighbor.

- Sepia filter

  - outputRed = (inputRed * 0.393) + (inputGreen * 0.769) + (inputBlue * 0.189)
  - outputGreen = (inputRed * 0.349) + (inputGreen * 0.686) + (inputBlue * 0.168)
  - outputBlue = (inputRed * 0.272) + (inputGreen * 0.534) + (inputBlue * 0.131)
 

- Overall average

  - 𝑜𝑢𝑡𝑝𝑢𝑡 = 𝑖𝑛𝑝𝑢𝑡 𝑐ℎ𝑎𝑛𝑛𝑒𝑙 * 0. 4 + 𝑚𝑒𝑎𝑛 𝑐ℎ𝑎𝑛𝑛𝑒𝑙 * 0. 6
  
 - Cross over the picture
 
 Implemented in Serial and Parallel code.
 
 run with makefile and g++
 ```
 ./ImageFilters.out ut.bmp

 ```
