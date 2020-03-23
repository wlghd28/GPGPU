#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <Windows.h>
#include <string.h>
#include <tchar.h>
#include <stdbool.h>

#include "deviceInfo.h"
#include "resource.h"
// BMP �̹����� �ҷ����� ���� �۾�
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
	//unsigned char alpha;
	unsigned char blue;
	unsigned char green;
	unsigned char red;
}rgb;
#pragma pack(pop)	

BITMAPFILEHEADER new_background_bfh;
BITMAPINFOHEADER new_background_bih;

struct Palette* rgbpix;

unsigned char* red_output;
unsigned char* green_output;
unsigned char* blue_output;

int bihwidth;		// �̹��� ���� ������
int bihheight;		// �̹��� ���� ������ (line ��)


double total_Time_CPU;			// CPU�� ���� ���� �ð�
double total_Time_GPU;			// GPU�� ���� ���� �ð�
double tmp_Time = 0;			// ��� �ð��� ���� ��Ű�� ���� ���� �ӽ� ����


void Draw();		// ����� RGB���� ȭ�鿡 ����ϴ� �Լ�.

// kernel�� �о char pointer����
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

//����̽� init, Ŀ�� ����
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

	// �ؽ�Ʈ���Ϸκ��� ���α׷� �б�
	char * source = readSource("convolution.cl");

	// compile program
	program = clCreateProgramWithSource(context, 1,
		(const char **)&source, NULL, NULL);
	cl_int build_status;
	build_status = clBuildProgram(program, 1, &device, NULL, NULL,
		NULL);

	//Ŀ�� ������ ����
	simpleKernel = clCreateKernel(program, "simpleKernel", NULL);

}

//���ۻ��� �� write
void bufferWrite()
{

	// �޸� ���� ����
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
		red[i] = rgbpix[i].red;
		green[i] = rgbpix[i].green;
		blue[i] = rgbpix[i].blue;
		//printf("%d %d %d\n", red[i], green[i], blue[i]);
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

}

void runKernel(int per, int size)
{
	int totalWorkItemsX = bihwidth * bihheight;
	int totalWorkItemsY = 1;

	size_t globalSize[2] = { totalWorkItemsX, totalWorkItemsY };
	//float *minVal, *maxVal;


	// Ŀ�� �Ű����� ���� 
	clSetKernelArg(simpleKernel, 0, sizeof(cl_mem), &d_red);
	clSetKernelArg(simpleKernel, 1, sizeof(cl_mem), &d_green);
	clSetKernelArg(simpleKernel, 2, sizeof(cl_mem), &d_blue);

	clSetKernelArg(simpleKernel, 3, sizeof(cl_mem), &d_red_output);
	clSetKernelArg(simpleKernel, 4, sizeof(cl_mem), &d_green_output);
	clSetKernelArg(simpleKernel, 5, sizeof(cl_mem), &d_blue_output);

	clSetKernelArg(simpleKernel, 6, sizeof(int), &per);
	clSetKernelArg(simpleKernel, 7, sizeof(int), &size);

	clEnqueueNDRangeKernel(queue, simpleKernel, 2, NULL, globalSize,
		NULL, 0, NULL, NULL);
	// �Ϸ� ��� 
	clFinish(queue);
	

	clEnqueueReadBuffer(queue, d_red_output, CL_TRUE, 0,
		bihwidth * bihheight * sizeof(unsigned char), red_output, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_green_output, CL_TRUE, 0,
		bihwidth * bihheight * sizeof(unsigned char), green_output, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_blue_output, CL_TRUE, 0,
		bihwidth * bihheight * sizeof(unsigned char), blue_output, 0, NULL, NULL);


}
void Release()
{
	// ������
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}
void CpuCal(int size) {

	for (int i = 0; i < size; i++) {
		QueryPerformanceCounter(&tot_beginClock); //�ð����� ����
		for (int j = 0; j < bihwidth * bihheight; j++) {
			red_output[j] = (rgbpix[j].red * i / size);
			green_output[j] = (rgbpix[j].green * i / size);
			blue_output[j] = (rgbpix[j].blue * i / size);
		}
		QueryPerformanceCounter(&tot_endClock);
		tmp_Time = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
		total_Time_CPU += tmp_Time;
		Draw();
	}

}

