void handleSignal(int sig) {
    (void)sig; // Silence unused warning
    g_exit_requested = 1;
    const char *msg = "\nExiting... Saving log.\n";
    write(STDOUT_FILENO, msg, strlen(msg)); 
    
    // Log final exit entry
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "| SIGNAL_EXIT | %-10s | %5s | %-15s | %s\n", "SIGNAL", "---", "---", "Session Ended via Ctrl+C");
        if (write(fd, buf, strlen(buf)) < 0) {
             const char *err = "Error writing exit log\n";
             write(STDERR_FILENO, err, strlen(err));
        }
        close(fd);
    }
}