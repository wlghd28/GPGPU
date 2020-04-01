// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable


__kernel
void simpleKernel(
	__global unsigned char* pix,
	__global int* pixE,
	int bpl,
	int div_bpl,
	int div_bph
)
{	
	uint dstYStride = get_global_size(0);
	uint dstXStride = get_global_size(1);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);
	uint dstIndex = globalRow * dstYStride + globalCol;

	int quant_err;
	int tmpindex;
	//unsigned char tmppix;

	// globalCol / div_bpl != div_num 이 조건이 핵심
	if(globalCol / div_bpl != bpl / div_bpl && globalRow % div_bph == 0 && globalCol % div_bpl == 0)
	{ 
		for(int y = 0; y < div_bph; y++)
		{ 
			for(int x = 0; x < div_bpl; x++)
			{		
				tmpindex =  dstIndex + bpl * y + x; 
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
}