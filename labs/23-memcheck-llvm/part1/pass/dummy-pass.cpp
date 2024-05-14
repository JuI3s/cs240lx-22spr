#include <iostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace
{
  struct SkeletonPass : public ModulePass
  {
    static char ID;
    SkeletonPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M)
    {
      auto &context = M.getContext();
      // Define the arguments and return type of print_hello.
      FunctionType *printHelloType = FunctionType::get(
          // Return type of function.
          Type::getVoidTy(context),
          // No arguments.
          false);

      // TODO: Define the arguments and return type of print_with_arg.
      FunctionCallee printHello =
          M.getOrInsertFunction("print_hello", printHelloType);

      FunctionType *printWithArgType = FunctionType::get(
          Type::getVoidTy(context),
          {Type::getInt32Ty(context)},
          false);

      // TODO: Make A FunctionCallee for the print_with_arg function.
      FunctionCallee printWithArg = M.getOrInsertFunction("print_with_arg", printWithArgType);

      bool insertedHelloStart = false;
      bool insertedTestStart = false;

      // Loop through all the functions.
      for (auto &F : M)
      {
        // Loop through all the basic blocks.
        for (auto &B : F)
        {
          // Loop through all the instructions.
          for (auto &I : B)
          {

            // std::cerr << "F.getName(): " << F.getName().begin() << " is_test: " << (F.getName() == "test") << std::endl;

            if (F.getName() == "main" && !insertedHelloStart)
            {
              IRBuilder<> builder(&I);
              builder.SetInsertPoint(&I);
              builder.CreateCall(printHello, {}, "");
              // Only want to insert the call
              // at the start of main.
              insertedHelloStart = true;
            }

            if (F.getName() == "test" && !insertedTestStart)
            {
              IRBuilder<> builder(&I);
              builder.SetInsertPoint(&I);
              builder.CreateCall(printWithArg, {F.arg_begin()}, "");

              insertedTestStart = true;
            }

            std::cerr << "2xxxx" << std::endl;
            if (CallInst *call = dyn_cast<CallInst>(&I))
            {
              // Check if function being called is test.
              if (call->getCalledFunction()->getName() == "test")
              {
                auto fn = call->getCalledFunction();
                auto it = fn->arg_begin();
                for (auto a = fn->arg_begin(); a != fn->arg_end(); ++a)
                {
                  std::cerr << "got an arg" << a << std::endl;
                }
                if (it != fn->arg_end())
                  std::cerr << "got an arg" << fn << std::endl;
                else
                  std::cerr << "no arg!" << fn << std::endl;

                // TODO: Get the first argument of test.

                // TODO: Create a call to print_with_arg that also passes
                // the first argument to print_with_arg.
              }
            }
          }
        }
      }
      // Return true because we change the module.
      return true;
    }
  };
} // namespace

char SkeletonPass::ID = 0;
// Register the pass. To invoke the pass, compile using --dummy.
static RegisterPass<SkeletonPass> X("dummy", "dummy function pass");
