#include "FegenManager.h"
#include "FegenParser.h"
#include "FegenParserBaseVisitor.h"
#include "Scope.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

fegen::Function::Function(std::string name,
                          std::vector<Value *> &&inputTypeList,
                          TypePtr returnType)
    : name(name), inputTypeList(inputTypeList), returnType(returnType) {}

fegen::Function *fegen::Function::get(std::string name,
                                      std::vector<Value *> inputTypeList,
                                      TypePtr returnType) {
  return new fegen::Function(name, std::move(inputTypeList), returnType);
}
std::string fegen::Function::getName() { return this->name; }

std::vector<fegen::Value *> &fegen::Function::getInputTypeList() {
  return this->inputTypeList;
}

fegen::Value *fegen::Function::getInputTypeList(size_t i) {
  return this->inputTypeList[i];
}

fegen::TypePtr fegen::Function::getReturnType() { return this->returnType; }

fegen::Operation::Operation(std::string dialectName, std::string operationName,
                            std::vector<Value *> &&arguments,
                            std::vector<Value *> &&results,
                            fegen::FegenParser::BodySpecContext *ctx)
    : dialectName(dialectName), arguments(arguments), results(results),
      ctx(ctx) {}

void fegen::Operation::setOpName(std::string name) {
  this->operationName = name;
}
std::string fegen::Operation::getOpName() { return this->operationName; }

std::vector<fegen::Value *> &fegen::Operation::getArguments() {
  return this->arguments;
}

fegen::Value *fegen::Operation::getArguments(size_t i) {
  return this->arguments[i];
}

std::vector<fegen::Value *> &fegen::Operation::getResults() {
  return this->results;
}

fegen::Value *fegen::Operation::getResults(size_t i) {
  return this->results[i];
}

fegen::Operation *fegen::Operation::get(std::string operationName,
                                        std::vector<Value *> arguments,
                                        std::vector<Value *> results,
                                        FegenParser::BodySpecContext *ctx) {
  return new fegen::Operation(fegen::Manager::getManager().moduleName,
                              operationName, std::move(arguments),
                              std::move(results), ctx);
}

// class FegenType

fegen::Type::Type(TypeKind kind, std::string name, TypeDefination *tyDef,
                  int typeLevel, bool isConstType)
    : kind(kind), typeName(name), typeDefine(tyDef), typeLevel(typeLevel),
      isConstType(isConstType) {}

fegen::Type::TypeKind fegen::Type::getTypeKind() { return this->kind; }

void fegen::Type::setTypeKind(fegen::Type::TypeKind kind) { this->kind = kind; }

fegen::TypeDefination *fegen::Type::getTypeDefination() {
  return this->typeDefine;
}

void fegen::Type::setTypeDefination(fegen::TypeDefination *tyDef) {
  this->typeDefine = tyDef;
}

std::string fegen::Type::getTypeName() { return this->typeName; }

int fegen::Type::getTypeLevel() { return this->typeLevel; }

bool fegen::Type::isConstant() { return this->isConstType; }

bool fegen::Type::isSameType(fegen::Type *type1, fegen::Type *type2) {
  if (type1->getTypeName() == type2->getTypeName())
    return true;
  else
    return false;
}

std::string fegen::Type::toStringForTypedef() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::Type::toStringForOpdef() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::Type::toStringForCppKind() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

fegen::TypePtr fegen::Type::getPlaceHolder() {
  return std::make_shared<PlaceHolderType>();
}

fegen::TypePtr fegen::Type::getMetaType() {
  return std::make_shared<MetaType>();
}

fegen::TypePtr fegen::Type::getMetaTemplateType() {
  return std::make_shared<MetaTemplate>();
}

fegen::TypePtr fegen::Type::getInt32Type() {
  return std::make_shared<IntegerType>(RightValue::getInteger(32));
}

fegen::TypePtr fegen::Type::getFloatType() {
  return std::make_shared<FloatPointType>(RightValue::getInteger(32));
}

fegen::TypePtr fegen::Type::getDoubleType() {
  return std::make_shared<FloatPointType>(RightValue::getInteger(64));
}

fegen::TypePtr fegen::Type::getBoolType() {
  return std::make_shared<IntegerType>(RightValue::getInteger(1));
}

fegen::TypePtr fegen::Type::getIntegerType(fegen::RightValue size) {
  return std::make_shared<IntegerType>(size);
}

fegen::TypePtr fegen::Type::getFloatPointType(fegen::RightValue size) {
  return std::make_shared<IntegerType>(size);
}

fegen::TypePtr fegen::Type::getStringType() {
  return std::make_shared<StringType>();
}

fegen::TypePtr fegen::Type::getListType(fegen::TypePtr elementType) {
  assert(elementType->typeLevel == 2 || elementType->typeLevel == 3);
  return std::make_shared<ListType>(
      fegen::RightValue::getTypeRightValue(elementType));
}

fegen::TypePtr fegen::Type::getListType(RightValue elementType) {
  auto ty = std::any_cast<fegen::TypePtr>(elementType.getContent());
  return Type::getListType(ty);
}

fegen::TypePtr fegen::Type::getVectorType(fegen::TypePtr elementType,
                                          fegen::RightValue size) {
  assert(elementType->typeLevel == 3);
  return std::make_shared<VectorType>(
      fegen::RightValue::getTypeRightValue(elementType), size);
}

fegen::TypePtr fegen::Type::getVectorType(RightValue elementType,
                                          RightValue size) {
  auto ty = std::any_cast<fegen::TypePtr>(elementType.getContent());
  return Type::getVectorType(ty, size);
}

fegen::TypePtr fegen::Type::getTensorType(fegen::TypePtr elementType,
                                          fegen::RightValue shape) {
  assert(elementType->typeLevel == 3);
  return std::make_shared<TensorType>(
      fegen::RightValue::getTypeRightValue(elementType), shape);
}

fegen::TypePtr fegen::Type::getTensorType(RightValue elementType,
                                          RightValue shape) {
  auto ty = std::any_cast<fegen::TypePtr>(elementType.getContent());
  return Type::getTensorType(ty, shape);
}

fegen::TypePtr fegen::Type::getOptionalType(fegen::TypePtr elementType) {
  assert(elementType->typeLevel == 2 || elementType->typeLevel == 3);
  return std::make_shared<OptionalType>(
      RightValue::getTypeRightValue(elementType));
}

fegen::TypePtr fegen::Type::getOptionalType(RightValue elementType) {
  auto ty = std::any_cast<fegen::TypePtr>(elementType.getContent());
  return Type::getOptionalType(ty);
}

fegen::TypePtr fegen::Type::getAnyType(fegen::RightValue elementTypes) {
  return std::make_shared<AnyType>(elementTypes);
}

