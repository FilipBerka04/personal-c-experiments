#include <bits/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdint.h>

// Flaga kontrolująca działanie pętli głównej
volatile sig_atomic_t keep_running = 1;

// Obsługa sygnału Ctrl+C (SIGINT)
void sigint_handler(int dummy) {
    keep_running = 0;
}

// Wrapper dla wywołania systemowego perf_event_open (brak go w glibc)
long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                     int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

// Funkcja pomocnicza: odczyt liczby całkowitej z pliku (np. type)
int read_sysfs_int(const char *path, int *val) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int res = fscanf(f, "%d", val);
    fclose(f);
    return (res == 1) ? 0 : -1;
}

// Funkcja pomocnicza: odczyt wartości szesnastkowej po znaku '=' (np. event=0x02)
int read_sysfs_hex_after_eq(const char *path, uint64_t *val) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char buf[64];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    char *ptr = strchr(buf, '=');
    if (ptr) {
        *val = strtoull(ptr + 1, NULL, 16);
        return 0;
    }
    return -1;
}

// Funkcja pomocnicza: odczyt ułamka z pliku (np. scale)
int read_sysfs_double(const char *path, double *val) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int res = fscanf(f, "%lf", val);
    fclose(f);
    return (res == 1) ? 0 : -1;
}

void summary(){
    keep_running = 0;
}

int main() {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = summary;
    sigaction(SIGINT, &action, NULL);

    int type;
    uint64_t config;
    double scale;
    read_sysfs_int("/sys/bus/event_source/devices/power/type", &type);
    read_sysfs_hex_after_eq("/sys/bus/event_source/devices/power/events/energy-pkg", &config);
    read_sysfs_double("/sys/bus/event_source/devices/power/events/energy-pkg.scale", &scale);

    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type = type;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.disabled = 1;

    // 3. Otwarcie licznika
    // pid = -1 (wszystkie procesy), cpu = 0 (zdarzenia RAPL są z reguły przypisane do całego układu CPU, odczytujemy z rdzenia 0)
    int fd = perf_event_open(&attr, -1, 0, -1, 0);
    if (fd == -1) {
        perror("Błąd: perf_event_open nie powiodło się. Brak uprawnień (perf_event_paranoid) lub brak wsparcia.");
        return 1;
    }

    // Reset i uruchomienie licznika
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    uint64_t val = 0, prev_val = 0;
    read(fd, &prev_val, sizeof(uint64_t));
    uint64_t start_val = prev_val;


    sleep(1);
    // 4. Pętla główna: pomiar co sekundę
    while (keep_running) {
        if (read(fd, &val, sizeof(uint64_t)) == sizeof(uint64_t)) {
            // Energia w Dżulach to surowy odczyt różnicy pomnożony przez skalę.
            // Ponieważ ułamek czasu wynosi 1 sekundę, Dżul/1s = Wat.
            double delta_energy = (val - prev_val) * scale;
            printf("Średnia moc przez ostatnią sekundę: %.2f W\n", delta_energy);
            prev_val = val;
        }
        sleep(1); 
    }

    // 5. Zatrzymanie licznika po wyjściu z pętli
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    // Ostatni odczyt w celu obliczenia całkowitego zużycia energii
    if (read(fd, &val, sizeof(uint64_t)) == sizeof(uint64_t)) {
        double total_energy = (val - start_val) * scale;
        printf("\nZakończono pomiar.\n");
        printf("Sumaryczna energia zużyta w trakcie działania programu: %.2f J\n", total_energy);
    }

    close(fd);
    return 0;
}
