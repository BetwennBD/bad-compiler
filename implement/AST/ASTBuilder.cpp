//
// Created by tangny on 2021/12/8.
//

#include <cassert>
#include <iostream>
#include <cmath>
#include "include/AST/ASTBuilder.h"

TranslationUnitDecl* ASTBuilder::constructAST(CSTNode *head, int verbose) {
    assert(head != NULL && "ASTBuilder receives invalid concrete syntax tree.\n");

    while(!nodeStack.empty()) nodeStack.pop();
    TranslationUnitDecl *translationUnitDecl = new TranslationUnitDecl();
    nodeStack.push(translationUnitDecl);
    recursiveVisitCST(head);

    assert(nodeStack.size() == 1 && "ASTBuilder receives invalid concrete syntax tree.\n");
    return translationUnitDecl;
}

void ASTBuilder::recursiveVisitCST(CSTNode *node) {
#define LISTEN(TYPE)                            \
    if(node->getListener() == #TYPE)            \
        enter##TYPE(node);
#include "include/AST/ListenerTypes.inc"
#ifdef LISTEN
#undef LISTEN
#endif

    for(auto child : node->children)
        recursiveVisitCST(child);

#define LISTEN(TYPE)                            \
    if(node->getListener() == #TYPE)            \
        quit##TYPE(node);
#include "include/AST/ListenerTypes.inc"
}

#define SEC_GET_DECL(DECL)                                                                                  \
    assert(nodeStack.top()->isDecl());                                                                      \
    Decl *pd##DECL = dynamic_cast<Decl*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##DECL->getKind() == Decl::k_##DECL);                                                          \
    DECL *p##DECL = dynamic_cast<DECL*>(pd##DECL);

#define SEC_GET_DECL_RANGE(DECL)                                                                            \
    assert(nodeStack.top()->isDecl());                                                                      \
    Decl *pd##DECL = dynamic_cast<Decl*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##DECL->getKind() >= Decl::first_##DECL && pd##DECL->getKind() <= Decl::last_##DECL);          \
    DECL *p##DECL = dynamic_cast<DECL*>(pd##DECL);

#define SEC_GET_STMT(STMT)                                                                                  \
    assert(nodeStack.top()->isStmt());                                                                      \
    Stmt *pd##STMT = dynamic_cast<Stmt*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##STMT->getKind() == Stmt::k_##STMT);                                                          \
    STMT *p##STMT = dynamic_cast<STMT*>(pd##STMT);

#define SEC_GET_STMT_RANGE(STMT)                                                                            \
    assert(nodeStack.top()->isStmt());                                                                      \
    Stmt *pd##STMT = dynamic_cast<Stmt*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##STMT->getKind() >= Stmt::first_##STMT && pd##STMT->getKind() <= Stmt::last_##STMT);          \
    STMT *p##STMT = dynamic_cast<STMT*>(pd##STMT);

#define SEC_GET_QAULTYPE()                                                                                  \
    assert(nodeStack.top()->isQualType());                                                                  \
    QualType *p##QualType = dynamic_cast<QualType*>(nodeStack.top());                                       \
    nodeStack.pop();


// ????????????(????????????)?????????
void ASTBuilder::enterRef(CSTNode *node) {}

//Fixme:
// ??????????????????????????????????????????
// ?????????????????????
// ?????????????????????
void ASTBuilder::quitRef(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    Expr *pDeclRefExpr = new DeclRefExpr(pNode->getId());
    exprCommonAction(parent, pDeclRefExpr, "Ref");
}

// ??????????????????
void ASTBuilder::enterNumLiteral(CSTNode *node) {}

void ASTBuilder::quitNumLiteral(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();

    std::string s = pNode->getId();
    Expr *pNumLiteral;
    // ??????????????????????????????
    if(s.find('E') == s.npos &&
        s.find('e') == s.npos &&
        s.find('.') == s.npos)
        pNumLiteral = new IntegerLiteral(std::stoi(s));
    else
        pNumLiteral = new FloatingLiteral(std::stod(s));

    exprCommonAction(parent, pNumLiteral, "NumLiteral");
}

// ??????????????????
void ASTBuilder::enterStrLiteral(CSTNode *node) {}

void ASTBuilder::quitStrLiteral(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    StringLiteral *pStringLiteral = new StringLiteral(pNode->getId());

    exprCommonAction(parent, pStringLiteral, "StrLiteral");
}

// ????????????????????????????????????
void ASTBuilder::enterSelector(CSTNode *node) {
    SelectorArray *selectorArray = new SelectorArray();

    // ??????selector('[]', '.', '->')??????????????????????????????????????????????????????????????????
    assert(node->getChildren()[1]->isTerminal());
    if(node->getChildren()[1]->getType() == "[") {
        IndexSelector *indexSelector = new IndexSelector();
        selectorArray->addSelector(indexSelector);
    }
    else if(node->getChildren()[1]->getType() == ".") {
        FieldSelector *fieldSelector = new FieldSelector();
        assert(node->getChildren()[2]->isTerminal());
        fieldSelector->setName(node->getChildren()[2]->getId());
        selectorArray->addSelector(fieldSelector);
    }
    else if(node->getChildren()[1]->getType() == "PTR_OP") {
        DerefSelector *derefSelector = new DerefSelector();
        selectorArray->addSelector(derefSelector);
        FieldSelector *fieldSelector = new FieldSelector();
        assert(node->getChildren()[2]->isTerminal());
        fieldSelector->setName(node->getChildren()[2]->getId());
        selectorArray->addSelector(fieldSelector);
    }
    else {
        assert(0 && "In enterSelector: invalid selector type");
    }

    nodeStack.push(selectorArray);
}

void ASTBuilder::quitSelector(CSTNode *node) {
    SEC_GET_STMT(SelectorArray)

    if(auto tempStmt = dynamic_cast<SelectorArray*>(nodeStack.top())) {
        // ???????????????pSelectorArray??????????????????SelectorArray??????
        for (auto selector: tempStmt->getSelectors())
            pSelectorArray->addSelector(selector);
        nodeStack.pop();
        delete tempStmt;
        nodeStack.push(pSelectorArray);
        return;
    }

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pSelectorArray, "Selector");
}

// ??????????????????
void ASTBuilder::enterFuncCall(CSTNode *node) {
    CallExpr *callExpr = new CallExpr();
    nodeStack.push(callExpr);
}

void ASTBuilder::quitFuncCall(CSTNode *node) {
    SEC_GET_STMT(CallExpr)

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pCallExpr, "FuncCall");
}