fegen::TypePtr
fegen::Type::getCustomeType(std::vector<fegen::RightValue> params,
                            fegen::TypeDefination *tydef) {
  return std::make_shared<CustomeType>(params, tydef);
}

fegen::TypePtr
fegen::Type::getTemplateType(fegen::TypeDefination *typeDefination) {
  return std::make_shared<TemplateType>(typeDefination);
}

/// @brief get name of Type Instance by jointsing template name and parameters,
/// for example: Integer + 32 --> Integer<32>
/// @return joint name
std::string jointTypeName(std::string templateName,
                          std::vector<fegen::RightValue> parameters) {
  if (parameters.empty()) {
    return templateName;
  }
  std::string res = templateName;
  res.append("<");
  size_t count = parameters.size();
  auto firstParamStr = parameters[0].toString();
  res.append(firstParamStr);
  for (size_t i = 1; i <= count - 1; i++) {
    auto paramStr = parameters[i].toString();
    res.append(", ");
    res.append(paramStr);
  }
  res.append(">");
  return res;
}

// class PlaceHolderType
fegen::PlaceHolderType::PlaceHolderType()
    : Type(fegen::Type::TypeKind::CPP, FEGEN_PLACEHOLDER,
           fegen::Manager::getManager().getTypeDefination(FEGEN_PLACEHOLDER), 0,
           true) {}

// class MetaType
fegen::MetaType::MetaType()
    : Type(fegen::Type::TypeKind::CPP, FEGEN_TYPE,
           fegen::Manager::getManager().getTypeDefination(FEGEN_TYPE), 2,
           true) {}

std::string fegen::MetaType::toStringForTypedef() { return "\"Type\""; }

// class MetaTemplate
fegen::MetaTemplate::MetaTemplate()
    : Type(fegen::Type::TypeKind::CPP, FEGEN_TYPETEMPLATE,
           fegen::Manager::getManager().getTypeDefination(FEGEN_TYPETEMPLATE),
           1, true) {}

// class IntegerType

fegen::IntegerType::IntegerType(RightValue size, TypeDefination *tyDef)
    : Type(fegen::Type::TypeKind::CPP, jointTypeName(FEGEN_INTEGER, {size}),
           tyDef, 3, size.isConstant()),
      size(size) {}

fegen::IntegerType::IntegerType(fegen::RightValue size)
    : Type(fegen::Type::TypeKind::CPP, jointTypeName(FEGEN_INTEGER, {size}),
           fegen::Manager::getManager().getTypeDefination(FEGEN_INTEGER), 3,
           size.isConstant()),
      size(size) {}

std::string fegen::IntegerType::toStringForTypedef() {
  auto content = std::any_cast<largestInt>(this->size.getContent());
  if (content == 32) {
    return "\"int\"";
  } else if (content == 1) {
    return "\"bool\"";
  } else if (content == 64) {
    return "\"long\"";
  } else if (content == 16) {
    return "\"short\"";
  } else {
    std::cerr << "unsupport type: " << this->getTypeName() << std::endl;
    assert(false);
  }
}

std::string fegen::IntegerType::toStringForOpdef() {
  auto content = std::any_cast<largestInt>(this->size.getContent());
  if (content == 32) {
    return "I32";
  } else if (content == 64) {
    return "I64";
  } else if (content == 16) {
    return "I16";
  } else {
    std::cerr << "unsupport type: " << this->getTypeName() << std::endl;
    assert(false);
  }
}

std::string fegen::IntegerType::toStringForCppKind() {
  auto content = std::any_cast<largestInt>(this->size.getContent());
  if (content == 32) {
    return "int";
  }
  if (content == 64) {
    return "long";
  } else if (content == 16) {
    return "short";
  } else {
    std::cerr << "unsupport type: " << this->getTypeName() << std::endl;
    assert(false);
  }
}

// class FloatPointType
fegen::FloatPointType::FloatPointType(fegen::RightValue size)
    : Type(fegen::Type::TypeKind::CPP, jointTypeName(FEGEN_FLOATPOINT, {size}),
           fegen::Manager::getManager().getTypeDefination(FEGEN_FLOATPOINT), 3,
           size.isConstant()),
      size(size) {}

std::string fegen::FloatPointType::toStringForTypedef() {
  auto content = std::any_cast<largestInt>(this->size.getContent());
  if (content == 32) {
    return "\"float\"";
  } else if (content == 64) {
    return "\"double\"";
  } else {
    std::cerr << "unsupport type: " << this->getTypeName() << std::endl;
    assert(false);
  }
}

std::string fegen::FloatPointType::toStringForOpdef() {
  return "FloatPointType::toStringForOpdef";
}

std::string fegen::FloatPointType::toStringForCppKind() {
  auto content = std::any_cast<largestInt>(this->size.getContent());
  if (content == 32) {
    return "float";
  }
  if (content == 64) {
    return "double";
  } else {
    std::cerr << "unsupport type: " << this->getTypeName() << std::endl;
    assert(false);
  }
}

// class StringType
fegen::StringType::StringType()
    : Type(fegen::Type::TypeKind::CPP, FEGEN_STRING,
           fegen::Manager::getManager().getTypeDefination(FEGEN_STRING), 3,
           true) {}

// class ListType
fegen::ListType::ListType(fegen::RightValue elementType)
    : Type(fegen::Type::TypeKind::CPP, jointTypeName(FEGEN_LIST, {elementType}),
           fegen::Manager::getManager().getTypeDefination(FEGEN_LIST),
           std::any_cast<fegen::TypePtr>(elementType.getContent())->getTypeLevel(),
           elementType.isConstant()),
      elementType(elementType) {}

std::string fegen::ListType::toStringForTypedef() {
  std::string res = "ArrayRefParameter<";
  res.append(this->elementType.toStringForTypedef());
  res.append(">");
  return res;
}

std::string fegen::ListType::toStringForOpdef() {
  std::string res = "Variadic<";
  res.append(this->elementType.toStringForOpdef());
  res.append(">");
  return res;
}

std::string fegen::ListType::toStringForCppKind() {
  std::string res = "std::vector<";
  res.append(this->elementType.toStringForCppKind());
  res.append(">");
  return res;
}

// class VectorType
fegen::VectorType::VectorType(RightValue elementType, RightValue size)
    : Type(fegen::Type::TypeKind::CPP,
           jointTypeName(FEGEN_VECTOR, {elementType, size}),
           fegen::Manager::getManager().getTypeDefination(FEGEN_VECTOR), 3,
           (elementType.isConstant() && size.isConstant())),
      elementType(elementType), size(size) {}

// class TensorType
fegen::TensorType::TensorType(RightValue elementType, RightValue shape)
    : Type(fegen::Type::TypeKind::CPP,
           jointTypeName(FEGEN_TENSOR, {elementType, shape}),
           fegen::Manager::getManager().getTypeDefination(FEGEN_TENSOR), 3,
           (elementType.isConstant() && shape.isConstant())),
      elementType(elementType), shape(shape) {}

