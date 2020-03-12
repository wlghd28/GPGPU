
// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

__kernel
void simpleKernel(
	__global unsigned char* red_A,
	__global unsigned char* green_A,
	__global unsigned char* blue_A,
	__global unsigned char* red_B,
	__global unsigned char* green_B,
	__global unsigned char* blue_B,
	__global unsigned char* red_output,
	__global unsigned char* green_output,
	__global unsigned char* blue_output
)
{	

	uint dstYStride = get_global_size(0);
	uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);


	red_output[dstIndex] = (red_A[dstIndex] * 0.5) + (red_B[dstIndex] * 0.5);
	green_output[dstIndex] = (green_A[dstIndex] * 0.5) + (green_B[dstIndex] * 0.5);
	blue_output[dstIndex] = (blue_A[dstIndex] * 0.5) + (blue_B[dstIndex] * 0.5);

}