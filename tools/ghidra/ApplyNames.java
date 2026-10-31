// Headless script: applies tools/ghidra/names.tsv to the open program (PC or arcade, chosen by the script argument
// "pc" / "arcade"). Columns: kind (func / data), PC address, arcade address, name, comment. "-" = not in that version.
// Functions are renamed (created if missing); data addresses get a label.
// Stale names: a user-defined function name or label at an address that names.tsv no longer names for this version
// (a pair moved to another routine) goes back to Ghidra's default name (functions, comment cleared) or is deleted
// (labels). Only this script sets user-defined names, so nothing else is touched.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ApplyNames extends GhidraScript {
    @Override
    public void run() throws Exception {
        String which = getScriptArgs()[0];
        File f = new File(getSourceFile().getParentFile().getAbsolutePath(), "names.tsv");
        int col = which.equals("pc") ? 1 : 2, n = 0;
        List<String> lines = Files.readAllLines(f.toPath());
        Set<Address> named = new HashSet<>();
        for (String line : lines) {
            if (line.startsWith("#") || line.isBlank()) continue;
            String[] c = line.split("\t");
            if (c.length < 4 || c[col].equals("-")) continue;
            named.add(toAddr(Long.parseLong(c[col], 16)));
        }
        SymbolTable st = currentProgram.getSymbolTable();
        int reset = 0, deleted = 0;
        List<Symbol> stale = new ArrayList<>();
        for (Symbol s : st.getAllSymbols(false)) {
            if (s.getSource() != SourceType.USER_DEFINED || named.contains(s.getAddress())) continue;
            if (s.getSymbolType() == SymbolType.FUNCTION || s.getSymbolType() == SymbolType.LABEL) stale.add(s);
        }
        for (Symbol s : stale) {
            if (s.getSymbolType() == SymbolType.FUNCTION) {
                Function fn = getFunctionAt(s.getAddress());
                try {
                    s.setName(null, SourceType.DEFAULT);
                    if (fn != null) fn.setComment(null);
                    ++reset;
                } catch (Exception e) {
                    println("could not reset " + s.getName() + " at " + s.getAddress() + ": " + e.getMessage());
                }
            } else if (s.delete()) {
                ++deleted;
            }
        }
        println("stale names: " + reset + " functions reset, " + deleted + " labels deleted");
        for (String line : lines) {
            if (line.startsWith("#") || line.isBlank()) continue;
            String[] c = line.split("\t");
            if (c.length < 4 || c[col].equals("-")) continue;
            Address a = toAddr(Long.parseLong(c[col], 16));
            String name = c[3];
            if (c[0].equals("func")) {
                Function fn = getFunctionAt(a);
                if (fn == null && getFunctionContaining(a) != null) {
                    // inside another function (e.g. a Ghidra split merged back by FixSplits.java): a label only
                    st.createLabel(a, name, SourceType.USER_DEFINED).setPrimary();
                    ++n;
                    continue;
                }
                if (fn == null) fn = createFunction(a, name);
                if (fn != null) fn.setName(name, SourceType.USER_DEFINED);
                if (fn != null && c.length > 4) fn.setComment(c[4]);
            } else {
                st.createLabel(a, name, SourceType.USER_DEFINED).setPrimary();
            }
            ++n;
        }
        println("names applied: " + n);
    }
}