// class OptionalType
fegen::OptionalType::OptionalType(RightValue elementType)
    : Type(fegen::Type::TypeKind::CPP,
           jointTypeName(FEGEN_OPTINAL, {elementType}),
           fegen::Manager::getManager().getTypeDefination(FEGEN_OPTINAL),
           std::any_cast<fegen::TypePtr>(elementType.getContent())->getTypeLevel(),
           elementType.isConstant()),
      elementType(elementType) {}

// class AnyType

inline int getTypeLevelOfListType(fegen::RightValue& elementTypes) {
  auto listContent = std::any_cast<std::vector<fegen::RightValue::ExprPtr>>(elementTypes.getContent());
  fegen::TypePtr ty = std::any_cast<fegen::TypePtr>(listContent[0]->getContent());
  return ty->getTypeLevel();
}

fegen::AnyType::AnyType(RightValue elementTypes)
    : Type(fegen::Type::TypeKind::CPP, jointTypeName(FEGEN_ANY, {elementTypes}),
           fegen::Manager::getManager().getTypeDefination(FEGEN_ANY),
           getTypeLevelOfListType(elementTypes),
           elementTypes.isConstant()),
      elementTypes(elementTypes) {}

// class CustomeType
inline bool isAllConstant(std::vector<fegen::RightValue> &params) {
  for (auto v : params) {
    if (!v.isConstant()) {
      return false;
    }
  }
  return true;
}

fegen::CustomeType::CustomeType(std::vector<RightValue> params,
                                TypeDefination *tydef)
    : Type(fegen::Type::TypeKind::CPP, jointTypeName(FEGEN_ANY, params), tydef,
           3, isAllConstant(params)),
      params(params) {}

// class TemplateType
fegen::TemplateType::TemplateType(TypeDefination *tydef)
    : Type(fegen::Type::TypeKind::CPP, tydef->getName(), tydef, 2, true) {}

fegen::TypePtr
fegen::TemplateType::instantiate(std::vector<RightValue> params) {
  auto tydef = this->getTypeDefination();
  if (tydef->isCustome()) {
    return Type::getCustomeType(params, tydef);
  } else if (tydef->getName() == FEGEN_INTEGER) {
    assert(params.size() == 1);
    return Type::getIntegerType(params[0]);
  } else if (tydef->getName() == FEGEN_FLOATPOINT) {
    assert(params.size() == 1);
    return Type::getFloatPointType(params[0]);
  } else if (tydef->getName() == FEGEN_STRING) {
    assert(params.size() == 0);
    return Type::getStringType();
  } else if (tydef->getName() == FEGEN_LIST) {
    assert(params.size() == 1);
    return Type::getListType(params[0]);
  } else if (tydef->getName() == FEGEN_VECTOR) {
    assert(params.size() == 2);
    return Type::getVectorType(params[0], params[1]);
  } else if (tydef->getName() == FEGEN_TENSOR) {
    assert(params.size() == 2);
    return Type::getTensorType(params[0], params[1]);
  } else if (tydef->getName() == FEGEN_OPTINAL) {
    assert(params.size() == 1);
    return Type::getOptionalType(params[0]);
  } else if (tydef->getName() == FEGEN_ANY) {
    assert(params.size() == 1);
    return Type::getAnyType(params[0]);
  } else {
    assert(false);
  }
}

std::string fegen::TemplateType::toStringForTypedef() {
  auto tyd = this->getTypeDefination();
  if (tyd->isCustome()) {
    return this->getTypeDefination()->getName();
  } else if (tyd->getName() == FEGEN_INTEGER) {
    return "Builtin_IntegerAttr";
  } else if (tyd->getName() == FEGEN_FLOATPOINT) {
    return "Builtin_FloatAttr";
  } else {
    std::cerr << "unsupport type: " << this->getTypeName() << std::endl;
    assert(false);
  }
}

std::string fegen::TemplateType::toStringForOpdef() {
  auto tyd = this->getTypeDefination();
  if (tyd->isCustome()) {
    return this->getTypeDefination()->getName();
  } else if (tyd->getName() == FEGEN_INTEGER) {
    return "Builtin_Integer";
  } else {
    std::cerr << "unsupport type: " << this->getTypeName() << std::endl;
    assert(false);
  }
}

// class FegenTypeDefination
fegen::TypeDefination::TypeDefination(
    std::string dialectName, std::string name,
    std::vector<fegen::Value *> parameters,
    FegenParser::TypeDefinationDeclContext *ctx, bool ifCustome)
    : dialectName(std::move(dialectName)), name(std::move(name)),
      parameters(std::move(parameters)), ctx(ctx), ifCustome(ifCustome) {}

fegen::TypeDefination *
fegen::TypeDefination::get(std::string dialectName, std::string name,
                           std::vector<fegen::Value *> parameters,
                           FegenParser::TypeDefinationDeclContext *ctx,
                           bool ifCustome) {
  return new fegen::TypeDefination(std::move(dialectName), std::move(name),
                                   std::move(parameters), ctx, ifCustome);
}

std::string fegen::TypeDefination::getDialectName() {
  return this->dialectName;
}

void fegen::TypeDefination::setDialectName(std::string name) {
  this->dialectName = name;
}

std::string fegen::TypeDefination::getName() { return this->name; }

std::string fegen::TypeDefination::getMnemonic() {
  if (this->mnemonic.empty()) {
    this->mnemonic = this->name;
    std::transform(this->mnemonic.begin(), this->mnemonic.end(),
                   this->mnemonic.begin(), ::tolower);
  }
  return this->mnemonic;
}

void fegen::TypeDefination::setName(std::string name) { this->name = name; }

const std::vector<fegen::Value *> &fegen::TypeDefination::getParameters() {
  return this->parameters;
}

fegen::FegenParser::TypeDefinationDeclContext *fegen::TypeDefination::getCtx() {
  return this->ctx;
}

void fegen::TypeDefination::setCtx(
    FegenParser::TypeDefinationDeclContext *ctx) {
  this->ctx = ctx;
}

bool fegen::TypeDefination::isCustome() { return this->ifCustome; }

// class Expression

fegen::RightValue::Expression::Expression(bool ifTerminal, LiteralKind kind,
                                          bool isConstexpr)
    : ifTerminal(ifTerminal), kind(kind), ifConstexpr(isConstexpr) {}

bool fegen::RightValue::Expression::isTerminal() { return this->ifTerminal; }

fegen::RightValue::LiteralKind fegen::RightValue::Expression::getKind() {
  return this->kind;
}

bool fegen::RightValue::Expression::isConstexpr() { return this->ifConstexpr; }

std::shared_ptr<fegen::RightValue::ExpressionTerminal>
fegen::RightValue::Expression::getPlaceHolder() {
  return std::make_shared<fegen::RightValue::PlaceHolder>();
}

