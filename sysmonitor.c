/* sysmonitor.c
 * SysMonitor++ - System Monitoring Tool
 * Group Project - TMN4133 System Programming
 * Group 08
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define MAX_PROCS 4096
#define TOP_N 5
#define LOG_FILE "syslog.txt"

// Global flag for loop control
volatile sig_atomic_t g_exit_requested = 0;

/* --- Low-Level Helpers --- */

// Helper: Read file content into a buffer
ssize_t read_file_to_buf(const char *path, char *buf, size_t bufsize) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (errno != ENOENT) {
            fprintf(stderr, "Failed to open %s: ", path);
            perror("");
        }
        return -1;
    }
    
    ssize_t total = 0;
    ssize_t r;
    while ((r = read(fd, buf + total, bufsize - 1 - total)) > 0) {
        total += r;
        if (total >= (ssize_t)(bufsize - 1)) break;
    }

    if (r < 0) {
        perror("Error reading file content");
        close(fd);
        return -1;
    }

    buf[total] = '\0';
    if (close(fd) < 0) {
        perror("Error closing file");
    }
    return total;
}

// Helper: Get formatted timestamp
void get_timestamp(char *buf, size_t sz) {
    time_t t = time(NULL);
    struct tm tm;
    if (localtime_r(&t, &tm) == NULL) {
        perror("Error getting local time");
        strncpy(buf, "UNKNOWN_TIME", sz);
        return;
    }
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S", &tm);
}

/* --- System Stats Collectors --- */

// Collects Memory Info (Total & Used)
void get_memory_snapshot(long *total, long *used) {
    char buf[4096];
    *total = 0; *used = 0;
    
    // Error message handled inside read_file_to_buf
    if (read_file_to_buf("/proc/meminfo", buf, sizeof(buf)) < 0) return;

    long memFree = 0;
    char *line = strtok(buf, "\n");
    while (line) {
        if (strncmp(line, "MemTotal:", 9) == 0) sscanf(line, "MemTotal: %ld kB", total);
        else if (strncmp(line, "MemFree:", 8) == 0) sscanf(line, "MemFree: %ld kB", &memFree);
        line = strtok(NULL, "\n");
    }
    *used = (*total > memFree) ? (*total - memFree) : 0;
}

// Collects CPU Usage % (Blocking 100ms)
double get_cpu_snapshot() {
    char buf[1024];
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long prevIdle, prevTotal, currIdle, currTotal;

    // Take the first snapshot
    if (read_file_to_buf("/proc/stat", buf, sizeof(buf)) < 0) return 0.0;
    sscanf(buf, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    prevIdle = idle + iowait;
    prevTotal = prevIdle + user + nice + system + irq + softirq + steal;

    usleep(100000); // Wait 100ms

    // Take the second snapshot
    if (read_file_to_buf("/proc/stat", buf, sizeof(buf)) < 0) return 0.0;
    sscanf(buf, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    currIdle = idle + iowait;
    currTotal = currIdle + user + nice + system + irq + softirq + steal;

    unsigned long long totald = currTotal - prevTotal;
    unsigned long long idled = currIdle - prevIdle;

    if (totald == 0) return 0.0;
    return (double)(totald - idled) * 100.0 / (double)totald;
}

/* --- Central Logging Function --- */

void log_action(const char *mode, const char *details, double cpu, long memTotal, long memUsed) {
    char ts[64];
    get_timestamp(ts, sizeof(ts));

    // Create a memory string like "500M/8000M"
    char memStr[64];
    snprintf(memStr, sizeof(memStr), "%ldM / %ldM", memUsed/1024, memTotal/1024);

    char row[2048];
    snprintf(row, sizeof(row), "| %s | %-10s | %5.1f%% | %-15s | %s", ts, mode, cpu, memStr, details);

    // Write to file with error checking
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error opening log file");
        return;
    }

    // Add header if file is new/empty
    off_t current_pos = lseek(fd, 0, SEEK_END);
    if (current_pos == (off_t)-1) {
        perror("Error seeking in log file");
        close(fd);
        return;
    }

    if (current_pos == 0) {
        const char *header = 
        "|      Time Stamp       |    Mode    |  Cpu % |   Memory Usage  |              Details             |\n"
        "+---------------------+------------+--------+-----------------+--------------------------------+\n";
        if (write(fd, header, strlen(header)) == -1) {
            perror("Error writing header to log file");
        }
    }
    
    char line_out[2100];
    snprintf(line_out, sizeof(line_out), "%s\n", row);
    
    if (write(fd, line_out, strlen(line_out)) == -1) {
        perror("Error writing data to log file");
    }
    
    if (close(fd) < 0) {
        perror("Error closing log file");
    }
}

/* --- Signal Handler --- */

void handleSignal(int sig) {
    (void)sig; // Silence unused warning
    g_exit_requested = 1;
    const char *msg = "\nExiting... Saving log.\n";
    write(STDOUT_FILENO, msg, strlen(msg)); 
    
    // Log final exit entry
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        char ts[64];
        // We avoid calling get_timestamp here as localtime isn't strictly async-signal-safe,
        // but for this assignment, we leave it or mock it.
        // A safer way in handler:
        time_t t = time(NULL);
        // (Assuming minimal risk for assignment context)
        
        char buf[256];
        snprintf(buf, sizeof(buf), "| SIGNAL_EXIT | %-10s | %5s | %-15s | %s\n", "SIGNAL", "---", "---", "Session Ended via Ctrl+C");
        if (write(fd, buf, strlen(buf)) < 0) {
             // Avoid perror in signal handler (not async-signal-safe), usually just write to stderr
             const char *err = "Error writing exit log\n";
             write(STDERR_FILENO, err, strlen(err));
        }
        close(fd);
    }
}

