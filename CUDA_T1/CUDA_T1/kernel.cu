#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <process.h>

// 마스크값 배열
RGBTRIPLE Mask[25];

int threadsPerBlock = 1024;

double total_Time_CPU = 0;
double total_Time_GPU = 0;
LARGE_INTEGER beginClock, endClock, clockFreq;
LARGE_INTEGER tot_beginClock, tot_endClock, tot_clockFreq;

//BYTE * pix;
//BYTE * b_pix;
RGBTRIPLE * pix;
RGBTRIPLE * b_pix;
BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;
BITMAPFILEHEADER b_bfh;
BITMAPINFOHEADER b_bih;

// 이미지 정보를 다루기 위해 사용하는 변수
int channel = 5;
int bpl, b_bpl;
int bpl_size, b_bpl_size;
int width, height, b_width, b_height;
int pix_size;
int b_pix_size;	// 5 X 5개 만큼 복붙한 이미지 사이즈
int pad, b_pad;		// 패딩 메모리
//unsigned char* pix; // 원본 이미지
//unsigned char* pix_out; // GPU 연산결과 이미지
BYTE * trash;

void GraphicInfo();				// 현재 장착된 그래픽카드의 정보를 불러온다
char str[100];
char str_Extend[100];
void Fwrite_Extend(char * fn);		// 연산된 픽셀값을 bmp파일로 저장한다
void Fwrite(char * fn);
void Draw();					// pix 데이터를 화면으로 출력
void b_Draw();					// b_pix 데이터를 화면으로 출력
cudaError_t extendWithCuda(RGBTRIPLE* b_pix, int size);

__global__ void extendKernel(RGBTRIPLE* d_b_pix, RGBTRIPLE* d_pix, RGBTRIPLE* mask, const int width, const int b_width, const int height)
{
    int i = threadIdx.x + blockIdx.x * blockDim.x;
	int px = i % width;
	int py = (i % (b_width * height)) / b_width;
	int ip = px + py * width;
	int mx = (i % b_width) / width;
	int my = i / (b_width * height);
	int im = mx + my * 5;

	d_b_pix[i].rgbtBlue = d_pix[ip].rgbtBlue;
	d_b_pix[i].rgbtGreen = d_pix[ip].rgbtGreen;
	d_b_pix[i].rgbtRed = d_pix[ip].rgbtRed;

	d_b_pix[i].rgbtBlue += mask[im].rgbtBlue;
	d_b_pix[i].rgbtGreen += mask[im].rgbtGreen;
	d_b_pix[i].rgbtRed += mask[im].rgbtRed;

	if (d_b_pix[i].rgbtBlue > 255)
		d_b_pix[i].rgbtBlue = 255;
	if (d_b_pix[i].rgbtBlue < 0)
		d_b_pix[i].rgbtBlue = 0;
	if (d_b_pix[i].rgbtGreen > 255)
		d_b_pix[i].rgbtGreen = 255;
	if (d_b_pix[i].rgbtGreen < 0)
		d_b_pix[i].rgbtGreen = 0;
	if (d_b_pix[i].rgbtRed > 255)
		d_b_pix[i].rgbtRed = 255;
	if (d_b_pix[i].rgbtRed < 0)
		d_b_pix[i].rgbtRed = 0;

	//d_b_pix[i] = d_pix[i2];
}