// ???????????????????????????
void ASTBuilder::enterPostOp(CSTNode *node) {
    assert(node->getChildren().size() == 2 && node->getChildren()[1]->isTerminal());
    UnaryOperator *unaryOperator = new UnaryOperator();
    std::string opKind = node->getChildren()[1]->getType();

    if(opKind == "INC_OP")
        unaryOperator->setOp(UnaryOperator::_post_inc);
    else if(opKind == "DEC_OP")
        unaryOperator->setOp(UnaryOperator::_post_dec);
    else
        assert(false && "invalid Post Op");
    nodeStack.push(unaryOperator);
}

void ASTBuilder::quitPostOp(CSTNode *node) {
    SEC_GET_STMT(UnaryOperator)

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pUnaryOperator, "PostOp");
}

// ???????????????????????????
void ASTBuilder::enterPreOp(CSTNode *node) {
    assert(node->getChildren().size() == 2);
    std::string opKind;
    if(node->getChildren()[0]->isTerminal())
        opKind = node->getChildren()[0]->getType();
    else {
        CSTNode *pNode = node->getChildren()[0];
        assert(pNode->getChildren().size() == 1 && pNode->getChildren()[0]->isTerminal());
        opKind = pNode->getChildren()[0]->getType();
    }

    if(opKind == "*") {
        SelectorArray *selectorArray = new SelectorArray();
        DerefSelector *derefSelector = new DerefSelector();
        selectorArray->addSelector(derefSelector);

        nodeStack.push(selectorArray);
        return;
    }

    UnaryOperator *unaryOperator = new UnaryOperator();
    if(opKind == "INC_OP")
        unaryOperator->setOp(UnaryOperator::_pre_inc);
    else if(opKind == "DEC_OP")
        unaryOperator->setOp(UnaryOperator::_pre_dec);
    else if(opKind == "&")
        unaryOperator->setOp(UnaryOperator::_address_of);
    else if(opKind == "~")
        unaryOperator->setOp(UnaryOperator::_bit_not);
    else if(opKind == "!")
        unaryOperator->setOp(UnaryOperator::_not);
    else if(opKind == "+")
        unaryOperator->setOp(UnaryOperator::_unary_plus);
    else if(opKind == "-")
        unaryOperator->setOp(UnaryOperator::_unary_minus);
    else
        assert(false && "invalid Pre Op");
    nodeStack.push(unaryOperator);
}

