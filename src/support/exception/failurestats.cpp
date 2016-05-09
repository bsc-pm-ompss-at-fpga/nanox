
#include "atomic.hpp"
#include "failurestats.hpp"

using namespace nanos;
using namespace nanos::error;

template<>
Atomic<unsigned> FailureStats<CheckpointFailure>::_counter = 0;

template<>
Atomic<unsigned> FailureStats<RestoreFailure>::_counter    = 0;

template<>
Atomic<unsigned> FailureStats<ExecutionFailure>::_counter  = 0;

template<>
Atomic<unsigned> FailureStats<ErrorInjection>::_counter    = 0;

template<>
Atomic<unsigned> FailureStats<DiscardedTask>::_counter     = 0;

template<>
Atomic<unsigned> FailureStats<TaskRecovery>::_counter      = 0;

