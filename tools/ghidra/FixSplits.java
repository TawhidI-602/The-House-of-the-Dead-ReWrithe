// Headless pre-script: folds Ghidra "split" functions back into the function they belong to.
// A function G is a split of F when nothing calls G or points at it (no call or data reference to its entry), and the
// code of F runs into it (and its address is not stored anywhere as a 32-bit word, e.g. a task routine): the instruction right before G's entry falls through into it, or a branch inside F jumps
// to it. G is removed and F's body is recomputed from its entry (so the decompiled C of F is complete again).
// Repeats until nothing changes. Writes the merged pairs to <out>.tsv (parent, split) when an output prefix is given.
// Usage: -preScript FixSplits.java <out prefix> [dry]   (dry: only list the splits)
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class FixSplits extends GhidraScript {
    Set<Long> words = new HashSet<>();   // every aligned 32-bit value in memory: a split's address never appears as a pointer

    Function splitParent(Function g) {
        Listing l = currentProgram.getListing();
        Address e = g.getEntryPoint();
        if (words.contains(e.getOffset())) return null;
        Function viaJump = null;
        for (Reference r : currentProgram.getReferenceManager().getReferencesTo(e)) {
            RefType t = r.getReferenceType();
            if (t.isCall() || t.isData() || !t.isFlow()) return null;
            Function f = getFunctionContaining(r.getFromAddress());
            if (f == null || f == g || f.isThunk()) return null;
            // a jump back to code before the function's entry is shared code (e.g. a Watcom tail merge), not a split
            if (e.compareTo(f.getEntryPoint()) < 0) return null;
            if (viaJump != null && viaJump != f) return null;
            viaJump = f;
        }
        Instruction prev = l.getInstructionBefore(e);
        if (prev != null && e.equals(prev.getFallThrough())) {
            Function f = getFunctionContaining(prev.getAddress());
            if (f != null && f != g && !f.isThunk() && (viaJump == null || viaJump == f)) return f;
        }
        return viaJump;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        boolean dry = args.length > 1 && args[1].equals("dry");
        FunctionManager fm = currentProgram.getFunctionManager();
        for (ghidra.program.model.mem.MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            if (!b.isInitialized()) continue;
            byte[] buf = new byte[(int) Math.min(b.getSize(), 64L << 20)];
            b.getBytes(b.getStart(), buf);
            int step = currentProgram.getLanguage().getProcessor().toString().contains("x86") ? 1 : 4;   // x86: unaligned
            for (int i = 0; i + 4 <= buf.length; i += step)
                words.add((buf[i] & 0xFFL) | (buf[i + 1] & 0xFFL) << 8 | (buf[i + 2] & 0xFFL) << 16 | (buf[i + 3] & 0xFFL) << 24);
        }
        List<String> merged = new ArrayList<>();
        for (int pass = 0; pass < 50; ++pass) {
            int n = 0;
            List<Function> all = new ArrayList<>();
            for (Function f : fm.getFunctions(true)) all.add(f);
            for (Function g : all) {
                if (fm.getFunctionAt(g.getEntryPoint()) == null || g.isThunk() || g.isExternal()) continue;
                Function f = splitParent(g);
                if (f == null) continue;
                String line = f.getEntryPoint() + "\t" + f.getName() + "\t" + g.getEntryPoint() + "\t" + g.getName();
                merged.add(line);
                ++n;
                if (dry) continue;
                Address fe = f.getEntryPoint();
                fm.removeFunction(g.getEntryPoint());
                Function f2 = fm.getFunctionAt(fe);
                if (f2 != null) CreateFunctionCmd.fixupFunctionBody(currentProgram, f2, monitor);
            }
            if (n == 0 || dry) break;
        }
        println("splits merged: " + merged.size());
        if (args.length > 0 && (dry || !merged.isEmpty())) {
            File out = new File(args[0] + ".tsv");
            boolean fresh = dry || !out.exists();
            try (PrintWriter w = new PrintWriter(new FileWriter(out, !fresh))) {   // appends: a log of every merge done
                if (fresh) w.println("parent\tparent_name\tsplit\tsplit_name");
                for (String s : merged) w.println(s);
            }
        }
    }
}
