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