std::shared_ptr<fegen::RightValue::ExpressionTerminal>
fegen::RightValue::Expression::getInteger(largestInt content, size_t size) {
  return std::make_shared<fegen::RightValue::IntegerLiteral>(content, size);
}

std::shared_ptr<fegen::RightValue::ExpressionTerminal>
fegen::RightValue::Expression::getFloatPoint(long double content, size_t size) {
  return std::make_shared<fegen::RightValue::FloatPointLiteral>(content, size);
}

std::shared_ptr<fegen::RightValue::ExpressionTerminal>
fegen::RightValue::Expression::getString(std::string content) {
  return std::make_shared<fegen::RightValue::StringLiteral>(content);
}

std::shared_ptr<fegen::RightValue::ExpressionTerminal>
fegen::RightValue::Expression::getTypeRightValue(fegen::TypePtr content) {
  return std::make_shared<fegen::RightValue::TypeLiteral>(content);
}

std::shared_ptr<fegen::RightValue::ExpressionTerminal>
fegen::RightValue::Expression::getList(
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> &content) {
  return std::make_shared<fegen::RightValue::ListLiteral>(content);
}

std::shared_ptr<fegen::RightValue::ExpressionTerminal>
fegen::RightValue::Expression::getLeftValue(fegen::Value *content) {
  return std::make_shared<fegen::RightValue::LeftValue>(content);
}

std::shared_ptr<fegen::RightValue::OperatorCall>
fegen::RightValue::Expression::binaryOperation(
    std::shared_ptr<fegen::RightValue::Expression> lhs,
    std::shared_ptr<fegen::RightValue::Expression> rhs, FegenOperator op) {
  TypePtr resTy = fegen::inferenceType({lhs, rhs}, op);
  return std::make_shared<fegen::RightValue::OperatorCall>(
      op,
      std::vector<std::shared_ptr<fegen::RightValue::Expression>>{lhs, rhs});
}

std::shared_ptr<fegen::RightValue::OperatorCall>
fegen::RightValue::Expression::unaryOperation(
    std::shared_ptr<fegen::RightValue::Expression> v, FegenOperator op) {
  TypePtr resTy = fegen::inferenceType({v}, op);
  return std::make_shared<fegen::RightValue::OperatorCall>(
      op, std::vector<std::shared_ptr<fegen::RightValue::Expression>>{v});
}

// class ExpressionNode

fegen::RightValue::ExpressionNode::ExpressionNode(LiteralKind kind,
                                                  bool ifConstexpr)
    : Expression(false, kind, ifConstexpr) {}

std::string fegen::RightValue::ExpressionNode::toString() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::RightValue::ExpressionNode::toStringForTypedef() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::RightValue::ExpressionNode::toStringForOpdef() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::RightValue::ExpressionNode::toStringForCppKind() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

fegen::TypePtr fegen::RightValue::ExpressionNode::getType() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
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

// class FunctionCall
inline bool isFuncParamsAllConstant(
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> &params) {
  for (auto param : params) {
    if (!param->isConstexpr()) {
      return false;
    }
  }
  return true;
}

// TODO: invoke methods of FegenFunction
fegen::RightValue::FunctionCall::FunctionCall(
    fegen::Function *func,
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> params)
    : ExpressionNode(fegen::RightValue::LiteralKind::FUNC_CALL,
                     isFuncParamsAllConstant(params)),
      func(func), params(std::move(params)) {}

std::string fegen::RightValue::FunctionCall::toString() {
  return "FunctionCall::toString";
}

std::string fegen::RightValue::FunctionCall::toStringForTypedef() {
  return "FunctionCall::toStringForTypedef";
}

std::string fegen::RightValue::FunctionCall::toStringForOpdef() {
  return "FunctionCall::toStringForOpdef";
}

std::string fegen::RightValue::FunctionCall::toStringForCppKind() {
  return "FunctionCall::toStringForCppKind";
}

std::any fegen::RightValue::FunctionCall::getContent() { return this; }

fegen::TypePtr fegen::RightValue::FunctionCall::getType() {
  return this->func->getReturnType();
}

// class OperationCall
fegen::RightValue::OperationCall::OperationCall(
    fegen::Operation *op,
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> params)
    : ExpressionNode(fegen::RightValue::LiteralKind::OPERATION_CALL,
                     isFuncParamsAllConstant(params)),
      op(op), params(std::move(params)) {}

std::string fegen::RightValue::OperationCall::toString() {
  return "OperationCall::toString";
}

std::string fegen::RightValue::OperationCall::toStringForTypedef() {
  return "OperationCall::toStringForTypedef";
}

std::string fegen::RightValue::OperationCall::toStringForOpdef() {
  return "OperationCall::toStringForOpdef";
}

std::string fegen::RightValue::OperationCall::toStringForCppKind() {
  return "OperationCall::toStringForCppKind";
}

std::any fegen::RightValue::OperationCall::getContent() { return this; }

fegen::TypePtr fegen::RightValue::OperationCall::getType() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

// class OperatorCall
fegen::RightValue::OperatorCall::OperatorCall(
    fegen::FegenOperator op,
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> params)
    : ExpressionNode(fegen::RightValue::LiteralKind::OPERATION_CALL,
                     isFuncParamsAllConstant(params)),
      op(op), params(std::move(params)) {}

std::string fegen::RightValue::OperatorCall::toString() {
  return "OperatorCall::toString";
}

std::string fegen::RightValue::OperatorCall::toStringForTypedef() {
  return "OperatorCall::toStringForTypedef";
}

std::string fegen::RightValue::OperatorCall::toStringForOpdef() {
  return "OperatorCall::toStringForOpdef";
}

std::string fegen::RightValue::OperatorCall::toStringForCppKind() {
  return "OperatorCall::toStringForCppKind";
}

std::any fegen::RightValue::OperatorCall::getContent() { return this; }

fegen::TypePtr fegen::RightValue::OperatorCall::getType() {
  return inferenceType(this->params, this->op);
}

// class ExpressionTerminal
fegen::RightValue::ExpressionTerminal::ExpressionTerminal(
    fegen::RightValue::LiteralKind kind, bool ifConstexpr)
    : Expression(true, kind, ifConstexpr) {}

std::string fegen::RightValue::ExpressionTerminal::toString() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::RightValue::ExpressionTerminal::toStringForTypedef() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::RightValue::ExpressionTerminal::toStringForOpdef() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

std::string fegen::RightValue::ExpressionTerminal::toStringForCppKind() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

fegen::TypePtr fegen::RightValue::ExpressionTerminal::getType() {
  assert(FEGEN_NOT_IMPLEMENTED_ERROR);
}

// class PlaceHolder
fegen::RightValue::PlaceHolder::PlaceHolder()
    : ExpressionTerminal(fegen::RightValue::LiteralKind::MONOSTATE, true) {}

