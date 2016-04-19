/**
 * Proxy for /usr/include/glib-2.0
 * This proxy obviates re-coding to eliminate existing glib use.
 *
 * Glib is for portability, on top of ANSI C.
 * This is an alternative.
 * This is without GPL license.
 * This is not as robust as Glib: little checking is done.
 *
 * This is a limited subset: only what is used in imageSynth.
 */
#include "buildSwitches.h"

 // Certain configurations use glib defines of structs GRand and GArray
#ifdef SYNTH_USE_GLIB
#include <libgimp/gimp.h>
#endif


#include <stdlib.h>   // size_t, calloc
#include <string.h>   // memcpy
#include "glibProxy.h"

/************************************************************************/
/* Pseudo Random Number Generator                                       */
/************************************************************************/
GRand * s_rand_new_with_seed(guint seed)
{
	srand(seed);
	return (void*)NULL;  // not used
}


guint s_rand_int_range(GRand * prng, guint lowerBound, guint upperBound)
{
	// Prevent division by zero when both bounds 0
	if (upperBound < 1) return 0;

	// Make it inclusive
	upperBound -= 1;

	// Conventional formula for random int in range [lowerBound, upperBound] inclusive
	return lowerBound + rand() / (RAND_MAX / (upperBound - lowerBound + 1) + 1);
}




/************************************************************************/
/* GArray Definitions                                                   */
/************************************************************************/
GArray* GArrayAllocate(gboolean zero_terminated,
	gboolean clear,
	guint type_size,
	guint reserved_size)
{
	GRealArray *array = reinterpret_cast<GRealArray*>(calloc(1, sizeof(GRealArray)));
	array->data = reinterpret_cast<unsigned char*>(calloc(reserved_size, type_size));
	array->len = 0;
	array->capacity = reserved_size;
	array->type_size = type_size;

	return (GArray*)array;
}


void GArrayFree(GArray * array, int cascade)
{
	// Ignore cascade: always free both
	assert(array->data);
	assert(array);
	free(array->data);
	free(array);  // free GRealArray
	array = NULL;
}


GArray* GArrayAppendVal(GArray *array, ConstVoidPtr data, int len)
{
	GRealArray *realArr = (GRealArray*)array;
	guint originalLength = realArr->len;

	if (realArr->len == realArr->capacity)
	{
		guint newCapacity = realArr->capacity * 2;
		GRealArray *newArray = reinterpret_cast<GRealArray*>(calloc(1, sizeof(GRealArray)));
		newArray->data = reinterpret_cast<unsigned char*>(calloc(newCapacity, realArr->type_size));
		newArray->len = realArr->len;
		newArray->capacity = newCapacity;
		newArray->type_size = realArr->type_size;

		memcpy(newArray->data, realArr->data, realArr->type_size * realArr->len);
		free(realArr->data);
		free(realArr);
		realArr = newArray;
		array = (GArray*)realArr;
	}

	memcpy((array->data + (realArr->type_size * originalLength)), data, realArr->type_size);
	array->len += 1;

	return array;
}

void GArraySort(GArray * array, CompareFunc compare_func)
{
	GRealArray *rarray = (GRealArray*)array;
	qsort(array->data, array->len, rarray->type_size, compare_func);
}
