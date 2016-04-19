/*
 Engine does repeated passes with a closed feedback loop to quit early if no improvement is made.

 First pass: target is empty and patches are shotgun patterns, i.e. sparse, mostly from outside the target.

 Second pass: target is synthesized but poorly.  Use patches from the poor target to refine the target.
 Patches are non-sparse i.e. contiguous, but not necessarily square or symmetric.
 Second pass refines every pixel in the target.

 Third and further passes refine a subset of the target.
 It is debatable whether third and subsequent passes should continue to refine every pixel in the target.

 Note the original made passes of increasing size,
 resynthesizing early synthesized pixels early.

  Copyright (C) 2010, 2011  Lloyd Konneker

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 Threaded version, using alternative 1.

 Alternative 1:
 Each pass divides targetPoints among threads and rejoins before the next pass.
 Here, one thread may be reading pixels that another thread is synthesizing,
 but no two threads are synthesizing the same pixel.

 Alternative 2:
 one thread is started for each pass, with each thread working on a prefix of the same targetPoints.
 This is more like Harrison's implementation,
 where early synthesized pixels are repeated sooner rather than after all pixels have been synthesized once.
 There would be more contention, and write contention, over target pixels.
 Probably each thread would need to lag already running threads.
 Each thread would not have the same amount of work.

 Sept. 2011.  One experiment shows it is no faster, and produces different, grainy results.
*/
#pragma once
#ifndef REFINER_THREAD_H_
#define REFINER_THREAD_H_

#include <thread>
#include <mutex>


// When synthesize() is threaded, it needs a single argument.
// Wrapper struct for single arg to synthesize
typedef struct synthArgsStruct {
	TImageSynthParameters *parameters;		// IN
	guint threadIndex;
	guint startTargetIndex;
	guint endTargetIndex;					// IN // array pointers
	TFormatIndices* indices;				// IN
	Map * targetMap;						// IN/OUT
	Map* corpusMap;							// IN
	Map* recentProberMap;					// IN/OUT
	Map* hasValueMap;						// IN/OUT
	Map* sourceOfMap;						// IN/OUT
	PointVector targetPoints;				// IN
	PointVector corpusPoints;				// IN
	PointVector sortedOffsets;				// IN
	GRand *prng;
	gushort * corpusTargetMetric;			// array pointers TPixelelMetricFunc
	guint * mapsMetric;						// TMapPixelelMetricFunc
	std::function<void()> deepProgressCallback;         // void func(void)
	int* cancelFlag;  // flag set when canceled
} SynthArgs;


// Pack args into wrapper struct to pass to thread func
static void
newSynthesisArgs(
	SynthArgs* args,
	TImageSynthParameters *parameters,  // IN
	guint threadIndex,
	guint startTargetIndex,
	guint endTargetIndex,  // IN
	TFormatIndices* indices,  // IN
	Map * targetMap,      // IN/OUT
	Map* corpusMap,       // IN
	Map* recentProberMap, // IN/OUT
	Map* hasValueMap,     // IN/OUT
	Map* sourceOfMap,     // IN/OUT
	PointVector targetPoints, // IN
	PointVector corpusPoints, // IN
	PointVector sortedOffsets, // IN
	GRand *prng,
	TPixelelMetricFunc corpusTargetMetric,  // array pointers
	TMapPixelelMetricFunc mapsMetric,
	void(*deepProgressCallback)(),
	int* cancelFlag)
{
	args->parameters = parameters;
	args->threadIndex = threadIndex;
	args->startTargetIndex = startTargetIndex;
	args->endTargetIndex = endTargetIndex;
	args->indices = indices;
	args->targetMap = targetMap;
	args->corpusMap = corpusMap;
	args->recentProberMap = recentProberMap;
	args->hasValueMap = hasValueMap;
	args->sourceOfMap = sourceOfMap;
	args->targetPoints = targetPoints;
	args->corpusPoints = corpusPoints;
	args->sortedOffsets = sortedOffsets;
	args->prng = prng;
	args->corpusTargetMetric = corpusTargetMetric;
	args->mapsMetric = mapsMetric;
	args->deepProgressCallback = deepProgressCallback;
	args->cancelFlag = cancelFlag;
}


