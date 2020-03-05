
// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

__kernel
void simpleKernel(__global int* inputArray_A,
	__global int* inputArray_B,
	__global int* outputArray
)
{

	uint dstYStride = get_global_size(0);
	uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);

	outputArray[dstIndex] = inputArray_A[dstIndex]+inputArray_B[dstIndex];

}