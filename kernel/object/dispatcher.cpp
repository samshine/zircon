// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <object/dispatcher.h>

//#include <arch/ops.h>
#if WITH_LIB_KTRACE
#include <lib/ktrace.h>
#endif
#include <fbl/atomic.h>
#include <object/state_tracker.h>

namespace {
// The first 1K koids are reserved.
fbl::atomic<zx_koid_t> global_koid(1024ULL);

zx_koid_t GenerateKernelObjectId() {
    return global_koid.fetch_add(1ULL);
}

}  // namespace

Dispatcher::Dispatcher()
    : koid_(GenerateKernelObjectId()),
      handle_count_(0u) {
}

Dispatcher::~Dispatcher() {
#if WITH_LIB_KTRACE
    ktrace(TAG_OBJECT_DELETE, (uint32_t)koid_, 0, 0, 0);
#endif
}

zx_status_t Dispatcher::add_observer(StateObserver* observer) {
    auto state_tracker = get_state_tracker();
    if (!state_tracker)
        return ZX_ERR_NOT_SUPPORTED;
    state_tracker->AddObserver(observer, nullptr);
    return ZX_OK;
}

zx_status_t Dispatcher::user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer) {
    if (peer)
        return ZX_ERR_NOT_SUPPORTED;

    auto state_tracker = get_state_tracker();
    if (!state_tracker)
        return ZX_ERR_NOT_SUPPORTED;

    // Generic objects can set all USER_SIGNALs. Particular object
    // types (events and eventpairs) may be able to set more.
    if ((set_mask & ~ZX_USER_SIGNAL_ALL) || (clear_mask & ~ZX_USER_SIGNAL_ALL))
        return ZX_ERR_INVALID_ARGS;

    state_tracker->UpdateState(clear_mask, set_mask);
    return ZX_OK;
}
