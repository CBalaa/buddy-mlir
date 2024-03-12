#ifndef FEGEN_FEGENVISITOR_H
#define FEGEN_FEGENVISITOR_H

#include "FegenParserBaseVisitor.h"
#include "FegenManager.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <map>
#include <string>

using namespace antlr4;

namespace fegen{


class FegenVisitor : public FegenParserBaseVisitor {
private:
  FegenManager manager;
public:
  FegenVisitor() {}
  
};
}
#endif