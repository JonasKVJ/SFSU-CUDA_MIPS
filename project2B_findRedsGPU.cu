/****                                                                           
     File: findRedsDriver.cu
     Date: 5/3/2018
     By: Bill Hsu
****/
/*
 * How to compile and execute:
 * source ~whsu/lees.bash_profile
 * nvcc findRedsGPU.cu -o frgpu -lm -Wno-deprecated-gpu-targets
 * ./frgpu
 * Submission by: Jonas Vinter-Jensen, 912941515
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cuda.h>

#define NUMPARTICLES 1024
#define NEIGHBORHOOD .05
#define THREADSPERBLOCK 4

void initPos(float*);

float findDistance(float*, int, int);

__device__ float findDistanceGPU(float*, int, int);

void dumpResults(int index[]);

__global__ void findRedsGPU(float* p, int* numI);

int main(int argc, const char* argv[])
{
    cudaEvent_t start, stop;
    float time;

    float* pos;
    float* dpos;
    int* numReds;
    int* dnumReds;

    pos = (float*) malloc(NUMPARTICLES * 4 * sizeof(float));
    numReds = (int*) malloc(NUMPARTICLES * sizeof(int));

    initPos(pos);

    // your code to allocate device arrays for pos and numReds go here
    cudaMalloc((void**) &dpos, NUMPARTICLES * 4 * sizeof(float));
    cudaMalloc((void**) &dnumReds, NUMPARTICLES * sizeof(int));
    cudaMemcpy(dpos, pos, NUMPARTICLES * 4 * sizeof(float), cudaMemcpyHostToDevice); //dpos = copy(pos)

    // create timer events
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start, 0); //(event, stream)

    /* invoke kernel findRedsGPU here */
    findRedsGPU<<<NUMPARTICLES/THREADSPERBLOCK, THREADSPERBLOCK>>>(dpos, dnumReds);

    cudaThreadSynchronize();

    // your code to copy results to numReds[] go here
    cudaMemcpy(numReds, dnumReds, NUMPARTICLES * sizeof(int), cudaMemcpyDeviceToHost); //numReds = copy(dnumReds)

    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop); //waits for record event to complete
    cudaEventElapsedTime(&time, start, stop);

    printf("Elapsed time = %f\n", time);

    dumpResults(numReds);

    free(pos);
    cudaFree(dpos);
    free(numReds);
    cudaFree(dnumReds);

    return 0;
}

void initPos(float* p)
{
    // your code for initializing pos goes here
    int i;
    for (i = 0; i < NUMPARTICLES; i++)
    {
        p[i * 4] = rand() / (float) RAND_MAX; //p.x
        p[i * 4 + 1] = rand() / (float) RAND_MAX; //p.y
        p[i * 4 + 2] = rand() / (float) RAND_MAX; //p.z

        int colorChoice;
        colorChoice = random() % 3;
        if (colorChoice == 0)
        {
            p[i * 4 + 3] = 0xff0000; //p.color = red
        }
        else if (colorChoice == 1)
        {
            p[i * 4 + 3] = 0x00ff00; //p.color = green
        }
        else
        {
            p[i * 4 + 3] = 0x0000ff; //p.color = blue
        }
    }
}

__device__ float findDistanceGPU(float* p, int i, int j)
{
    // your code for calculating distance for particle i and j
    float dx, dy, dz;

    dx = p[i * 4] - p[j * 4]; //x2-x1
    dy = p[i * 4 + 1] - p[j * 4 + 1]; //y2-y1
    dz = p[i * 4 + 2] - p[j * 4 + 2]; //z2-z1

    return (sqrt(dx * dx + dy * dy + dz * dz));
}

__global__ void findRedsGPU(float* p, int* numI)
{
    // your code for counting red particles goes here
    int k;
    float distance;

    int p2_num = blockDim.x*blockIdx.x + threadIdx.x;
    for (k = 0; k < NUMPARTICLES; k++)
    {
        /*Every time a new (blockId, threadId) permutation occurs, initialize number of red particles near particle
         * k to 0 for every first k-loop iteration of the pairs.*/
        if(k==0)
        {
            numI[p2_num] = 0;
        }
        if (k != p2_num)
        {
            /* calculate distance between particles k, p2_num */
            distance = findDistanceGPU(p, k, p2_num);

            /* if distance < r and color is red, increment count */
            if (distance < NEIGHBORHOOD && p[p2_num * 4 + 3] == 0xff0000)
            {
                numI[k]++;
            }
        }
    }

}

void dumpResults(int index[])
{
    int i;
    FILE* fp;

    fp = fopen("./dump.out", "w");

    for (i = 0; i < NUMPARTICLES; i++)
    {
        fprintf(fp, "%d %d\n", i, index[i]);
    }
    fclose(fp);
}
