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