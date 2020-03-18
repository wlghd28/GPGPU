#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <Windows.h>
#include <string.h>
#include <tchar.h>
#include <stdbool.h>

#include "deviceInfo.h"
#include "resource.h"
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
	unsigned char alpha;
	unsigned char blue;
	unsigned char green;
	unsigned char red;
}rgb;
#pragma pack(pop)	


int bihwidth;		// 이미지 가로 사이즈
int bihheight;		// 이미지 세로 사이즈 (line 수)

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

	FILE * fp;
	struct Bitmapfileheader bfh;
	struct Bitmapinfoheader bih;
	struct Palette rgb;

	fp = fopen("background.bmp", "rb");
	if (fp == NULL)
	{
		printf("file not found");
		getchar();
		exit(0);
	}
	fread(&bfh, sizeof(bfh), 1, fp);
	fread(&bih, sizeof(bfh), 1, fp);

	bihwidth = ((bih.biwidth + 3) / 4) * 4;
	bihheight = bih.biheight;

	// 메모리 버퍼 생성
	d_red = clCreateBuffer(context, CL_MEM_READ_ONLY,
		bihwidth * bihheight * sizeof(unsigned char), NULL, NULL);
	d_green = clCreateBuffer(context, CL_MEM_READ_ONLY,
		bihwidth * bihheight * sizeof(unsigned char), NULL, NULL);
	d_blue = clCreateBuffer(context, CL_MEM_READ_ONLY,
		bihwidth * bihheight * sizeof(unsigned char), NULL, NULL);
	d_red_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
		bihwidth * bihheight * sizeof(unsigned char), NULL, NULL);
	d_green_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
		bihwidth * bihheight * sizeof(unsigned char), NULL, NULL);
	d_blue_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
		bihwidth * bihheight * sizeof(unsigned char), NULL, NULL);


	unsigned char* red = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	unsigned char* green = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	unsigned char* blue = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	
	for (int i = 0; i < bihwidth * bihheight; i++) {
		fread(&rgb, 4, 1, fp);
		red[i] = rgb.red;
		green[i] = rgb.green;
		blue[i] = rgb.blue;
	}


	clEnqueueWriteBuffer(queue, d_red, CL_TRUE, 0, sizeof(unsigned char) * bihwidth * bihheight,
		red, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_green, CL_TRUE, 0, sizeof(unsigned char) * bihwidth * bihheight,
		green, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_blue, CL_TRUE, 0, sizeof(unsigned char) * bihwidth * bihheight,
		blue, 0, NULL, NULL);

	free(red);
	free(green);
	free(blue);

	fclose(fp);

}

void runKernel(long int per, long int size)
{
	int totalWorkItemsX = bihwidth * bihheight;
	int totalWorkItemsY = 1;

	size_t globalSize[2] = { totalWorkItemsX, totalWorkItemsY };
	//float *minVal, *maxVal;


	// 커널 매개변수 설정 
	clSetKernelArg(simpleKernel, 0, sizeof(cl_mem), &d_red);
	clSetKernelArg(simpleKernel, 1, sizeof(cl_mem), &d_green);
	clSetKernelArg(simpleKernel, 2, sizeof(cl_mem), &d_blue);

	clSetKernelArg(simpleKernel, 3, sizeof(cl_mem), &d_red_output);
	clSetKernelArg(simpleKernel, 4, sizeof(cl_mem), &d_green_output);
	clSetKernelArg(simpleKernel, 5, sizeof(cl_mem), &d_blue_output);

	clSetKernelArg(simpleKernel, 6, sizeof(long int), &per);
	clSetKernelArg(simpleKernel, 7, sizeof(long int), &size);
	clEnqueueNDRangeKernel(queue, simpleKernel, 2, NULL, globalSize,
		NULL, 0, NULL, NULL);
	// 완료 대기 
	clFinish(queue);
	

	unsigned char* red_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	unsigned char* green_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	unsigned char* blue_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);

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
	for (int i = 0; i < bihheight; i++)
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
void CpuCal(long int size) {

	FILE* fp;
	struct Bitmapfileheader bfh;
	struct Bitmapinfoheader bih;
	struct Palette* rgb;
	struct Palette* outputArray;

	fp = fopen("background.bmp", "rb");

	if (fp == NULL)
	{
		printf("file not found");
		getchar();
		exit(0);
	}

	fread(&bfh, sizeof(bfh), 1, fp);
	fread(&bih, sizeof(bih), 1, fp);

	bihwidth = ((bih.biwidth + 3) / 4) * 4;
	bihheight = bih.biheight;

	rgb = (struct Palette*)malloc(sizeof(struct Palette) * bihwidth * bihheight);
	outputArray = (struct Palette*)malloc(sizeof(struct Palette) * bihwidth * bihheight);

	for (int i = 0; i < bihwidth * bihheight; i++)
	{
		fread(&rgb[i], 4, 1, fp);
	}

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < bihwidth * bihheight; j++) {
			outputArray[j].red = (rgb[j].red * i / size);
			outputArray[j].green = (rgb[j].green * i / size);
			outputArray[j].blue = (rgb[j].blue * i / size);
		}
	}

	/*
	int index = 0;
	HDC hdc;
	hdc = GetDC(NULL);
	for (int i = 0; i< bihheight; i++)
	{
		for (int j = 0; j < bihwidth; j++)
		{
			SetPixel(hdc, j, bihheight - i - 1, RGB(outputArray[index].red, outputArray[index].green, outputArray[index].blue));
			index++;
		}
	}
	ReleaseDC(NULL, hdc);
	*/

	free(rgb);
	free(outputArray);

	fclose(fp);
}

int main(int argc, char** argv) {

	long int data_size;	// 데이터 양 변수
	long int i = 0;		// 반복문 변수

	printf("실험할 데이터 량 입력 : ");
	scanf("%d", &data_size);

	QueryPerformanceFrequency(&tot_clockFreq);

	// OpenCL 디바이스, 커널 셋업
	CLInit();
	
	QueryPerformanceCounter(&tot_beginClock); //시간측정 시작

	//디바이스 쪽 버퍼 생성 및 write								 
	bufferWrite();

	for (i = 0; i < data_size; i++) {
		//커널 실행
		runKernel(i, data_size);
	}

	QueryPerformanceCounter(&tot_endClock);
	double totalTime = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_GPU : %f ms\n", totalTime * 1000);

	Release();

	printf("\n");

	// CPU 연산
	QueryPerformanceCounter(&tot_beginClock); //시간측정 시작

	//CPU 연산
	CpuCal(data_size);

	QueryPerformanceCounter(&tot_endClock);
	double totalTime2 = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_CPU : %f ms\n", totalTime2 * 1000);

	return 0;
}
