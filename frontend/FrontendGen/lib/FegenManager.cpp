#include "FegenManager.h"
#include "Scope.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>

fegen::FegenFunction::FegenFunction(llvm::StringRef name,
                                    std::vector<FegenType *> &&inputTypeList,
                                    FegenType *returnType)
    : name(name), inputTypeList(inputTypeList), returnType(returnType) {}

fegen::FegenFunction *
fegen::FegenFunction::get(llvm::StringRef name,
                          std::vector<FegenType *> inputTypeList,
                          FegenType *returnType) {
  return new fegen::FegenFunction(name, std::move(inputTypeList), returnType);
}

fegen::FegenOperation::FegenOperation(llvm::StringRef dialectName,
                                      llvm::StringRef operationName,
                                      std::vector<FegenValue *> &&arguments,
                                      std::vector<FegenValue *> &&results)
    : dialectName(dialectName), arguments(arguments), results(results) {}

fegen::FegenOperation *fegen::FegenOperation::get(
    llvm::StringRef dialectName, llvm::StringRef operationName,
    std::vector<FegenValue *> arguments, std::vector<FegenValue *> results) {
  return new fegen::FegenOperation(dialectName, operationName,
                                   std::move(arguments), std::move(results));
}

// class FegenType

/// @brief get name of Type Instance by jointsing template name and parameters,
/// for example: Integer + 32 --> Integer<32>
/// @return joint name
std::string jointTypeName(std::string templateName,
                          const std::vector<fegen::FegenValue *> &parameters) {
  if (parameters.empty()) {
    return templateName;
  }
  std::string res = templateName;
  res.append("<");
  size_t count = parameters.size();
  auto firstParamStr = parameters[0]->getContentString();
  res.append(firstParamStr);
  for (size_t i = 1; i <= count - 1; i++) {
    auto paramStr = parameters[i]->getContentString();
    res.append(", ");
    res.append(paramStr);
  }
  res.append(">");
  return res;
}

fegen::FegenType::FegenType(fegen::FegenType::TypeKind kind,
                            std::vector<FegenValue *> parameters,
                            FegenTypeDefination *tyDef, int typeLevel)
    : kind(kind), typeName(jointTypeName(tyDef->getName(), parameters)),
      parameters(std::move(parameters)), typeDefine(tyDef),
      typeLevel((typeLevel)) {}

fegen::FegenType::FegenType(const fegen::FegenType &fty)
    : kind(fty.kind),
      typeName(jointTypeName(fty.typeDefine->getName(), fty.parameters)),
      typeDefine(fty.typeDefine), typeLevel(fty.typeLevel) {
  // deep copy parameters
  for (auto paramPtr : fty.parameters) {
    this->parameters.push_back(new fegen::FegenValue(*paramPtr));
  }
}

fegen::FegenType::FegenType(fegen::FegenType &&fty)
    : kind(fty.kind),
      typeName(jointTypeName(fty.typeDefine->getName(), fty.parameters)),
      parameters(std::move(fty.parameters)), typeDefine(fty.typeDefine),
      typeLevel(fty.typeLevel) {}

fegen::FegenType::TypeKind fegen::FegenType::getTypeKind() {
  return this->kind;
}

void fegen::FegenType::setTypeKind(fegen::FegenType::TypeKind kind) {
  this->kind = kind;
}

std::vector<fegen::FegenValue *> &fegen::FegenType::getParameters() {
  return this->parameters;
}

fegen::FegenValue *fegen::FegenType::getParameters(size_t i) {
  return this->parameters[i];
}

void fegen::FegenType::setParameters(std::vector<fegen::FegenValue *> &params) {
  this->parameters = params;
  // set parameters and level up!
  this->typeLevel++;
}

fegen::FegenTypeDefination *fegen::FegenType::getTypeDefination() {
  return this->typeDefine;
}

void fegen::FegenType::setTypeDefination(fegen::FegenTypeDefination *tyDef) {
  this->typeDefine = tyDef;
}

std::string fegen::FegenType::getTypeName() { return this->typeName; }

int fegen::FegenType::getTypeLevel() { return this->typeLevel; }

fegen::FegenType::~FegenType() {
  for (auto p : this->parameters) {
    delete p;
  }
}

fegen::FegenType fegen::FegenType::getPlaceHolder() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_PLACEHOLDER),
      0);
}

fegen::FegenType fegen::FegenType::getMetaType() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_TYPE), 2);
}

fegen::FegenType fegen::FegenType::getMetaTemplateType() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_TYPETEMPLATE),
      1);
}

fegen::FegenType fegen::FegenType::getInt32Type() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_INTEGER), 3);
}

fegen::FegenType fegen::FegenType::getFloatType() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_FLOATPOINT), 3);
}

fegen::FegenType fegen::FegenType::getDoubleType() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_FLOATPOINT), 3);
}

fegen::FegenType fegen::FegenType::getBoolType() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_INTEGER), 3);
}

fegen::FegenType fegen::FegenType::getIntegerType(fegen::FegenValue *size) {
  if (size->getContent<int>() == 32)
    return fegen::FegenType::getInt32Type();
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {size},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_INTEGER), 3);
}

