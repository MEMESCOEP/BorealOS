## IMPORTS ##
from strip_ansi import strip_ansi
from threading import Thread
from datetime import datetime
from queue import Queue, Empty
from enum import Enum
import subprocess
import traceback
import curses
import psutil
import time
import sys
import os


## ENUMS ##
class StatusMsgTypes(Enum):
    NONE    = 3
    INFO    = 4
    WARNING = 5
    ERROR   = 6
    DEBUG   = 7


## VARIABLES ##
BYTES_TO_MB = 1024 * 1024
CWD = os.getcwd()
TerminalSize = [0, 0, 0] # COLS, ROWS, ROWS/2 + 1
WinLineNums = [0, 0, 0] # BSSW, BOSW, SSSW
BuildStartTime = datetime.now()
MakeMsgQueue = Queue()
MakeRetQueue = Queue()
SpinnerQueue = Queue()
SysStatusUpdateFrequency = 0.5
SpinnerInterval = 0.1
NoExitKeypress = False
MakeProcDone = False
IsBuilding = False
Clean = False
BuildStatusSubWin = None
BuildOutputSubWin = None
SysStatusSubWin = None
BuildStatusWin = None
BuildOutputWin = None
SysStatusWin = None
BuildLog = None
LastUpdateSecond = 0
ExitCode = 0