/* --- Feature Functions --- */

void performCPU(int log_it, const char *mode) {
    double cpu = get_cpu_snapshot();
    long mTotal, mUsed;
    get_memory_snapshot(&mTotal, &mUsed);

    printf("\n+----------------------+----------------------+\n");
    printf("| %-20s | %-20s |\n", "        Metric", "        Value");
    printf("+----------------------+----------------------+\n");
    
    char valStr[32];
    snprintf(valStr, sizeof(valStr), "%.2f%%", cpu);
    
    printf("| %-20s | %-20s |\n", "Current CPU Usage", valStr);
    printf("+----------------------+----------------------+\n");

    if (log_it) {
        log_action(mode, "Checked CPU Usage", cpu, mTotal, mUsed);
    }
}

void performMemory(int log_it, const char *mode) {
    double cpu = get_cpu_snapshot(); 
    long mTotal, mUsed;
    get_memory_snapshot(&mTotal, &mUsed);

    printf("\n+----------------------+----------------------+-----------------------+\n");
    printf("| %-20s | %-20s | %-20s |\n", "Total Memory (MB)", "Used Memory (MB)", "Available Memory (MB)");
    printf("+----------------------+----------------------+-----------------------+\n");
    printf("| %-20ld | %-20ld | %-21ld |\n", mTotal/1024, mUsed/1024, mTotal/1024 - mUsed/1024);
    printf("+----------------------+----------------------+-----------------------+\n");

    if (log_it) {
        log_action(mode, "Checked Memory Usage", cpu, mTotal, mUsed);
    }
}

typedef struct {
    pid_t pid;
    char name[256];
    double cpu_percent;
} proc_info_t;

void performProcesses(int log_it, const char *mode) {
    double sys_cpu = get_cpu_snapshot();
    long mTotal, mUsed;
    get_memory_snapshot(&mTotal, &mUsed);

    DIR *d = opendir("/proc");
    if (!d) {
        perror("Error opening /proc directory");
        return;
    }

    proc_info_t *procs = malloc(sizeof(proc_info_t) * MAX_PROCS);
    if (!procs) {
        perror("Error allocating memory for process list");
        closedir(d);
        return;
    }

    int count = 0;
    
    // Get uptime for calc
    double uptime = 0.0;
    char upBuf[128];
    if (read_file_to_buf("/proc/uptime", upBuf, sizeof(upBuf)) > 0) 
        sscanf(upBuf, "%lf", &uptime);
        
    long clk_tck = sysconf(_SC_CLK_TCK);
    struct dirent *entry;

    // We reset errno before reading directory
    errno = 0;
    while ((entry = readdir(d)) != NULL) {
        if (!isdigit(entry->d_name[0])) continue;
        pid_t pid = atoi(entry->d_name);
        char path[256], buf[2048];
        
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        if (read_file_to_buf(path, buf, sizeof(buf)) < 0) continue;

        char *rparen = strrchr(buf, ')'); 
        if (!rparen) continue;
        char *lparen = strchr(buf, '(');
        char name[256] = "unknown";
        if (lparen && rparen && rparen > lparen) {
            size_t len = rparen - lparen - 1;
            if (len >= sizeof(name)) len = sizeof(name) - 1;
            strncpy(name, lparen + 1, len);
            name[len] = '\0';
        }

        unsigned long long utime = 0, stime = 0, starttime = 0;
        char *p = rparen + 2;
        int i = 0;
        char *tok = strtok(p, " ");
        while (tok) {
            if (i == 11) utime = strtoull(tok, NULL, 10);
            if (i == 12) stime = strtoull(tok, NULL, 10);
            if (i == 19) starttime = strtoull(tok, NULL, 10);
            tok = strtok(NULL, " ");
            i++;
        }

        double total_time = (double)(utime + stime) / clk_tck;
        double seconds = uptime - ((double)starttime / clk_tck);
        double cpu_usage = (seconds > 0) ? 100.0 * (total_time / seconds) : 0.0;

        procs[count].pid = pid;
        strcpy(procs[count].name, name);
        procs[count].cpu_percent = cpu_usage;
        count++;
        if (count >= MAX_PROCS) break;
    }

    // Check if readdir failed specifically
    if (errno != 0) {
        perror("Error reading directory entry");
    }

    if (closedir(d) < 0) {
        perror("Error closing /proc directory");
    }

    // Sort Processes
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (procs[j].cpu_percent > procs[i].cpu_percent) {
                proc_info_t temp = procs[i];
                procs[i] = procs[j];
                procs[j] = temp;
            }
        }
    }

   printf("\n");
    printf("+----------+--------------------------+----------+\n");
    printf("| %-8s | %-24s | %-8s |\n", "  PID", "      PROCESS NAME", "  CPU %");
    printf("+----------+--------------------------+----------+\n");

    char detail_str[512] = "Top 5: ";

    for (int i = 0; i < TOP_N && i < count; i++) {
        printf("| %-8d | %-24.24s | %-7.2f%% |\n", procs[i].pid, procs[i].name, procs[i].cpu_percent);
        
        char entry_str[300]; 
        snprintf(entry_str, sizeof(entry_str), "(%s:%.1f%%) ", procs[i].name, procs[i].cpu_percent);
        strncat(detail_str, entry_str, sizeof(detail_str) - strlen(detail_str) - 1);
    }
    printf("+----------+--------------------------+----------+\n");
    free(procs);

    if (log_it) {
        log_action(mode, detail_str, sys_cpu, mTotal, mUsed);
    }
}

