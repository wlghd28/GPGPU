// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable


__kernel
void simpleKernel(
	__global int* err,
	__global int quant_err,
	__global int* di_num
)
{	

	uint dstYStride = get_global_size(0);
	uint dstXStride = get_global_size(1);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);
	uint dstIndex = globalRow * dstYStride + globalCol * dstXStride;

	err[dstIndex] += quant_err * di_num[dstIndex] / 16;

}