## FUNCTIONS ##
# Print a colored status message in a window
def WriteStatusMsg(Window, Message, WinLineIndex, MsgType, IncrementLine = True):
    Height, Width = Window.getmaxyx()

    # Clamp the line number to the window's height
    WinLineNums[WinLineIndex] = min(WinLineNums[WinLineIndex], Height - 1)

    # Print the message and its type
    if MsgType == StatusMsgTypes.NONE:
        Window.attron(curses.color_pair(3))
        Window.addstr(WinLineNums[WinLineIndex], 0, Message)

        if IncrementLine == True:
            WinLineNums[WinLineIndex] += max(1, (len(Message) // (Width - 2)) + 1)

    else:
        Window.attron(curses.color_pair(3))
        Window.addstr(WinLineNums[WinLineIndex], 0, "[")
        Window.attron(curses.color_pair(MsgType.value))
        Window.addstr(WinLineNums[WinLineIndex], 1, str(MsgType.name))
        Window.attron(curses.color_pair(3))
        Window.addstr(WinLineNums[WinLineIndex], len(f"[{MsgType.name}"), f"] >> {Message}")

        if IncrementLine == True:
            WinLineNums[WinLineIndex] += max(1, (len(f"[{MsgType.name}] >> {Message}") // (Width - 2)) + 1)

    # Scroll the window up so new text can be displayed
    if WinLineNums[WinLineIndex] > Height - 1:
        Window.scroll()
        WinLineNums[WinLineIndex] = Height - 1
    
    # Refresh the window so our changes are actually visible
    Window.refresh()
    BuildLog.write(f"\n[{str(MsgType.name)}] >> {Message}")

# Run a program
def RunProcess(ProcessArray, MsgQueue, RetQueue):
    try:
        MakeProc = subprocess.Popen(
            ProcessArray,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        for Line in MakeProc.stdout:
            MsgQueue.put(strip_ansi(Line.rstrip('\n').rstrip('\r')))

        MakeProc.wait()
        MakeProc.stdout.close()
        RetQueue.put(MakeProc.returncode)

    except Exception as EX:
        WriteStatusMsg(BuildOutputSubWin, EX, 1, StatusMsgTypes.ERROR)

        if hasattr(EX, "code"):
            RetQueue.put((str(EX), EX.code))

        else:
            RetQueue.put((str(EX), -1))

# Set up a top-level window (the one with the border and title) by drawing the border and title
def DrawTopLevelWindow(Window, Title):
    Window.attron(curses.color_pair(1))
    Window.box()
    Window.addstr(0, 3, "┤ ")
    Window.attron(curses.color_pair(2))
    Window.addstr(0, 5, Title)
    Window.attron(curses.color_pair(1))
    Window.addstr(0, len(Title) + 5, " ├")
    Window.refresh()

# Draw a spinner in the build output window
def DrawSpinner(SpinQueue):
    while IsBuilding == True:
        for SpinCharacter in ['|', '/', '-', '\\']:
            SpinQueue.put(SpinCharacter)
            time.sleep(SpinnerInterval)

# Update the system status window with the current build start time and duration, CPU usage and temperature (if the temp is available), memory usage, and battery percentage (if one or more batteries are installed)
def UpdateSystemStatus():
    # Update statistics
    BuildDuration = (datetime.now() - BuildStartTime).total_seconds()
    CPUString = "CPU: %.2f%% used, <CPUTEMP>°C" % psutil.cpu_percent(interval=None)
    TotalMEM = psutil.virtual_memory().total / BYTES_TO_MB
    MEMUsed = psutil.virtual_memory().used / BYTES_TO_MB

    # Erase the system stats sub window and repopulate it
    SysStatusSubWin.erase()
    SysStatusSubWin.addstr(0, 0, "Build duration: %02d:%02d:%02d:%02d" % (BuildDuration // 86400, (BuildDuration // 3600 % 24), (BuildDuration // 60) % 60, BuildDuration % 60))
    SysStatusSubWin.addstr(1, 0, f"    Started at: {BuildStartTime.strftime('%Y-%m-%d %H:%M:%S')}")
    SysStatusSubWin.addstr(2, 0, f"    Ended at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    SysStatusSubWin.addstr(5, 0, "MEM: %.2fMB / %.2fMB (%.2f%%)" % (MEMUsed, TotalMEM, (MEMUsed / TotalMEM) * 100.0))

    # Make sure psutil actually provides CPU temperature data before we try to access it
    if hasattr(psutil, "sensors_temperatures") == False or psutil.sensors_temperatures() == {} or "coretemp" not in psutil.sensors_temperatures():
        CPUString = CPUString.replace("<CPUTEMP>", "<TEMPGET_FAIL>")

    else:
        CPUString = CPUString.replace("<CPUTEMP>", str(psutil.sensors_temperatures()["coretemp"][0].current))

    SysStatusSubWin.addstr(4, 0, CPUString)

    # Make sure psutil actually provides battery data before we try to access it
    if psutil.sensors_battery() == None:
        SysStatusSubWin.addstr(6, 0, "BAT: N/A (no batteries are installed)")

    else:
        SysStatusSubWin.addstr(6, 0, "BAT: %.2f%% (%s)" % (psutil.sensors_battery().percent, "charging" if psutil.sensors_battery().power_plugged == True else "discharging"))
    
    # Refresh the window so the changes we just made are actually visible
    SysStatusSubWin.refresh()

# Start the a process and validate its return code, and update the system stats
def StartProcess(ProcessArray):
    WriteStatusMsg(BuildStatusSubWin, f"Running process \"{ProcessArray}\"...", 0, StatusMsgTypes.INFO)

    LastSysStatsUpdateTime = time.time()
    MakeThread = Thread(target=RunProcess, args=(ProcessArray, MakeMsgQueue, MakeRetQueue,), daemon=True)
    MakeThread.start()

    while MakeThread.is_alive() == True:
        time.sleep(0.01)

        if time.time() - LastSysStatsUpdateTime >= 0.5:
            LastSysStatsUpdateTime = time.time()
            UpdateSystemStatus()

        try:
            WriteStatusMsg(BuildOutputSubWin, MakeMsgQueue.get_nowait(), 1, StatusMsgTypes.NONE)
            
        except Empty:
            pass

        try:
            BuildOutputSubWin.addstr(WinLineNums[1], 0, SpinnerQueue.get_nowait())
            SpinnerQueue.queue.clear()
            BuildOutputSubWin.refresh()
            
        except Empty:
            pass

    while MakeMsgQueue.qsize() != 0:
        try:
            WriteStatusMsg(BuildOutputSubWin, MakeMsgQueue.get_nowait(), 1, StatusMsgTypes.NONE)
            
        except Empty:
            pass

    try:
        # Make sure the make process exits with code 0, this means that there were no errors
        RetCode = MakeRetQueue.get_nowait()

        if isinstance(RetCode, tuple):
            assert RetCode[1] == 0, f"Process {"finished" if RetCode[1] == 0 else "failed"} with message \"{RetCode[0]}\" (return code = {RetCode[1]})"
            WriteStatusMsg(BuildOutputSubWin, f"Process {"finished" if RetCode[1] == 0 else "failed"} with message \"{RetCode[0]}\" (return code = {RetCode[1]})\n", 1, StatusMsgTypes.INFO if RetCode == 0 else StatusMsgTypes.ERROR)

        else:
            assert RetCode == 0, f"Process {"finished" if RetCode == 0 else "failed"} with return code {RetCode}"
            WriteStatusMsg(BuildOutputSubWin, f"Process {"finished" if RetCode == 0 else "failed"} with return code {RetCode}\n", 1, StatusMsgTypes.INFO if RetCode == 0 else StatusMsgTypes.ERROR)
        
    except Empty:
        WriteStatusMsg(BuildStatusSubWin, "Process thread did not provide a return code.", 0, StatusMsgTypes.WARNING)

# Copy config files to the buildroot directory and call make to build the distro
# TODO: Add the ability to write a build log to the disk so a user can debug easier
def StartBuild():
    global BuildStatusWin, BuildOutputWin, SysStatusWin, DownloadPKGName, TerminalSize, BuildStartTime, IsBuilding, Clean, ExitCode
    BuildStartTime = datetime.now()
    IsBuilding = True

    try:
        WriteStatusMsg(BuildStatusSubWin, f"Build started at: {BuildStartTime}", 0, StatusMsgTypes.INFO)
        Thread(target=DrawSpinner, args=(SpinnerQueue,), daemon=True).start()
        StartProcess(["make", "clean"])
        StartProcess("make")

    except Exception as EX:
        WriteStatusMsg(BuildStatusSubWin, f"Build failed: {EX}", 0, StatusMsgTypes.ERROR)
        BuildLog.write(f"\n{traceback.format_exc()}")
        BuildLog.write(f"\nWLN0: {WinLineNums[0]}")
        BuildLog.write(f"\nWLN1: {WinLineNums[1]}")
        BuildLog.write(f"\nWLN2: {WinLineNums[2]}")
        ExitCode = -1

    IsBuilding = False

# Main curses wrapper function; used to set up the terminal and start the build
def Main(STDScr):
    global BuildStatusSubWin, BuildOutputSubWin, SysStatusSubWin, BuildStatusWin, BuildOutputWin, SysStatusWin, TerminalSize, Clean, ExitCode

    try:
        # Disable the terminal cursor and mouse input
        print("[INFO] >> Disabling terminal cursor and mouse input...")
        curses.mousemask(0)
        curses.curs_set(0)

        print("[INFO] >> Getting terminal size...")
        Rows, Cols = STDScr.getmaxyx()
        TerminalSize = [Cols - 1, Rows, (Rows // 2) + 1]

        # Make sure the terminal is big enough to fit all the text (80x20)
        print(f"[INFO] >> Terminal size is {TerminalSize[0] + 1}x{TerminalSize[1]}.")

        if TerminalSize[0] + 1 < 80 or TerminalSize[1] < 20:
            raise Exception(f"Build failed: Your terminal is too small ({TerminalSize[0]}x{TerminalSize[1]}), please use a terminal that is at least 80x20.")

        print("[INFO] >> Setting up windows...")
        BuildStatusSubWin = curses.newwin(TerminalSize[2] - 4, (TerminalSize[0] // 2) - 2, TerminalSize[2] + 1, 1)
        BuildOutputSubWin = curses.newwin(TerminalSize[2] - 2, TerminalSize[0] - 1, 1, 1)
        SysStatusSubWin = curses.newwin(TerminalSize[2] - 4, (TerminalSize[0] - (TerminalSize[0] // 2)) - 1, TerminalSize[2] + 1, (TerminalSize[0] // 2) + 1)
        BuildStatusWin = curses.newwin(TerminalSize[2] - 2, TerminalSize[0] // 2, TerminalSize[2], 0)
        BuildOutputWin = curses.newwin(TerminalSize[2], TerminalSize[0] + 1, 0, 0)
        SysStatusWin = curses.newwin(TerminalSize[2] - 2, (TerminalSize[0] - (TerminalSize[0] // 2)) + 1, TerminalSize[2], (TerminalSize[0] // 2))

        print("[INFO] >> Setting up color pair(s)...")
        curses.start_color()
        curses.init_pair(1, curses.COLOR_MAGENTA, curses.COLOR_BLACK) # Window border color
        curses.init_pair(2, curses.COLOR_CYAN, curses.COLOR_BLACK)    # Window title color
        curses.init_pair(3, curses.COLOR_WHITE, curses.COLOR_BLACK)   # Text color
        curses.init_pair(4, curses.COLOR_GREEN, curses.COLOR_BLACK)   # Info message type color
        curses.init_pair(5, curses.COLOR_YELLOW, curses.COLOR_BLACK)  # Warning message type color
        curses.init_pair(6, curses.COLOR_RED, curses.COLOR_BLACK)     # Error message type color
        curses.init_pair(7, curses.COLOR_CYAN, curses.COLOR_BLACK)    # Debug message type color

        print("[INFO] >> Doing initial draw...")
        STDScr.clear()
        STDScr.refresh()
        DrawTopLevelWindow(BuildStatusWin, "BUILD STATUS")
        DrawTopLevelWindow(BuildOutputWin, "BUILD OUTPUT")
        DrawTopLevelWindow(SysStatusWin, "SYSTEM STATUS")

        # Build status sub window (for scrollable text)
        BuildStatusSubWin.scrollok(True)
        BuildStatusSubWin.refresh()

        # Build output sub window (for scrollable text from subprocess output)
        BuildOutputSubWin.scrollok(True)
        BuildOutputSubWin.refresh()

        # Start the build (only Linux systems are supported at the moment)
        UpdateSystemStatus()
        StartBuild()
        
        # The build has stopped now, wait for the user to press a key and then exit the program
        if NoExitKeypress == False:
            WriteStatusMsg(BuildStatusSubWin, "<== PRESS ANY KEY TO EXIT ==>\n", 0, StatusMsgTypes.NONE)
            curses.flushinp()
            STDScr.getch()

        print("[INFO] >> Compilation script closed.")

    except Exception as EX:
        print(f"[ERROR] >> Compilation script error: {EX}. See the traceback below for details.")
        traceback.print_exc()
        BuildLog.write(f"\n{traceback.format_exc()}")
        BuildLog.write(f"\nWLN0: {WinLineNums[0]}")
        BuildLog.write(f"\nWLN1: {WinLineNums[1]}")
        BuildLog.write(f"\nWLN2: {WinLineNums[2]}")
        ExitCode = -1


## MAIN CODE ##
if __name__ == "__main__":
    print("[INFO] >> Opening build log file...")
    BuildLog = open("BuildLog.txt", "w")

    print("[INFO] >> Parsing CMD args...")
    # Parse command line arguments
    for Arg in sys.argv:
        if Arg == "--NoExitKeypress":
            NoExitKeypress = True
    
    print("[INFO] >> Starting curses...")
    curses.wrapper(Main)

    print("[INFO] >> Closing build log file...")
    BuildLog.close()

    print(f"[INFO] >> Quitting with exit code {ExitCode}...")
    exit(ExitCode)