void ASTBuilder::quitPreOp(CSTNode *node) {
    // ????????????????????????
    if(!node->getChildren()[0]->isTerminal()) {
        std::string opKind;
        opKind = node->getChildren()[0]->getChildren()[0]->getType();
        if(opKind == "*")  {
            SEC_GET_STMT(SelectorArray)

            if(auto tempStmt = dynamic_cast<SelectorArray*>(nodeStack.top())) {
                // ???????????????pSelectorArray??????????????????SelectorArray??????
                for (auto selector: tempStmt->getSelectors())
                    pSelectorArray->addSelector(selector);
                nodeStack.pop();
                delete tempStmt;
                nodeStack.push(pSelectorArray);
                return;
            }

            AbstractASTNode *parent = nodeStack.top();
            exprCommonAction(parent, pSelectorArray, "Selector");
            return;
        }
    }

    SEC_GET_STMT(UnaryOperator)

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pUnaryOperator, "PreOp");
}

// ????????????????????????
void ASTBuilder::enterCstExpr(CSTNode *node) {
    CastExpr *castExpr = new CastExpr();
    nodeStack.push(castExpr);
}

void ASTBuilder::quitCstExpr(CSTNode *node) {
    SEC_GET_STMT_RANGE(CastExpr)

    assert(pCastExpr->getKind() == Stmt::k_ExplicitCastExpr);

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CallExpr) {
        dynamic_cast<CallExpr*>(parent)->addArg(pCastExpr);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setInitializer(pCastExpr);
        return;
    }

    raiseRuleViolation("CstExpr", parent);
}

// ???????????????????????????
void ASTBuilder::enterBinOp(CSTNode *node) {
    assert(node->getChildren().size() == 3);

    std::string opKind;
    if(node->getChildren()[1]->isTerminal())
        opKind = node->getChildren()[1]->getType();
    else {
        CSTNode *pNode = node->getChildren()[1];
        assert(pNode->getChildren().size() == 1 && pNode->getChildren()[0]->isTerminal());
        opKind = pNode->getChildren()[0]->getType();
    }

    BinaryOperator *binaryOperator = new BinaryOperator();
    assert(opMap.find(opKind) != opMap.end());
    binaryOperator->setOp(opMap[opKind]);
    nodeStack.push(binaryOperator);
}

void ASTBuilder::quitBinOp(CSTNode *node) {
    SEC_GET_STMT(BinaryOperator)

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pBinaryOperator, "BinOp");
}

// ??????????????????
void ASTBuilder::enterDeclaration(CSTNode *node) {
    DeclStmt *declStmt = new DeclStmt();
    nodeStack.push(declStmt);
}

void ASTBuilder::quitDeclaration(CSTNode *node) {
    SEC_GET_STMT(DeclStmt);

    if(typeDeclCp) {
        pDeclStmt->setTypeDecl(typeDeclCp);
        typeDeclCp = nullptr;
    }

    AbstractASTNode *parent = nodeStack.top();
    // ????????????????????????????????????????????????
    declTypeCp = QualType();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_TranslationUnitDecl) {
        dynamic_cast<TranslationUnitDecl*>(parent)->addStmt(pDeclStmt);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CompoundStmt) {
        dynamic_cast<CompoundStmt*>(parent)->addStmt(pDeclStmt);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ForStmt) {
        dynamic_cast<ForStmt*>(parent)->setInit(pDeclStmt);
        return;
    }

    raiseRuleViolation("Declaration", parent);
}

// ?????????????????????
void ASTBuilder::enterRecordDeclaration(CSTNode *node) {
    RecordDecl *recordDecl = new RecordDecl();
    // ???tag
    if(node->getChildren()[1]->getType() == "IDENTIFIER") {
        assert(node->getChildren()[1]->isTerminal());
        recordDecl->setName(node->getChildren()[1]->getId());
    }

    // ??????recordType
    if(node->getChildren()[0]->getChildren()[0]->getType() == "STRUCT")
        recordDecl->setStruct();
    else if(node->getChildren()[0]->getChildren()[0]->getType() == "UNION")
        recordDecl->setUnion();
    else
        assert(0 && "invalid record type");

    nodeStack.push(recordDecl);
}

