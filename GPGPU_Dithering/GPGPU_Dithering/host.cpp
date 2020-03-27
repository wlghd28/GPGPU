// Add you host code.
#include "deviceInfo.h"
// BMP �̹����� �ҷ����� ���� �۾�
#pragma warning(disable : 4996)

double total_Time_GPU, total_Time_CPU, tmp_time;

BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;

RGBQUAD* rgb;
unsigned char* pix;
int * pixE;
int quant_error;
int bpl;

unsigned char* pix_output;
int* pixE_1;
int* pixE_2;
int* pixE_3;
int* pixE_4;


char* readSource(char* kernelPath);
void CLInit();
void bufferWrite();
void runKernel();
void Release();

void CpuCal();
void FwriteCPU();
void FwriteGPU();
int main(int argc, char** argv) {

	FILE * fp;
	fp = fopen("EDIMAGE.bmp", "rb");

	fread(&bfh, sizeof(bfh), 1, fp);
	fread(&bih, sizeof(bih), 1, fp);

	rgb = (RGBQUAD*)malloc(sizeof(RGBQUAD) * 256);
	fread(rgb, sizeof(RGBQUAD), 256, fp);
	// BPL�� �����ֱ� ���ؼ� �ȼ��������� ����� 4�� ����� ����
	bpl = (bih.biWidth + 3) / 4 * 4;

	pix = (unsigned char *)malloc(sizeof(unsigned char) * bpl * bih.biHeight);
	memset(pix, 0, sizeof(unsigned char) * bpl * bih.biHeight);
	fread(pix, sizeof(unsigned char), bpl * bih.biHeight, fp);

	pix_output = (unsigned char *)malloc(sizeof(unsigned char) * bpl * bih.biHeight);
	memcpy(pix_output, pix, sizeof(unsigned char) * bpl * bih.biHeight);

	pixE = (int *)malloc(sizeof(int) * bpl * bih.biHeight);
	memset(pixE, 0, sizeof(int) * bpl * bih.biHeight);
	pixE_1 = (int *)malloc(sizeof(int) * bpl * bih.biHeight);
	memset(pixE_1, 0, sizeof(int) * bpl * bih.biHeight);
	pixE_2 = (int *)malloc(sizeof(int) * bpl * bih.biHeight);
	memset(pixE_2, 0, sizeof(int) * bpl * bih.biHeight);
	pixE_3 = (int *)malloc(sizeof(int) * bpl * bih.biHeight);
	memset(pixE_3, 0, sizeof(int) * bpl * bih.biHeight);
	pixE_4 = (int *)malloc(sizeof(int) * bpl * bih.biHeight);
	memset(pixE_4, 0, sizeof(int) * bpl * bih.biHeight);



	// GPU ������ ���� ���� �ð� �ʱ�ȭ.
	total_Time_GPU = 0;
	// GPU ����
	QueryPerformanceFrequency(&tot_clockFreq);	// �ð��� �����ϱ����� �غ�
	// OpenCL ����̽�, Ŀ�� �¾�
	CLInit();


	QueryPerformanceCounter(&tot_beginClock); // �ð����� ����
	// ����̽� �� ���� ���� �� write								 
	bufferWrite();
	//Ŀ�� ����
	runKernel();
	QueryPerformanceCounter(&tot_endClock);
	total_Time_GPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
		
	printf("Total processing Time_GPU : %f ms\n", total_Time_GPU * 1000);

	Release();

	printf("\n");
	system("pause");

	FwriteGPU();

	// CPU ������ ���� ���� �ð� �ʱ�ȭ
	total_Time_CPU = 0;
	QueryPerformanceCounter(&tot_beginClock); // �ð����� ����
	// CPU ����
	CpuCal();
	QueryPerformanceCounter(&tot_endClock);
	total_Time_CPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_CPU : %f ms\n", total_Time_CPU * 1000);

	printf("Time_CPU/Time_GPU = %.3lf\n", (double)total_Time_CPU / total_Time_GPU);

	system("pause");

	FwriteCPU();



	free(rgb);

	free(pix);
	free(pix_output);

	free(pixE);
	free(pixE_1);
	free(pixE_2);
	free(pixE_3);
	free(pixE_4);

	fclose(fp);

	return 0;
}
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
	char * source = readSource("dithering.cl");

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
	d_pix = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bpl * bih.biHeight * sizeof(unsigned char), NULL, NULL);
	d_pixE_1 = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bpl * bih.biHeight * sizeof(int), NULL, NULL);
	d_pixE_2 = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bpl * bih.biHeight * sizeof(int), NULL, NULL);
	d_pixE_3 = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bpl * bih.biHeight * sizeof(int), NULL, NULL);
	d_pixE_4 = clCreateBuffer(context, CL_MEM_READ_WRITE,
		bpl * bih.biHeight * sizeof(int), NULL, NULL);



	clEnqueueWriteBuffer(queue, d_pix, CL_TRUE, 0, bpl * bih.biHeight * sizeof(unsigned char),
		pix_output, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_pixE_1, CL_TRUE, 0, bpl * bih.biHeight * sizeof(int),
		pixE_1, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_pixE_2, CL_TRUE, 0, bpl * bih.biHeight * sizeof(int),
		pixE_2, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_pixE_3, CL_TRUE, 0, bpl * bih.biHeight * sizeof(int),
		pixE_3, 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, d_pixE_4, CL_TRUE, 0, bpl * bih.biHeight * sizeof(int),
		pixE_4, 0, NULL, NULL);

}

