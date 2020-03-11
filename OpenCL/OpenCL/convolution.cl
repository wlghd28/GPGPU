
// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

__kernel
void simpleKernel(
	__global struct Palette* inputArray_A,
	__global struct Palette* inputArray_B,
	__global struct Palette* outputArray
)
{	

	uint dstYStride = get_global_size(0);
	uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);

	outputArray[dstIndex].red = (inputArray_A[dstIndex].red * 0.5) + (inputArray_B[dstIndex].red * 0.5);
	outputArray[dstIndex].green = (inputArray_A[dstIndex].green * 0.5) + (inputArray_B[dstIndex].green * 0.5);
	outputArray[dstIndex].blue = (inputArray_A[dstIndex].blue * 0.5) + (inputArray_B[dstIndex].blue * 0.5);
	
	printf("정상실행");
	/*
	inputArray_A[dstIndex] = dstIndex + 1;
	inputArray_B[dstIndex] = (dstIndex + 1) * 2 + 2;
	outputArray[dstIndex] = inputArray_A[dstIndex]+inputArray_B[dstIndex];
	printf("%d ", inputArray_A[dstIndex]);
	printf("%d ", inputArray_B[dstIndex]);
	printf("%d ", outputArray[dstIndex]);
	*/

}