void ASTBuilder::quitRecordDeclaration(CSTNode *node) {
    SEC_GET_DECL(RecordDecl)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isQualType()) {
        UnknownType * unknownType = new UnknownType(pRecordDecl->getName());
        dynamic_cast<QualType*>(parent)->setType(unknownType);
        // ????????????????????????
        // size??????2?????????:
        // struct_or_union_specifier -> struct_or_union IDENTIFIER
        if(node->getChildren().size() > 2)
            typeDeclCp = pRecordDecl;
        return;
    }

    raiseRuleViolation("RecordDeclaration", parent);
}

void ASTBuilder::enterDeclarationInRecord(CSTNode *node) {
    DeclStmt *declStmt = new DeclStmt();
    nodeStack.push(declStmt);
}

void ASTBuilder::quitDeclarationInRecord(CSTNode *node) {
    SEC_GET_STMT(DeclStmt);

    if(typeDeclCp) {
        pDeclStmt->setTypeDecl(typeDeclCp);
        typeDeclCp = nullptr;
    }

    AbstractASTNode *parent = nodeStack.top();
    // ????????????????????????????????????????????????
    declTypeCp = QualType();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_RecordDecl) {
        dynamic_cast<RecordDecl*>(parent)->addDeclStmt(pDeclStmt);
        return;
    }

    raiseRuleViolation("DeclarationInRecord", parent);
}

// ?????????????????????
void ASTBuilder::enterEnumDeclaration(CSTNode *node) {
    EnumDecl *enumDecl = new EnumDecl();
    // ???tag
    if(node->getChildren()[1]->getType() == "IDENTIFIER") {
        assert(node->getChildren()[1]->isTerminal());
        enumDecl->setName(node->getChildren()[1]->getId());
    }

    nodeStack.push(enumDecl);
}

void ASTBuilder::quitEnumDeclaration(CSTNode *node) {
    SEC_GET_DECL(EnumDecl)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isQualType()) {
        UnknownType * unknownType = new UnknownType(pEnumDecl->getName());
        dynamic_cast<QualType*>(parent)->setType(unknownType);
        // ????????????????????????
        // size??????2?????????:
        // enum_specifier -> ENUM IDENTIFIER
        if(node->getChildren().size() > 2)
            typeDeclCp = pEnumDecl;
        return;
    }

    raiseRuleViolation("EnumDeclaration", parent);
}

// ????????????????????????enter?????????????????????????????????????????????
void ASTBuilder::enterDeclarationInEnum(CSTNode *node) {
    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_EnumDecl) {
        assert(node->getChildren()[0]->getType() == "IDENTIFIER");
        dynamic_cast<EnumDecl*>(parent)->addEnumerator(node->getChildren()[0]->getId(), nullptr);
        return;
    }

    raiseRuleViolation("DeclarationInEnum", parent);
}

void ASTBuilder::quitDeclarationInEnum(CSTNode *node) {}

// ??????????????????(Int, Bool???)?????????
void ASTBuilder::enterCommonSpName(CSTNode *node) {}

void ASTBuilder::quitCommonSpName(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pnode = node->getChildren()[0];
    assert(pnode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isQualType()) {
        if (pnode->getType() == "VOID") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_void));
            return;
        }
        if (pnode->getType() == "INT") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_int));
            return;
        }
        if (pnode->getType() == "UNSIGNED") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_unsigned));
            return;
        }
        if (pnode->getType() == "CHAR") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_char));
            return;
        }
        if (pnode->getType() == "BOOL") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_bool));
            return;
        }
        if (pnode->getType() == "SHORT") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_short));
            return;
        }
        if (pnode->getType() == "LONG") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_long));
            return;
        }
        if (pnode->getType() == "FLOAT") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_float));
            return;
        }
        if (pnode->getType() == "DOUBLE") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_double));
            return;
        }
        if (pnode->getType() == "PORT") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_port));
            return;
        }
        std::string a;
    }

    raiseRuleViolation("CommonSpName", parent);
}

// ????????????Qualifier
void ASTBuilder::enterQfName(CSTNode *node) {}

void ASTBuilder::quitQfName(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isQualType()) {
        if (pNode->getType() == "CONST") {
            dynamic_cast<QualType*>(parent)->setConst();
            return;
        }
        if (pNode->getType() == "VOLATILE") {
            dynamic_cast<QualType*>(parent)->setVolatile();
            return;
        }
        if (pNode->getType() == "RESTRICT") {
            dynamic_cast<QualType*>(parent)->setRestrict();
            return;
        }
        if (pNode->getType() == "ATOMIC") {
            dynamic_cast<QualType*>(parent)->setAtomic();
            return;
        }
    }

    raiseRuleViolation("QfName", parent);
}

