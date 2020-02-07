#pragma once

#include "globals.h"
#include "heap.h"
#include "runtime.h"
#include "visitor.h"

namespace py {

class Scavenger;

class ScavengeVisitor : public PointerVisitor {
 public:
  explicit ScavengeVisitor(Scavenger* scavenger) : scavenger_(scavenger) {}

  void visitPointer(RawObject* pointer, PointerKind kind) override;

 private:
  Scavenger* scavenger_;

  DISALLOW_COPY_AND_ASSIGN(ScavengeVisitor);
};

class Scavenger {
 public:
  explicit Scavenger(Runtime* runtime);
  ~Scavenger();

  ScavengeVisitor* visitor() { return &visitor_; }

  RawObject scavenge();

  void scavengePointer(RawObject* pointer);

 private:
  RawObject transport(RawObject old_object);

  bool hasWhiteReferent(RawObject reference);

  void processDelayedReferences();

  void processFinalizableReferences();

  void processNativeList(ListEntry* entry);

  void processGrayObjects();

  void processApiHandles();

  void processRoots();

  void processLayouts();

  ScavengeVisitor visitor_;
  Runtime* runtime_;
  Space* from_;
  Space* to_;
  uword scan_;
  RawMutableTuple layouts_;
  RawObject delayed_references_;
  RawObject delayed_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(Scavenger);
};

}  // namespace py