std::any fegen::RightValue::PlaceHolder::getContent() {
  return std::monostate();
}

std::string fegen::RightValue::PlaceHolder::toString() { return ""; }

// class IntegerLiteral
fegen::RightValue::IntegerLiteral::IntegerLiteral(largestInt content,
                                                  size_t size)
    : ExpressionTerminal(fegen::RightValue::LiteralKind::INT, true),
      content(content) {}

std::any fegen::RightValue::IntegerLiteral::getContent() {
  return this->content;
}

std::string fegen::RightValue::IntegerLiteral::toString() {
  return std::to_string(this->content);
}

fegen::TypePtr fegen::RightValue::IntegerLiteral::getType() {
  return fegen::Type::getIntegerType(fegen::RightValue::getInteger(this->size));
}

// class FloatPointLiteral
fegen::RightValue::FloatPointLiteral::FloatPointLiteral(long double content,
                                                        size_t size)
    : ExpressionTerminal(fegen::RightValue::LiteralKind::FLOAT, true),
      content(content) {}

std::any fegen::RightValue::FloatPointLiteral::getContent() {
  return this->content;
}

std::string fegen::RightValue::FloatPointLiteral::toString() {
  return std::to_string(this->content);
}

fegen::TypePtr fegen::RightValue::FloatPointLiteral::getType() {
  return fegen::Type::getFloatPointType(
      fegen::RightValue::getInteger(this->size));
}

// class StringLiteral
fegen::RightValue::StringLiteral::StringLiteral(std::string content)
    : ExpressionTerminal(fegen::RightValue::LiteralKind::STRING, true),
      content(content) {}

std::any fegen::RightValue::StringLiteral::getContent() {
  return this->content;
}

std::string fegen::RightValue::StringLiteral::toString() {
  std::string res;
  res.append("\"");
  res.append(this->content);
  res.append("\"");
  return res;
}

fegen::TypePtr fegen::RightValue::StringLiteral::getType() {
  return fegen::Type::getStringType();
}

// class TypeLiteral

// Check params of content and return ture if params are all const expr.
inline bool isParamsConstant(fegen::TypePtr content) {
  // for (auto param : content.getParameters()) {
  //   if (!param->getExpr()->isConstexpr()) {
  //     return false;
  //   }
  // }
  return true;
}

// Get type of type literal.
fegen::TypePtr getTypeLiteralType(fegen::TypePtr content) {
  if (content->getTypeLevel() == 2) {
    return fegen::Type::getMetaTemplateType();
  } else if (content->getTypeLevel() == 3) {
    return fegen::Type::getMetaType();
  } else {
    return fegen::Type::getPlaceHolder();
  }
}

fegen::RightValue::TypeLiteral::TypeLiteral(fegen::TypePtr content)
    : ExpressionTerminal(fegen::RightValue::LiteralKind::TYPE,
                         content->isConstant()),
      content(content) {}

std::any fegen::RightValue::TypeLiteral::getContent() { return this->content; }

std::string fegen::RightValue::TypeLiteral::toString() {
  return this->content->getTypeName();
}

std::string fegen::RightValue::TypeLiteral::toStringForTypedef() {
  return this->content->toStringForTypedef();
}

std::string fegen::RightValue::TypeLiteral::toStringForOpdef() {
  return this->content->toStringForOpdef();
}

std::string fegen::RightValue::TypeLiteral::toStringForCppKind() {
  return this->content->toStringForCppKind();
}

fegen::TypePtr fegen::RightValue::TypeLiteral::getType() {
  if (this->content->getTypeLevel() == 2) {
    return fegen::Type::getMetaTemplateType();
  } else if (this->content->getTypeLevel() == 3) {
    return fegen::Type::getMetaType();
  } else {
    assert(false);
  }
}

// Return ture if all Expressions in content are all true.
bool isExpressionListConst(
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> &content) {
  for (auto p : content) {
    if (!p->isConstexpr()) {
      return false;
      break;
    }
  }
  return true;
}

fegen::RightValue::ListLiteral::ListLiteral(
    std::vector<std::shared_ptr<Expression>> &content)
    : ExpressionTerminal(fegen::RightValue::LiteralKind::VECTOR,
                         isExpressionListConst(content)),
      content(content) {}

std::any fegen::RightValue::ListLiteral::getContent() { return this->content; }

std::string fegen::RightValue::ListLiteral::toString() {
  std::string res;
  res.append("[");
  for (size_t i = 0; i <= this->content.size() - 1; i++) {
    res.append(this->content[i]->toString());
    if (i != this->content.size() - 1) {
      res.append(", ");
    }
  }
  res.append("]");
  return res;
}

std::string fegen::RightValue::ListLiteral::toStringForTypedef() {
  std::string res;
  res.append("[");
  for (size_t i = 0; i <= this->content.size() - 1; i++) {
    res.append(this->content[i]->toStringForTypedef());
    if (i != this->content.size() - 1) {
      res.append(", ");
    }
  }
  res.append("]");
  return res;
}

std::string fegen::RightValue::ListLiteral::toStringForOpdef() {
  std::string res;
  res.append("[");
  for (size_t i = 0; i <= this->content.size() - 1; i++) {
    res.append(this->content[i]->toStringForOpdef());
    if (i != this->content.size() - 1) {
      res.append(", ");
    }
  }
  res.append("]");
  return res;
}

fegen::TypePtr fegen::RightValue::ListLiteral::getType() {
  return fegen::Type::getListType(this->content[0]->getType());
}

// class LeftValue
fegen::RightValue::LeftValue::LeftValue(fegen::Value *content)
    : ExpressionTerminal(fegen::RightValue::LiteralKind::LEFT_VAR,
                         content->getExpr()->isConstexpr()),
      content(content) {}

std::any fegen::RightValue::LeftValue::getContent() { return this->content; }

std::string fegen::RightValue::LeftValue::toString() {
  return this->content->getName();
}

fegen::TypePtr fegen::RightValue::LeftValue::getType() {
  return this->content->getType();
}

// class FegenRightValue
fegen::RightValue::RightValue(
    std::shared_ptr<fegen::RightValue::Expression> content)
    : content(content) {}

fegen::RightValue::LiteralKind fegen::RightValue::getLiteralKind() {
  return this->content->getKind();
}

std::string fegen::RightValue::toString() { return this->content->toString(); }

std::string fegen::RightValue::toStringForTypedef() {
  return this->content->toStringForTypedef();
}

std::string fegen::RightValue::toStringForOpdef() {
  return this->content->toStringForOpdef();
}

std::string fegen::RightValue::toStringForCppKind() {
  return this->content->toStringForCppKind();
}

std::any fegen::RightValue::getContent() { return this->content->getContent(); }

