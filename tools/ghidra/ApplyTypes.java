// Headless pre-script: builds the game's structures from tools/ghidra/types.tsv and applies them to the open program
// (PC or arcade, chosen by the script argument "pc" / "arcade"), so the decompiled C reads obj->state instead of
// *(short *)(param_1 + 0x5c). Run after ApplyNames.java (it uses the names for nothing, only addresses).
//
// types.tsv lines (tab-separated, '#' = comment):
//   struct  <name>  <arcade size hex>  <pc size hex or ->  <comment>
//   field   <struct>  <arcade offset hex or ->  <pc offset hex, '=' (arcade + the struct's PC shift) or ->  <type>  <name>  <comment>
//   pcshift <struct>  <from arcade offset hex>  <delta hex>      fields at or after the offset move by delta on the PC
//   extends <struct>  <base struct>       the struct starts with all the base's fields (its own fields then override)
//   param   <arcade func hex or ->  <pc func hex or ->  <index 1..>  <type>  <name>
//   global  <pc addr hex or ->  <arcade addr hex or ->  <type>
// Types: byte char short ushort int uint float double void code (a code pointer) or a struct name, then any number of
// '*', then an optional [n] (array). A struct name in a field of the PC build means its PC layout.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.*;
import ghidra.program.model.symbol.SourceType;
import java.io.File;
import java.nio.file.Files;
import java.util.*;

public class ApplyTypes extends GhidraScript {
    boolean pc;
    DataTypeManager dtm;
    CategoryPath cat = new CategoryPath("/hotd");
    Map<String, StructureDataType> structs = new LinkedHashMap<>();
    Map<String, Long> pcShiftFrom = new HashMap<>(), pcShiftBy = new HashMap<>();

    DataType base(String n) {
        switch (n) {
            case "byte": return ByteDataType.dataType;
            case "char": return CharDataType.dataType;
            case "short": return ShortDataType.dataType;
            case "ushort": return UnsignedShortDataType.dataType;
            case "int": return IntegerDataType.dataType;
            case "uint": return UnsignedIntegerDataType.dataType;
            case "float": return FloatDataType.dataType;
            case "double": return DoubleDataType.dataType;
            case "void": return VoidDataType.dataType;
            case "code": return new PointerDataType(new FunctionDefinitionDataType("routine"), 4, dtm);
        }
        DataType d = dtm.getDataType(cat, n);
        if (d == null) throw new IllegalArgumentException("unknown type " + n);
        return d;
    }

    DataType parse(String t) {
        t = t.trim();
        int count = -1;
        int lb = t.indexOf('[');
        if (lb >= 0) { count = Integer.parseInt(t.substring(lb + 1, t.indexOf(']'))); t = t.substring(0, lb).trim(); }
        int stars = 0;
        while (t.endsWith("*")) { ++stars; t = t.substring(0, t.length() - 1).trim(); }
        DataType d = base(t);
        for (int i = 0; i < stars; ++i) d = new PointerDataType(d, 4, dtm);
        if (count > 0) d = new ArrayDataType(d, count, d.getLength(), dtm);
        return d;
    }

    long hex(String s) { return Long.parseLong(s.replace("0x", ""), 16); }

