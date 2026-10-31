// Headless script for THOTD.EXE: creates the functions Ghidra's analysis missed.
// 1. Every relocated dword in the data section that points into BEGTEXT is a code pointer (function tables, callbacks):
//    disassemble there and create a function.
// 2. Then repeatedly: the first undefined byte after each function (skipping int3/nop/zero padding) that disassembles
//    cleanly starts another function (Watcom places functions back to back).
// Afterwards the auto-analysis runs again over the changes.
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.reloc.*;
import java.util.*;

public class FindFunctionsPc extends GhidraScript {
    Address codeStart, codeEnd;

    boolean inCode(Address a) { return a.compareTo(codeStart) >= 0 && a.compareTo(codeEnd) < 0; }

    boolean tryCreate(Address a) {
        Listing l = currentProgram.getListing();
        if (getFunctionContaining(a) != null) return false;
        if (l.getInstructionContaining(a) != null && l.getInstructionAt(a) == null) return false;   // middle of an instruction
        if (l.getInstructionAt(a) == null) {
            DisassembleCommand dc = new DisassembleCommand(a, null, true);
            if (!dc.applyTo(currentProgram, monitor) || l.getInstructionAt(a) == null) return false;
        }
        CreateFunctionCmd cf = new CreateFunctionCmd(a);
        return cf.applyTo(currentProgram, monitor);
    }

    // Watcom alignment filler between functions: int3 / nop / zero bytes, "mov eax,eax" (8B C0),
    // "lea eax,[eax+0]" (8D 40 00), "lea eax,[eax+0]" with a 32-bit displacement (8D 80 00 00 00 00)
    int padding(Memory mem, Address p) throws Exception {
        int b0 = mem.getByte(p) & 0xFF;
        if (b0 == 0xCC || b0 == 0x90 || b0 == 0x00) return 1;
        byte[] b = new byte[6];
        try { mem.getBytes(p, b); } catch (MemoryAccessException e) { return 0; }
        if (b0 == 0x8B && (b[1] & 0xFF) == 0xC0) return 2;
        if (b0 == 0x8D && (b[1] & 0xFF) == 0x40 && b[2] == 0) return 3;
        if (b0 == 0x8D && (b[1] & 0xFF) == 0x80 && b[2] == 0 && b[3] == 0 && b[4] == 0 && b[5] == 0) return 6;
        return 0;
    }

    @Override
    public void run() throws Exception {
        MemoryBlock text = currentProgram.getMemory().getBlock("BEGTEXT");
        codeStart = text.getStart();
        codeEnd = text.getEnd().add(1);
        Memory mem = currentProgram.getMemory();
        int fromRelocs = 0, fromGaps = 0;
        Iterator<Relocation> it = currentProgram.getRelocationTable().getRelocations();
        List<Address> targets = new ArrayList<>();
        while (it.hasNext()) {
            Relocation r = it.next();
            Address at = r.getAddress();
            if (inCode(at)) continue;   // pointers inside code are operands (or switch tables, which analysis handles)
            try {
                Address t = toAddr(mem.getInt(at) & 0xFFFFFFFFL);
                if (inCode(t)) targets.add(t);
            } catch (MemoryAccessException e) { }
        }
        for (Address t : targets) if (tryCreate(t)) fromRelocs++;
        println("functions from data code pointers: " + fromRelocs + " (of " + targets.size() + " pointers)");
        for (int pass = 0; pass < 1000; ++pass) {
            int made = 0;
            List<Address> ends = new ArrayList<>();
            for (Function f : currentProgram.getFunctionManager().getFunctions(true))
                if (inCode(f.getEntryPoint())) ends.add(f.getBody().getMaxAddress().add(1));
            for (Address a : ends) {
                Address p = a;
                while (inCode(p) && getFunctionContaining(p) == null) {
                    int n = padding(mem, p);
                    if (n == 0) break;
                    p = p.add(n);
                }
                if (inCode(p) && getFunctionContaining(p) == null && currentProgram.getListing().getDefinedDataAt(p) == null
                    && tryCreate(p)) made++;
            }
            fromGaps += made;
            println("pass " + pass + ": " + made + " functions after gaps");
            if (made == 0) break;
        }
        println("functions from gaps: " + fromGaps + ", total " + currentProgram.getFunctionManager().getFunctionCount());
        analyzeChanges(currentProgram);
        // Watcom register convention for everything except Windows callbacks (names with @N are __stdcall) and
        // thunks; parameters are cleared so the decompiler infers them from the registers the function reads.
        int set = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.isThunk() || f.isExternal() || f.getName().contains("@")) continue;
            f.setCustomVariableStorage(false);
            f.replaceParameters(new java.util.ArrayList<Variable>(), Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
                                true, ghidra.program.model.symbol.SourceType.DEFAULT);
            f.setCallingConvention("__watcall");
            ++set;
        }
        println("__watcall set on " + set + " functions");
    }
}