static void * synthesisThread(void * uncastArgs)
{
	SynthArgs* args = reinterpret_cast<SynthArgs*>(uncastArgs);

	// Unpack wrapped args
	TImageSynthParameters * parameters = args->parameters;
	guint threadIndex = args->threadIndex;
	guint startTargetIndex = args->startTargetIndex;
	guint endTargetIndex = args->endTargetIndex;
	TFormatIndices* indices = args->indices;
	Map * targetMap = args->targetMap;
	Map* corpusMap = args->corpusMap;
	Map* recentProberMap = args->recentProberMap;
	Map* hasValueMap = args->hasValueMap;
	Map* sourceOfMap = args->sourceOfMap;
	PointVector targetPoints = args->targetPoints;
	PointVector corpusPoints = args->corpusPoints;
	PointVector sortedOffsets = args->sortedOffsets;
	GRand *prng = args->prng;
	gushort * corpusTargetMetric = args->corpusTargetMetric; // array pointers TPixelelMetricFunc
	guint * mapsMetric = args->mapsMetric;
	std::function<void()> deepProgressCallback = args->deepProgressCallback;
	int* cancelFlag = args->cancelFlag;

	gulong betters = synthesize(  // gulong so can be cast to void *
		parameters,
		threadIndex,
		startTargetIndex,
		endTargetIndex,
		indices,
		targetMap,
		corpusMap,
		recentProberMap,
		hasValueMap,
		sourceOfMap,
		targetPoints,
		corpusPoints,
		sortedOffsets,
		prng,
		corpusTargetMetric,
		mapsMetric,
		deepProgressCallback,
		cancelFlag
		);
	return (void*)betters;
}


static void startThread(
	SynthArgs* args,
	std::thread* theThread,
	guint threadIndex,
	guint start,
	guint end,
	TImageSynthParameters* parameters,
	TFormatIndices* indices,
	Map* targetMap,
	Map* corpusMap,
	Map* recentProberMap,
	Map* hasValueMap,
	Map* sourceOfMap,
	PointVector targetPoints,
	PointVector corpusPoints,
	PointVector sortedOffsets,
	GRand *prng,
	TPixelelMetricFunc corpusTargetMetric,  // array pointers
	TMapPixelelMetricFunc mapsMetric,
	void(*deepProgressCallback)(),
	int* cancelFlag)
{
	newSynthesisArgs(
		args,
		parameters,
		threadIndex, // thread specific
		start,      // thread specific
		end,        // thread specific
		indices,
		targetMap,
		corpusMap,
		recentProberMap,
		hasValueMap,
		sourceOfMap,
		targetPoints,
		corpusPoints,
		sortedOffsets,
		prng,
		corpusTargetMetric,
		mapsMetric,
		deepProgressCallback,
		cancelFlag);

	//pthread_create(thread, NULL, synthesisThread, (void * __restrict__) args);

}

// Alternative 1

