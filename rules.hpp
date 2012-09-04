#pragma once
class LabelAllocator {
  uint32_t m_allocated;
public:
  LabelAllocator () : m_allocated (0) {}
  uint32_t Allocate () {
    return ++m_allocated;
  }
};

struct Rule {
    uint32_t label;
    enum Type {
        Push,
        Pop,
        Output,
        Resubmit
    };
    Type type;
    uint32_t data;
};