int main(int argc, char** argv) {

	FILE * fp;
	// RGB �ȼ��� �޸� �Ҵ�

	fp = fopen("background.dib", "rb");
	if (fp == NULL)
	{
		printf("file not found");
		getchar();
		exit(0);
	}


	fread(&new_background_bfh, sizeof(BITMAPFILEHEADER), 1, fp);
	fread(&new_background_bih, sizeof(BITMAPINFOHEADER), 1, fp);

	bihwidth = new_background_bih.biWidth;
	bihheight = new_background_bih.biHeight;
	rgbpix = (struct Palette*)malloc(sizeof(struct Palette) * bihwidth * bihheight);

	for (int i = 0; i < bihwidth * bihheight; i++)
	{
		fread(&rgbpix[i], 3, 1, fp);
	}
	red_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	green_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);
	blue_output = (unsigned char*)malloc(sizeof(unsigned char) * bihwidth * bihheight);

	int data_size;	// ������ �� ����
	int i = 0;		// �ݺ��� ����

	printf("������ ������ �� �Է� : ");
	scanf("%d", &data_size);


	// GPU ������ ���� ���� �ð� �ʱ�ȭ.
	total_Time_GPU = 0;
	// GPU ����
	QueryPerformanceFrequency(&tot_clockFreq);	// �ð��� �����ϱ����� �غ�
	// OpenCL ����̽�, Ŀ�� �¾�
	CLInit();


	QueryPerformanceCounter(&tot_beginClock); // ���� Write �ð����� ����
	// ����̽� �� ���� ���� �� write								 
	bufferWrite();
	QueryPerformanceCounter(&tot_endClock);
	tmp_Time = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	total_Time_GPU += tmp_Time;

	for (i = 0; i < data_size; i++) {
		QueryPerformanceCounter(&tot_beginClock); // runKernel �ð����� ����
		//Ŀ�� ����
		runKernel(i, data_size);
		QueryPerformanceCounter(&tot_endClock);
		tmp_Time = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
		total_Time_GPU += tmp_Time;
		Draw();
	}

	printf("Total processing Time_GPU : %f ms\n", total_Time_GPU * 1000);

	Release();

	printf("\n");
	system("pause");
	

	// CPU ������ ���� ���� �ð� �ʱ�ȭ
	total_Time_CPU = 0;

	// CPU ����
	CpuCal(data_size);

	printf("Total processing Time_CPU : %f ms\n", total_Time_CPU * 1000);
	
	printf("Time_CPU/Time_GPU = %.3lf\n", (double)total_Time_CPU/total_Time_GPU);
	free(rgbpix);
	free(red_output);
	free(green_output);
	free(blue_output);

	fclose(fp);

	system("pause");
	return 0;
}
// ����� RGB ���� ȭ�鿡 ��½�Ų��.
void Draw()
{
	struct Palette* pal = (struct Palette*)malloc(sizeof(struct Palette) * bihwidth * bihheight);
	HDC hdc;
	hdc = GetDC(NULL);

	for (int i = 0; i < bihwidth * bihheight; i++)
	{
		pal[i].red = red_output[i];
		pal[i].green = green_output[i];
		pal[i].blue = blue_output[i];
	}

	SetDIBitsToDevice(hdc, 0, 0, bihwidth, bihheight, 0, 0, 0, bihheight,
		(BYTE *)pal, (const BITMAPINFO *)&new_background_bih, DIB_RGB_COLORS);

	ReleaseDC(NULL, hdc);
	free(pal);

}