static void
refiner(
	TImageSynthParameters parameters,
	TFormatIndices* indices,
	Map* targetMap,
	Map* corpusMap,
	Map* recentProberMap,
	Map* hasValueMap,
	Map* sourceOfMap,
	PointVector targetPoints,
	PointVector corpusPoints,
	PointVector sortedOffsets,
	GRand *prng,
	TPixelelMetricFunc corpusTargetMetric,  // array pointers
	TMapPixelelMetricFunc mapsMetric,
	void(*progressCallback)(int, void*),
	void *contextInfo,
	int* cancelFlag
	)
{
	guint pass;
	TRepetionParameters repetition_params;


	// Synthesize in threads.  Note proxies in glibProxy.h for POSIX threads
	std::thread procThreads[THREAD_LIMIT];

	// If not using glib proxied to pthread by glibProxy.h
	std::mutex  mutexProgress;
	//g_static_mutex_init(&mutex);  // defined in synthesize.h
	//g_static_mutex_init(&mutexProgress);


	SynthArgs synthArgs[THREAD_LIMIT];


	// For progress
	guint estimatedPixelCountToCompletion;
	// !!! These are global (in the parent) updated by child threads executing callback function deepProgressCallback.
	// The callback must synchronize over them.
	guint completedPixelCount = 0;  // use atomic add
	guint priorReportedPercentComplete = 0; // use mutexProgress

	/*
	Nested function is gcc extension.
	Called from inside synthesize() every 4k target pixels.
	Convert to a percent of estimated total pixels to synthesis.
	Callback invoking process every 1 percent.
	Engine may quit early (not complete all passes): then progress makes a large jump.

	This function has local variables that are threadsafe, but the global variables are in the parent thread
	and must be synchronized.
	*/
	// 	void
	// 		deepProgressCallback()
	// 	{
	// 		guint percentComplete;
	// 
	// 		// Nonsynchronized: completedPixelCount += IMAGE_SYNTH_CALLBACK_COUNT;
	// 		(void)__sync_add_and_fetch(&completedPixelCount, IMAGE_SYNTH_CALLBACK_COUNT);
	// 		percentComplete = ((float)completedPixelCount / estimatedPixelCountToCompletion) * 100;
	// 		if (percentComplete > priorReportedPercentComplete)
	// 		{
	// 			// mutex lock for two reasons:
	// 			// 1) calls to libgmp, gdk, gtk which are thread aware but not thread safe
	// 			// 2) incrementing global variable priorReportedPercentComplete
	// 			// Note threads can still underreport percent complete but it is inconsequential.
	// 			g_static_mutex_lock(&mutexProgress);
	// 			// Alternatively, use gdk_thread_enter()
	// 			progressCallback((int)percentComplete, contextInfo);  // Forward deep progress callback to calling process
	// 			priorReportedPercentComplete = percentComplete;
	// 			g_static_mutex_unlock(&mutexProgress);
	// 		}
	// 	}

	prepare_repetition_parameters(repetition_params, targetPoints->len);
	estimatedPixelCountToCompletion = estimatePixelsToSynth(repetition_params);

	for (pass = 0; pass < MAX_PASSES; pass++)
	{
		guint endTargetIndex = repetition_params[pass][1];
		gulong betters = 0;

		guint threadIndex;
		for (threadIndex = 0; threadIndex < THREAD_LIMIT; threadIndex++)
		{
			startThread(
				&synthArgs[threadIndex], &procThreads[threadIndex], threadIndex, // thread specific
				0, endTargetIndex,      // Every thread works on a prefix of targetPoints, splits it modulo threadIndex
				&parameters,
				indices,
				targetMap,
				corpusMap,
				recentProberMap,
				hasValueMap,
				sourceOfMap,
				targetPoints,
				corpusPoints,
				sortedOffsets,
				prng,
				corpusTargetMetric, mapsMetric,
				NULL,
				cancelFlag
				);
		}

		// Wait for threads to complete; rejoin them
		for (threadIndex = 0; threadIndex < THREAD_LIMIT; threadIndex++)
		{
			gulong temp = 1;

			//pthread_join(procThreads[threadIndex], (void**)&temp);

			betters += temp;
		}


		// nil unless DEBUG
		print_pass_stats(pass, repetition_params[pass][1], betters);
		// printf("Pass %d betters %ld\n", pass, betters);

		/* Break if a small fraction of target is bettered
		This is a fraction of total target points,
		not the possibly smaller count of target attempts this pass.
		Or break on small integral change: if ( targetPoints_size / integralColorChange < 10 ) {
		*/
		if ((float)betters / targetPoints->len < (IMAGE_SYNTH_TERMINATE_FRACTION))
		{
			// printf("Quitting early after %d passes. Betters %ld\n", pass+1, betters);
			break;
		}

		// Simple progress: percent of passes complete.
		// This is not ideal, a maximum of MAX_PASSES callbacks, typically six.
		// And the later passes are much shorter than earlier passes.
		// progressCallback( (int) ((pass+1.0)/(MAX_PASSES+1)*100), contextInfo);
	}
}


#endif /* REFINER_THREAD_H_ */