#ifndef FEGEN_FEGENVISITOR_H
#define FEGEN_FEGENVISITOR_H

#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include "FegenManager.h"
#include "FegenParserBaseVisitor.h"

using namespace antlr4;

namespace fegen {

// std::any visitLexerRule(antlr4::ParserRuleContext *ctx)

class FegenVisitor : public FegenParserBaseVisitor {
private:
  FegenManager manager;

public:
  FegenVisitor() {}

  std::any
  visitParserRuleSpec(FegenParser::ParserRuleSpecContext *ctx) override {
    auto ruleList =
        std::any_cast<std::vector<FegenRule *>>(this->visit(ctx->ruleBlock()));
    auto ruleNode =
        FegenNode::get(ruleList, ctx, FegenNode::NodeType::PARSER_RULE);
    // set source node for rules
    for (auto rule : ruleList) {
      rule->setSrc(ruleNode);
    }
    this->manager.nodeMap.insert({ctx->ParserRuleName()->getText(), ruleNode});
    return nullptr;
  }

  std::any visitRuleAltList(FegenParser::RuleAltListContext *ctx) override {
    std::vector<FegenRule *> ruleList;
    for (auto alt : ctx->actionAlt()) {
      auto fegenRule = std::any_cast<FegenRule *>(this->visit(alt));
      ruleList.push_back(fegenRule);
    }
    return ruleList;
  }

  std::any visitActionAlt(FegenParser::ActionAltContext *ctx) override {
    auto rawRule = this->visit(ctx->alternative());
    if (ctx->actionBlock()) {
      auto blockValues = std::any_cast<
          std::tuple<std::vector<FegenValue *> *, std::vector<FegenValue *> *>>(
          this->visit(ctx->actionBlock()));
      auto inputs = std::get<0>(blockValues);
      auto returns = std::get<1>(blockValues);
      auto rule = std::any_cast<FegenRule *>(rawRule);
      for (auto in : *inputs) {
        auto flag = rule->addInput(in);
        // todo: throw error if flag is false
      }
      for (auto out : *returns) {
        auto flag = rule->addReturn(out);
        // todo: throw error if flag is false
      }
    }

    return rawRule;
  }

  // return FegenRule Object
  // TODO: do more check
  std::any visitAlternative(FegenParser::AlternativeContext *ctx) override {
    auto content = ctx->getText();
    auto rule = FegenRule::get(content, nullptr, ctx);
    return rule;
  }

  std::any visitLexerRuleSpec(FegenParser::LexerRuleSpecContext *ctx) override {
    auto ruleList = std::any_cast<std::vector<FegenRule *>>(
        this->visit(ctx->lexerRuleBlock()));
    auto ruleNode =
        FegenNode::get(ruleList, ctx, FegenNode::NodeType::LEXER_RULE);
    this->manager.nodeMap.insert({ctx->LexerRuleName()->getText(), ruleNode});
    return nullptr;
  }

  std::any visitActionBlock(FegenParser::ActionBlockContext *ctx) override {
      std::vector<FegenValue *> *inputs = nullptr;
      std::vector<FegenValue *> *returns = nullptr;
      if (ctx->inputsSpec()) {
        inputs = std::any_cast<std::vector<FegenValue *> *>(
            this->visit(ctx->inputsSpec()));
      }
      if (ctx->returnsSpec()) {
        returns = std::any_cast<std::vector<FegenValue *> *>(
            this->visit(ctx->returnsSpec()));
      }
      if (ctx->actionSpec()) {
        this->visit(ctx->actionSpec());
      }
      return std::tuple(inputs, returns);
    }

  std::any visitVarDecls(FegenParser::VarDeclsContext *ctx) override {
    std::vector<FegenValue *> varList = nullptr;
      for(auto var : ctx->varElement()){
          auto value = std::any_cast<FegenValue *>(this->visit(var));
          varList.push_back(value);
      }
      return varList;
  }

  std::any visitVarElement(FegenParser::VarElementContext *ctx) override {
    auto type = std::any_cast<string> (this->visit(ctx->typeSpec()));
    auto name = std::any_cast<string> (this->visit(ctx->identifier()));
    return FegenValue::get(false, type, name, ctx);
  }

  std::any visitBuiltinTypeInstances(FegenParser::BuiltinTypeInstancesContext *ctx) override{
    return ctx->child()->getText();
  }

  std::any visitIdentifier(FegenParser::IdentifierContext *ctx) override{
    return ctx->child()->getText();
  }

  // std::any visitStatemnetBlock(FegenParser::StatemnetBlockContext *ctx) override {
  //   std::vector<FegenValue *> ruleList = nullptr;
  //   for(auto state : ctx->statement()){
  //     auto value = 
  //   }
  // }

  std::any visitVarDeclStmt(FegenParser::VarDeclStmtContext *ctx) override {
     auto type = std::any_cast<string> (this->visit(ctx->typeSpec()));
     auto name = std::any_cast<string> (this->visit(ctx->identifier()));
     auto value = std::any_cast<FegenValue *> (this->visit(ctx->expression()));
     // TODO:type check
  }

  std::any visitFunctionCall(FegenPaser::FunctionCallContext *ctx) override {
      std::string funcName = std::any_cast<string> (this->visit(ctx->identifier()));
      std::vector<FegenValue *> *args = nullptr;
      if (ctx->expression()) {
          args = std::any_cast<std::vector<FegenValue *> *>(
              this->visit(ctx->expression()));
      }
      return FegenFunction::get(funcName, args);
  }
  
  void G4Gen(){
      for(map<llvm::StringRef, FegenNode *>::iterator it = FegenManager::nodeMap.begin(); it != FegenManager::nodeMap.end(); it++){
          std::cout << it->first << "\n :" << std::endl;
          std::vector<FegenRule *> rules = it->second;
          for(std::vector<FegenRule *>::iterator rule = rules.begin(); rule != rules.end(); rule++){
                std::cout << (*rule)->content << "\n " << std::endl;
                if(rule != rules.end() - 1)
                    std::cout << "|\t" << std::endl;
          }
          std::cout << ";\n" << std::endl;
      }
      
  }
}; // namespace fegen
}
#endif