fegen::FegenType fegen::FegenType::getFloatPointType(fegen::FegenValue *size) {
  if (size->getContent<int>() == 32) {
    return fegen::FegenType::getFloatType();
  } else if (size->getContent<int>() == 64) {
    return fegen::FegenType::getDoubleType();
  }
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {size},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_FLOATPOINT), 3);
}

fegen::FegenType fegen::FegenType::getCharType() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_CHAR), 3);
}

fegen::FegenType fegen::FegenType::getStringType() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_STRING), 3);
}

fegen::FegenType fegen::FegenType::getVectorType(fegen::FegenValue *size,
                                                 fegen::FegenType elementType) {
  assert(elementType.typeLevel == 3);
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP,
      {size,
       fegen::FegenValue::get(fegen::FegenType::getMetaType(), "elementType",
                              fegen::FegenRightValue::get(elementType))},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_VECTOR),
      elementType.typeLevel);
}

fegen::FegenType fegen::FegenType::getTensorType(fegen::FegenValue *shape,
                                                 fegen::FegenType elementType) {
  assert(elementType.typeLevel == 3);
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP,
      {shape,
       fegen::FegenValue::get(fegen::FegenType::getMetaType(), "elementType",
                              fegen::FegenRightValue::get(elementType))},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_TENSOR),
      elementType.typeLevel);
}

// List<elementType>
fegen::FegenType fegen::FegenType::getListType(fegen::FegenType elementType) {
  assert(elementType.typeLevel == 2 || elementType.typeLevel == 3);
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP,
      {fegen::FegenValue::get(
          elementType.typeLevel == 2 ? fegen::FegenType::getMetaTemplateType()
                                     : fegen::FegenType::getMetaType(),
          "elementType", fegen::FegenRightValue::get(elementType))},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_LIST),
      elementType.typeLevel);
}

// Optional<elementType>
fegen::FegenType
fegen::FegenType::getOptionalType(fegen::FegenType elementType) {
  assert(elementType.typeLevel == 2 || elementType.typeLevel == 3);
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP,
      {fegen::FegenValue::get(
          elementType.typeLevel == 2 ? fegen::FegenType::getMetaTemplateType()
                                     : fegen::FegenType::getMetaType(),
          "elementType", fegen::FegenRightValue::get(elementType))},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_OPTINAL),
      elementType.typeLevel);
}

// Any<elementType1, elementType2, ...>
fegen::FegenType
fegen::FegenType::getAnyType(std::vector<fegen::FegenType> elementTypes) {
  std::vector<fegen::FegenValue *> p_elemTy;
  int i = 0;
  std::string name("elementType_");
  auto tyLevel = elementTypes[0].typeLevel;
  assert(tyLevel == 2 || tyLevel == 3);
  auto tyty = tyLevel == 2 ? fegen::FegenType::getMetaTemplateType()
                           : fegen::FegenType::getMetaType();
  for (auto &ty : elementTypes) {
    assert(ty.typeLevel == tyLevel);
    p_elemTy.push_back(fegen::FegenValue::get(tyty, name + std::to_string(i),
                                              fegen::FegenRightValue::get(ty)));
    i++;
  }
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, p_elemTy,
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_ANY), tyLevel);
}

fegen::FegenType fegen::FegenType::getIntegerTemplate() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_INTEGER), 2);
}

fegen::FegenType fegen::FegenType::getFloatPointTemplate() {
  return fegen::FegenType(
      fegen::FegenType::TypeKind::CPP, {},
      fegen::FegenManager::getManager().getTypeDefination(FEGEN_FLOATPOINT), 2);
}

fegen::FegenType
fegen::FegenType::getInstanceType(fegen::FegenTypeDefination *typeDefination,
                                  std::vector<fegen::FegenValue *> parameters) {
  return fegen::FegenType(fegen::FegenType::TypeKind::CPP, parameters,
                          typeDefination, 3);
}
fegen::FegenType
fegen::FegenType::getTemplateType(fegen::FegenTypeDefination *typeDefination) {
  return fegen::FegenType(fegen::FegenType::TypeKind::CPP, {}, typeDefination,
                          2);
}

// class FegenTypeDefination
fegen::FegenTypeDefination::FegenTypeDefination(
    std::string dialectName, std::string name,
    std::vector<fegen::FegenValue *> parameters,
    FegenParser::TypeDefinationDeclContext *ctx, bool ifCustome)
    : dialectName(std::move(dialectName)), name(std::move(name)),
      parameters(std::move(parameters)), ctx(ctx), ifCustome(ifCustome) {}

fegen::FegenTypeDefination *
fegen::FegenTypeDefination::get(std::string dialectName, std::string name,
                                std::vector<fegen::FegenValue *> parameters,
                                FegenParser::TypeDefinationDeclContext *ctx,
                                bool ifCustome) {
  return new fegen::FegenTypeDefination(std::move(dialectName), std::move(name),
                                        std::move(parameters), ctx, ifCustome);
}

std::string fegen::FegenTypeDefination::getDialectName() {
  return this->dialectName;
}