// ??????????????????(????????????)???????????????
//Fixme:
// ??????????????????????????????
void ASTBuilder::enterTypeSp(CSTNode *node) {
    QualType *qualType = new QualType();
    nodeStack.push(qualType);
}

void ASTBuilder::quitTypeSp(CSTNode *node) {
    SEC_GET_QAULTYPE()

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DeclStmt) {
        // ????????????????????????????????????????????????
        declTypeCp = *pQualType;
        return;
    }
    if(parent->isQualType()) {
        dynamic_cast<QualType*>(parent)->setType(pQualType->getType());
        return;
    }

    raiseRuleViolation("TypeSp", parent);
}

// ????????????Qualifier
//Fixme:
// ??????????????????????????????
void ASTBuilder::enterTypeQf(CSTNode *node) {
    QualType *qualType = new QualType();
    nodeStack.push(qualType);
}

void ASTBuilder::quitTypeQf(CSTNode *node) {
    SEC_GET_QAULTYPE()

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DeclStmt) {
        // ????????????????????????????????????????????????
        declTypeCp = *pQualType;
        return;
    }
    if(parent->isQualType()) {
        if(pQualType->isConst())
            dynamic_cast<QualType*>(parent)->setConst();
        if(pQualType->isVolatile())
            dynamic_cast<QualType*>(parent)->setVolatile();
        if(pQualType->isRestrict())
            dynamic_cast<QualType*>(parent)->setRestrict();
        if(pQualType->isAtomic())
            dynamic_cast<QualType*>(parent)->setAtomic();
        return;
    }

    raiseRuleViolation("TypeQf", parent);
}

// ????????????????????????????????????????????????
// ??????????????????
void ASTBuilder::enterDeclarator(CSTNode *node) {
    VarDecl *varDecl = new VarDecl();
    nodeStack.push(varDecl);
}

void ASTBuilder::quitDeclarator(CSTNode *node) {
    SEC_GET_DECL(VarDecl);
    // ?????????????????????
    // ????????????????????????????????????
    if(pVarDecl->getQualType().isUncertainType())
        pVarDecl->setQualType(declTypeCp);

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DeclStmt) {
        dynamic_cast<DeclStmt*>(parent)->addDecl(pVarDecl);
        return;
    }

    raiseRuleViolation("Declarator", parent);
}

// ??????????????????????????????????????????????????????
void ASTBuilder::enterDeclaratorName(CSTNode *node) {}

void ASTBuilder::quitDeclaratorName(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setName(pNode->getId());
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setName(pNode->getId());
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        dynamic_cast<ParamVarDecl*>(parent)->setName(pNode->getId());
        return;
    }
    if(parent->isQualType() &&
            Type::first_ArrayType <= dynamic_cast<QualType*>(parent)->getTypeKind() &&
            dynamic_cast<QualType*>(parent)->getTypeKind() <= Type::last_ArrayType) {
        hasStoreName = true;
        storedName = pNode->getId();
        return;
    }
    raiseRuleViolation("DeclaratorName", parent);
}

// ????????????????????????
// ????????????????????????
// ???????????????????????????????????????????????????

//Fixme:
// ??????????????????????????????????????????
// e.g. int a[const 10]
void ASTBuilder::enterArray(CSTNode *node) {
    QualType *qualType = new QualType();
    VariableArrayType *tempType = new VariableArrayType();
    tempType->setElementType(declTypeCp.getType());
    qualType->setType(tempType);
    nodeStack.push(qualType);
}

void ASTBuilder::quitArray(CSTNode *node) {
    SEC_GET_QAULTYPE();

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        //Fixme:
        // ????????????????????????
        // ????????????????????????
        // ????????????????????????????????????
        QualType oldQualType = dynamic_cast<VarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        oldQualType.setType(newType);
        dynamic_cast<VarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        QualType oldQualType = dynamic_cast<ParamVarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        oldQualType.setType(newType);
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isQualType() && dynamic_cast<QualType*>(parent)->getTypeKind() == Type::k_VariableArrayType) {
        // ?????????????????????incompleteType
        Type *T = dynamic_cast<QualType*>(parent)->getType();
        dynamic_cast<VariableArrayType*>(T)->setElementType(pQualType->getType());
        return;
    }

    raiseRuleViolation("Array", parent);
}

