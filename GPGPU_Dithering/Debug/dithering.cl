// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable


__kernel
void simpleKernel(
	__global unsigned char* pix,
	__global int* pixE_1,
	__global int* pixE_2,
	__global int* pixE_3,
	__global int* pixE_4
)
{	

	uint dstYStride = get_global_size(0);
	uint dstXStride = get_global_size(1);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);
	uint dstIndex = globalRow * dstYStride + globalCol * dstXStride;

	//err[dstIndex] += quant_err * di_num[dstIndex] / 16;

	int quant_err = 0;
	unsigned char oldpixel;

	oldpixel = pix[dstIndex];
	// Thresh Holder
	pix[dstIndex] = oldpixel / 128 * 255;

	quant_err = pix[dstIndex] - oldpixel;

	pixE_1[dstIndex] = quant_err * 7 / 16;
	pixE_2[dstIndex] = quant_err * 3 / 16;
	pixE_3[dstIndex] = quant_err * 5 / 16;
	pixE_4[dstIndex] = quant_err * 1 / 16;

}