void fegen::FegenTypeDefination::setDialectName(std::string name) {
  this->dialectName = name;
}

std::string fegen::FegenTypeDefination::getName() { return this->name; }

std::string fegen::FegenTypeDefination::getMnemonic() {
  if (this->mnemonic.empty()) {
    this->mnemonic = this->name;
    std::transform(this->mnemonic.begin(), this->mnemonic.end(),
                   this->mnemonic.begin(), ::tolower);
  }
  return this->mnemonic;
}

void fegen::FegenTypeDefination::setName(std::string name) {
  this->name = name;
}

const std::vector<fegen::FegenValue *> &
fegen::FegenTypeDefination::getParameters() {
  return this->parameters;
}

fegen::FegenParser::TypeDefinationDeclContext *
fegen::FegenTypeDefination::getCtx() {
  return this->ctx;
}

void fegen::FegenTypeDefination::setCtx(
    FegenParser::TypeDefinationDeclContext *ctx) {
  this->ctx = ctx;
}

bool fegen::FegenTypeDefination::isCustome() { return this->ifCustome; }

// class Expression

fegen::FegenRightValue::Expression::Expression(bool ifTerminal,
                                               LiteralKind kind,
                                               FegenType &exprTy,
                                               bool isConstexpr)
    : ifTerminal(ifTerminal), kind(kind), exprType(exprTy),
      ifConstexpr(isConstexpr) {}

bool fegen::FegenRightValue::Expression::isTerminal() {
  return this->ifTerminal;
}

fegen::FegenRightValue::LiteralKind
fegen::FegenRightValue::Expression::getKind() {
  return this->kind;
}

std::any fegen::FegenRightValue::Expression::getContent() {
  if (this->ifTerminal) {
    auto tPtr =
        dynamic_cast<fegen::FegenRightValue::ExpressionTerminal *>(this);
    return tPtr->content;
  } else {
    return dynamic_cast<fegen::FegenRightValue::ExpressionNode *>(this);
    ;
  }
}

bool fegen::FegenRightValue::Expression::isConstexpr() {
  return this->ifConstexpr;
}

// class ExpressionNode

fegen::FegenRightValue::ExpressionNode::ExpressionNode(
    std::vector<fegen::FegenRightValue::Expression *> params,
    std::variant<fegen::FegenFunction *, fegen::FegenOperation *,
                 fegen::FegenOperator>
        op,
    FegenType &exprTy, bool ifConstexpr)
    : Expression(false, fegen::FegenRightValue::LiteralKind::EXPRESSION, exprTy,
                 ifConstexpr),
      op(op), params(params) {}

fegen::FegenRightValue::ExpressionNode::~ExpressionNode() {
  for (auto p : this->params) {
    delete p;
  }
}

std::string fegen::FegenRightValue::ExpressionNode::toString() {
  // TODO: toString
  return "todo: fegen::FegenRightValue::ExpressionNode::toString";
}

inline bool isBinaryOperator(fegen::FegenOperator &op) {
  switch (op) {
  case fegen::FegenOperator::NEG:
  case fegen::FegenOperator::NOT:
    return false;
  default:
    return true;
  }
}

inline std::string OperatorToString(fegen::FegenOperator &op) {
  switch (op) {
  case fegen::FegenOperator::ADD:
    return "+";
  case fegen::FegenOperator::SUB:
    return "-";
  case fegen::FegenOperator::MUL:
    return "*";
  case fegen::FegenOperator::DIV:
    return "/";
  default:
    std::cerr << "unsupproted operator." << std::endl;
    exit(0);
  }
}

std::string fegen::FegenRightValue::ExpressionNode::toStringForTypedef(
    std::string (*mapFunc)(std::string)) {
  // make sure that op is operator
  assert(this->op.index() == 2);
  auto opt = std::get<2>(op);
  assert(isBinaryOperator(opt));
  std::string res;
  std::string lhs = this->params[0]->toStringForTypedef(mapFunc);
  std::string rhs = this->params[1]->toStringForTypedef(mapFunc);
  res.append(lhs);
  res.append(OperatorToString(opt));
  res.append(rhs);
  return res;
}

std::any fegen::FegenRightValue::ExpressionNode::getContent() { return this; }

fegen::FegenRightValue::ExpressionNode *
fegen::FegenRightValue::ExpressionNode::binaryOperation(
    fegen::FegenRightValue::Expression *lhs,
    fegen::FegenRightValue::Expression *rhs, FegenOperator op) {
  // TODO: infer type kind: cpp, attribute, or operand
  FegenType resTy = fegen::inferenceType({lhs, rhs}, op);
  return new fegen::FegenRightValue::ExpressionNode(
      {lhs, rhs}, op, resTy, (lhs->isConstexpr() && rhs->isConstexpr()));
}

fegen::FegenRightValue::ExpressionNode *
fegen::FegenRightValue::ExpressionNode::unaryOperation(
    fegen::FegenRightValue::Expression *v, FegenOperator op) {
  // TODO: infer type kind: cpp, attribute, or operand
  FegenType resTy = fegen::inferenceType({v}, op);
  return new fegen::FegenRightValue::ExpressionNode({v}, op, resTy,
                                                    v->isConstexpr());
}

