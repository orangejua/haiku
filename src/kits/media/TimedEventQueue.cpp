/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: TimedEventQueue.cpp
 *  DESCR: used by BMediaEventLooper
 ***********************************************************************/

#include <TimedEventQueue.h>

#include <string.h>

#include "debug.h"
#include "TimedEventQueuePrivate.h"


// #pragma mark - struct media_timed_event


media_timed_event::media_timed_event()
{
	CALLED();
	memset(this, 0, sizeof(*this));
}


media_timed_event::media_timed_event(bigtime_t inTime, int32 inType)
{
	CALLED();
	memset(this, 0, sizeof(*this));
	event_time = inTime;
	type = inType;
}


media_timed_event::media_timed_event(bigtime_t inTime, int32 inType,
	void* inPointer, uint32 inCleanup)
{
	CALLED();
	memset(this, 0, sizeof(*this));
	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
}


media_timed_event::media_timed_event(bigtime_t inTime, int32 inType,
	void* inPointer, uint32 inCleanup, int32 inData, int64 inBigdata,
	char* inUserData, size_t dataSize)
{
	CALLED();
	memset(this, 0, sizeof(*this));
	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
	data = inData;
	bigdata = inBigdata;
	memcpy(user_data, inUserData,
		min_c(sizeof(media_timed_event::user_data), dataSize));
}


media_timed_event::media_timed_event(const media_timed_event& clone)
{
	CALLED();
	*this = clone;
}


void
media_timed_event::operator=(const media_timed_event& clone)
{
	CALLED();
	memcpy(this, &clone, sizeof(*this));
}


media_timed_event::~media_timed_event()
{
	CALLED();
}


// #pragma mark - global operators


bool
operator==(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return (memcmp(&a, &b, sizeof(media_timed_event)) == 0);
}


bool
operator!=(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return (memcmp(&a, &b, sizeof(media_timed_event)) != 0);
}


bool
operator<(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return a.event_time < b.event_time;
}


bool
operator>(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return a.event_time > b.event_time;
}


// #pragma mark - public BTimedEventQueue


void*
BTimedEventQueue::operator new(size_t size)
{
	CALLED();
	return ::operator new(size);
}


void
BTimedEventQueue::operator delete(void* ptr, size_t size)
{
	CALLED();
	return ::operator delete(ptr);
}


BTimedEventQueue::BTimedEventQueue()
	:
	fImplementation(new _event_queue_imp)
{
	CALLED();
}


BTimedEventQueue::~BTimedEventQueue()
{
	CALLED();
	delete fImplementation;
}


status_t
BTimedEventQueue::AddEvent(const media_timed_event& event)
{
	CALLED();
	return fImplementation->AddEvent(event);
}


status_t
BTimedEventQueue::RemoveEvent(const media_timed_event* event)
{
	CALLED();
	return fImplementation->RemoveEvent(event);
}


status_t
BTimedEventQueue::RemoveFirstEvent(media_timed_event* outEvent)
{
	CALLED();
	return fImplementation->RemoveFirstEvent(outEvent);
}


bool
BTimedEventQueue::HasEvents() const
{
	CALLED();
	return fImplementation->HasEvents();
}


int32
BTimedEventQueue::EventCount() const
{
	CALLED();
	return fImplementation->EventCount();
}


const media_timed_event*
BTimedEventQueue::FirstEvent() const
{
	CALLED();
	return fImplementation->FirstEvent();
}


bigtime_t
BTimedEventQueue::FirstEventTime() const
{
	CALLED();
	return fImplementation->FirstEventTime();
}


const media_timed_event*
BTimedEventQueue::LastEvent() const
{
	CALLED();
	return fImplementation->LastEvent();
}


bigtime_t
BTimedEventQueue::LastEventTime() const
{
	CALLED();
	return fImplementation->LastEventTime();
}


const media_timed_event*
BTimedEventQueue::FindFirstMatch(bigtime_t eventTime, time_direction direction,
	bool inclusive, int32 eventType)
{
	CALLED();
	return fImplementation->FindFirstMatch(eventTime, direction, inclusive,
		eventType);
}


status_t
BTimedEventQueue::DoForEach(for_each_hook hook, void *context,
	bigtime_t eventTime, time_direction direction, bool inclusive,
	int32 eventType)
{
	CALLED();
	return fImplementation->DoForEach(hook, context, eventTime, direction,
		inclusive, eventType);
}


void
BTimedEventQueue::SetCleanupHook(cleanup_hook hook, void* context)
{
	CALLED();
	fImplementation->SetCleanupHook(hook, context);
}


status_t
BTimedEventQueue::FlushEvents(bigtime_t eventTime, time_direction direction,
	bool inclusive, int32 eventType)
{
	CALLED();
	return fImplementation->FlushEvents(eventTime, direction, inclusive,
		eventType);
}


// #pragma mark - private BTimedEventQueue


/*
// unimplemented
BTimedEventQueue::BTimedEventQueue(const BTimedEventQueue &other)
BTimedEventQueue &BTimedEventQueue::operator=(const BTimedEventQueue &other)
*/


status_t BTimedEventQueue::_Reserved_BTimedEventQueue_0(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_1(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_2(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_3(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_4(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_5(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_6(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_7(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_8(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_9(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_10(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_11(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_12(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_13(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_14(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_15(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_16(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_17(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_18(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_19(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_20(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_21(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_22(void*, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_23(void*, ...) { return B_ERROR; }