// ??????static????????????
// ???????????????????????????
void ASTBuilder::enterStaticModifierArray(CSTNode *) {
    QualType *qualType = new QualType();
    VariableArrayType *tempType = new VariableArrayType();
    tempType->setStatic();
    tempType->setElementType(declTypeCp.getType());
    qualType->setType(tempType);
    nodeStack.push(qualType);
}

void ASTBuilder::quitStaticModifierArray(CSTNode *node) {
    SEC_GET_QAULTYPE();

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        QualType oldQualType = dynamic_cast<VarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        oldQualType.setType(newType);
        dynamic_cast<VarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        QualType oldQualType = dynamic_cast<ParamVarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        oldQualType.setType(newType);
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isQualType() && dynamic_cast<QualType*>(parent)->getTypeKind() == Type::k_VariableArrayType) {
        // ?????????????????????incompleteType
        Type *T = dynamic_cast<QualType*>(parent)->getType();
        dynamic_cast<VariableArrayType*>(T)->setElementType(pQualType->getType());
        return;
    }

    raiseRuleViolation("StaticArray", parent);
}

// ??????Incomplete????????????
// ???????????????????????????
void ASTBuilder::enterIncompleteArray(CSTNode *) {
    QualType *qualType = new QualType();
    IncompleteArrayType *tempType = new IncompleteArrayType();
    tempType->setElementType(declTypeCp.getType());
    qualType->setType(tempType);
    nodeStack.push(qualType);
}

void ASTBuilder::quitIncompleteArray(CSTNode *node) {
    SEC_GET_QAULTYPE();

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        QualType oldQualType = dynamic_cast<VarDecl*>(parent)->getQualType();
        IncompleteArrayType *newType = dynamic_cast<IncompleteArrayType*>(pQualType->getType());
        assert(newType);
        oldQualType.setType(newType);
        dynamic_cast<VarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        QualType oldQualType = dynamic_cast<ParamVarDecl*>(parent)->getQualType();
        IncompleteArrayType *newType = dynamic_cast<IncompleteArrayType*>(pQualType->getType());
        assert(newType);
        oldQualType.setType(newType);
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }

    raiseRuleViolation("IncompleteArray", parent);
}

// ??????????????????
void ASTBuilder::enterPointer(CSTNode *) {
    AbstractASTNode *parent = nodeStack.top();

    if(auto p = dynamic_cast<ParamVarDecl*>(parent)) {
        PointerType *newType = new PointerType();
        QualType *pN = new QualType(p->getQualType());
        newType->setPointeeType(pN);
        p->setQualType(QualType(newType));
        return;
    }
    else if(auto p = dynamic_cast<FunctionDecl*>(parent)) {
        PointerType *newType = new PointerType();
        QualType *pN = new QualType(p->getQualType());
        newType->setPointeeType(pN);
        p->setQualType(QualType(newType));
        return;
    }

    // DeclStmt
    PointerType *newType = new PointerType();
    QualType *pDeclTypeCp = new QualType(declTypeCp);
    newType->setPointeeType(pDeclTypeCp);
    declTypeCp = QualType(newType);
}

void ASTBuilder::quitPointer(CSTNode *node) {}

// ????????????????????????
void ASTBuilder::enterFormalParamDef(CSTNode *node) {
    ParamVarDecl *paramVarDecl = new ParamVarDecl();
    nodeStack.push(paramVarDecl);
}

void ASTBuilder::quitFormalParamDef(CSTNode *node) {
    SEC_GET_DECL(ParamVarDecl)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->addParam(pParamVarDecl);
        return;
    }

    raiseRuleViolation("FormalParamDef", parent);
}

// ??????????????????????????????????????????
void ASTBuilder::enterFullType(CSTNode *node) {
    QualType *qualType = new QualType();
    nodeStack.push(qualType);
}

void ASTBuilder::quitFullType(CSTNode *node) {
    SEC_GET_QAULTYPE()

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CastExpr) {
        nodeStack.pop();
        ExplicitCastExpr *explicitCastExpr = new ExplicitCastExpr(*pQualType,
                                                                  dynamic_cast<CastExpr*>(parent)->getCastedExpr());
        delete parent;
        nodeStack.push(explicitCastExpr);
        return;
    }

    raiseRuleViolation("FullType", parent);
}

