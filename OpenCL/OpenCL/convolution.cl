
// TODO: Add OpenCL kernel code here.
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

__kernel
void simpleKernel(__global FILE* inputArray_A,
	__global FILE* inputArray_B,
	__global COLORREF* outputArray
)
{	
	int bg_width_A;
	int bg_height_A;
	/*
	int bg_width_B;
	int bg_height_B;
	*/

	uint dstYStride = get_global_size(0);
	uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
	uint globalRow = get_global_id(1);
	uint globalCol = get_global_id(0);

	fread(&bfh, sizeof(bfh), 1, inputArray_A);
	fread(&bih, sizeof(bih), 1, inputArray_A);

	bg_width_A = bih.biwidth;
	bg_height_A = bih.biheight;

	/*
	fread(&bfh, sizeof(bfh), 1, inputArray_B);
	fread(&bih, sizeof(bih), 1, inputArray_B);

	bg_width_B = bih.biwidth;
	bg_height_B = bih.biheight;
	*/

	int index = 0;
	for (int i = 0; i<bg_height_A; i++)
	{
		for (int j = 0; j<bg_width_A; j++)
		{ 
			fread(&rgb1, 4, 1, inputArray_A);
			fread(&rgb2, 4, 1, inputArray_B);
			// 배경화면의 RGB 정보를 좌표에 따라 저장.
			outputArray[index++] = RGB((rgb1.red * (0.5)) + (rgb2.red * (0.5)) , (rgb1.green * (0.5)) + (rgb2.green * (0.5)), (rgb1.blue * (0.5)) + (rgb2.blue * (0.5)));
		}
	}

	fclose(inputArray_A);
	fclose(inputArray_B)



	/*
	inputArray_A[dstIndex] = dstIndex + 1;
	inputArray_B[dstIndex] = (dstIndex + 1) * 2 + 2;
	outputArray[dstIndex] = inputArray_A[dstIndex]+inputArray_B[dstIndex];
	printf("%d ", inputArray_A[dstIndex]);
	printf("%d ", inputArray_B[dstIndex]);
	printf("%d ", outputArray[dstIndex]);
	*/


}