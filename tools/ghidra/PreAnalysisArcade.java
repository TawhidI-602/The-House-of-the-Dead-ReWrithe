// Headless pre-script for the arcade program (maincpu.bin imported raw at 0): adds the rest of the Model 2 memory map
// the program uses: work RAM 0x500000 (1 MB, uninitialized) and the data ROM at 0x02000000 (main_data.bin, 32 MB).
// Script argument: the regions folder.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.*;
import java.io.*;

public class PreAnalysisArcade extends GhidraScript {
    @Override
    public void run() throws Exception {
        String regions = getScriptArgs()[0];
        Memory mem = currentProgram.getMemory();
        MemoryBlock rom = mem.getBlock(toAddr(0));
        rom.setName("maincpu");
        rom.setWrite(false);
        rom.setExecute(true);
        if (mem.getBlock(toAddr(0x500000)) == null) {
            MemoryBlock ram = mem.createUninitializedBlock("workram", toAddr(0x500000), 0x100000, false);
            ram.setWrite(true);
        }
        if (mem.getBlock(toAddr(0x02000000)) == null) {
            File f = new File(regions, "main_data.bin");
            try (InputStream in = new FileInputStream(f)) {
                MemoryBlock data = mem.createInitializedBlock("main_data", toAddr(0x02000000), in, f.length(), monitor, false);
                data.setWrite(false);
            }
        }
        println("memory map set");
    }
}