/*// ???????????????????????????????????????
void ASTBuilder::enterSingleInit(CSTNode *node) {
    Expr *expr = new Expr();
    nodeStack.push(expr);
}

void ASTBuilder::quitSingleInit(CSTNode *node) {
    SEC_GET_STMT_RANGE(Expr);

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setInitializer(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ExplicitCastExpr) {
        dynamic_cast<CastExpr*>(parent)->setCastedExpr(pExpr);
        return;
    }

    raiseRuleViolation("SingleInit", parent);
}*/

// ????????????Stmt??????
// ????????????????????????????????????
void ASTBuilder::enterVirtualStmt(CSTNode *node) {
    VirtualStmt *virtualStmt = new VirtualStmt();
    nodeStack.push(virtualStmt);
}

void ASTBuilder::quitVirtualStmt(CSTNode *node) {
    SEC_GET_STMT(VirtualStmt)
    assert(pVirtualStmt->hasRealStmt());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CompoundStmt) {
        dynamic_cast<CompoundStmt*>(parent)->addStmt(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_IfStmt) {
        if(!dynamic_cast<IfStmt*>(parent)->hasThen())
            dynamic_cast<IfStmt*>(parent)->setThen(pVirtualStmt->getRealStmt());
        else
            dynamic_cast<IfStmt*>(parent)->setElse(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DoWhileStmt) {
        dynamic_cast<WhileStmt*>(parent)->setBody(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_WhileStmt) {
        dynamic_cast<WhileStmt*>(parent)->setBody(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ForStmt) {
        dynamic_cast<ForStmt*>(parent)->setBody(pVirtualStmt->getRealStmt());
        return;
    }

    raiseRuleViolation("VirtualStmt", parent);
}

// ??????????????????
void ASTBuilder::enterCmpdStmt(CSTNode *node) {
    CompoundStmt *compoundStmt = new CompoundStmt();
    nodeStack.push(compoundStmt);
}

void ASTBuilder::quitCmpdStmt(CSTNode *node) {
    SEC_GET_STMT(CompoundStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setBody(pCompoundStmt);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pCompoundStmt);
        return;
    }

    raiseRuleViolation("CmpdStmt", parent);
}

// ??????If??????
// ??????????????????IF???IF_ELSE??????
// ?????????????????????????????????????????????-?????????????????????????????????
void ASTBuilder::enterIfStmt(CSTNode *node) {
    IfStmt *ifStmt = new IfStmt();
    nodeStack.push(ifStmt);
}

void ASTBuilder::quitIfStmt(CSTNode *node) {
    SEC_GET_STMT(IfStmt)
    assert(pIfStmt->hasThen());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pIfStmt);
        return;
    }

    raiseRuleViolation("IfStmt", parent);
}

// ??????If-Else??????
// ??????????????????IF???IF_ELSE??????
// ?????????????????????????????????????????????-?????????????????????????????????
void ASTBuilder::enterIfElseStmt(CSTNode *node) {
    IfStmt *ifStmt = new IfStmt();
    nodeStack.push(ifStmt);
}

void ASTBuilder::quitIfElseStmt(CSTNode *node) {
    SEC_GET_STMT(IfStmt)
    assert(pIfStmt->hasElse());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pIfStmt);
        return;
    }

    raiseRuleViolation("IfElseStmt", parent);
}

// ??????While??????
// ??????????????????WHILE???DO_WHILE??????
// ????????????????????????
void ASTBuilder::enterDoWhileStmt(CSTNode *node) {
    DoWhileStmt *doWhileStmt = new DoWhileStmt();
    nodeStack.push(doWhileStmt);
}

void ASTBuilder::quitDoWhileStmt(CSTNode *node) {
    SEC_GET_STMT(DoWhileStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pDoWhileStmt);
        return;
    }

    raiseRuleViolation("DoWhileStmt", parent);
}

// ??????While??????
// ??????????????????WHILE???DO_WHILE??????
// ??????????????????????????????
void ASTBuilder::enterWhileStmt(CSTNode *node) {
    WhileStmt *whileStmt = new WhileStmt();
    nodeStack.push(whileStmt);
}

void ASTBuilder::quitWhileStmt(CSTNode *node) {
    SEC_GET_STMT(WhileStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pWhileStmt);
        return;
    }

    raiseRuleViolation("WhileStmt", parent);
}

// ??????For??????
void ASTBuilder::enterForStmt(CSTNode *node) {
    ForStmt *forStmt = new ForStmt();
    nodeStack.push(forStmt);
}

void ASTBuilder::quitForStmt(CSTNode *node) {
    SEC_GET_STMT(ForStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pForStmt);
        return;
    }

    raiseRuleViolation("ForStmt", parent);
}

