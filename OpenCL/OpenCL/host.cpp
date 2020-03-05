#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <Windows.h>
#include <string.h>
#include <tchar.h>
#include <stdbool.h>

#include "deviceInfo.h"

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

	// �ؽ�Ʈ���Ϸκ��� ���α׷� �б�
	char* source = readSource("convolution.cl");

	// compile program
	program = clCreateProgramWithSource(context, 1,
		(const char**)&source, NULL, NULL);
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

	d_inputArray_A = clCreateBuffer(context, CL_MEM_READ_WRITE,
		10 * sizeof(int), NULL, NULL);
	d_inputArray_B = clCreateBuffer(context, CL_MEM_READ_WRITE,
		10 * sizeof(int), NULL, NULL);
	d_outputArray = clCreateBuffer(context, CL_MEM_READ_WRITE,
		10 * sizeof(int), NULL, NULL);

	int inputArray_A[10] = { 2,2,2,2,2,3,3,3,3,3 };
	int inputArray_B[10] = { 1,1,1,1,1,4,4,4,4,4 };


	int i;
	printf("Array A : ");
	for (i = 0; i < 10; i++)
	{
		printf("%d ", inputArray_A[i]);
	}
	printf("\n");
	printf("Array B : ");
	for (i = 0; i < 10; i++)
	{
		printf("%d ", inputArray_B[i]);
	}
	printf("\n");

	clEnqueueWriteBuffer(queue, d_inputArray_A, CL_TRUE, 0, 10 * sizeof(int),
		inputArray_A, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_inputArray_B, CL_TRUE, 0, 10 * sizeof(int),
		inputArray_B, 0, NULL, NULL);


}

void runKernel()
{

	int i;

	int totalWorkItemsX = 10;
	int totalWorkItemsY = 1;

	size_t globalSize[2] = { totalWorkItemsX, totalWorkItemsY };

	float *minVal, *maxVal;


	// Ŀ�� �Ű����� ���� 
	clSetKernelArg(simpleKernel, 0, sizeof(cl_mem), &d_inputArray_A);
	clSetKernelArg(simpleKernel, 1, sizeof(cl_mem), &d_inputArray_B);
	clSetKernelArg(simpleKernel, 2, sizeof(cl_mem), &d_outputArray);


	clEnqueueNDRangeKernel(queue, simpleKernel, 2, NULL, globalSize,
		NULL, 0, NULL, NULL);

	// �Ϸ� ��� 
	clFinish(queue);

	int outputArray[10];
	clEnqueueReadBuffer(queue, d_outputArray, CL_TRUE, 0,
		10 * sizeof(int), outputArray, 0, NULL, NULL);


	printf("output  : ");
	for (i = 0; i < 10; i++)
	{
		printf("%d ", outputArray[i]);
	}
	printf("\n");
}

void Release()
{
	// ������
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}

void CpuCal() {
	int inputArray_A[10] = { 2,2,2,2,2,3,3,3,3,3 };
	int inputArray_B[10] = { 1,1,1,1,1,4,4,4,4,4 };
	int outputArray[10];

	int i;
	printf("Array A : ");
	for (i = 0; i < 10; i++)
	{
		printf("%d ", inputArray_A[i]);
	}
	printf("\n");
	printf("Array B : ");
	for (i = 0; i < 10; i++)
	{
		printf("%d ", inputArray_B[i]);
	}
	printf("\n");

	for (i = 0; i < 10; i++)
	{
		outputArray[i] = inputArray_A[i] + inputArray_B[i];
	}

	printf("output  : ");
	for (i = 0; i < 10; i++)
	{
		printf("%d ", outputArray[i]);
	}
	printf("\n");
}

int main(int argc, char** argv) {


	QueryPerformanceFrequency(&tot_clockFreq);

	// OpenCL ����̽�, Ŀ�� �¾�
	CLInit();
	
	QueryPerformanceCounter(&tot_beginClock); //�ð����� ����
											 
	bufferWrite();							  //����̽� �� ���� ���� �� write
	//Ŀ�� ����
	runKernel();

	QueryPerformanceCounter(&tot_endClock);
	double totalTime = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_GPU : %f ms\n", totalTime * 1000);

	system("pause");

	Release();

	// CPU ����
	QueryPerformanceCounter(&tot_beginClock); //�ð����� ����

	CpuCal();

	QueryPerformanceCounter(&tot_endClock);
	double totalTime2 = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_CPU : %f ms\n", totalTime2 * 1000);



	return 0;




}
// Add you host code