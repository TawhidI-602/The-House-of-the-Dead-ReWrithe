// Headless pre-script for THOTD.EXE: enables the decompiler's parameter ID (register arguments of the Watcom convention).
import ghidra.app.script.GhidraScript;
import java.util.Map;

public class PreAnalysisPc extends GhidraScript {
    @Override
    public void run() throws Exception {
        Map<String, String> opts = getCurrentAnalysisOptionsAndValues(currentProgram);
        if (opts.containsKey("Decompiler Parameter ID")) setAnalysisOption(currentProgram, "Decompiler Parameter ID", "true");
        println("analysis options set");
    }
}