void performContinuous(int interval) {
    if (interval < 1) interval = 1;
    
    while (!g_exit_requested) {
        printf("\033[2J\033[H"); // Clear screen
        printf("=== Continuous Monitoring (Ctrl+C to stop) ===\n");
        
        double cpu = get_cpu_snapshot();
        long mTotal, mUsed;
        get_memory_snapshot(&mTotal, &mUsed);

        printf("CPU Usage: %.2f%%\n\n", cpu);
        printf("\n+----------------------+----------------------+-----------------------+\n");
        printf("| %-20s | %-20s | %-20s |\n", "Total Memory (MB)", "Used Memory (MB)", "Available Memory (MB)");
        printf("+----------------------+----------------------+-----------------------+\n");
        printf("| %-20ld | %-20ld | %-21ld |\n", mTotal/1024, mUsed/1024, mTotal/1024 - mUsed/1024);
        printf("+----------------------+----------------------+-----------------------+\n");
        
        performProcesses(0, "Continuous"); 

        log_action("Continuous", "Refreshed Data Snapshot", cpu, mTotal, mUsed);

        for (int i = 0; i < interval * 10; i++) {
            if (g_exit_requested) break;
            usleep(100000);
        }
    }
}

/* --- Main & Menu --- */
void showMenu() {
    int choice;
    while (!g_exit_requested) {
        printf("\n=== SysMonitor++ Menu ===\n");
        printf("1. CPU Usage\n");
        printf("2. Memory Usage\n");
        printf("3. Top 5 Processes\n");
        printf("4. Continuous Monitoring\n");
        printf("5. Exit\n");
        printf("Select Option: ");

        if (scanf("%d", &choice) != 1) {
            // Check if it was a real error or just bad input
            if (ferror(stdin)) {
                perror("Error reading input");
                clearerr(stdin);
            }
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1: performCPU(1, "Menu"); break;
            case 2: performMemory(1, "Menu"); break;
            case 3: performProcesses(1, "Menu"); break;
            case 4: 
                {
                    int interval;
                    printf("Enter interval (seconds): ");
                    if (scanf("%d", &interval) == 1) {
                        performContinuous(interval);
                    } else {
                        printf("Invalid interval.\n");
                        while (getchar() != '\n');
                    }
                }
                break;
            case 5:
                {   
                    double cpu = get_cpu_snapshot();
                    long mTotal, mUsed;
                    get_memory_snapshot(&mTotal, &mUsed);

                    log_action("Menu", "End program by option 5", cpu, mTotal, mUsed);

                    printf("Exiting...\n");
                    g_exit_requested = 1;
                }
                break;
            default:
                printf("\nInvalid option.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    // Setup Signal Handling
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("Error setting up signal handler");
        return 1;
    }

    if (argc > 1) {
        if (strcmp(argv[1], "-m") == 0) {
            if (argc < 3) {
                fprintf(stderr, "Error: missing parameter. Use -m [cpu/mem/proc]\n");
                return 1;
            }
            if (strcmp(argv[2], "cpu") == 0)      performCPU(1, "CLI");
            else if (strcmp(argv[2], "mem") == 0) performMemory(1, "CLI");
            else if (strcmp(argv[2], "proc") == 0) performProcesses(1, "CLI");
            else fprintf(stderr, "Error: invalid parameter.\n");
        } 
        else if (strcmp(argv[1], "-c") == 0) {
             int interval = (argc >= 3) ? atoi(argv[2]) : 2;
             performContinuous(interval);
        }
        // NEW: Handle the -h flag explicitly so the user can actually get help
        else if (strcmp(argv[1], "-h") == 0) {
            printf("Usage: ./sysmonitor -m [cpu|mem|proc] OR -c [interval]\n");
        }
        // MODIFIED: Handle unknown flags (e.g., -x)
        else {
            printf("Invalid option. Use -h for help\n");
            return 1; // Return non-zero to indicate error
        }
    } else {
        showMenu();
    }
    return 0;
}
