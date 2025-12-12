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
                printf("Exiting...\n");
                g_exit_requested = 1;
                break;
            default:
                printf("Invalid option.\n");
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
        else {
            printf("Usage: ./sysmonitor -m [cpu|mem|proc] OR -c [interval]\n");
        }
    } else {
        showMenu();
    }
    return 0;
}