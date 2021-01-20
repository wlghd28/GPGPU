#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <process.h>

int threadsPerBlock = 1024;

double total_Time_CPU = 0;
double total_Time_GPU = 0;
LARGE_INTEGER beginClock, endClock, clockFreq;
LARGE_INTEGER tot_beginClock, tot_endClock, tot_clockFreq;

typedef struct RGB {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
}RGB;
RGB * b_pix;	// 5 X 5개 복붙한 이미지 픽셀
RGB * pix;
BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;

// 이미지 정보를 다루기 위해 사용하는 변수
int bpl, b_bpl;
int width, height, b_width, b_height;
int pix_size;
int b_pix_size;	// 5 X 5개 만큼 복붙한 이미지 사이즈
//unsigned char* pix; // 원본 이미지
//unsigned char* pix_out; // GPU 연산결과 이미지

void GraphicInfo();				// 현재 장착된 그래픽카드의 정보를 불러온다
char str[100];
void Fwrite(char * fn);		// 연산된 픽셀값을 bmp파일로 저장한다

cudaError_t addWithCuda(RGB* a, int size);

__global__ void addKernel(RGB* a, RGB* d_pix, const int width, const int b_width, const int height)
{
    int i = threadIdx.x + blockIdx.x * blockDim.x;
	int px = (i % b_width) % width;
	int py = (i % (b_width * height)) / b_width;
	int i2 = px + width * py;
	a[i].red = d_pix[i2].red;
	a[i].green = d_pix[i2].green;
	a[i].blue = d_pix[i2].blue;
}

int main()
{
	GraphicInfo();
	FILE * fp;
	fp = fopen("323test1.bmp", "rb");

	if (fp == NULL)
	{
		printf("File Not Found!!\n");
		return 0;
	}
	// 파일헤더, 정보헤더 읽어들인다
	fread(&bfh, sizeof(bfh), 1, fp);
	fread(&bih, sizeof(bih), 1, fp);

	width = bih.biWidth;
	height = bih.biHeight;
	b_width = width * 5;
	b_height = height * 5;

	// BPL을 맞춰주기 위해서 픽셀데이터의 사이즈를 4의 배수로 조정
	bpl = (width + 3) / 4 * 4;
	b_bpl = (b_width + 3) / 4 * 4;

	// 이미지 사이즈 정보
	pix_size = bih.biSizeImage / 3;
	b_pix_size = bih.biSizeImage * 25 / 3;
	printf("Image size : %d X %d\n", width, height);
	printf("Memory size : %d byte\n", pix_size * 3);
	printf("5 X 5 Image size : %d X %d\n", b_width, b_height);
	printf("Memory size : %d byte\n", b_pix_size * 3);

	// 원본 이미지 데이터 읽어 들인다
	pix = (RGB *)calloc(bpl * height, sizeof(RGB));
	fread(pix, sizeof(RGB), pix_size, fp);

	b_pix = (RGB *)calloc(b_pix_size, sizeof(RGB));

	QueryPerformanceFrequency(&tot_clockFreq);	// 시간을 측정하기위한 준비

	/*
	QueryPerformanceCounter(&tot_beginClock); // CPU 시간측정 시작
	for (int i = 0; i < pix_size; i++)
	{
		pix[i] += 5;
		//printf("%d\n", pix[i]);
	}
	QueryPerformanceCounter(&tot_endClock); // CPU 시간측정 종료
	total_Time_CPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;

	sprintf(str, "323test1.bmp_CPU.bmp");
	Fwrite(str);
	*/
	
	QueryPerformanceCounter(&tot_beginClock); // GPU 시간측정 시작
	// Add vectors in parallel.
	cudaError_t cudaStatus = addWithCuda(b_pix, threadsPerBlock);
	QueryPerformanceCounter(&tot_endClock); // GPU 시간측정 종료
	total_Time_GPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;

	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "addWithCuda failed!");
		return 1;
	}
	
	// cudaDeviceReset must be called before exiting in order for profiling and
	// tracing tools such as Nsight and Visual Profiler to show complete traces.
	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceReset failed!");
		return 1;
	}

	printf("CPU 실행시간 : %f\nGPU 실행시간 : %f\n",
		total_Time_CPU * 1000, total_Time_GPU * 1000);

	sprintf(str, "323test1_GPU.bmp");
	Fwrite(str);
	free(pix);
	free(b_pix);
	fclose(fp);

	return 0;
}