fegen::TypePtr fegen::RightValue::getType() { return this->content->getType(); }

std::shared_ptr<fegen::RightValue::Expression> fegen::RightValue::getExpr() {
  return this->content;
}

bool fegen::RightValue::isConstant() { return this->content->isConstexpr(); }

fegen::RightValue fegen::RightValue::getPlaceHolder() {
  return fegen::RightValue(fegen::RightValue::Expression::getPlaceHolder());
}

fegen::RightValue fegen::RightValue::getInteger(largestInt content,
                                                size_t size) {
  return fegen::RightValue(
      fegen::RightValue::Expression::getInteger(content, size));
}

fegen::RightValue fegen::RightValue::getFloatPoint(long double content,
                                                   size_t size) {
  return fegen::RightValue(
      fegen::RightValue::Expression::getFloatPoint(content, size));
}
fegen::RightValue fegen::RightValue::getString(std::string content) {
  return fegen::RightValue(fegen::RightValue::Expression::getString(content));
}
fegen::RightValue fegen::RightValue::getTypeRightValue(fegen::TypePtr content) {
  return fegen::RightValue(
      fegen::RightValue::Expression::getTypeRightValue(content));
}

fegen::RightValue fegen::RightValue::getList(
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> &content) {
  return fegen::RightValue(fegen::RightValue::Expression::getList(content));
}
fegen::RightValue fegen::RightValue::getLeftValue(fegen::Value *content) {
  return fegen::RightValue(
      fegen::RightValue::Expression::getLeftValue(content));
}

fegen::RightValue fegen::RightValue::getByExpr(
    std::shared_ptr<fegen::RightValue::Expression> expr) {
  assert(expr != nullptr);
  return fegen::RightValue(expr);
}

// class FegenValue
fegen::Value::Value(fegen::TypePtr type, std::string name,
                    fegen::RightValue content)
    : type(type), name(std::move(name)), content(std::move(content)) {}

fegen::Value::Value(const fegen::Value &rhs)
    : type(rhs.type), name(rhs.name), content(rhs.content) {}
fegen::Value::Value(fegen::Value &&rhs)
    : type(std::move(rhs.type)), name(std::move(rhs.name)),
      content(std::move(rhs.content)) {}

fegen::Value *fegen::Value::get(fegen::TypePtr type, std::string name,
                                RightValue content) {
  return new fegen::Value(type, std::move(name), std::move(content));
}

fegen::TypePtr fegen::Value::getType() { return this->type; }

std::string fegen::Value::getName() { return this->name; }

void fegen::Value::setContent(fegen::RightValue content) {
  this->content = content;
}

fegen::RightValue::LiteralKind fegen::Value::getContentKind() {
  return this->content.getLiteralKind();
}

std::string fegen::Value::getContentString() {
  return this->content.toString();
}

std::string fegen::Value::getContentStringForTypedef() {
  return this->content.toStringForTypedef();
}

std::string fegen::Value::getContentStringForOpdef() {
  return this->content.toStringForOpdef();
}

std::string fegen::Value::getContentStringForCppKind() {
  return this->content.toStringForCppKind();
}

std::shared_ptr<fegen::RightValue::Expression> fegen::Value::getExpr() {
  return this->content.getExpr();
}

fegen::ParserRule::ParserRule(std::string content, fegen::ParserNode *src,
                              antlr4::ParserRuleContext *ctx)
    : content(content), src(src), ctx(ctx) {}

fegen::ParserRule *fegen::ParserRule::get(std::string content,
                                          fegen::ParserNode *src,
                                          antlr4::ParserRuleContext *ctx) {
  return new fegen::ParserRule(content, src, ctx);
}

llvm::StringRef fegen::ParserRule::getContent() { return this->content; }

bool fegen::ParserRule::addInput(fegen::Value input) {
  auto name = input.getName();
  if (this->inputs.count(name) == 0) {
    return false;
  }
  this->inputs.insert({name, new fegen::Value(input)});
  return true;
}

bool fegen::ParserRule::addReturn(fegen::Value output) {
  auto name = output.getName();
  if (this->returns.count(name) == 0) {
    return false;
  }
  this->returns.insert({name, new fegen::Value(output)});
  return true;
}

void fegen::ParserRule::setSrc(ParserNode *src) { this->src = src; }

fegen::ParserNode::ParserNode(std::vector<fegen::ParserRule *> &&rules,
                              antlr4::ParserRuleContext *ctx,
                              fegen::ParserNode::NodeType ntype)
    : rules(rules), ctx(ctx), ntype(ntype) {}

fegen::ParserNode *
fegen::ParserNode::get(std::vector<fegen::ParserRule *> rules,
                       antlr4::ParserRuleContext *ctx,
                       fegen::ParserNode::NodeType ntype) {
  return new fegen::ParserNode(std::move(rules), ctx, ntype);
}
fegen::ParserNode *fegen::ParserNode::get(antlr4::ParserRuleContext *ctx,
                                          fegen::ParserNode::NodeType ntype) {
  std::vector<fegen::ParserRule *> rules;
  return new fegen::ParserNode(std::move(rules), ctx, ntype);
}

void fegen::ParserNode::addFegenRule(fegen::ParserRule *rule) {
  this->rules.push_back(rule);
}

fegen::ParserNode::~ParserNode() {
  for (auto rule : this->rules) {
    delete rule;
  }
}

