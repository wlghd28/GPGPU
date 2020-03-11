#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <Windows.h>
#include <string.h>
#include <tchar.h>
#include <stdbool.h>

#include "deviceInfo.h"
// BMP 이미지를 불러오기 위한 작업
#pragma warning(disable : 4996)
#pragma pack(push, 1)
struct Bitmapfileheader
{
	short int bmptype;
	int bfsize;
	short int bfreserved1, bfreserved2;
	int bfoffbits;
}bfh;
struct Bitmapinfoheader
{
	int bisize, biwidth, biheight;
	short int biplanes, bibitcount;
	int compression, sizeimage, xpelspermeter, ypelspermeter, biclrused, biclrimportant;
}bih;
struct Palette {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
}rgb;
#pragma pack(pop)	

const long int G_SIZE = 10000;

// kernel을 읽어서 char pointer생성
char* readSource(char* kernelPath) {

	cl_int status;
	FILE *fp;
	char *source;
	long int size;

	printf("Program file is: %s\n", kernelPath);

	fp = fopen(kernelPath, "rb");
	if (!fp) {
		printf("Could not open kernel file\n");
		exit(-1);
	}
	status = fseek(fp, 0, SEEK_END);
	if (status != 0) {
		printf("Error seeking to end of file\n");
		exit(-1);
	}
	size = ftell(fp);
	if (size < 0) {
		printf("Error getting file position\n");
		exit(-1);
	}

	rewind(fp);

	source = (char *)malloc(size + 1);

	int i;
	for (i = 0; i < size + 1; i++) {
		source[i] = '\0';
	}

	if (source == NULL) {
		printf("Error allocating space for the kernel source\n");
		exit(-1);
	}

	fread(source, 1, size, fp);
	source[size] = '\0';

	return source;
}

//디바이스 init, 커널 생성
void CLInit()
{
	int i, j;
	char* value;
	size_t valueSize;
	cl_uint platformCount;
	cl_platform_id* platforms;
	cl_uint deviceCount;
	cl_device_id* devices;
	cl_uint maxComputeUnits;

	// get all platforms
	clGetPlatformIDs(0, NULL, &platformCount);
	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platformCount);
	clGetPlatformIDs(platformCount, platforms, NULL);

	for (i = 0; i < platformCount; i++) {

		// get all devices
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
		devices = (cl_device_id*)malloc(sizeof(cl_device_id) * deviceCount);
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

		// for each device print critical attributes
		for (j = 0; j < deviceCount; j++) {

			// print device name
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
			printf("platform %d. Device %d: %s\n", i + 1, j + 1, value);
			free(value);
			
			// print hardware device version
			clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, valueSize, value, NULL);
			printf(" %d.%d Hardware version: %s\n", i + 1, 1, value);
			free(value);

			// print software driver version
			clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, valueSize, value, NULL);
			printf(" %d.%d Software version: %s\n", i + 1, 2, value);
			free(value);

			// print c version supported by compiler for device
			clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
			printf(" %d.%d OpenCL C version: %s\n", i + 1, 3, value);
			free(value);

			// print parallel compute units
			clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS,
				sizeof(maxComputeUnits), &maxComputeUnits, NULL);
			printf(" %d.%d Parallel compute units: %d\n", i + 1, 4, maxComputeUnits);
		}
	}
	int platformNum;
	int deviceNum;
	printf("\n\nSELECT PLATFORM('1' ~ '%d') : ", platformCount);
	scanf("%d", &platformNum);
	printf("\n");
	printf("SELECT DEVICE('1' ~ '%d') : ", deviceCount);
	scanf("%d", &deviceNum);
	printf("\n");
	clGetDeviceIDs(platforms[platformNum - 1], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

	device = devices[deviceNum - 1];

	//create context
	context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);

	//create command queue
	queue = clCreateCommandQueue(context, device, 0, NULL);

	// 텍스트파일로부터 프로그램 읽기
	char* source = readSource("convolution.cl");

	// compile program
	program = clCreateProgramWithSource(context, 1,
		(const char**)&source, NULL, NULL);
	cl_int build_status;
	build_status = clBuildProgram(program, 1, &device, NULL, NULL,
		NULL);

	//커널 포인터 생성
	simpleKernel = clCreateKernel(program, "simpleKernel", NULL);

}

