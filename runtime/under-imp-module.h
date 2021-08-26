/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#pragma once

namespace py {

class Thread;

void importAcquireLock(Thread* thread);
bool importReleaseLock(Thread* thread);

}  // namespace py
