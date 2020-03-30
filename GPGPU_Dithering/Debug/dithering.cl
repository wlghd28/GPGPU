// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable


__kernel
void simpleKernel(
	__global unsigned char* pix,
	__global int* pixE,
	int bpl
)
{	

	uint dstYStride = get_global_size(0);
	uint dstXStride = get_global_size(1);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);
	uint dstIndex = globalRow * dstYStride + globalCol * dstXStride;

	//int dev = (bpl / 4) * (bpl / 4);
	//int num = 1;
	pix[dstIndex] = 5;

	/*
	if((globalRow % num == 0) && (globalCol % num == 0)){ 
		int tmpindex;
		int quant_err;
		for(int y = 0; y < num; y ++){ 
			for(int x = 0; x < num; x++){		
				tmpindex = dstIndex + bpl * y + x; 
				// 병렬처리가 가능한 부분.
				pixE[tmpindex] += pix[tmpindex];
				pix[tmpindex] = pixE[tmpindex] / 128 * 255;

				quant_err = pixE[tmpindex] - pix[tmpindex];

				pixE[tmpindex + 1] += quant_err * 7 / 16;
				pixE[tmpindex + bpl - 1] += quant_err * 3 / 16;
				pixE[tmpindex + bpl] += quant_err * 5 / 16;
				pixE[tmpindex + bpl + 1] += quant_err * 1 / 16;
			}
		}
	}
	*/
}