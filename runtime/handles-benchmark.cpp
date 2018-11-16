#include "benchmark/benchmark.h"

#include "handles.h"
#include "objects.h"
#include "visitor.h"

namespace python {

static void BM_HandleCreationDestruction(benchmark::State& state) {
  Handles handles;
  HandleScope scope(&handles);

  auto o1 = reinterpret_cast<Object*>(0xFEEDFACE);

  for (auto _ : state) {
    Handle<Object> h1(&scope, o1);
  }
}
BENCHMARK(BM_HandleCreationDestruction);

class NothingVisitor : public PointerVisitor {
 public:
  virtual void visitPointer(Object**) {
    visit_count++;
  }

  int visit_count = 0;
};

static void BM_Visit(benchmark::State& state) {
  Handles handles;
  HandleScope scope(&handles);

  Handle<Object> h1(&scope, reinterpret_cast<Object*>(0xFEEDFACE));
  Handle<Object> h2(&scope, reinterpret_cast<Object*>(0xFEEDFACF));
  Handle<Object> h3(&scope, reinterpret_cast<Object*>(0xFEEDFAD0));
  Handle<Object> h4(&scope, reinterpret_cast<Object*>(0xFEEDFAD1));
  Handle<Object> h5(&scope, reinterpret_cast<Object*>(0xFEEDFAD2));
  Handle<Object> h6(&scope, reinterpret_cast<Object*>(0xFEEDFAD3));
  Handle<Object> h7(&scope, reinterpret_cast<Object*>(0xFEEDFAD4));
  Handle<Object> h8(&scope, reinterpret_cast<Object*>(0xFEEDFAD5));
  Handle<Object> h9(&scope, reinterpret_cast<Object*>(0xFEEDFAD6));

  NothingVisitor visitor;
  for (auto _ : state) {
    handles.visitPointers(&visitor);
  }
}
BENCHMARK(BM_Visit);

} // namespace python
