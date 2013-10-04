/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/Request.h>

#include <new>

#include <package/Context.h>
#include <package/JobQueue.h>


namespace BPackageKit {


using BPrivate::JobQueue;


BRequest::BRequest(const BContext& context)
	:
	fContext(context),
	fJobQueue(new (std::nothrow) JobQueue())
{
	fInitStatus = fJobQueue == NULL ? B_NO_MEMORY : B_OK;
}


BRequest::~BRequest()
{
}


status_t
BRequest::InitCheck() const
{
	return fInitStatus;
}


BJob*
BRequest::PopRunnableJob()
{
	if (fJobQueue == NULL)
		return NULL;

	return fJobQueue->Pop();
}


status_t
BRequest::Process(bool failIfCanceledOnly)
{
	status_t error = InitCheck();
	if (error != B_OK)
		return error;

	error = CreateInitialJobs();
	if (error != B_OK)
		return error;

	while (BJob* job = PopRunnableJob()) {
		error = job->Run();
		delete job;
		if (error != B_OK) {
			if (!failIfCanceledOnly || error == B_CANCELED)
				return error;
		}
	}

	return B_OK;
}


status_t
BRequest::QueueJob(BJob* job)
{
	if (fJobQueue == NULL)
		return B_NO_INIT;

	job->AddStateListener(this);
	job->AddStateListener(&fContext.JobStateListener());

	return fJobQueue->AddJob(job);
}


}	// namespace BPackageKit