// class ExpressionTerminal
fegen::FegenRightValue::ExpressionTerminal::ExpressionTerminal(
    primLiteralType c, fegen::FegenRightValue::LiteralKind kind,
    FegenType exprTy, bool ifConstexpr)
    : Expression(true, kind, exprTy, ifConstexpr), content(c) {}

fegen::FegenRightValue::ExpressionTerminal::~ExpressionTerminal() {
  if (this->kind == fegen::FegenRightValue::LiteralKind::VECTOR) {
    auto &v = std::get<std::vector<Expression *>>(this->content);
    for (auto p : v) {
      delete p;
    }
  }
}

std::string fegen::FegenRightValue::ExpressionTerminal::toString() {
  // TODO: toString
  return "todo: fegen::FegenRightValue::ExpressionTerminal::toString";
}

std::string fegen::FegenRightValue::ExpressionTerminal::toStringForTypedef(
    std::string (*mapFunc)(std::string)) {
  switch (this->kind) {
  case fegen::FegenRightValue::LiteralKind::INT:
    return std::to_string(std::get<int>(this->content));
  case fegen::FegenRightValue::LiteralKind::FLOAT:
    return std::to_string(std::get<float>(this->content));
  case fegen::FegenRightValue::LiteralKind::TYPE: {
    auto typeName =
        std::get<FegenType>(this->content).getTypeDefination()->getName();
    return mapFunc(typeName);
  }
  case fegen::FegenRightValue::LiteralKind::VECTOR: {
    std::string res;
    res.append("[");
    auto exprs = std::get<std::vector<Expression *>>(this->content);
    for (size_t i = 0; i <= exprs.size() - 1; i++) {
      res.append(exprs[i]->toStringForTypedef(mapFunc));
      if (i != exprs.size() - 1) {
        res.append(", ");
      }
    }
    res.append("]");
    return res;
  }
  default: {
    std::cerr << "unsupport expression" << std::endl;
    exit(0);
  }
  }
}

std::any fegen::FegenRightValue::ExpressionTerminal::getContent() {
  switch (this->kind) {
  case fegen::FegenRightValue::LiteralKind::INT:
    return std::get<int>(this->content);
  case fegen::FegenRightValue::LiteralKind::FLOAT:
    return std::get<float>(this->content);
  case fegen::FegenRightValue::LiteralKind::STRING:
    return std::get<std::string>(this->content);
  case fegen::FegenRightValue::LiteralKind::TYPE:
    return std::get<FegenType>(this->content);
  case fegen::FegenRightValue::LiteralKind::VECTOR:
    return std::get<std::vector<Expression *>>(this->content);
  case fegen::FegenRightValue::LiteralKind::LEFT_VAR:
    return std::get<FegenValue *>(this->content);
  default:
    return std::monostate();
  }
}

fegen::FegenRightValue::ExpressionTerminal *
fegen::FegenRightValue::ExpressionTerminal::get(std::monostate content) {
  return new fegen::FegenRightValue::ExpressionTerminal(
      content, fegen::FegenRightValue::LiteralKind::MONOSTATE,
      fegen::FegenType::getPlaceHolder(), true);
}

fegen::FegenRightValue::ExpressionTerminal *
fegen::FegenRightValue::ExpressionTerminal::get(int content) {
  return new fegen::FegenRightValue::ExpressionTerminal(
      content, fegen::FegenRightValue::LiteralKind::INT,
      fegen::FegenType::getInt32Type(), true);
}

fegen::FegenRightValue::ExpressionTerminal *
fegen::FegenRightValue::ExpressionTerminal::get(float content) {
  return new fegen::FegenRightValue::ExpressionTerminal(
      content, fegen::FegenRightValue::LiteralKind::FLOAT,
      fegen::FegenType::getFloatType(), true);
}

fegen::FegenRightValue::ExpressionTerminal *
fegen::FegenRightValue::ExpressionTerminal::get(std::string content) {
  return new fegen::FegenRightValue::ExpressionTerminal(
      content, fegen::FegenRightValue::LiteralKind::STRING,
      fegen::FegenType::getStringType(), true);
}

fegen::FegenRightValue::ExpressionTerminal *
fegen::FegenRightValue::ExpressionTerminal::get(fegen::FegenType &content) {
  bool ifConstexpr = true;
  for (auto param : content.getParameters()) {
    if (!param->getExpr()->isConstexpr()) {
      ifConstexpr = false;
      break;
    }
  }
  if (content.getTypeLevel() == 2) {
    return new fegen::FegenRightValue::ExpressionTerminal(
        content, fegen::FegenRightValue::LiteralKind::TYPE,
        fegen::FegenType::getMetaTemplateType(), ifConstexpr);
  } else if (content.getTypeLevel() == 3) {
    return new fegen::FegenRightValue::ExpressionTerminal(
        content, fegen::FegenRightValue::LiteralKind::TYPE,
        fegen::FegenType::getMetaType(), ifConstexpr);
  } else {
    return new fegen::FegenRightValue::ExpressionTerminal(
        content, fegen::FegenRightValue::LiteralKind::TYPE,
        fegen::FegenType::getPlaceHolder(), ifConstexpr);
  }
}