// ??????????????????
void ASTBuilder::enterReturn( CSTNode* node ) {
    ReturnStmt *returnStmt = new ReturnStmt();
    nodeStack.push(returnStmt);
}

void ASTBuilder::quitReturn( CSTNode* node ) {
    SEC_GET_STMT(ReturnStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pReturnStmt);
        return;
    }

    raiseRuleViolation("Return", parent);
}

// ??????????????????
void ASTBuilder::enterFuncDef( CSTNode* node ) {
    FunctionDecl *functionDecl = new FunctionDecl("__NULL", QualType());
    nodeStack.push(functionDecl);
}

void ASTBuilder::quitFuncDef( CSTNode* node ) {
    SEC_GET_DECL(FunctionDecl)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_TranslationUnitDecl) {
        dynamic_cast<TranslationUnitDecl*>(parent)->addDecl(pFunctionDecl);
        return;
    }

    raiseRuleViolation("FuncDef", parent);
}

void ASTBuilder::exprCommonAction(AbstractASTNode *parent, Expr *pExpr, std::string ctx) {
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CallExpr) {
        dynamic_cast<CallExpr*>(parent)->addArg(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ExplicitCastExpr) {
        dynamic_cast<CastExpr*>(parent)->setCastedExpr(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_UnaryOperator) {
        dynamic_cast<UnaryOperator*>(parent)->setSubExpr(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_BinaryOperator) {
        auto pParent = dynamic_cast<BinaryOperator*>(parent);
        if(!pParent->hasLHS()) {
            pParent->setLHS(pExpr);
        }
        else if(!pParent->hasRHS()) {
            pParent->setRHS(pExpr);
        }
        else raiseRuleViolation(ctx, parent);

        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_SelectorArray) {
        auto pParent = dynamic_cast<SelectorArray*>(parent);
        if(!pParent->hasSubExpr())
            pParent->setSubExpr(pExpr);
        else {
            int numSelectors = pParent->getNumSelectors();
            auto tempSelector = pParent->getSelector(numSelectors - 1);
            if (auto tempIndexSelector = dynamic_cast<IndexSelector*>(tempSelector)) {
                tempIndexSelector->setIdxExpr(pExpr);
            }
        }

        return;
    }
    if(parent->isQualType() && dynamic_cast<QualType*>(parent)->getTypeKind() == Type::k_VariableArrayType) {
        if(VariableArrayType* tempType =
                dynamic_cast<VariableArrayType*>(dynamic_cast<QualType*>(parent)->getType()))
            tempType->setSizeExpr(pExpr);
        return;
    }
    // ?????????????????????
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_IfStmt) {
        dynamic_cast<IfStmt*>(parent)->setCond(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DoWhileStmt) {
        dynamic_cast<DoWhileStmt*>(parent)->setCond(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_WhileStmt) {
        dynamic_cast<WhileStmt*>(parent)->setCond(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ForStmt) {
        if(!dynamic_cast<ForStmt*>(parent)->hasCond())
            dynamic_cast<ForStmt*>(parent)->setCond(pExpr);
        else if(!dynamic_cast<ForStmt*>(parent)->hasInc())
            dynamic_cast<ForStmt*>(parent)->setInc(pExpr);
        else std::cout << "Error in IfStmt ( attempt to fill when both condition and Inc are filled )." << std::endl;
        return;
    }
    // ??????????????????
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ReturnStmt) {
        dynamic_cast<ReturnStmt*>(parent)->setRetValue(pExpr);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setInitializer(pExpr);
        return;
    }
    // ???????????????????????????
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_EnumDecl) {
        int numEnum = dynamic_cast<EnumDecl*>(parent)->getNumEnumerators();
        dynamic_cast<EnumDecl*>(parent)->setEnumeratorExpr(numEnum - 1, pExpr);
        return;
    }

    raiseRuleViolation(ctx, parent);
}

void raiseRuleViolation( std::string ctx, AbstractASTNode* parent ) {
    std::cout << "Error in " << ctx << " ( Parent is ";
    assert(!parent->isQualType());
    std::cout << (parent->isDecl() ? "Decl:" : "Stmt:") << (parent->isDecl() ?
                    static_cast<short>(dynamic_cast<Decl*>(parent)->getKind()) :
                    static_cast<short>(dynamic_cast<Stmt*>(parent)->getKind())) << " )." << std::endl;
}