#include <clang/Basic/Version.h> // For CLANG_VERSION_MAJOR
#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

/** Stringfy constant integer token in `s`.  */
#define STRINGFY_VALUE(s) STRINGFY(s)

/** Stringfy given expression `s`.  */
#define STRINGFY(s) #s

class RecursiveFunctionPrint
{
  public:
  RecursiveFunctionPrint(ASTUnit *ast)
    : AST(ast)
  {}

  void Print_Function_Names(void);
  void Print_Function_Names(Decl *decl);

  private:
  ASTUnit *AST;
};

int main(int argc, char **argv)
{
  if (argc != 2) {
    llvm::outs() << "Usage: " << argv[0] << " <file-to-compile>\n";
    return 1;
  }

  const std::vector<const char *> clang_args =  {
    "",
    // For some reason libtooling do not pass the clang include folder.  Pass this then.
    "-I/usr/lib64/clang/" STRINGFY_VALUE(CLANG_VERSION_MAJOR) "/include",
    "-fno-builtin",
    // Compile the file provided in argv[1].
    "-c", argv[1],
  };

  // Boilerplate code.
  DiagnosticOptions *diagopts = new DiagnosticOptions();
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags = CompilerInstance::createDiagnostics(diagopts);

  /* Create the object that holds the command line options that will be passed
     to the compiler.  */
  clang::CreateInvocationOptions CIOpts;
  CIOpts.Diags = Diags;
  std::shared_ptr<CompilerInvocation> CInvok = clang::createInvocation(clang_args, std::move(CIOpts));

  /* Create the file manager, specifying the filesystem to be used as the real
     one (i.e. the disk itself).  */
  FileManager *FileMgr = new FileManager(FileSystemOptions(), vfs::getRealFileSystem());
  std::shared_ptr<PCHContainerOperations> PCHContainerOps = std::make_shared<PCHContainerOperations>();

  /* Finally, create the ASTUnit object.  */
  std::unique_ptr<ASTUnit> AST = ASTUnit::LoadFromCompilerInvocation(
      CInvok, PCHContainerOps, Diags, FileMgr, false, CaptureDiagsKind::None, 1,
      TU_Complete, false, false, false);

  /* Check if the compilation was successful.  */
  if (AST == nullptr || AST->getDiagnostics().hasErrorOccurred()) {
    llvm::outs() << "Error building the ASTUnit.\n";
    return 1;
  }

  /* Print function names with location.  */
  RecursiveFunctionPrint(AST.get()).Print_Function_Names();

  return 0;
}

void RecursiveFunctionPrint::Print_Function_Names(void)
{
  Decl *tu = AST->getASTContext().getTranslationUnitDecl();
  Print_Function_Names(tu);
}

void RecursiveFunctionPrint::Print_Function_Names(Decl *decl)
{
  if (decl == nullptr)
    return;

  /* Check and cast the abstract Decl into a FunctionDecl.  */
  if (FunctionDecl *fdecl = dyn_cast<FunctionDecl>(decl)) {
    /* Print.  */
    llvm::outs() << fdecl->getNameAsString();
    if (fdecl->getBeginLoc().isValid()) {
      /* Get the SourceManager, which contains information about the source files.  */
      SourceManager &sm = AST->getSourceManager();
      /* Get the PresumedLoc, which contains more information about the location
         of the declaration, such as the filename, line, column.  */
      PresumedLoc ploc = sm.getPresumedLoc(fdecl->getBeginLoc());
      llvm::outs() << " at " <<
                      ploc.getFilename() << ":" << ploc.getLine() << ":" << ploc.getColumn() << '\n';
    } else {
      /* Some declarations does not have a valid source origin, like the ones
         inserted by the compiler.  */
      llvm::outs() << '\n';
    }
  }

  if (DeclContext *declctx = dyn_cast<DeclContext>(decl)) {
    for (Decl *d : declctx->decls()) {
      Print_Function_Names(d);
    }
  }
}