void fegen::Manager::setModuleName(std::string name) {
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

fegen::Manager::Manager() {}

namespace fegen {

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

class StmtGenerator : FegenParserBaseVisitor {
private:
  Manager &manager;
  Emitter &emitter;

public:
  StmtGenerator(Emitter &emitter)
      : manager(Manager::getManager()), emitter(emitter) {}
  std::any visitVarDeclStmt(FegenParser::VarDeclStmtContext *ctx) override {
    auto var = manager.getStmtContent<Value *>(ctx->identifier());
    switch (var->getType()->getTypeKind()) {
    case fegen::Type::TypeKind::CPP: {
      this->emitter << var->getType()->toStringForCppKind() << " "
                    << var->getName();
      if (ctx->expression()) {
        auto expr = this->manager.getStmtContent<RightValue::Expression *>(
            ctx->expression());
        this->emitter << " = " << expr->toStringForCppKind();
      }
      this->emitter << ";";
      this->emitter.newLine();
      break;
    }
    case fegen::Type::TypeKind::ATTRIBUTE: {
      break;
    }
    case fegen::Type::TypeKind::OPERAND: {
      break;
    }
    }
    return nullptr;
  }

  std::any visitAssignStmt(FegenParser::AssignStmtContext *ctx) override {}

  std::any visitFunctionCall(FegenParser::FunctionCallContext *ctx) override {}

  std::any visitOpInvokeStmt(FegenParser::OpInvokeStmtContext *ctx) override {}

  std::any visitIfStmt(FegenParser::IfStmtContext *ctx) override {}

  std::any visitForStmt(FegenParser::ForStmtContext *ctx) override {}
};

} // namespace fegen

void fegen::Manager::emitG4() {
  std::ofstream fileStream;
  fileStream.open(this->moduleName + ".g4");
  fegen::Emitter emitter(fileStream);
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
}

// TODO: emit to file
void fegen::Manager::emitTypeDefination() {
  std::ofstream fileStream;
  fileStream.open(this->moduleName + "Types.td");
  fegen::Emitter emitter(fileStream);
  // file head
  std::string mn(this->moduleName);
  std::transform(mn.begin(), mn.end(), mn.begin(), ::toupper);
  emitter << "#ifndef " << mn << "_TYPE_TD";
  emitter.newLine();
  emitter << "#define " << mn << "_TYPE_TD";
  emitter << "\n";
  emitter.newLine();

  // include files
  emitter << "include \"mlir/IR/AttrTypeBase.td\"";
  emitter.newLine();
  emitter << "include \"" << this->moduleName << "Dialect.td\"";
  emitter << "\n";
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
    emitter << "let summary = \"This is generated by buddy fegen.\";";
    emitter.newLine();
    // description
    emitter << "let description = [{ This is generated by buddy fegen. }];";
    emitter.newLine();
    // parameters
    emitter << "let parameters = ( ins";
    emitter.newLine();
    emitter.tab();
    for (size_t i = 0; i <= tyDef->getParameters().size() - 1; i++) {
      auto param = tyDef->getParameters()[i];
      auto paramTy = param->getType();
      auto paramName = param->getName();
      auto paramTyStr = paramTy->toStringForTypedef();
      emitter << paramTyStr << ":"
              << "$" << paramName;
      if (i != tyDef->getParameters().size() - 1) {
        emitter << ", ";
      }
      emitter.newLine();
    }
    emitter.shiftTab();
    emitter << ");";
    emitter.newLine();
    // assemblyFormat
    // TODO: handle list, Type ...
    emitter << "let assemblyFormat = [{";
    emitter.newLine();
    emitter.tab();
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
    emitter.shiftTab();
    emitter.newLine();
    emitter << "}];";
    emitter.newLine();
    emitter.shiftTab();
    emitter << "}";
    emitter.newLine();
  }
  emitter.shiftTab();
  emitter << "\n";
  emitter << "#endif // " << mn << "_TYPE_TD";
  fileStream.close();
}

void fegen::Manager::emitOpDefination() {
  std::ofstream fileStream;
  fileStream.open(this->moduleName + "Ops.td");
  fegen::Emitter emitter(fileStream);

  // file head
  std::string mn(this->moduleName);
  std::transform(mn.begin(), mn.end(), mn.begin(), ::toupper);
  emitter << "#ifndef " << mn << "_OPS_TD";
  emitter.newLine();
  emitter << "#define " << mn << "_OPS_TD";
  emitter << "\n";
  emitter.newLine();

  // TODO: custome include files
  // include
  emitter << "include \"mlir/IR/BuiltinAttributes.td\"";
  emitter.newLine();
  emitter << "include \"mlir/IR/BuiltinTypes.td\"";
  emitter.newLine();
  emitter << "include \"mlir/IR/CommonAttrConstraints.td\"";
  emitter.newLine();
  emitter << "include \"" << this->moduleName << "Dialect.td\"";
  emitter.newLine();
  emitter << "include \"" << this->moduleName << "Types.td\"";
  emitter.newLine();
  emitter << "\n";

  // op class defination
  std::string classname = this->moduleName + "Op";
  emitter << "class " << classname
          << "<string mnemonic, list<Trait> traits = []>:";
  emitter.newLine();
  emitter.tab();
  emitter << "Op<Toy_Dialect, mnemonic, traits>;";
  emitter << "\n";
  emitter.shiftTab();
  emitter.newLine();

  // op definations
  for (auto pair : this->operationMap) {
    auto opName = pair.first;
    auto opDef = pair.second;
    // head of def
    emitter << "def " << opName << " : " << classname << "<\"" << opName
            << "\", [Pure]> {";
    emitter.newLine();
    emitter.tab();
    // summary and description
    emitter << "let summary = \"This is generated by buddy fegen.\";";
    emitter.newLine();
    emitter << "let description = [{This is generated by buddy fegen.}];";
    emitter.newLine();
    // arguments
    emitter << "let arguments = ( ins ";
    emitter.newLine();
    emitter.tab();
    for (auto param : opDef->getArguments()) {
      auto paramTyStr = param->getType()->toStringForOpdef();
      auto paramName = param->getName();
      emitter << paramTyStr << " : $" << paramName;
      emitter.newLine();
    }
    emitter.shiftTab();
    emitter << ");";
    emitter.newLine();
    // results
    emitter << "let results = (outs ";
    emitter.newLine();
    emitter.tab();
    for (auto param : opDef->getArguments()) {
      auto paramTyStr = param->getType()->toStringForOpdef();
      auto paramName = param->getName();
      emitter << paramTyStr << " : $" << paramName;
      emitter.newLine();
    }
    emitter.shiftTab();
    emitter << ");";
    emitter.newLine();
    // end of def
    emitter.shiftTab();
    emitter << "}";
    emitter.newLine();
  }

  // end of file
  emitter << "\n";
  emitter << "#endif // " << mn << "_DIALECT_TD";
  fileStream.close();
}

void fegen::Manager::emitDialectDefination() {
  std::ofstream fileStream;
  fileStream.open(this->moduleName + "Dialect.td");
  fegen::Emitter emitter(fileStream);

  // file head
  std::string mn(this->moduleName);
  std::transform(mn.begin(), mn.end(), mn.begin(), ::toupper);
  emitter << "#ifndef " << mn << "_DIALECT_TD";
  emitter.newLine();
  emitter << "#define " << mn << "_DIALECT_TD";
  emitter << "\n";
  emitter.newLine();

  // include
  emitter << "include \"mlir/IR/OpBase.td\"";
  emitter << "\n";
  emitter.newLine();

  // dialect defination
  emitter << "def " << this->moduleName << "_Dialect : Dialect {";
  emitter.newLine();
  emitter.tab();
  emitter << "let name = \"" << this->moduleName << "\";";
  emitter.newLine();
  emitter << "let summary = \"This is generated by buddy fegen.\";";
  emitter.newLine();
  emitter << "let description = [{This is generated by buddy fegen.}];";
  emitter.newLine();
  emitter << "let cppNamespace = \"::mlir::" << this->moduleName << "\";";
  emitter.newLine();
  emitter << "let extraClassDeclaration = [{";
  emitter.newLine();
  emitter.tab();
  emitter << "/// Register all types.";
  emitter.newLine();
  emitter << "void registerTypes();";
  emitter.newLine();
  emitter.shiftTab();
  emitter << "}];";
  emitter.newLine();
  emitter.shiftTab();
  emitter << "}";
  emitter.newLine();

  // end of file
  emitter << "#endif // " << mn << "_DIALECT_TD";
  fileStream.close();
}