    @Override
    public void run() throws Exception {
        pc = getScriptArgs()[0].equals("pc");
        dtm = currentProgram.getDataTypeManager();
        File f = new File(getSourceFile().getParentFile().getAbsolutePath(), "types.tsv");
        List<String[]> lines = new ArrayList<>();
        for (String line : Files.readAllLines(f.toPath())) {
            if (line.startsWith("#") || line.isBlank()) continue;
            lines.add(line.split("\t", -1));
        }
        // pass 1: empty structures of the right size (so fields can point at any of them)
        for (String[] c : lines) {
            if (c[0].equals("pcshift")) { pcShiftFrom.put(c[1], hex(c[2])); pcShiftBy.put(c[1], hex(c[3])); }
            if (!c[0].equals("struct")) continue;
            String size = pc ? c[3] : c[2];
            if (size.equals("-")) continue;
            StructureDataType s = new StructureDataType(cat, c[1], (int) hex(size), dtm);
            if (c.length > 4) s.setDescription(c[4]);
            structs.put(c[1], s);
            dtm.addDataType(s, DataTypeConflictHandler.REPLACE_HANDLER);
        }
        // pass 2: fields; a struct that extends another ("extends <struct> <base>") gets the base's fields first, then
        // its own (which may rename / retype the base's work slots)
        Map<String, String> baseOf = new HashMap<>();
        for (String[] c : lines)
            if (c[0].equals("extends")) {
                baseOf.put(c[1], c[2]);
                if (pcShiftFrom.containsKey(c[2]) && !pcShiftFrom.containsKey(c[1])) {
                    pcShiftFrom.put(c[1], pcShiftFrom.get(c[2]));
                    pcShiftBy.put(c[1], pcShiftBy.get(c[2]));
                }
            }
        List<String[]> fields = new ArrayList<>();
        for (String s : structs.keySet()) {
            String b = baseOf.get(s);
            if (b != null)
                for (String[] c : lines)
                    if (c[0].equals("field") && c[1].equals(b)) { String[] cc = c.clone(); cc[1] = s; fields.add(cc); }
            for (String[] c : lines)
                if (c[0].equals("field") && c[1].equals(s)) fields.add(c);
        }
        int nf = 0;
        for (String[] c : fields) {
            if (!structs.containsKey(c[1])) continue;
            long off;
            if (pc) {
                if (c[3].equals("-")) continue;
                if (c[3].equals("=")) {
                    if (c[2].equals("-")) continue;
                    off = hex(c[2]);
                    if (pcShiftFrom.containsKey(c[1]) && off >= pcShiftFrom.get(c[1])) off += pcShiftBy.get(c[1]);
                } else off = hex(c[3]);
            } else {
                if (c[2].equals("-")) continue;
                off = hex(c[2]);
            }
            Structure s = (Structure) dtm.getDataType(cat, c[1]);
            DataType t = parse(c[4]);
            try {
                s.replaceAtOffset((int) off, t, t.getLength(), c[5], c.length > 6 ? c[6] : null);
                ++nf;
            } catch (Exception e) {
                println("field " + c[1] + "." + c[5] + " @" + Long.toHexString(off) + ": " + e.getMessage());
            }
        }
        println("structures: " + structs.size() + ", fields: " + nf);
        // pass 3: globals
        int ng = 0;
        for (String[] c : lines) {
            if (!c[0].equals("global")) continue;
            String a = pc ? c[1] : c[2];
            if (a.equals("-")) continue;
            Address addr = toAddr(hex(a));
            DataType t = parse(c[3]);
            try {
                clearListing(addr, addr.add(t.getLength() - 1));
                createData(addr, t);
                ++ng;
            } catch (Exception e) {
                println("global @" + a + ": " + e.getMessage());
            }
        }
        // pass 4: parameters (commit the decompiler's parameters first, then set the typed ones)
        Map<Long, List<String[]>> params = new TreeMap<>();
        for (String[] c : lines) {
            if (!c[0].equals("param")) continue;
            String a = pc ? c[2] : c[1];
            if (a.equals("-")) continue;
            params.computeIfAbsent(hex(a), k -> new ArrayList<>()).add(c);
        }
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        int np = 0, nfail = 0;
        for (Map.Entry<Long, List<String[]>> e : params.entrySet()) {
            Function fn = getFunctionAt(toAddr(e.getKey()));
            if (fn == null) { ++nfail; continue; }
            try {
                if (fn.getSignatureSource() != SourceType.USER_DEFINED) {
                    DecompileResults r = d.decompileFunction(fn, 60, monitor);
                    HighFunction h = r == null ? null : r.getHighFunction();
                    if (h == null) { ++nfail; continue; }
                    HighFunctionDBUtil.commitParamsToDatabase(h, true, HighFunctionDBUtil.ReturnCommitOption.NO_COMMIT,
                                                              SourceType.USER_DEFINED);
                }
                for (String[] c : e.getValue()) {
                    int i = Integer.parseInt(c[3]) - 1;
                    Parameter p = fn.getParameter(i);
                    if (p == null) { println("no parameter " + (i + 1) + " in " + fn.getName()); continue; }
                    p.setDataType(parse(c[4]), SourceType.USER_DEFINED);
                    if (c.length > 5 && !c[5].isEmpty()) p.setName(c[5], SourceType.USER_DEFINED);
                    ++np;
                }
            } catch (Exception ex) {
                ++nfail;
                println("param " + fn.getName() + ": " + ex.getMessage());
            }
        }
        d.dispose();
        println("globals typed: " + ng + ", parameters typed: " + np + " (" + nfail + " functions failed)");
    }
}
