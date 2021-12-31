//
// Created by tangny on 2021/12/14.
//

#ifndef FRONTEND_IRGENERATOR_H
#define FRONTEND_IRGENERATOR_H

#include <iostream>
#include <map>

#include "include/AST/RecursiveASTVisitor.h"
#include "include/AST/Decl.h"
#include "include/AST/Stmt.h"
#include "include/AST/Expr.h"
#include "include/AST/Type.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

class IRGenerator {
private:
//    std::unique_ptr<llvm::LLVMContext> context;
//    std::unique_ptr<llvm::Module> module;
//    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::LLVMContext *context;
    llvm::Module *module;
    llvm::IRBuilder<> *builder;

    llvm::BasicBlock *curBlock;
    llvm::Function *currentFunction;

    // 记录当前函数有无Return语句
    bool hasRet;

    // 用于直接写硬件接口
    //Todo:
    // 先放在全局，如果有可能就移到局部
    std::map<std::string, llvm::TrackingVH<llvm::Value>> ports;

    struct BasicBlockInfo {
        std::map<std::string, llvm::TrackingVH<llvm::Value>> defs;

        std::map<llvm::PHINode*, DeclRefExpr* > incompletePhis;

        bool sealed = false;
    };

    std::map<llvm::BasicBlock*, BasicBlockInfo> currentBlockInfo;
    std::map<std::string, llvm::GlobalObject*> globals;

    std::map<Type*, llvm::Type*> typeCache;

private:
    llvm::Type *VoidTy;
    llvm::Type *Int1Ty;
    llvm::Type *Int8Ty;
    llvm::Type *Int16Ty;
    llvm::Type *Int32Ty;
    llvm::Type *FloatTy;
    llvm::Type *Int32PtrTy;

public:
    IRGenerator( llvm::StringRef targetTriple, llvm::DataLayout dataLayout, std::string source_name ) {
//        context = std::make_unique<llvm::LLVMContext>();
        context = new llvm::LLVMContext();
//        module = std::make_unique<llvm::Module>("tny_de_home", *context);
        module = new llvm::Module(source_name, *context);
        module->setTargetTriple(targetTriple);
        module->setDataLayout(dataLayout);
//        builder = std::make_unique<llvm::IRBuilder<>>(*context);
        builder = new llvm::IRBuilder<>(*context);

        VoidTy = llvm::Type::getVoidTy(*context);
        Int1Ty = llvm::Type::getInt1Ty(*context);
        Int8Ty = llvm::Type::getInt8Ty(*context);
        Int16Ty = llvm::Type::getInt8Ty(*context);
        Int32Ty = llvm::Type::getInt32Ty(*context);
        FloatTy = llvm::Type::getFloatTy(*context);
        Int32PtrTy = llvm::Type::getInt32PtrTy(*context);
    }

    llvm::Module* getModule() { return module; }

private:
    void writeGlobalVariable( std::string, llvm::Value* );

    llvm::Value* readGlobalVariable( DeclRefExpr* );

    void writeLocalVariable( llvm::BasicBlock*, std::string, llvm::Value* );

    llvm::Value* readLocalVariable( llvm::BasicBlock*, DeclRefExpr* );

    // 用于写入局部变量和全局变量
    // 仅可在变量赋值时使用
    // 在变量声明时必须分别使用Local和Global
    void writeVariable( llvm::BasicBlock*, std::string, llvm::Value* );

    llvm::Value* readVariable( llvm::BasicBlock*, DeclRefExpr* );

    llvm::Value* readLocalVariableRecursive( llvm::BasicBlock*, DeclRefExpr* );

    llvm::PHINode* addEmptyPhi( llvm::BasicBlock*, llvm::Type* );

    // 读写硬件端口
    llvm::Value* readPort( DeclRefExpr* );

    void writePort( std::string, llvm::Value* );

    // 添加phi表达式中的项
    void addPhiOperands( llvm::BasicBlock*, DeclRefExpr*, llvm::PHINode* );

    void sealBlock(llvm::BasicBlock *BB);

    llvm::Value* readVariable( llvm::BasicBlock*, Expr* );

    llvm::Type* convertType( QualType );

private:
    void setCurrentBlock( llvm::BasicBlock* BB ) {
        curBlock = BB;
        builder->SetInsertPoint(curBlock);
    }

    llvm::BasicBlock* createBasicBlock( std::string name ) {
        return llvm::BasicBlock::Create(*context, name, currentFunction);
    }

public:
    void emitLLVMIR(TranslationUnitDecl *translationUnitDecl, bool print) {
        int N = translationUnitDecl->getNumStmts();
        for(int i = 0;i < N;++i) {
            assert(translationUnitDecl->getStmt(i)->getKind() == Stmt::k_DeclStmt);
            emitGlobalDecl(dynamic_cast<DeclStmt*>(translationUnitDecl->getStmt(i)));
        }
        N = translationUnitDecl->getNumDecls();
        for(int i = 0;i < N;++i) {
            assert(translationUnitDecl->getDecl(i)->getKind() == Decl::k_FunctionDecl);
            emitFunctionDecl(dynamic_cast<FunctionDecl*>(translationUnitDecl->getDecl(i)));
        }
        if(print) module->print(llvm::errs(), nullptr);
    }

public:
    llvm::Function* emitFunctionDecl( FunctionDecl* );

    // 除Expr外Stmt的IR生成函数
    void emitCompoundStmt( CompoundStmt* );

    void emitGlobalDecl( DeclStmt* );

    void emitLocalDecl( DeclStmt* );

    void emitForStmt( ForStmt* );

    void emitIfStmt( IfStmt* );

    void emitWhileStmt( WhileStmt* );

    void emitDoWhileStmt( DoWhileStmt* );

    void emitReturnStmt( ReturnStmt* );

    // Expr的IR生成函数
    llvm::Value* emitBinaryOperator( BinaryOperator* );

    llvm::Value* emitUnaryOperator( UnaryOperator* );

    llvm::Value* emitSelectorArray( SelectorArray* );

    llvm::Value* emitAssignOperator( BinaryOperator* );

    llvm::Value* emitDeclRefExpr( DeclRefExpr* );

    llvm::Value* emitIntegerLiteral( IntegerLiteral* );

    llvm::Value* emitFloatingLiteral( FloatingLiteral* );

    llvm::Value* emitCallExpr( CallExpr* );

    llvm::Value* emitCastExpr( CastExpr* );

    // 对多种可能性语句的IR释放helper函数
    void emitStmtHelper( Stmt * );

    llvm::Value* emitExprHelper( Expr * );
};

#endif //FRONTEND_IRGENERATOR_H