void fegen::Manager::emitTdFiles() {
  this->emitDialectDefination();
  this->emitTypeDefination();
  this->emitOpDefination();
}

void fegen::Manager::initbuiltinTypes() {
  // placeholder type
  auto placeholderTypeDefination = fegen::TypeDefination::get(
      FEGEN_DIALECT_NAME, FEGEN_PLACEHOLDER, {}, nullptr, false);
  this->typeDefMap.insert({FEGEN_PLACEHOLDER, placeholderTypeDefination});

  // Type
  this->typeDefMap.insert(
      {FEGEN_TYPE, fegen::TypeDefination::get(FEGEN_DIALECT_NAME, FEGEN_TYPE,
                                              {}, nullptr, false)});

  // TypeTemplate
  this->typeDefMap.insert(
      {FEGEN_TYPETEMPLATE,
       fegen::TypeDefination::get(FEGEN_DIALECT_NAME, FEGEN_TYPETEMPLATE, {},
                                  nullptr, false)});

  // Integer
  auto intTydef = fegen::TypeDefination::get(FEGEN_DIALECT_NAME, FEGEN_INTEGER,
                                             {}, nullptr, false);
  auto paramOfIntTydef = Value::get(
      std::make_shared<IntegerType>(RightValue::getInteger(32), intTydef),
      "size", fegen::RightValue::getPlaceHolder());
  intTydef->parameters.push_back(paramOfIntTydef);
  this->typeDefMap.insert({FEGEN_INTEGER, intTydef});

  // FloatPoint
  this->typeDefMap.insert(
      {FEGEN_FLOATPOINT,
       fegen::TypeDefination::get(
           FEGEN_DIALECT_NAME, FEGEN_FLOATPOINT,
           {fegen::Value::get(fegen::Type::getInt32Type(), "size",
                              fegen::RightValue::getPlaceHolder())},
           nullptr, false)});

  // String
  this->typeDefMap.insert({FEGEN_STRING, fegen::TypeDefination::get(
                                             FEGEN_DIALECT_NAME, FEGEN_STRING,
                                             {}, nullptr, false)});

  // Vector
  this->typeDefMap.insert(
      {FEGEN_VECTOR,
       fegen::TypeDefination::get(
           FEGEN_DIALECT_NAME, FEGEN_VECTOR,
           {
               fegen::Value::get(fegen::Type::getMetaType(), "elementType",
                                 fegen::RightValue::getPlaceHolder()),
               fegen::Value::get(fegen::Type::getInt32Type(), "size",
                                 fegen::RightValue::getPlaceHolder()),
           },
           nullptr, false)});

  // List (this should be ahead of Tensor and Any Type defination)
  this->typeDefMap.insert(
      {FEGEN_LIST,
       fegen::TypeDefination::get(
           FEGEN_DIALECT_NAME, FEGEN_LIST,
           {fegen::Value::get(fegen::Type::getMetaType(), "elementType",
                              fegen::RightValue::getPlaceHolder())},
           nullptr, false)});

  // Tensor
  this->typeDefMap.insert(
      {FEGEN_TENSOR,
       fegen::TypeDefination::get(
           FEGEN_DIALECT_NAME, FEGEN_TENSOR,
           {fegen::Value::get(fegen::Type::getMetaType(), "elementType",
                              fegen::RightValue::getPlaceHolder()),
            fegen::Value::get(
                fegen::Type::getListType(fegen::Type::getInt32Type()), "shape",
                fegen::RightValue::getPlaceHolder())},
           nullptr, false)});

  // Optional
  this->typeDefMap.insert(
      {FEGEN_OPTINAL,
       fegen::TypeDefination::get(
           FEGEN_DIALECT_NAME, FEGEN_OPTINAL,
           {fegen::Value::get(fegen::Type::getMetaType(), "elementType",
                              fegen::RightValue::getPlaceHolder())},
           nullptr, false)});

  // Any
  this->typeDefMap.insert(
      {FEGEN_ANY, fegen::TypeDefination::get(
                      FEGEN_DIALECT_NAME, FEGEN_ANY,
                      {fegen::Value::get(
                          fegen::Type::getListType(fegen::Type::getMetaType()),
                          "elementType", fegen::RightValue::getPlaceHolder())},
                      nullptr, false)});
}

fegen::TypeDefination *fegen::Manager::getTypeDefination(std::string name) {
  auto it = this->typeDefMap.find(name);
  if (it != this->typeDefMap.end()) {
    return it->second;
  }
  assert(false);
}

bool fegen::Manager::addTypeDefination(fegen::TypeDefination *tyDef) {
  if (this->typeDefMap.count(tyDef->name) != 0) {
    return false;
  }
  this->typeDefMap[tyDef->name] = tyDef;
  return true;
}

fegen::Operation *fegen::Manager::getOperationDefination(std::string name) {
  return this->operationMap[name];
}

bool fegen::Manager::addOperationDefination(fegen::Operation *opDef) {
  if (this->operationMap.count(opDef->getOpName()) != 0) {
    return false;
  }
  this->operationMap[opDef->getOpName()] = opDef;
  return true;
}

void fegen::Manager::addStmtContent(antlr4::ParserRuleContext *ctx,
                                    std::any content) {
  this->stmtContentMap.insert({ctx, content});
}

fegen::Manager &fegen::Manager::getManager() {
  static fegen::Manager fmg;
  return fmg;
}

fegen::Manager::~Manager() {
  // release nodes
  for (auto node_pair : this->nodeMap) {
    delete node_pair.second;
  }
}

fegen::TypePtr fegen::inferenceType(
    std::vector<std::shared_ptr<fegen::RightValue::Expression>> operands,
    fegen::FegenOperator op) {
  // TODO: infer type
  return fegen::Type::getInt32Type();
}

namespace fegen {

// class StmtVisitor : public FegenParserBaseVisitor{
// public:
// };

}
void fegen::Manager::emitBuiltinFunction() {
  Emitter emitter(std::cout);
  for (auto function_pair : this->functionMap) {
    auto functionName = function_pair.first;
    auto function = function_pair.second;
    auto paraList = function->getInputTypeList();
    emitter << function->getReturnType()->toStringForTypedef() << " "
            << functionName << "(";
    for (auto para : paraList) {
      emitter << para->getContentStringForTypedef() << " " << para->getName();
      if (para != paraList.back())
        emitter << ", ";
    }
    emitter << "){";
    emitter.newLine();
    emitter.tab();
    // TODO::function body

    emitter.shiftTab();
    emitter.newLine();
    emitter << "}";
  }
}