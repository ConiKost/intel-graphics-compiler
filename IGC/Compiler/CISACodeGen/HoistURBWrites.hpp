/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/MapVector.h>
#include <llvm/Analysis/PostDominators.h>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

class HoistURBWrites : public llvm::FunctionPass
{
    llvm::DominatorTree* DT = nullptr;
    llvm::PostDominatorTree* PDT = nullptr;;
    llvm::LoopInfo* LI = nullptr;;

public:
    static char ID; // Pass identification

    HoistURBWrites();

    virtual bool runOnFunction(llvm::Function& F) override;

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<llvm::DominatorTreeWrapperPass>();
        AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
        AU.addRequired<llvm::LoopInfoWrapperPass>();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addPreserved<llvm::DominatorTreeWrapperPass>();
        AU.addPreserved<llvm::PostDominatorTreeWrapperPass>();
        AU.addPreserved<llvm::LoopInfoWrapperPass>();
    }
    virtual llvm::StringRef getPassName() const override
    {
        return "HoistURBWrites";
    }

private:
    void gatherLastURBReadInEachBB(llvm::Function& F);
    llvm::Instruction* searchBackForAliasedURBRead(
        llvm::Instruction* urbWrite);

    void hoistURBWriteInBB(llvm::BasicBlock& BB);

    /// local processing
    bool isSafeToHoistURBWriteInstruction(
        llvm::Instruction* inst,
        llvm::Instruction*& tgtInst);

    /// data members for local-hoisting
    llvm::MapVector<llvm::Instruction*, llvm::Instruction*> instMovDataMap;
    llvm::DenseMap<llvm::BasicBlock*, llvm::Instruction*> basicBlockReadInstructionMap;
};
} // namespace IGC

