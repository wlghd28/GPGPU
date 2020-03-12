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

const long int G_SIZE = 1000;

int bihwidth;
int bihheight;

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
	char * value;
	size_t valueSize;
	cl_uint platformCount;
	cl_platform_id * platforms;
	cl_uint deviceCount;
	cl_device_id * devices;
	cl_uint maxComputeUnits;

	// get all platforms
	clGetPlatformIDs(0, NULL, &platformCount);
	platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * platformCount);
	clGetPlatformIDs(platformCount, platforms, NULL);

	for (i = 0; i < platformCount; i++) {

		// get all devices
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
		devices = (cl_device_id *)malloc(sizeof(cl_device_id) * deviceCount);
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

		// for each device print critical attributes
		for (j = 0; j < deviceCount; j++) {

			// print device name
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
			value = (char *)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
			printf("platform %d. Device %d: %s\n", i + 1, j + 1, value);
			free(value);
			
			// print hardware device version
			clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, 0, NULL, &valueSize);
			value = (char *)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, valueSize, value, NULL);
			printf(" %d.%d Hardware version: %s\n", i + 1, 1, value);
			free(value);

			// print software driver version
			clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, 0, NULL, &valueSize);
			value = (char *)malloc(valueSize);
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
	char * source = readSource("convolution.cl");

	// compile program
	program = clCreateProgramWithSource(context, 1,
		(const char **)&source, NULL, NULL);
	cl_int build_status;
	build_status = clBuildProgram(program, 1, &device, NULL, NULL,
		NULL);

	//커널 포인터 생성
	simpleKernel = clCreateKernel(program, "simpleKernel", NULL);

}

//버퍼생성 및 write
void bufferWrite()
{

	FILE * fp_A;
	FILE * fp_B;
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

	bihwidth = bih.biwidth;
	bihheight = bih.biheight;

	// 메모리 버퍼 생성

	d_red_A = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_green_A = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_blue_A = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_red_B = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_green_B = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_blue_B = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_red_output = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_green_output = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);
	d_blue_output = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bih.biwidth * bih.biheight * sizeof(unsigned char), NULL, NULL);


	unsigned char* red_A = (unsigned char*)malloc(sizeof(unsigned char) * bih.biwidth * bih.biheight);
	unsigned char* green_A = (unsigned char*)malloc(sizeof(unsigned char) * bih.biwidth * bih.biheight);
	unsigned char* blue_A = (unsigned char*)malloc(sizeof(unsigned char) * bih.biwidth * bih.biheight);
	unsigned char* red_B = (unsigned char*)malloc(sizeof(unsigned char) * bih.biwidth * bih.biheight);
	unsigned char* green_B = (unsigned char*)malloc(sizeof(unsigned char) * bih.biwidth * bih.biheight);
	unsigned char* blue_B = (unsigned char*)malloc(sizeof(unsigned char) * bih.biwidth * bih.biheight);

	
	for (int i = 0; i < bih.biwidth * bih.biheight; i++) {
		fread(&rgb1, 3, 1, fp_A);
		fread(&rgb2, 3, 1, fp_B);
		red_A[i] = rgb1.red;
		green_A[i] = rgb1.green;
		blue_A[i] = rgb1.blue;

		red_B[i] = rgb2.red;
		green_B[i] = rgb2.green;
		blue_B[i] = rgb2.blue;
	}

	
	clEnqueueWriteBuffer(queue, d_red_A, CL_TRUE, 0, sizeof(unsigned char) * bih.biwidth * bih.biheight,
		red_A, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_green_A, CL_TRUE, 0, sizeof(unsigned char) * bih.biwidth * bih.biheight,
		green_A, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_blue_A, CL_TRUE, 0, sizeof(unsigned char) * bih.biwidth * bih.biheight,
		blue_A, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_red_B, CL_TRUE, 0, sizeof(unsigned char) * bih.biwidth * bih.biheight,
		red_B, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_green_B, CL_TRUE, 0, sizeof(unsigned char) * bih.biwidth * bih.biheight,
		green_B, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_blue_B, CL_TRUE, 0, sizeof(unsigned char) * bih.biwidth * bih.biheight,
		blue_B, 0, NULL, NULL);

	free(red_A);
	free(green_A);
	free(blue_A);
	free(red_B);
	free(green_B);
	free(blue_B);

	fclose(fp_A);
	fclose(fp_B);
}

