// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable


__kernel
void simpleKernel(
	__global unsigned char* red,
	__global unsigned char* green,
	__global unsigned char* blue,
	__global unsigned char* red_output,
	__global unsigned char* green_output,
	__global unsigned char* blue_output,
	int per,
	int size
)
{	

	uint dstYStride = get_global_size(0);
	uint dstXStride = get_global_size(1);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);
	uint dstIndex = globalRow * dstYStride + globalCol * dstXStride;

	red_output[dstIndex] = red[dstIndex] * per / size;
	green_output[dstIndex] = green[dstIndex] * per / size;
	blue_output[dstIndex] = blue[dstIndex] * per / size;

}