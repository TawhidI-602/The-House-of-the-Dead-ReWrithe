// Headless script for the arcade program: creates functions at every target of a "call" (CTRL 0x09) or absolute
// "callx" (MEMB mode 12) word in maincpu, then walks the functions packed back to back (zero words between them),
// like FindFunctionsPc.java. Also code pointers stored in tables (see below).
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class FindFunctionsArcade extends GhidraScript {
    static final long CODE_END = 0x200000;

    boolean tryCreate(Address a) {
        Listing l = currentProgram.getListing();
        if (getFunctionContaining(a) != null) return false;
        if (l.getInstructionContaining(a) != null && l.getInstructionAt(a) == null) return false;
        if (l.getInstructionAt(a) == null) {
            DisassembleCommand dc = new DisassembleCommand(a, null, true);
            if (!dc.applyTo(currentProgram, monitor) || l.getInstructionAt(a) == null) return false;
        }
        return new CreateFunctionCmd(a).applyTo(currentProgram, monitor);
    }

    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Map<Long, Integer> targets = new TreeMap<>();
        for (long a = 0; a + 8 <= CODE_END; a += 4) {
            int w = mem.getInt(toAddr(a));
            int op = (w >>> 24) & 0xFF;
            long t = -1;
            if (op == 0x09) {
                int d = w & 0xFFFFFC;
                if ((d & 0x800000) != 0) d -= 0x1000000;
                t = a + d;
            } else if (op == 0x86 && ((w >> 12) & 1) == 1 && ((w >> 10) & 15) == 12) {
                t = mem.getInt(toAddr(a + 4)) & 0xFFFFFFFFL;
            }
            if (t >= 0x100 && t < CODE_END && (t & 3) == 0) targets.merge(t, 1, Integer::sum);
        }
        // code pointers in tables (opcode handlers, behaviour routines): any aligned word in maincpu or main_data whose value
        // is a code address right after a "ret" (0x0A000000) or a zero padding word
        Set<Long> pointers = new TreeSet<>();
        for (MemoryBlock b : new MemoryBlock[] { mem.getBlock("maincpu"), mem.getBlock("main_data") }) {
            for (long a = b.getStart().getOffset(); a + 4 <= b.getEnd().getOffset(); a += 4) {
                long v = mem.getInt(toAddr(a)) & 0xFFFFFFFFL;
                if (v < 0x1000 || v >= CODE_END || (v & 3) != 0) continue;
                int before = mem.getInt(toAddr(v - 4));
                if ((before == 0x0A000000 || before == 0) && mem.getInt(toAddr(v)) != 0) pointers.add(v);
            }
        }
        int made = 0;
        for (long t : targets.keySet()) if (tryCreate(toAddr(t))) made++;
        int fromPointers = 0;
        for (long t : pointers) if (tryCreate(toAddr(t))) fromPointers++;
        println("table pointers: " + pointers.size() + ", functions created: " + fromPointers);
        println("call targets: " + targets.size() + ", functions created: " + made);
        int fromGaps = 0;
        for (int pass = 0; pass < 1000; ++pass) {
            int n = 0;
            List<Address> ends = new ArrayList<>();
            for (Function f : currentProgram.getFunctionManager().getFunctions(true))
                if (f.getEntryPoint().getOffset() < CODE_END) ends.add(f.getBody().getMaxAddress().add(1));
            for (Address p : ends) {
                while (p.getOffset() < CODE_END && getFunctionContaining(p) == null && mem.getInt(p) == 0) p = p.add(4);
                if (p.getOffset() < CODE_END && getFunctionContaining(p) == null && (p.getOffset() & 3) == 0
                    && currentProgram.getListing().getDefinedDataAt(p) == null && tryCreate(p)) n++;
            }
            fromGaps += n;
            if (n == 0) break;
        }
        println("functions after gaps: " + fromGaps + ", total " + currentProgram.getFunctionManager().getFunctionCount());
        analyzeChanges(currentProgram);
    }
}
