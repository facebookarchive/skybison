#pragma once

#include "globals.h"
#include "heap.h"
#include "runtime.h"
#include "visitor.h"

namespace python {

class Scavenger;

class ScavengeVisitor : public PointerVisitor {
 public:
  explicit ScavengeVisitor(Scavenger* scavenger) : scavenger_(scavenger) {}

  void visitPointer(Object** pointer) override;

 private:
  Scavenger* scavenger_;

  DISALLOW_COPY_AND_ASSIGN(ScavengeVisitor);
};

class Scavenger {
 public:
  explicit Scavenger(Runtime* runtime);
  ~Scavenger();

  ScavengeVisitor* visitor() { return &visitor_; }

  Object* scavenge();

  void scavengePointer(Object** pointer);

 private:
  Object* transport(Object* old_object);

  bool hasWhiteReferent(Object* reference);

  void processDelayedReferences();

  void processGrayObjects();

  void processRoots();

  ScavengeVisitor visitor_;
  Runtime* runtime_;
  Space* from_;
  Space* to_;
  Object* delayed_references_;
  Object* delayed_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(Scavenger);
};

}  // namespace python
