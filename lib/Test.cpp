#include <cstddef>
#include <cstdio>
#include <iostream>

#include "glibProxy.h"  
#include "engineParams.h"
#include "imageSynth.h"
#include "map.h" 
 

static void dumpBuffer(ImageBuffer* buffer,	unsigned int pixelelsPerPixel)
{
	unsigned int row;
	unsigned int col;
	unsigned int pixelel;

	for (row = 0; row < buffer->height; row++)
	{
		for (col = 0; col < buffer->width; col++)
		{
			for (pixelel = 0; pixelel < pixelelsPerPixel; pixelel++) {
				printf("%02hhx ", buffer->data[row * buffer->rowBytes + col * pixelelsPerPixel + pixelel]);
			}
			printf(" ");			
		}
		printf("\n");
	}
}


/**
 * \brief Dump engine's image pixmap
 */ 
static void dumpImage(unsigned int count, Map targetMap)
{
	unsigned int i;

	for (i = 0; i<count; i++)
	{
		// hh is char, x is hex notation, O is left pad with zeroes, 2 is field width
		printf("%02hhx ", static_cast<char>(targetMap.data->data[i]));
		if (i % 5 == 4)
		{
			printf("\n");	// spaces after every 5 pixelels, ie after a pixel
		}
	}
	printf("\n");
}


/**
 * \brief The progress callback
 */
void progressCallback(int percent, void * context)
{
	printf("Percent %d\n", percent);
}


/**
 * \brief Function to test the resynthesizer
 */
static void test(const char * description,
	ImageBuffer* buffer,
	ImageBuffer* mask,
	TImageFormat format,
	unsigned int pixelelCount,
	const char * expect,
	TImageSynthParameters* parameters)
{
	int error;
	int cancelFlag = 0;

	printf("\n");  
	printf("%s", description); 
	printf("\n");

	printf("Before:\n");
	dumpBuffer(buffer, pixelelCount);

	error = imageSynth(buffer, mask, format, parameters, progressCallback, (void*)0, &cancelFlag);
	
	if (error) 
	{
		printf("Error: ImageSynth returned error: %d\n", error);
	}

	printf("Expected:\n"); 
	printf("%s", expect); 
	printf("\n");

	printf("Result:\n");
	dumpBuffer(buffer, pixelelCount);
}


// Test harness, small images
// !!! Here Alpha FF is total opacity.  Alpha 0 is total transparency.
int main()
{
	// A 3x3 image where the center pixel will be synthesized
	unsigned char image[42] = {
		0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,	// 3*RGBA and 2 trailing pad byte = 14
		0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,
		0,0,0,0, 0,0,0,0, 0,0,0,8, 0,0
	};

	unsigned char mask[12] = {
		0,0,		0, 0, 	// 3*mask and trailing pad byte = 4
		0,0xFF,	0, 0,	// 0xFF == totally selected
		0,0,		0, 0
	};

	// A 1x3 image where the middle pixel will be synthesized.
	// First pixel opaque
	// Third pixel transparent
	unsigned char image2[14] = {
		128,128,128,0xFF, 1,1,1,1, 0,0,0,0, 0,0	// 3*RGBA and 2 trailing pad byte = 14
	};

	unsigned char mask2[4] = {
		0,0xFF,0, 0 	// 3*mask and 1 trailing pad byte = 4
	};

	unsigned char mask3[8] = {
		0,   0,0, 0, 	// 3*mask and 1 trailing pad byte = 4
		0,0xFF,0, 0
	};

	unsigned char maskEmpty[4] = {
		0,0,0, 0 	// 3*mask and 1 trailing pad byte = 4
	};
	
	// A 1x3 image where the all pixel transparent and the middle pixel will be synthesized.
	unsigned char image3[14] = {
		128,128,128,0, 1,1,1,1, 0,0,0,0, 0,0	// 3*RGBA and 2 trailing pad byte
	};

	// A 2x3 RGB image 
	unsigned char imageRGB[22] = {
		128,128,128, 1,1,1, 2,2,2, 5,5,	// 3*RGB and 2 trailing pad byte = 11
		64,64,64, 4,4,4, 3,3,3, 7,7
	};

	// 1x3 Grey alpha
	// First pixel opaque, second to be synthesized, third transparent
	unsigned char imageGrayA[8] = {
		128,0xFF, 64,1, 1,0, 0,0	// 3*GA and 2 trailing pad byte = 8
	};

	// 1x3 Grey 
	unsigned char imageGray[5] = {
		128, 64, 1, 0,0	// 3*GA and 2 trailing pad byte = 8
	};

	// !!! Note width, height, rowBytes in that order
	ImageBuffer testImage = { (unsigned char*)&image, 3, 3, 14 };
	ImageBuffer testImage2 = { (unsigned char*)&image2, 3, 1, 14 };
	ImageBuffer testImage3 = { (unsigned char*)&image3, 3, 1, 14 };
	ImageBuffer testImageRGB = { (unsigned char*)&imageRGB, 3, 2, 11 };
	ImageBuffer testImageGrayA = { (unsigned char*)&imageGrayA, 3, 1, 8 };
	ImageBuffer testImageGray = { (unsigned char*)&imageGray, 3, 1, 5 };

	ImageBuffer testMask = { (unsigned char*)&mask, 3, 3, 4 };
	ImageBuffer testMask2 = { (unsigned char*)&mask2, 3, 1, 4 };
	ImageBuffer testMask3 = { (unsigned char*)&mask3, 3, 2, 4 };
	ImageBuffer testMaskBad = { (unsigned char*)&mask3, 1, 1, 1 };
	ImageBuffer testMaskEmpty = { (unsigned char*)&maskEmpty, 3, 1, 4 };

	TImageSynthParameters parameters;
	setDefaultParams(&parameters);

	{
		// !!!! Set this to 1 to test cancellation !!!!
		int cancelFlag = 0;

		printf("\nTest center pixel synthesized but alpha unchanged (OR test cancellation).\n");
		printf("Before\n");
		dumpBuffer(&testImage, 4);
		imageSynth(&testImage, &testMask, T_RGBA, &parameters, progressCallback, (void*)0, &cancelFlag);
	}

	printf("After\n");
	dumpBuffer(&testImage, 4);

	/*
	(1,1) should be changed to 0,0,0,1
	i.e. the alpha byte 1 should not be changed
	but the color should be 0,0,0 from unmasked and partially opaque region.
	Other pixels are not changed.
	*/

	// Note these tests destroy the input image, which can't be used twice

	test("Test mix of full transparency and opaque", &testImage2, &testMask2, T_RGBA, 4,
		"80 80 80 ff  80 80 80 01  00 00 00 00", &parameters);

	test("Test RGB w/o alpha", &testImageRGB, &testMask3, T_RGB, 3,
		"80 80 80  01 01 01  02 02 02\n40 40 40  01 01 01  03 03 03", &parameters);

	test("Test Gray w/ alpha", &testImageGrayA, &testMask2, T_GrayA, 2,
		"80 ff  80 01  01 00", &parameters);

	// Also tests passing NULL parameters for defaults
	test("Test Gray w/o alpha", &testImageGray, &testMask2, T_Gray, 1,
		"80  01  01", (TImageSynthParameters*)NULL);

    std::cout << std::endl << __FUNCTION__ << ": DONE. Press any key to exit..." << std::endl;
    std::cin.get();

	return(0);
}