int main()
{
	for (int i = 0; i < 25; i++)
	{
		Mask[i].rgbtRed = i * 37;
		Mask[i].rgbtGreen = 50;
		Mask[i].rgbtBlue = i * 23;
	}

	GraphicInfo();
	FILE * fp;
	
	//fp = fopen("test3.bmp", "rb");
	//fp = fopen("lenna_406.bmp", "rb");
	//fp = fopen("323test5.bmp", "rb");
	fp = fopen("input.bmp", "rb");

	if (fp == NULL)
	{
		printf("File Not Found!!\n");
		system("pause");
		return 0;
	}
	// 파일헤더, 정보헤더 읽어들인다
	fread(&bfh, sizeof(bfh), 1, fp);
	fread(&bih, sizeof(bih), 1, fp);

	width = bih.biWidth;
	height = bih.biHeight;
	b_width = width * channel;
	b_height = height * channel;

	
	// BPL을 맞춰주기 위해서 픽셀데이터의 메모리를 4의 배수로 조정
	bpl = (width * 3 + 3) / 4 * 4;
	b_bpl = (b_width * 3 + 3) / 4 * 4;

	// 패딩 값 계산
	pad = bpl - width * 3;
	b_pad = b_bpl - b_width * 3;

	// BPL을 맞춘 메모리 사이즈
	bpl_size = bpl * height;
	b_bpl_size = b_bpl * b_height;

	// 순수 이미지 사이즈
	pix_size = width * height;
	b_pix_size = b_width * b_height;

	printf("Image size : %d X %d\n", width, height);
	printf("Memory size : %d byte\n", bpl_size);
	printf("%d X %d Image size : %d X %d\n", channel, channel, b_width, b_height);
	printf("%d X %d Memory size : %d byte\n", channel, channel, b_bpl_size);

	// 쓰레기 값
	trash = (BYTE *)calloc(b_pad, sizeof(BYTE));
	// 원본 이미지 데이터
	pix = (RGBTRIPLE *)calloc(pix_size, sizeof(RGBTRIPLE));
	for (int i = 0; i < height; i++)
	{
		fread(pix + (i * width), sizeof(RGBTRIPLE), width, fp);
		fread(&trash, sizeof(BYTE), pad, fp);
	}

	// 5 X 5 이미지 데이터
	b_pix = (RGBTRIPLE *)calloc(b_pix_size, sizeof(RGBTRIPLE));

	/*
	for(int i = 0; i < 1000; i++)
	{
		Draw();
	}
	*/

	QueryPerformanceFrequency(&tot_clockFreq);	// 시간을 측정하기위한 준비

	QueryPerformanceCounter(&tot_beginClock); // CPU 시간측정 시작
	for (int i = 0; i < b_pix_size; i++)
	{
		int px = i % width;
		int py = (i % (b_width * height)) / b_width;
		int ip = px + py * width;
		int mx = (i % b_width) / width;
		int my = i / (b_width * height);
		int im = mx + my * 5;

		b_pix[i].rgbtBlue = pix[ip].rgbtBlue;
		b_pix[i].rgbtGreen = pix[ip].rgbtGreen;
		b_pix[i].rgbtRed = pix[ip].rgbtRed;

		b_pix[i].rgbtBlue += Mask[im].rgbtBlue;
		b_pix[i].rgbtGreen += Mask[im].rgbtGreen;
		b_pix[i].rgbtRed += Mask[im].rgbtRed;

		if (b_pix[i].rgbtBlue > 255)
			b_pix[i].rgbtBlue = 255;
		if (b_pix[i].rgbtBlue < 0)
			b_pix[i].rgbtBlue = 0;
		if (b_pix[i].rgbtGreen > 255)
			b_pix[i].rgbtGreen = 255;
		if (b_pix[i].rgbtGreen < 0)
			b_pix[i].rgbtGreen = 0;
		if (b_pix[i].rgbtRed > 255)
			b_pix[i].rgbtRed = 255;
		if (b_pix[i].rgbtRed < 0)
			b_pix[i].rgbtRed = 0;
	}
	QueryPerformanceCounter(&tot_endClock); // CPU 시간측정 종료
	total_Time_CPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;

	//sprintf(str_Extend, "323test5_Extend_CPU.bmp");
	sprintf(str_Extend, "output_CPU.bmp");

	//Fwrite(str);
	Fwrite_Extend(str_Extend);

	memset(b_pix, 0, sizeof(unsigned char) * b_pix_size);


	QueryPerformanceCounter(&tot_beginClock); // GPU 시간측정 시작
	// Add vectors in parallel.
	cudaError_t cudaStatus = extendWithCuda(b_pix, threadsPerBlock);
	QueryPerformanceCounter(&tot_endClock); // GPU 시간측정 종료
	total_Time_GPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;

	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "extendWithCuda failed!");
		system("pause");
		return 1;
	}
	
	// cudaDeviceReset must be called before exiting in order for profiling and
	// tracing tools such as Nsight and Visual Profiler to show complete traces.
	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceReset failed!");
		system("pause");
		return 1;
	}
	printf("CPU 실행시간 : %f\nGPU 실행시간 : %f\n",
		total_Time_CPU * 1000, total_Time_GPU * 1000);

	printf("CPU / GPU = %lf\n",
		total_Time_CPU / total_Time_GPU);

	//sprintf(str, "test_GPU.bmp");
	//sprintf(str_Extend, "test3_Extend.bmp");
	//sprintf(str_Extend, "lenna_406_Extend.bmp");
	//sprintf(str_Extend, "323test5_Extend_GPU.bmp");
	sprintf(str_Extend, "output_GPU.bmp");

	//Fwrite(str);
	Fwrite_Extend(str_Extend);

	/*
	for (int i = 0; i < 1000; i++)
	{
		b_Draw();
	}
	*/


	free(pix);
	free(b_pix);
	free(trash);
	fclose(fp);

	system("pause");
	return 0;
}

