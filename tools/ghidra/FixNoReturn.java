// Headless script (arcade and PC): Ghidra's non-returning-function analysis marks some ordinary functions as "no
// return" (in the arcade program e.g. rand 0x34A50), and then the code after every call to them is cut off in the
// decompiled C ("Subroutine does not return"). This clears the flag on every function that contains a return
// instruction (i960 "ret", x86 "RET"), restores the fall-through of each call to it, disassembles what follows the
// call and re-computes the bodies of the calling functions. Functions without a return (task_exit, which unwinds)
// keep the flag, and so do longjmp-like functions that load their return frame (i960 "ld ..., pfp", arcade 0x23B20:
// they return into another frame, never to the caller). Safe to run again.
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;

public class FixNoReturn extends GhidraScript {
    @Override
    public void run() throws Exception {
        Listing l = currentProgram.getListing();
        List<Function> fixed = new ArrayList<>();
        for (Function f : l.getFunctions(true)) {
            boolean ret = false, longjmp = false;
            for (Instruction i : l.getInstructions(f.getBody(), true)) {
                String m = i.getMnemonicString().toLowerCase();
                if (m.equals("ret") || m.startsWith("ret")) ret = true;
                if (m.startsWith("ld") && i.toString().toLowerCase().endsWith("pfp")) longjmp = true;
            }
            if (longjmp) {
                if (!f.hasNoReturn()) { f.setNoReturn(true); println("no-return restored: " + f.getName()); }
                continue;
            }
            if (f.hasNoReturn() && ret) { f.setNoReturn(false); fixed.add(f); }
        }
        Set<Function> callers = new HashSet<>();
        int sites = 0;
        for (Function f : fixed) {
            for (Reference r : getReferencesTo(f.getEntryPoint())) {
                if (!r.getReferenceType().isCall()) continue;
                Instruction i = l.getInstructionAt(r.getFromAddress());
                if (i == null) continue;
                i.clearFallThroughOverride();
                i.setFlowOverride(FlowOverride.NONE);
                Address next = i.getMaxAddress().next();
                if (next != null && l.getInstructionAt(next) == null && l.getDefinedDataAt(next) == null) {
                    new DisassembleCommand(next, null, true).applyTo(currentProgram, monitor);
                }
                Function c = getFunctionContaining(i.getAddress());
                if (c != null) callers.add(c);
                sites++;
            }
        }
        for (Function c : callers) CreateFunctionCmd.fixupFunctionBody(currentProgram, c, monitor);
        // the code recovered after those calls calls functions that may not exist yet: create them
        int made = 0;
        for (Instruction i : l.getInstructions(true)) {
            if (!i.getFlowType().isCall()) continue;
            for (Address t : i.getFlows()) {
                if (getFunctionAt(t) != null || !currentProgram.getMemory().contains(t)) continue;
                if (l.getInstructionAt(t) == null) {
                    if (l.getInstructionContaining(t) != null || l.getDefinedDataContaining(t) != null) continue;
                    new DisassembleCommand(t, null, true).applyTo(currentProgram, monitor);
                }
                if (l.getInstructionAt(t) != null && new CreateFunctionCmd(t).applyTo(currentProgram, monitor)) made++;
            }
        }
        println("functions created at call targets: " + made);
        StringBuilder sb = new StringBuilder();
        for (Function f : fixed) sb.append(" ").append(f.getName()).append("@").append(f.getEntryPoint());
        println("no-return cleared on " + fixed.size() + " functions (" + sites + " call sites, " + callers.size()
                + " callers):" + sb);
    }
}
