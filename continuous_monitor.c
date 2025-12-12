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