// Helper function for using CUDA to add vectors in parallel.
cudaError_t extendWithCuda(RGBTRIPLE* b_pix, int thread)
{
	RGBTRIPLE * d_b_pix = 0;
	RGBTRIPLE * d_pix = 0;
	RGBTRIPLE * mask = 0;
    cudaError_t cudaStatus;

    // Choose which GPU to run on, change this on a multi-GPU system.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        goto Error;
    }

    // Allocate GPU buffers for three vectors (two input, one output)    .
    cudaStatus = cudaMalloc((void**)&d_b_pix, b_pix_size * sizeof(RGBTRIPLE));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

	cudaStatus = cudaMalloc((void**)&d_pix, pix_size * sizeof(RGBTRIPLE));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	cudaStatus = cudaMalloc((void**)&mask, 25 * sizeof(RGBTRIPLE));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

    // Copy input vectors from host memory to GPU buffers.
	cudaStatus = cudaMemcpy(d_pix, pix, pix_size * sizeof(RGBTRIPLE), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	cudaStatus = cudaMemcpy(mask, Mask, 25 * sizeof(RGBTRIPLE), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

    // Launch a kernel on the GPU with one thread for each element.
	// 함수명<<<블록 수, 스레드 수>>>(매개변수);
    extendKernel<<< (b_pix_size + thread - 1) / thread, thread >>>(d_b_pix, d_pix, mask, width, b_width, height);

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
    cudaStatus = cudaMemcpy(b_pix, d_b_pix, b_pix_size * sizeof(RGBTRIPLE), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!");
        goto Error;
    }



Error:
    cudaFree(d_b_pix);
	cudaFree(d_pix);
	cudaFree(mask);

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

void Fwrite(char * fn)
{
	FILE * fp2 = fopen(fn, "wb");
	fwrite(&bfh, sizeof(bfh), 1, fp2);
	fwrite(&bih, sizeof(bih), 1, fp2);

	for (int i = 0; i < height; i++)
	{
		fwrite(pix + (i * width * 3), sizeof(BYTE), width * 3, fp2);
		fwrite(&trash, sizeof(BYTE), pad, fp2);
	}

	fclose(fp2);
}

// 데이터 픽셀값을 bmp파일로 쓴다.
void Fwrite_Extend(char * fn)
{
	FILE * fp2 = fopen(fn, "wb");
	b_bfh = bfh;
	b_bih = bih;
	b_bih.biWidth = b_width;
	b_bih.biHeight = b_height;
	b_bih.biSizeImage = b_bpl_size;
	b_bfh.bfSize = b_bih.biSizeImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	fwrite(&b_bfh, sizeof(bfh), 1, fp2);
	fwrite(&b_bih, sizeof(bih), 1, fp2);
	
	for (int i = 0; i < b_height; i++)
	{
		fwrite(b_pix + (i * b_width), sizeof(RGBTRIPLE), b_width, fp2);
		fwrite(&trash, sizeof(BYTE), b_pad, fp2);
	}
	
	fclose(fp2);
}
// 연산된 RGB 값을 화면에 출력시킨다.
void Draw()
{
	HDC hdc;
	hdc = GetDC(NULL);

	SetDIBitsToDevice(hdc, 0, 0, bpl, height, 0, 0, 0, height,
		(BYTE *)pix, (const BITMAPINFO *)&bih, DIB_RGB_COLORS);

	ReleaseDC(NULL, hdc);
}
void b_Draw()
{
	HDC hdc;
	hdc = GetDC(NULL);

	SetDIBitsToDevice(hdc, 0, 0, b_width, height * 2, 0, 0, 0, height * 2,
		(BYTE *)b_pix, (const BITMAPINFO *)&b_bih, DIB_RGB_COLORS);

	ReleaseDC(NULL, hdc);
}

