// Add you host code.
#include "deviceInfo.h"
// BMP 이미지를 불러오기 위한 작업
#pragma warning(disable : 4996)

double total_Time_GPU, total_Time_CPU, tmp_time;
int * err;
int * di_num;
int * output;

char* readSource(char* kernelPath);
void CLInit();
void bufferWrite();
void runKernel(int quant_err);
void Release();

void CpuCal(int quant_err);

int main(int argc, char** argv) {

	err = (int *)malloc(sizeof(int) * 4);
	di_num = (int *)malloc(sizeof(int) * 4);
	output = (int *)malloc(sizeof(int) * 4);
	memset(err, 0, sizeof(int) * 4);
	memset(di_num, 0, sizeof(int) * 4);
	memset(output, 0, sizeof(int) * 4);

	err[0] = 155;
	err[1] = 255;
	err[2] = 136;
	err[3] = 187;

	// floyd steinberg 알고리즘 연산 숫자 ( 7, 3, 5 ,1 )
	di_num[0] = 7;
	di_num[1] = 3;
	di_num[2] = 5;
	di_num[3] = 1;

	// GPU 연산을 위한 측정 시간 초기화.
	total_Time_GPU = 0;
	// GPU 연산
	QueryPerformanceFrequency(&tot_clockFreq);	// 시간을 측정하기위한 준비
	// OpenCL 디바이스, 커널 셋업
	CLInit();


	// 디바이스 쪽 버퍼 생성 및 write								 
	bufferWrite();
	QueryPerformanceCounter(&tot_beginClock); // 시간측정 시작
	//커널 실행
	runKernel(157);
	QueryPerformanceCounter(&tot_endClock);
	total_Time_GPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
		
	printf("Total processing Time_GPU : %f ms\n", total_Time_GPU * 1000);

	Release();

	printf("\n");
	system("pause");


	// CPU 연산을 위한 측정 시간 초기화
	total_Time_CPU = 0;
	QueryPerformanceCounter(&tot_beginClock); // 시간측정 시작
	// CPU 연산
	CpuCal(157);
	QueryPerformanceCounter(&tot_endClock);
	total_Time_CPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_CPU : %f ms\n", total_Time_CPU * 1000);

	printf("Time_CPU/Time_GPU = %.3lf\n", (double)total_Time_CPU / total_Time_GPU);


	system("pause");

	free(err);
	free(di_num);
	free(output);

	return 0;
}
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
	char * source = readSource("dithering.cl");

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

	// 메모리 버퍼 생성
	d_err = clCreateBuffer(context, CL_MEM_READ_ONLY,
		4 * sizeof(int), NULL, NULL);
	d_di_num = clCreateBuffer(context, CL_MEM_READ_WRITE,
		4 * sizeof(int), NULL, NULL);


	clEnqueueWriteBuffer(queue, d_err, CL_TRUE, 0, 4 * sizeof(int),
		err, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_di_num, CL_TRUE, 0, 4 * sizeof(int),
		di_num, 0, NULL, NULL);

}
void runKernel(int quant_err)
{
	int totalWorkItemsX = 4;
	int totalWorkItemsY = 1;

	size_t globalSize[2] = { totalWorkItemsX, totalWorkItemsY };
	//float *minVal, *maxVal;


	// 커널 매개변수 설정 
	clSetKernelArg(simpleKernel, 0, sizeof(cl_mem), &d_err);
	clSetKernelArg(simpleKernel, 1, sizeof(int), &quant_err);
	clSetKernelArg(simpleKernel, 2, sizeof(cl_mem), &d_di_num);

	clEnqueueNDRangeKernel(queue, simpleKernel, 2, NULL, globalSize,
		NULL, 0, NULL, NULL);
	// 완료 대기 
	clFinish(queue);


	clEnqueueReadBuffer(queue, d_err, CL_TRUE, 0,
		4 * sizeof(int), output, 0, NULL, NULL);

}
void Release()
{
	// 릴리즈
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}
void CpuCal(int quant_err) 
{
	for (int i = 0; i < 4; i++)
	{
		err[i] += quant_err * di_num[i] / 16;
	}
}