fegen::FegenRightValue::ExpressionTerminal *
fegen::FegenRightValue::ExpressionTerminal::get(
    std::vector<fegen::FegenRightValue::Expression *> &content) {
  bool ifConstexpr = true;
  for (auto p : content) {
    if (!p->isConstexpr()) {
      ifConstexpr = false;
      break;
    }
  }
  return new fegen::FegenRightValue::ExpressionTerminal(
      content, fegen::FegenRightValue::LiteralKind::VECTOR,
      fegen::FegenType::getListType(content[0]->exprType), ifConstexpr);
}

fegen::FegenRightValue::ExpressionTerminal *
fegen::FegenRightValue::ExpressionTerminal::get(fegen::FegenValue *content) {
  return new fegen::FegenRightValue::ExpressionTerminal(
      content, fegen::FegenRightValue::LiteralKind::LEFT_VAR,
      content->getType(), content->getExpr()->isConstexpr());
}

// class FegenRightValue
fegen::FegenRightValue::FegenRightValue(
    fegen::FegenRightValue::Expression *content)
    : content(content) {}

fegen::FegenRightValue::FegenRightValue(const fegen::FegenRightValue &rhs) {
  if (rhs.content->isTerminal()) {
    auto expr =
        dynamic_cast<fegen::FegenRightValue::ExpressionTerminal *>(rhs.content);
    this->content = new fegen::FegenRightValue::ExpressionTerminal(*expr);
  } else {
    auto expr =
        dynamic_cast<fegen::FegenRightValue::ExpressionNode *>(rhs.content);
    this->content = new fegen::FegenRightValue::ExpressionNode(*expr);
  }
}

fegen::FegenRightValue::FegenRightValue(fegen::FegenRightValue &&rhs) {
  this->content = rhs.content;
  rhs.content = nullptr;
}

fegen::FegenRightValue::LiteralKind fegen::FegenRightValue::getKind() {
  return this->content->getKind();
}

std::string fegen::FegenRightValue::toString() {
  // TODO: toString
  return "TODO: fegen::FegenRightValue::toString.";
}

std::string fegen::FegenRightValue::toStringForTypedef(
    std::string (*mapFunc)(std::string)) {
  return this->content->toStringForTypedef(mapFunc);
}

std::any fegen::FegenRightValue::getContent() {
  return this->content->getContent();
}

fegen::FegenRightValue::Expression *fegen::FegenRightValue::getExpr() {
  return this->content;
}

fegen::FegenRightValue fegen::FegenRightValue::get() {
  return fegen::FegenRightValue(
      fegen::FegenRightValue::ExpressionTerminal::get(std::monostate()));
}

fegen::FegenRightValue fegen::FegenRightValue::get(int content) {
  return fegen::FegenRightValue(
      fegen::FegenRightValue::ExpressionTerminal::get(content));
}
fegen::FegenRightValue fegen::FegenRightValue::get(float content) {
  return fegen::FegenRightValue(
      fegen::FegenRightValue::ExpressionTerminal::get(content));
}
fegen::FegenRightValue fegen::FegenRightValue::get(std::string content) {
  return fegen::FegenRightValue(
      fegen::FegenRightValue::ExpressionTerminal::get(content));
}
fegen::FegenRightValue fegen::FegenRightValue::get(fegen::FegenType &content) {
  return fegen::FegenRightValue(
      fegen::FegenRightValue::ExpressionTerminal::get(content));
}

fegen::FegenRightValue fegen::FegenRightValue::get(
    std::vector<fegen::FegenRightValue::Expression *> &content) {
  return fegen::FegenRightValue(
      fegen::FegenRightValue::ExpressionTerminal::get(content));
}
fegen::FegenRightValue fegen::FegenRightValue::get(fegen::FegenValue *content) {
  return fegen::FegenRightValue(
      fegen::FegenRightValue::ExpressionTerminal::get(content));
}

fegen::FegenRightValue
fegen::FegenRightValue::get(fegen::FegenRightValue::Expression *expr) {
  assert(expr != nullptr);
  return fegen::FegenRightValue(expr);
}

fegen::FegenRightValue::~FegenRightValue() { delete this->content; }

// class FegenValue
fegen::FegenValue::FegenValue(fegen::FegenType type, std::string name,
                              fegen::FegenRightValue content)
    : type(std::move(type)), name(std::move(name)),
      content(std::move(content)) {}

fegen::FegenValue::FegenValue(const fegen::FegenValue &rhs)
    : type(rhs.type), name(rhs.name), content(rhs.content) {}
fegen::FegenValue::FegenValue(fegen::FegenValue &&rhs)
    : type(std::move(rhs.type)), name(std::move(rhs.name)),
      content(std::move(rhs.content)) {}

fegen::FegenValue *fegen::FegenValue::get(fegen::FegenType type,
                                          std::string name,
                                          FegenRightValue content) {
  return new fegen::FegenValue(std::move(type), std::move(name),
                               std::move(content));
}