void runKernel()
{
	int totalWorkItemsX = bpl * bih.biHeight;
	int totalWorkItemsY = 1;

	size_t globalSize[2] = { totalWorkItemsX, totalWorkItemsY };
	//float *minVal, *maxVal;


	// Ŀ�� �Ű����� ���� 
	clSetKernelArg(simpleKernel, 0, sizeof(cl_mem), &d_pix);
	clSetKernelArg(simpleKernel, 1, sizeof(cl_mem), &d_pixE_1);
	clSetKernelArg(simpleKernel, 2, sizeof(cl_mem), &d_pixE_2);
	clSetKernelArg(simpleKernel, 3, sizeof(cl_mem), &d_pixE_3);
	clSetKernelArg(simpleKernel, 4, sizeof(cl_mem), &d_pixE_4);

	clEnqueueNDRangeKernel(queue, simpleKernel, 2, NULL, globalSize,
		NULL, 0, NULL, NULL);
	// �Ϸ� ��� 
	clFinish(queue);


	clEnqueueReadBuffer(queue, d_pix, CL_TRUE, 0,
		bpl * bih.biHeight * sizeof(unsigned char), pix_output, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_pixE_1, CL_TRUE, 0,
		bpl * bih.biHeight * sizeof(int), pixE_1, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_pixE_2, CL_TRUE, 0,
		bpl * bih.biHeight * sizeof(int), pixE_2, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_pixE_3, CL_TRUE, 0,
		bpl * bih.biHeight * sizeof(int), pixE_3, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_pixE_4, CL_TRUE, 0,
		bpl * bih.biHeight * sizeof(int), pixE_4, 0, NULL, NULL);

	/*
	for (int y = 1; y < bih.biHeight - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{
			pixE_1[y * bpl + x] += pix_output[y * bpl + x + 1];
			pixE_2[y * bpl + x] += pix_output[(y + 1) * bpl + x - 1];
			pixE_3[y * bpl + x] += pix_output[(y + 1) * bpl + x];
			pixE_4[y * bpl + x] += pix_output[(y + 1) * bpl + x + 1];

			pix_output[y * bpl + x + 1] = pixE_1[y * bpl + x] / 128 * 255;
			pix_output[(y + 1) * bpl + x - 1] = pixE_2[y * bpl + x] / 128 * 255;
			pix_output[(y + 1) * bpl + x] = pixE_3[y * bpl + x] / 128 * 255;
			pix_output[(y + 1) * bpl + x + 1] = pixE_4[y * bpl + x] / 128 * 255;
		}
	}
	*/
}

void Release()
{
	// ������
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}
void CpuCal() 
{
	int quant_err = 0;
	for (int y = 1; y < (bih.biHeight - 1); y++)
	{
		for (int x = 1; x < (bpl - 1); x++)
		{
			// ����ó���� ������ �κ�.
			pixE[y * bpl + x] += pix[y * bpl + x];;
			pix[y * bpl + x] = pixE[y * bpl + x] / 128 * 255;

			quant_error = pixE[y * bpl + x] - pix[y * bpl + x];

			pixE[y * bpl + x + 1] += quant_error * 7 / 16;
			pixE[(y + 1) * bpl + x - 1] += quant_error * 3 / 16;
			pixE[(y + 1) * bpl + x] += quant_error * 5 / 16;
			pixE[(y + 1) * bpl + x - 2] += quant_error * 1 / 16;
			// pix[y * bih.biWidth + x + 1] += pixE[y * bih.biWidth + x + 1];
		}
	}
}
void FwriteCPU()
{
	// ������ �ȼ����� bmp���Ϸ� ����.
	FILE * fp2 = fopen("new_EDIMAGE_CPU.bmp", "wb");
	fwrite(&bfh, sizeof(bfh), 1, fp2);
	fwrite(&bih, sizeof(bih), 1, fp2);
	fwrite(rgb, sizeof(RGBQUAD), 256, fp2);

	fwrite(pix, sizeof(unsigned char), bpl * bih.biHeight, fp2);
	fclose(fp2);
}
void FwriteGPU()
{
	// ������ �ȼ����� bmp���Ϸ� ����.
	FILE * fp2 = fopen("new_EDIMAGE_GPU.bmp", "wb");
	fwrite(&bfh, sizeof(bfh), 1, fp2);
	fwrite(&bih, sizeof(bih), 1, fp2);
	fwrite(rgb, sizeof(RGBQUAD), 256, fp2);

	fwrite(pix_output, sizeof(unsigned char), bpl * bih.biHeight, fp2);
	fclose(fp2);
}