void runKernel()
{
	int totalWorkItemsX = bihwidth * bihheight;
	int totalWorkItemsY = 1;

	size_t globalSize[2] = { totalWorkItemsX, totalWorkItemsY };
	//float *minVal, *maxVal;

	// 커널 매개변수 설정 
	clSetKernelArg(simpleKernel, 0, sizeof(cl_mem), &d_red_A);
	clSetKernelArg(simpleKernel, 1, sizeof(cl_mem), &d_green_A);
	clSetKernelArg(simpleKernel, 2, sizeof(cl_mem), &d_blue_A);
	clSetKernelArg(simpleKernel, 3, sizeof(cl_mem), &d_red_B);
	clSetKernelArg(simpleKernel, 4, sizeof(cl_mem), &d_green_B);
	clSetKernelArg(simpleKernel, 5, sizeof(cl_mem), &d_blue_B);
	clSetKernelArg(simpleKernel, 6, sizeof(cl_mem), &d_red_output);
	clSetKernelArg(simpleKernel, 7, sizeof(cl_mem), &d_green_output);
	clSetKernelArg(simpleKernel, 8, sizeof(cl_mem), &d_blue_output);

	clEnqueueNDRangeKernel(queue, simpleKernel, 2, NULL, globalSize,
		NULL, 0, NULL, NULL);
	// 완료 대기 
	clFinish(queue);
	

	unsigned char* red_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	unsigned char* green_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	unsigned char* blue_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	/*
	for (int i = 0; i < G_SIZE; i++) {
		printf("%d %d %d\n", outputArray[i].red, outputArray[i].green, outputArray[i].blue);
	}
	*/
	clEnqueueReadBuffer(queue, d_red_output, CL_TRUE, 0,
		bihwidth * bihheight * sizeof(unsigned char), red_output, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_green_output, CL_TRUE, 0,
		bihwidth * bihheight * sizeof(unsigned char), green_output, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_blue_output, CL_TRUE, 0,
		bihwidth * bihheight * sizeof(unsigned char), blue_output, 0, NULL, NULL);
	/*
	int index = 0;
	HDC hdc;
	hdc = GetDC(NULL);
	for (int i = 0; i< bihheight; i++)
	{
		for (int j = 0; j < bihwidth; j++)
		{
			SetPixel(hdc, j, bihheight - i - 1, RGB(red_output[index], green_output[index], blue_output[index]));
			index++;
		}
	}
	ReleaseDC(NULL, hdc);
	*/
	free(red_output);
	free(green_output);
	free(blue_output);
}

void Release()
{
	// 릴리즈
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}
void CpuCal() {

	FILE* fp_A;
	FILE* fp_B;
	struct Bitmapfileheader bfh;
	struct Bitmapinfoheader bih;
	COLORREF* output;
	struct Palette rgb1, rgb2;

	struct Palette* outputArray;

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

	outputArray = (struct Palette*)malloc(sizeof(struct Palette) * bih.biwidth * bih.biheight);;

	for (int i = 0; i < bih.biheight * bih.biwidth; i++)
	{	
		fread(&rgb1, 3, 1, fp_A);
		fread(&rgb2, 3, 1, fp_B);
		outputArray[i].red = (rgb1.red * (i + 1)/ bih.biheight * bih.biwidth) + (rgb2.red * (i + 1) / bih.biheight * bih.biwidth);
		outputArray[i].green = (rgb1.green * (i + 1) / bih.biheight * bih.biwidth) + (rgb2.green * (i + 1) / bih.biheight * bih.biwidth);
		outputArray[i].blue = (rgb1.blue * (i + 1) / bih.biheight * bih.biwidth) + (rgb2.blue * (i + 1) / bih.biheight * bih.biwidth);
	}
	/*
	int index = 0;
	HDC hdc, hdc_tmp;
	hdc = GetDC(NULL);
	for (int i = 0; i< bih.biheight; i++)
	{
		for (int j = 0; j<bih.biwidth; j++)
		{
			SetPixel(hdc, j, bih.biheight - i - 1, RGB(outputArray[index].red, outputArray[index].green, outputArray[index].blue));
			index++;
		}
	}
	ReleaseDC(NULL, hdc);
	*/

	free(outputArray);

	fclose(fp_A);
	fclose(fp_B);

}

int main(int argc, char** argv) {

	QueryPerformanceFrequency(&tot_clockFreq);

	// OpenCL 디바이스, 커널 셋업
	CLInit();
	for(int i = 0; i < G_SIZE; i++) 
	{
		QueryPerformanceCounter(&tot_beginClock); //시간측정 시작

	//디바이스 쪽 버퍼 생성 및 write								 
		bufferWrite();

		//커널 실행
		runKernel();

		QueryPerformanceCounter(&tot_endClock);
		double totalTime = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
		printf("Total processing Time_GPU : %f ms     ", totalTime * 1000);

		//system("pause");

		Release();

		// CPU 연산
		QueryPerformanceCounter(&tot_beginClock); //시간측정 시작

		CpuCal();

		QueryPerformanceCounter(&tot_endClock);
		double totalTime2 = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
		printf("Total processing Time_CPU : %f ms\n", totalTime2 * 1000);

		printf("\n");
	}


	return 0;

}
