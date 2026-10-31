// Headless post-script: writes every function's decompiled C to <out>.c and an index to <out>_index.tsv
// (address, name, size, calling convention, callers, callees, strings referenced).
// Usage: analyzeHeadless ... -postScript ExportDecompiled.java <out prefix>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.StringDataInstance;
import java.io.*;
import java.util.*;

public class ExportDecompiled extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : "decompiled";
        DecompInterface d = new DecompInterface();
        DecompileOptions opt = new DecompileOptions();
        d.setOptions(opt);
        d.toggleCCode(true);
        d.openProgram(currentProgram);
        Listing listing = currentProgram.getListing();
        ReferenceManager refs = currentProgram.getReferenceManager();
        try (PrintWriter c = new PrintWriter(new FileWriter(out + ".c"));
             PrintWriter idx = new PrintWriter(new FileWriter(out + "_index.tsv"))) {
            idx.println("address\tname\tsize\tconvention\tcallers\tcallees\tstrings");
            int n = 0;
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                if (monitor.isCancelled()) break;
                if (f.isThunk() || f.isExternal()) continue;
                StringBuilder strs = new StringBuilder();
                for (Address a : f.getBody().getAddresses(true)) {
                    for (Reference r : refs.getReferencesFrom(a)) {
                        Data data = listing.getDataAt(r.getToAddress());
                        if (data != null && data.hasStringValue()) {
                            String s = String.valueOf(data.getValue()).replace("\t", " ").replace("\n", "\n").replace("\r", "");
                            if (s.length() > 60) s = s.substring(0, 60);
                            if (strs.length() < 400) strs.append(strs.length() > 0 ? " | " : "").append(s);
                        }
                    }
                }
                Set<String> callers = new TreeSet<>(), callees = new TreeSet<>();
                for (Function g : f.getCallingFunctions(monitor)) callers.add(g.getEntryPoint().toString());
                for (Function g : f.getCalledFunctions(monitor)) callees.add(g.getEntryPoint().toString());
                idx.println(f.getEntryPoint() + "\t" + f.getName() + "\t" + f.getBody().getNumAddresses() + "\t" +
                            f.getCallingConventionName() + "\t" + String.join(",", callers) + "\t" + String.join(",", callees) + "\t" + strs);
                DecompileResults r = d.decompileFunction(f, 60, monitor);
                c.println("// ==== " + f.getName() + " @ " + f.getEntryPoint() + " (" + f.getBody().getNumAddresses() + " bytes)");
                if (r != null && r.decompileCompleted()) c.println(r.getDecompiledFunction().getC());
                else c.println("// decompile failed: " + (r == null ? "null" : r.getErrorMessage()));
                if (++n % 500 == 0) println("decompiled " + n);
            }
            println("functions: " + n);
        }
        d.dispose();
    }
}