fegen::FegenType &fegen::FegenValue::getType() { return this->type; }

std::string fegen::FegenValue::getName() { return this->name; }

fegen::FegenRightValue::LiteralKind fegen::FegenValue::getContentKind() {
  return this->content.getKind();
}

std::string fegen::FegenValue::getContentString() {
  return this->content.toString();
}

std::string fegen::FegenValue::getContentStringForTypedef(
    std::string (*mapFunc)(std::string)) {
  return this->content.toStringForTypedef(mapFunc);
}

fegen::FegenRightValue::Expression *fegen::FegenValue::getExpr() {
  return this->content.getExpr();
}

fegen::FegenRule::FegenRule(std::string content, fegen::FegenNode *src,
                            antlr4::ParserRuleContext *ctx)
    : content(content), src(src), ctx(ctx) {}

fegen::FegenRule *fegen::FegenRule::get(std::string content,
                                        fegen::FegenNode *src,
                                        antlr4::ParserRuleContext *ctx) {
  return new fegen::FegenRule(content, src, ctx);
}

llvm::StringRef fegen::FegenRule::getContent() { return this->content; }

bool fegen::FegenRule::addInput(fegen::FegenValue input) {
  auto name = input.getName();
  if (this->inputs.count(name) == 0) {
    return false;
  }
  this->inputs.insert({name, new fegen::FegenValue(input)});
  return true;
}

bool fegen::FegenRule::addReturn(fegen::FegenValue output) {
  auto name = output.getName();
  if (this->returns.count(name) == 0) {
    return false;
  }
  this->returns.insert({name, new fegen::FegenValue(output)});
  return true;
}

void fegen::FegenRule::setSrc(FegenNode *src) { this->src = src; }

fegen::FegenNode::FegenNode(std::vector<fegen::FegenRule *> &&rules,
                            antlr4::ParserRuleContext *ctx,
                            fegen::FegenNode::NodeType ntype)
    : rules(rules), ctx(ctx), ntype(ntype) {}

fegen::FegenNode *fegen::FegenNode::get(std::vector<fegen::FegenRule *> rules,
                                        antlr4::ParserRuleContext *ctx,
                                        fegen::FegenNode::NodeType ntype) {
  return new fegen::FegenNode(std::move(rules), ctx, ntype);
}
fegen::FegenNode *fegen::FegenNode::get(antlr4::ParserRuleContext *ctx,
                                        fegen::FegenNode::NodeType ntype) {
  std::vector<fegen::FegenRule *> rules;
  return new fegen::FegenNode(std::move(rules), ctx, ntype);
}

void fegen::FegenNode::addFegenRule(fegen::FegenRule *rule) {
  this->rules.push_back(rule);
}

fegen::FegenNode::~FegenNode() {
  for (auto rule : this->rules) {
    delete rule;
  }
}

void fegen::FegenManager::setModuleName(std::string name) {
  this->moduleName = name;
}

std::string getChildrenText(antlr4::tree::ParseTree *ctx) {
  std::string ruleText;
  for (auto child : ctx->children) {
    if (antlr4::tree::TerminalNode::is(child)) {
      ruleText.append(child->getText()).append(" ");
    } else {
      ruleText.append(getChildrenText(child)).append(" ");
    }
  }
  return ruleText;
}

fegen::FegenManager::FegenManager() {}

class Emitter {
private:
  std::ostream &stream;
  int tabCount;
  bool isNewLine;

public:
  Emitter() = delete;
  Emitter(Emitter &) = delete;
  Emitter(Emitter &&) = delete;
  Emitter(std::ostream &stream)
      : stream(stream), tabCount(0), isNewLine(true) {}
  void tab() { tabCount++; }

  void shiftTab() {
    tabCount--;
    if (tabCount < 0) {
      tabCount = 0;
    }
  }

  void newLine() {
    this->stream << std::endl;
    isNewLine = true;
  }

  std::ostream &operator<<(std::string s) {
    if (this->isNewLine) {
      for (int i = 0; i <= (this->tabCount - 1); i++) {
        this->stream << '\t';
      }
      this->isNewLine = false;
    }
    this->stream << s;
    return this->stream;
  }
};

// TODO: emit to file
std::string fegen::FegenManager::emitG4() {
  Emitter emitter(std::cout);
  emitter << "grammar " << this->moduleName << ";";
  emitter.newLine();
  for (auto node_pair : this->nodeMap) {
    auto nodeName = node_pair.first;
    auto node = node_pair.second;
    emitter << nodeName;
    emitter.newLine();
    emitter.tab();
    auto ruleCount = node->rules.size();
    if (ruleCount > 0) {
      emitter << ": " << getChildrenText(node->rules[0]->ctx);
      emitter.newLine();
      for (size_t i = 1; i <= ruleCount - 1; i++) {
        emitter << "| " << getChildrenText(node->rules[i]->ctx);
        emitter.newLine();
      }
      emitter << ";" << std::endl;
    }
    emitter.shiftTab();
    emitter.newLine();
  }
  return std::string();
}

