// firstrun: the support side of a player's run. hotd.log in the settings folder (every SDL_Log line with its time; the
// last run's log kept as hotd.prev.log), a crash report there (crash-<date>-<time>.txt: the exception, a stack trace
// with module + offset, the last log lines) and a friendly message box, and fatal start-up errors shown in a message
// box instead of a window that just closes. Only reads / writes files in the settings folder; nothing is sent anywhere.
#pragma once

#include <string>

namespace hotd::util {

// opens <dir>hotd.log (the previous one becomes hotd.prev.log) and copies every SDL_Log line into it (also to the
// console as before). Call once, early. `dir` ends with a slash.
void logToFile(const std::string& dir);
// the log file's path ("" before logToFile)
const std::string& logFile();

// the crash handler for every run: an unhandled exception, abort() or std::terminate writes <dir>crash-*.txt, prints
// "CRASH: ..." on stderr; with `messageBox` (a player's window, not a test) also a message box saying where the report is
void installCrashReport(const std::string& dir, bool messageBox);
// rescrash: the handler set again if another module (a graphics driver, an overlay) replaced it (logged once per
// module); main calls it after the graphics start and after every display change. A crash no handler sees (Windows
// ends the process at once) is reported at the next start: logToFile finds the run's marker (hotd.running) and writes
// crash-*-previous-run.txt from hotd.prev.log
void crashHandlerCheck();

// a start-up error the player must see: logged, and (with `messageBox`) shown in a message box. Returns 1 (exit code)
int fatalError(const std::string& what, bool messageBox);

// Windows: a console created for this process alone (Explorer, HOTDReWritheSetup.bat's start, Steam) is closed, so a player sees
// only the game's window; a console shared with a terminal stays (developers, tests). Its output is in hotd.log.
void dropOwnConsole();

} // namespace hotd::util