// Helper function for using CUDA to add vectors in parallel.
cudaError_t addWithCuda(RGB* a, int thread)
{
    RGB * dev_a = 0;
	RGB * d_pix = 0;
    cudaError_t cudaStatus;

    // Choose which GPU to run on, change this on a multi-GPU system.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        goto Error;
    }

    // Allocate GPU buffers for three vectors (one input, one output)    .
    cudaStatus = cudaMalloc((void**)&dev_a, b_pix_size * sizeof(RGB));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

	cudaStatus = cudaMalloc((void**)&d_pix, pix_size * sizeof(RGB));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}


    // Copy input vectors from host memory to GPU buffers.
    cudaStatus = cudaMemcpy(dev_a, a, b_pix_size * sizeof(RGB), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!");
        goto Error;
    }

	cudaStatus = cudaMemcpy(d_pix, pix, pix_size * sizeof(RGB), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

    // Launch a kernel on the GPU with one thread for each element.
	// 함수명<<<블록 수, 스레드 수>>>(매개변수);
    addKernel<<< b_pix_size/thread + 1, thread >>>(dev_a, d_pix, width, b_width, height);

    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
        goto Error;
    }
    
    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
        goto Error;
    }

    // Copy output vector from GPU buffer to host memory.
    cudaStatus = cudaMemcpy(a, dev_a, b_pix_size * sizeof(RGB), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!");
        goto Error;
    }



Error:
    cudaFree(dev_a);
	cudaFree(d_pix);

    return cudaStatus;
}
void GraphicInfo()
{
	cudaDeviceProp  prop;

	int count;
	cudaGetDeviceCount(&count);

	for (int i = 0; i < count; i++) {
		cudaGetDeviceProperties(&prop, i);
		printf("   --- General Information for device %d ---\n", i);
		printf("Name:  %s\n", prop.name);
		printf("Compute capability:  %d.%d\n", prop.major, prop.minor);
		printf("Clock rate:  %d\n", prop.clockRate);
		printf("Device copy overlap:  ");
		if (prop.deviceOverlap)
			printf("Enabled\n");
		else
			printf("Disabled\n");
		printf("Kernel execution timeout :  ");
		if (prop.kernelExecTimeoutEnabled)
			printf("Enabled\n");
		else
			printf("Disabled\n");
		printf("\n");

		printf("   --- Memory Information for device %d ---\n", i);
		printf("Total global mem:  %ld\n", prop.totalGlobalMem);
		printf("Total constant Mem:  %ld\n", prop.totalConstMem);
		printf("Max mem pitch:  %ld\n", prop.memPitch);
		printf("Texture Alignment:  %ld\n", prop.textureAlignment);
		printf("\n");

		printf("   --- MP Information for device %d ---\n", i);
		printf("Multiprocessor count:  %d\n", prop.multiProcessorCount);
		printf("Shared mem per mp:  %ld\n", prop.sharedMemPerBlock);
		printf("Registers per mp:  %d\n", prop.regsPerBlock);
		printf("Threads in warp:  %d\n", prop.warpSize);
		printf("Max threads per block:  %d\n", prop.maxThreadsPerBlock);
		printf("Max thread dimensions:  (%d, %d, %d)\n", prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
		printf("Max grid dimensions:  (%d, %d, %d)\n", prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
		printf("--------------------------------------\n");
		printf("\n");
	}

}

// 데이터 픽셀값을 bmp파일로 쓴다.
void Fwrite(char * fn)
{
	FILE * fp2 = fopen(fn, "wb");
	bih.biWidth *= 5;
	bih.biHeight *= 5;
	bih.biSizeImage *= 25;

	fwrite(&bfh, sizeof(bfh), 1, fp2);
	fwrite(&bih, sizeof(bih), 1, fp2);
	fwrite(b_pix, sizeof(RGB), b_pix_size, fp2);
	fclose(fp2);
}