// turn fegen type to td type
std::string getTdType(fegen::FegenType &ty) {
  if (ty.getTypeDefination()->isCustome()) {
    return ty.getTypeDefination()->getName();
  }
  std::string res;
  res.append(fegen::FegenManager::getManager()
                 .nameMapForTypeDef[ty.getTypeDefination()->getName()]);
  // parameters
  if (ty.getParameters().size() != 0) {
    res.append("<");
    for (size_t i = 0; i <= ty.getParameters().size() - 1; i++) {
      auto param = ty.getParameters(i);
      res.append(
          param->getContentStringForTypedef([](std::string key) -> std::string {
            return fegen::FegenManager::getManager().nameMapForTypeDef[key];
          }));
      if (i != ty.getParameters().size() - 1) {
        res.append(", ");
      }
    }
    res.append(">");
  }
  return res;
}

// TODO: emit to file
std::string fegen::FegenManager::emitTypeDefination() {
  Emitter emitter(std::cout);
  // file head
  std::string mn(this->moduleName);
  std::transform(mn.begin(), mn.end(), mn.begin(), ::toupper);
  emitter << "#ifndef " << mn << "_TYPE_TD";
  emitter.newLine();
  emitter << "#define " << mn << "_TYPE_TD" << std::endl;
  emitter.newLine();

  emitter << "include \"" << this->moduleName << "Dialect.td\"" << std::endl;
  // TODO: include files
  emitter.newLine();
  // Type class defination
  std::string typeClassName = this->moduleName + "Type";
  emitter << "class " << typeClassName
          << "<string typename, string typeMnemonic, list<Trait> traits = []>";
  emitter.tab();
  emitter << ": TypeDef<Toy_Dialect, typename, traits> {";
  emitter.newLine();
  emitter << "let mnemonic = typeMnemonic;";
  emitter.newLine();
  emitter.shiftTab();
  emitter << "}" << std::endl;
  emitter.newLine();

  for (auto pair : this->typeDefMap) {
    auto tyDef = pair.second;
    if (!tyDef->isCustome()) {
      continue;
    }
    auto typeName = pair.first;
    // head of typedef
    emitter << "def " << typeName << " : " << typeClassName << "<\"" << typeName
            << "\", \"" << tyDef->getMnemonic() << "\"> {";
    emitter.newLine();
    emitter.tab();
    // summary
    emitter << "let summary = \"TODO\";";
    emitter.newLine();
    // description
    emitter << "let description = [{ TODO }];";
    emitter.newLine();
    // parameters
    emitter << "let parameters = ( ins";
    emitter.newLine();
    emitter.tab();
    for (size_t i = 0; i <= tyDef->getParameters().size() - 1; i++) {
      auto param = tyDef->getParameters()[i];
      auto &paramTy = param->getType();
      auto paramName = param->getName();
      auto paramTyStr = getTdType(paramTy);
      emitter << paramTyStr << ":" << "$" << paramName;
      if (i != tyDef->getParameters().size() - 1) {
        emitter << ", ";
      }
      emitter.newLine();
    }
    emitter.shiftTab();
    emitter << ");" << std::endl;
    emitter.newLine();
    // assemblyFormat
    // TODO: handle list, Type ...
    emitter << "let assemblyFormat = [{";
    emitter.newLine();
    emitter << "`<` ";
    for (size_t i = 0; i <= tyDef->getParameters().size() - 1; i++) {
      auto param = tyDef->getParameters()[i];
      auto paramName = param->getName();
      emitter << "$" << paramName << " ";
      if (i != tyDef->getParameters().size() - 1) {
        emitter << "`x` ";
      }
    }
    emitter << "`>`";
    emitter.newLine();
    emitter << "}];";
    emitter.newLine();
  }
  emitter.shiftTab();
  emitter << "#endif // " << mn << "_TYPE_TD";

  return std::string();
}

