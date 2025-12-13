# TMN4133-Group08-SysMonitor++
**SysMonitor++** is a lightweight Linux_based system resource monitoring tool written in C programming laguage and utilizing Linux System calls and the /proc filesystem.
It can monitor CPU usage, memory usage, and active process in real-time without relying on external monitoring linraries.<br><br>
This project was developed as a group assignment for the TMN4133 System Programming Course. <br>
# Features<br>
- Menu-driven command line interface which including CPU usage, memory usage, top 5 processes, continuous monitoring and Exit options.
- Real-time CPU usage monitoring that show metric and value.
- Memory usage monitoring with including total memory, used memory and available memory in MB.
- Top 5 active processes by CPU usage, show the PID, process name and CPU percentage.
- Continuous monitoring mode with refresh interval.
- System Log record.
- Logging systemsnapshots to syslog.txt.
- Signal handling for graceful exit through CTRL + C.
- Robust error handling that using perror( ). <br>
# System Requirements<br>
- Linux Opereting System
- GNU GCC compiler
- Access to /proc filesystem
- Terminal environment<br>

# Compilation<br>
The following commnd use to compile the program:<br>
**<div align="center"> gcc sysmonitor.c -o sysmonitor</div> <br>**

# Execution
Use the following command to compile the program:<br>
**<div align="center"> ./sysmonitor</div> <br>**

# Menu Mode<br>
|Scenario | Description|
|----------|----------|
|'./sysmonitor>Option 1: CPU Usage'| Display CPU %|
|'./sysmonitor>Option 2: Memory Usage'| Display Memory Usage|
|'./sysmonitor>Option 3: Top 5 Processes'| Display Top 5 processes|
|'./sysmonitor>Option 4: Continuous Monitor'| Display a continuous monitor on processes|
|'./sysmonitor>Option 5: Exit'| Exiting the terminal|<br>


# Command_Line Mode<br>
| Command | Description|
|----------|-----------|
|'./sysmonitor -m proc'| Display top 5 processes usage|
|'./sysmonitor -m cpu'| List the CPU %|
|'./sysmonitor -m mem'| List the memory usage|
|'./sysmonitor -c 2'| Continuous monitoring every 2 second|<br>

# Logging <br>
- All monitoring output are saved to syslog.txt.
- **cat syslog.txt** to list the system log.
- Log file will be appended, not overwritten.
- Final log entry is saved when exiting via **Ctrl + C**.<br>

# Signal Handling<br>
- Press **Ctrl + C** during execution.
- Program captures teh **SIGNT**.
- Display exit message.
- Saves final log entry.
- Terminates Safrly.<br>

# Project Structure<br>
**TMN4133-Group08-SysMonitor.zip**<br>
├─ report.pdf<br>
├─ sysmonitor.c<br>
├─ syslog.txt<br>
├─ presentation_link.txt<br>
└─ README.md
<br>

# Group Members <br>
- **Khor Zhen Qin** (84237)
- **Kiew Chern Jun** (84242)
- **Lai Jia Hong** (84265)
- **Ting Ding Wei** (85931)<br>

# Course Infromation<br>
- **Course:** TMN 4133 System Programming
- **Institution:** Faculty of Computer Science and Information Technology
- **Project Weight:** 20%<br>
















