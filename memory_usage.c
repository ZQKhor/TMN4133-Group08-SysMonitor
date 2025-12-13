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