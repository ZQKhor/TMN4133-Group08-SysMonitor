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
        "|       Time Stamp       |    Mode     |  Cpu % |   Memory Usage  |              Details              |\n"
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