#include <clang/Basic/Version.h> // For CLANG_VERSION_MAJOR
#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;
using namespace llvm;

/** Stringfy constant integer token in `s`.  */
#define STRINGFY_VALUE(s) STRINGFY(s)

/** Stringfy given expression `s`.  */
#define STRINGFY(s) #s

static void Look_For_Symbol(ASTUnit *ast, const char *symbol);

class PrintVarDefUseVisitor : public RecursiveASTVisitor<PrintVarDefUseVisitor>
{
  public:
  PrintVarDefUseVisitor(ASTUnit *ast, const std::string &look_for)
    : RecursiveASTVisitor(),
      AST(ast),
      LookFor(look_for)
  {}

  /* For global variable declarations, as well as function parameters.  */
  bool VisitVarDecl(VarDecl *decl) {
    const std::string &name = decl->getNameAsString();
    if (name == LookFor) {
      SourceManager &sm = AST->getSourceManager();

      /* Get the PresumedLoc, which contains more information about the location
         of the declaration, such as the filename, line, column.  */
      PresumedLoc ploc = sm.getPresumedLoc(decl->getBeginLoc());

      /* Print.  */
      llvm::outs() << "Def of " << name << " at " <<
                      ploc.getFilename() << ":" << ploc.getLine() << ":" << ploc.getColumn() << '\n';
    }

    return true;
  }

  /* For the use of a variable in a function.  */
  bool VisitDeclRefExpr(DeclRefExpr *expr)
  {
    const std::string &name = expr->getNameInfo().getName().getAsString();
    if (name == LookFor) {
      SourceManager &sm = AST->getSourceManager();

      /* Get the PresumedLoc, which contains more information about the location
         of the declaration, such as the filename, line, column.  */
      PresumedLoc ploc = sm.getPresumedLoc(expr->getBeginLoc());

      /* Print.  */
      llvm::outs() << "Use of " << name << " at " <<
                      ploc.getFilename() << ":" << ploc.getLine() << ":" << ploc.getColumn() << '\n';
    }

    return true;
  }

  private:
  ASTUnit *AST;
  std::string LookFor;
};

int main(int argc, char **argv)
{
  if (argc != 3) {
    llvm::outs() << "Usage: " << argv[0] << " <file-to-compile> <variable-to-look-for>\n";
    return 1;
  }

  const std::vector<const char *> clang_args =  {
    "",
    // For some reason libtooling do not pass the clang include folder.  Pass this then.
    "-I/usr/lib64/clang/" STRINGFY_VALUE(CLANG_VERSION_MAJOR) "/include",
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
  Look_For_Symbol(AST.get(), argv[2]);

  return 0;
}

static void Look_For_Symbol(ASTUnit *ast, const char *symbol_name)
{
  // Create our visitor object.
  PrintVarDefUseVisitor Visitor(ast, symbol_name);

  // Traverse the Translation Unit.
  Visitor.TraverseDecl(ast->getASTContext().getTranslationUnitDecl());
}