//버퍼생성 및 write
void bufferWrite()
{

	// 메모리 버퍼 생성
	
	d_inputArray_A = clCreateBuffer(context, CL_MEM_READ_WRITE,
		G_SIZE * sizeof(struct Palette), NULL, NULL);
	d_inputArray_B = clCreateBuffer(context, CL_MEM_READ_WRITE,
		G_SIZE * sizeof(struct Palette), NULL, NULL);
	d_outputArray = clCreateBuffer(context, CL_MEM_READ_WRITE,
		G_SIZE * sizeof(struct Palette), NULL, NULL);

	FILE* fp_A;
	FILE* fp_B;
	struct Bitmapfileheader bfh;
	struct Bitmapinfoheader bih;
	struct Palette rgb1, rgb2;

	fp_A = fopen("background.bmp", "rb");
	fp_B = fopen("background2.bmp", "rb");
	if (fp_A == NULL || fp_B == NULL)
	{
		printf("file not found");
		getchar();
		exit(0);
	}
	fread(&bfh, sizeof(bfh), 1, fp_A);
	fread(&bih, sizeof(bih), 1, fp_A);

	struct Palette* inputArray_A = (struct Palette*)malloc(sizeof(struct Palette) * G_SIZE);
	struct Palette* inputArray_B = (struct Palette*)malloc(sizeof(struct Palette) * G_SIZE);
	
	for (int j = 0; j < bih.biheight; j++)
	{
		for (int k = 0; k < bih.biwidth; k++)
		{
			fread(&rgb1, 3, 1, fp_A);
			fread(&rgb2, 3, 1, fp_B);
		}
	}
	for (int i = 0; i < G_SIZE; i++) {
		inputArray_A[i].red = rgb1.red;
		inputArray_A[i].green = rgb1.green;
		inputArray_A[i].blue = rgb1.blue;
		inputArray_B[i].red = rgb2.red;
		inputArray_B[i].green = rgb2.green;
		inputArray_B[i].blue = rgb2.blue;
	}
	/*
	d_inputArray_A = clCreateBuffer(context, CL_MEM_READ_WRITE,
		G_SIZE * sizeof(int), NULL, NULL);
	d_inputArray_B = clCreateBuffer(context, CL_MEM_READ_WRITE,
		G_SIZE * sizeof(int), NULL, NULL);
	d_outputArray = clCreateBuffer(context, CL_MEM_READ_WRITE,
		G_SIZE * sizeof(int), NULL, NULL);
		*/
	clEnqueueWriteBuffer(queue, d_inputArray_A, CL_TRUE, 0, sizeof(struct Palette) * G_SIZE,
		inputArray_A, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_inputArray_B, CL_TRUE, 0, sizeof(struct Palette) * G_SIZE,
		inputArray_B, 0, NULL, NULL);

	fclose(fp_A);
	fclose(fp_B);

	free(inputArray_A);
	free(inputArray_B);
}

void runKernel()
{

	//int i;

	int totalWorkItemsX = G_SIZE;
	int totalWorkItemsY = 1;

	size_t globalSize[2] = { totalWorkItemsX, totalWorkItemsY };
	//float *minVal, *maxVal;

	// 커널 매개변수 설정 
	clSetKernelArg(simpleKernel, 0, sizeof(cl_mem), &d_inputArray_A);
	clSetKernelArg(simpleKernel, 1, sizeof(cl_mem), &d_inputArray_B);
	clSetKernelArg(simpleKernel, 2, sizeof(cl_mem), &d_outputArray);
	clEnqueueNDRangeKernel(queue, simpleKernel, 2, NULL, globalSize,
		NULL, 0, NULL, NULL);
	// 완료 대기 
	clFinish(queue);
	

	struct Palette* outputArray = (struct Palette*)malloc(sizeof(struct Palette) * G_SIZE);
	/*
	for (int i = 0; i < G_SIZE; i++) {
		printf("%d %d %d\n", outputArray[i].red, outputArray[i].green, outputArray[i].blue);
	}
	*/
	clEnqueueReadBuffer(queue, d_outputArray, CL_TRUE, 0,
		G_SIZE * sizeof(struct Palette), outputArray, 0, NULL, NULL);

	free(outputArray);
}

