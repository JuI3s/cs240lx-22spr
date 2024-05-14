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
  public:
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

      FunctionCallee printHello =
          M.getOrInsertFunction("print_hello", printHelloType);

      // Set up types of functions in the runtime library.
      FunctionType *logMallocType = FunctionType::get(
          // return type of function
          Type::getVoidTy(context),
          // argument type of function
          {Type::getInt8PtrTy(context), Type::getInt64Ty(context)},
          // not a variadic function
          false);

      FunctionType *logFreeType = FunctionType::get(
          Type::getVoidTy(context), {Type::getInt8PtrTy(context)}, false);

      FunctionType *initType = FunctionType::get(Type::getVoidTy(context), false);

      FunctionCallee mallocFunc =
          M.getOrInsertFunction("log_malloc", logMallocType);
      FunctionCallee freeFunc = M.getOrInsertFunction("log_free", logFreeType);
      FunctionCallee loadFunc = M.getOrInsertFunction("log_load", logFreeType);
      FunctionCallee storeFunc = M.getOrInsertFunction("log_store", logFreeType);
      FunctionCallee stackFunc = M.getOrInsertFunction("log_stack", logFreeType);
      FunctionCallee initFunc = M.getOrInsertFunction("init_check", initType);
      FunctionCallee exitFunc = M.getOrInsertFunction("exit_check", initType);

      bool insertedStart = false;
      Instruction *last;

      // Loop through all functions.
      for (auto &F : M)
      {
        // Loop through all basic blocks.
        for (auto &B : F)
        {
          // Loop through all instructions.
          for (auto &I : B)
          {

            if (F.getName() == "main" && !insertedStart)
            {
              // Call the init_check function before executing any instructions
              // in main.
              std::cerr << "Setting up init..." << std::endl;
              IRBuilder<> builder(&I);
              builder.SetInsertPoint(&I);
              builder.CreateCall(initFunc, {}, "");
              // builder.CreateCall(printHello, {}, "");

              insertedStart = true;
            }

            // Check if it was a call node.
            if (CallInst *call = dyn_cast<CallInst>(&I))
            {
              // Check if callee was malloc.
              if (call->getCalledFunction()->getName() == "malloc")
              {
                // Grab pointer returned by malloc.
                Value *pointer = cast<Value>(call);
                // Get size of allocated memory in bytes.
                Value *size = call->getOperand(0);
                // Insert call to log_malloc after malloc has returned.
                IRBuilder<> builder(call);
                std::cerr << "Malloc at address: " << pointer << std::endl;

                pointer = builder.CreatePointerCast(pointer, Type::getInt8PtrTy(context));
                std::cerr << "Malloc converted to address: " << pointer << std::endl;

                builder.SetInsertPoint((&I)->getNextNode());

                builder.CreateCall(mallocFunc, {pointer, size}, "");
              }

              if (call->getCalledFunction()->getName() == "free")
              {
                // Get pointer that is being freed.
                Value *pointer = call->getOperand(0);
                // Insert call to log_free after free has returned.
                IRBuilder<> builder(call);

                std::cerr << "Free at address: " << pointer << std::endl;
                pointer = builder.CreatePointerCast(pointer, Type::getInt8PtrTy(context));
                std::cerr << "Free converted address: " << pointer << std::endl;

                builder.SetInsertPoint((&I)->getNextNode());

                builder.CreateCall(freeFunc, {pointer}, "");
              }
            }

            // Instrument Loads
            if (LoadInst *load = dyn_cast<LoadInst>(&I))
            {
              // // TODO: Get the address where the load occurs.
              Value *addr = load->getPointerOperand();

              std::cerr << "Load at address: " << addr << std::endl;

              // // TODO: Cast the address to a uint8 ptr, as expected by log_load.
              IRBuilder<>
                  builder(&I);
              addr = builder.CreatePointerCast(addr, Type::getInt8PtrTy(context));

              std::cerr << "Load converted to address: " << addr << std::endl;

              // // TODO: Insert call to log_load.
              builder.SetInsertPoint(&I);
              builder.CreateCall(loadFunc, {addr}, "");
            }

            // Instrument Stores
            if (StoreInst *store = dyn_cast<StoreInst>(&I))
            {
              // // TODO: Get the address where the store occurs.
              Value *addr = store->getPointerOperand();

              std::cerr << "Store at address: " << addr << std::endl;
              std::cerr << "Store value: " << store->getValueOperand() << std::endl;

              // // TODO: Cast the address to a uint8 ptr, as expected by log_store.
              IRBuilder<>
                  builder(&I);

              // CastInst::CreatePointerCast(addr, Type::getInt8PtrTy(context));
              addr = builder.CreatePointerCast(addr, Type::getInt8PtrTy(context));

              std::cerr << "Store converted to address: " << addr << std::endl;

              // // TODO: Insert call to log_store.
              builder.SetInsertPoint(&I);
              builder.CreateCall(storeFunc, {addr}, "");
            }

            // Keep track of stack addresses to eliminate false positives.
            if (AllocaInst *stack = dyn_cast<AllocaInst>(&I))
            {
              // TODO: Get the address where the stack gets allocated.

              // StoreInst *str = new StoreInst(0, stack, false, stack);

              // Value *addr = str->getPointerOperand();

              // Value *val = cast<Value>(stack);

              Value *addr = stack->getOperand(0);

              Value *val = cast<Value>(stack);

              // Value *addr = cast<Value>(stack);

              // TODO: Cast the address to a uint8 ptr, as expected by log_stack.
              IRBuilder<>
                  builder(&I);
              // addr = builder.CreatePointerCast(addr, Type::getInt8PtrTy(context));

              // auto val = builder.CreateLoad(stack->getAllocatedType(), stack);
              // addr = val->getPointerOperand();
              builder.SetInsertPoint(&I);

              // val = builder.CreatePtrToInt(val, Type::getInt64Ty(context));
              std::cerr << "Value: " << val << std::endl;

              std::cerr
                  << "Arr size: " << stack->getArraySize() << std::endl;
              std::cerr << "Address name space: " << stack->getAddressSpace() << std::endl;
              std::cerr << "Stack addr: " << addr << std::endl;

              addr = builder.CreatePtrToInt(addr, Type::getInt32Ty(context));
              addr = builder.CreateIntToPtr(addr, Type::getInt8PtrTy(context));

              // TODO: Insert call to log_store.
              builder.CreateCall(stackFunc, {addr}, "");
            }

            last = &I;
          }
        }
      }
      // Call the exit_check function.
      IRBuilder<> builder(last);
      builder.SetInsertPoint(last);
      builder.CreateCall(exitFunc, {}, "");
      return true;
    }

    static char ID;
  };
} // namespace

char SkeletonPass::ID = 0;
// Register the Pass. To enable this pass, need to compile with --memcheck.
static RegisterPass<SkeletonPass> X("memcheck",
                                    "enable memory corruption detection.");
