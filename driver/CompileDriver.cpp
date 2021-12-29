#include <iostream>
#include <vector>

#include "include/Yacc/Grammar.h"
#include "include/Yacc/GrammarParser.h"
#include "include/Yacc/Constructor.h"
#include "include/Parser/CST.h"
#include "include/Parser/CSTBuilder.h"
#include "include/AST/ASTBuilder.h"
#include "include/ASTConsumers/ASTDumper.h"
#include "include/ASTConsumers/FillType.h"
#include "include/ASTConsumers/FillReference.h"
#include "include/ASTConsumers/CalculateConstant.h"
#include "include/ASTConsumers/ASTTypeCheck.h"
#include "include/Lexer/Lexer.h"
#include "include/ASTConsumers/IRGenerator.h"

#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/SourceMgr.h"

int main(int Argc, const char **Argv) {
    llvm::InitLLVM X(Argc, Argv);

    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

//    llvm::InitializeNativeTarget();
//    llvm::InitializeNativeTargetAsmPrinter();
//    llvm::InitializeNativeTargetAsmParser();

    GrammarSet grammarSet;
    parseProducer(grammarSet, "../etc/yacc_c99.y");
    LALRconstructor lalrConstructor(&grammarSet, 89, 68, 400);

//    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    auto targetTriple = "mips-unknown-linux-gnu";

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);

    if (!Target) {
        llvm::errs() << Error;
        return 1;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto targetMachine =
            Target->createTargetMachine(targetTriple, CPU, Features, opt, RM);

    auto Filename = "../output/out.s";
    std::error_code EC;
    llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message();
        return 1;
    }

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_AssemblyFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        llvm::errs() << "TheTargetMachine can't emit a file of this type";
        return 1;
    }

    for (int i = 0;i < 1;++i) {
        Lexer lexer;
        std::vector<LexUnit> lexVec = lexer.getAnalysis("../input/source.txt");

        CSTBuilder cstBuilder(&lalrConstructor, &grammarSet);
        CSTNode *cst = cstBuilder.constructCST(lexVec, 0);
        if(cst == NULL) {
            std::cout << "create CST failed";
            return 0;
        }

        ASTBuilder astBuilder;
        TranslationUnitDecl* ast = astBuilder.constructAST(cst);
        if(ast == NULL) {
            std::cout << "create CST failed";
            return 0;
        }

        FillReference fillReference;
        fillReference.traverseTranslationUnitDecl(ast);
        FillType fillType;
        fillType.traverseTranslationUnitDecl(ast);
        CalculateConstant calculateConstant;
        calculateConstant.traverseTranslationUnitDecl(ast);
        ASTTypeCheck astTypeCheck;
        astTypeCheck.traverseTranslationUnitDecl(ast);

        ASTDumper astDumper;
        astDumper.traverseTranslationUnitDecl(ast);

        IRGenerator irGenerator(targetTriple, targetMachine->createDataLayout(), "../etc/source.txt");
        irGenerator.emitLLVMIR(ast);

        pass.run(*irGenerator.getModule());
        dest.flush();

        deleteCST(cst);
    }
}