void fegen::FegenManager::initbuiltinTypes() {
  // TODO: add type name map
  this->cppTypeNameMap.insert({{FEGEN_TYPE, "::mlir::Type"}});

  this->builderTypeNameMap.insert({{FEGEN_TYPE, "::mlir::Type"}});

  this->tdTypeNameMap.insert({{FEGEN_TYPE, "\"Type\""},
                              {FEGEN_TYPETEMPLATE, "Type"},
                              {FEGEN_INTEGER, "Builtin_Integer"},
                              {FEGEN_FLOATPOINT, "TODO"},
                              {FEGEN_VECTOR, "TODO"},
                              {FEGEN_TENSOR, "TODO"},
                              {FEGEN_LIST, "Variadic"},
                              {FEGEN_OPTINAL, "TODO"},
                              {FEGEN_ANY, "TODO"}});

  this->nameMapForTypeDef.insert(
      {{FEGEN_LIST, "ArrayRefParameter"}, {FEGEN_TYPE, "\"Type\""}});

  // placeholder type
  auto placeholderTypeDefination = fegen::FegenTypeDefination::get(
      "fegen_builtin", FEGEN_PLACEHOLDER, {}, nullptr, false);
  this->typeDefMap.insert({FEGEN_PLACEHOLDER, placeholderTypeDefination});

  // Type
  this->typeDefMap.insert(
      {FEGEN_TYPE, fegen::FegenTypeDefination::get("fegen_builtin", FEGEN_TYPE,
                                                   {}, nullptr, false)});

  // TypeTemplate
  this->typeDefMap.insert(
      {FEGEN_TYPETEMPLATE,
       fegen::FegenTypeDefination::get("fegen_builtin", FEGEN_TYPETEMPLATE, {},
                                       nullptr, false)});

  // recursive define Integer Type
  // Integer<Integer<Integer<...>>>
  auto intTypeDefination = fegen::FegenTypeDefination::get(
      "fegen_builtin", FEGEN_INTEGER, {}, nullptr, false);
  auto intType = fegen::FegenType(fegen::FegenType::TypeKind::CPP, {},
                                  intTypeDefination, false);
  // parameters of Integer is int32(Integer<32>)
  intTypeDefination->parameters.push_back(
      fegen::FegenValue::get(intType, "size", fegen::FegenRightValue::get()));
  this->typeDefMap.insert({FEGEN_INTEGER, intTypeDefination});

  // FloatPoint
  this->typeDefMap.insert(
      {FEGEN_FLOATPOINT,
       fegen::FegenTypeDefination::get(
           "fegen_builtin", FEGEN_FLOATPOINT,
           {fegen::FegenValue::get(fegen::FegenType::getInt32Type(), "size",
                                   fegen::FegenRightValue::get())},
           nullptr, false)});

  // Char
  this->typeDefMap.insert(
      {FEGEN_CHAR, fegen::FegenTypeDefination::get("fegen_builtin", FEGEN_CHAR,
                                                   {}, nullptr, false)});

  // String
  this->typeDefMap.insert(
      {FEGEN_STRING, fegen::FegenTypeDefination::get(
                         "fegen_builtin", FEGEN_STRING, {}, nullptr, false)});

  // Vector
  this->typeDefMap.insert(
      {FEGEN_VECTOR,
       fegen::FegenTypeDefination::get(
           "fegen_builtin", FEGEN_VECTOR,
           {fegen::FegenValue::get(fegen::FegenType::getInt32Type(), "size",
                                   fegen::FegenRightValue::get()),
            fegen::FegenValue::get(fegen::FegenType::getMetaType(),
                                   "elementType",
                                   fegen::FegenRightValue::get())},
           nullptr, false)});

  // List (this should be ahead of Tensor and Any Type defination)
  this->typeDefMap.insert(
      {FEGEN_LIST, fegen::FegenTypeDefination::get(
                       "fegen_builtin", FEGEN_LIST,
                       {fegen::FegenValue::get(fegen::FegenType::getMetaType(),
                                               "elementType",
                                               fegen::FegenRightValue::get())},
                       nullptr, false)});

  // Tensor
  this->typeDefMap.insert(
      {FEGEN_TENSOR,
       fegen::FegenTypeDefination::get(
           "fegen_builtin", FEGEN_TENSOR,
           {fegen::FegenValue::get(
                fegen::FegenType::getListType(fegen::FegenType::getInt32Type()),
                "shape", fegen::FegenRightValue::get()),
            fegen::FegenValue::get(fegen::FegenType::getMetaType(),
                                   "elementType",
                                   fegen::FegenRightValue::get())},
           nullptr, false)});

  // Optional
  this->typeDefMap.insert(
      {FEGEN_OPTINAL, fegen::FegenTypeDefination::get(
                          "fegen_builtin", FEGEN_OPTINAL,
                          {fegen::FegenValue::get(
                              fegen::FegenType::getMetaType(), "elementType",
                              fegen::FegenRightValue::get())},
                          nullptr, false)});

  // Any
  this->typeDefMap.insert(
      {FEGEN_ANY,
       fegen::FegenTypeDefination::get(
           "fegen_builtin", FEGEN_ANY,
           {fegen::FegenValue::get(
               fegen::FegenType::getListType(fegen::FegenType::getMetaType()),
               "elementType", fegen::FegenRightValue::get())},
           nullptr, false)});
}

fegen::FegenTypeDefination *
fegen::FegenManager::getTypeDefination(std::string name) {
  return this->typeDefMap[name];
}

bool fegen::FegenManager::addTypeDefination(fegen::FegenTypeDefination *tyDef) {
  if (this->typeDefMap.count(tyDef->name) != 0) {
    return false;
  }
  this->typeDefMap[tyDef->name] = tyDef;
  return true;
}

fegen::FegenManager &fegen::FegenManager::getManager() {
  static fegen::FegenManager fmg;
  return fmg;
}

fegen::FegenManager::~FegenManager() {
  // release nodes
  for (auto node_pair : this->nodeMap) {
    delete node_pair.second;
  }
}

fegen::FegenType
fegen::inferenceType(std::vector<fegen::FegenRightValue::Expression *> operands,
                     fegen::FegenOperator op) {
  // TODO: infer type
  return fegen::FegenType::getInt32Type();
}