void Release()
{
	// 릴리즈
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}
void CpuCal() {

	int index = 0;
	FILE* fp_A;
	FILE* fp_B;
	struct Bitmapfileheader bfh;
	struct Bitmapinfoheader bih;
	COLORREF* output;
	struct Palette rgb1, rgb2;

	struct Palette* inputArray_A = (struct Palette*)malloc(sizeof(struct Palette) * G_SIZE);
	struct Palette* inputArray_B = (struct Palette*)malloc(sizeof(struct Palette) * G_SIZE);
	struct Palette* outputArray = (struct Palette*)malloc(sizeof(struct Palette) * G_SIZE);

	fp_A = fopen("background.bmp", "rb");
	fp_B = fopen("background2.bmp", "rb");

	if (fp_A == NULL)
	{
		printf("file not found");
		getchar();
		exit(0);
	}
	if (fp_B == NULL)
	{
		printf("file not found");
		getchar();
		exit(0);
	}
	fread(&bfh, sizeof(bfh), 1, fp_A);
	fread(&bih, sizeof(bih), 1, fp_A);

	for (int j = 0; j < bih.biheight; j++)
	{
		for (int k = 0; k < bih.biwidth; k++)
		{
			fread(&rgb1, 3, 1, fp_A);
			fread(&rgb2, 3, 1, fp_B);
			// 배경화면의 RGB 정보를 좌표에 따라 저장.
		}
	}

	for (int i = 0; i < G_SIZE; i++) {
		outputArray[i].red = (inputArray_A[i].red * 0.5) + (inputArray_B[i].red * 0.5);
		outputArray[i].green = (inputArray_A[i].green * 0.5) + (inputArray_B[i].green * 0.5);
		outputArray[i].blue = (inputArray_A[i].blue * 0.5) + (inputArray_B[i].blue * 0.5);
		//printf("%d %d %d\n", outputArray[i].red, outputArray[i].green, outputArray[i].blue);
	}

	fclose(fp_A);
	fclose(fp_B);

	free(inputArray_A);
	free(inputArray_B);
	free(outputArray);


	/*
	int* inputArray_A = (int*)malloc(sizeof(int) * G_SIZE);
	int* inputArray_B = (int*)malloc(sizeof(int) * G_SIZE);
	int* outputArray = (int*)malloc(sizeof(int) * G_SIZE);

	for (int i = 0; i < G_SIZE; i++)
	{
		inputArray_A[i] = i + 1;
		inputArray_B[i] = (i + 1) * 2 + 2;
	}

	int i;
	printf("Array A : ");
	for (i = 0; i < G_SIZE; i++)
	{
		printf("%d ", inputArray_A[i]);
	}
	printf("\n");
	printf("Array B : ");
	for (i = 0; i < G_SIZE; i++)
	{
		printf("%d ", inputArray_B[i]);
	}
	printf("\n");

	for (int i = 0; i < G_SIZE; i++)
	{
		outputArray[i] = inputArray_A[i] + inputArray_B[i];
	}

	//printf("output  : ");
	for (int i = 0; i < G_SIZE; i++)
	{
		printf("%d ", outputArray[i]);
	}
	//printf("\n");

	free(inputArray_A);
	free(inputArray_B);
	free(outputArray);
	*/

}

int main(int argc, char** argv) {

	QueryPerformanceFrequency(&tot_clockFreq);

	// OpenCL 디바이스, 커널 셋업
	CLInit();
	
	QueryPerformanceCounter(&tot_beginClock); //시간측정 시작

	//디바이스 쪽 버퍼 생성 및 write								 
	bufferWrite();	

	//커널 실행
	runKernel();

	QueryPerformanceCounter(&tot_endClock);
	double totalTime = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_GPU : %f ms\n", totalTime * 1000);

	system("pause");

	Release();

	// CPU 연산
	QueryPerformanceCounter(&tot_beginClock); //시간측정 시작

	CpuCal();

	QueryPerformanceCounter(&tot_endClock);
	double totalTime2 = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_CPU : %f ms\n", totalTime2 * 1000);



	return 0;

}
