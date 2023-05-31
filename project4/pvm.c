#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define PAGE_SIZE 4096

void printFrameInfo (unsigned long PFN) {
    int fd;
    off_t offset;
    uint64_t entry;
    uint64_t mapping_count;

    fd = open("/proc/kpageflags", O_RDONLY);
    if (fd == -1) {
        perror("Error opening /proc/kpageflags");
        exit(1);
    }

    // Calculate the offset based on the frame number (PFN)
    offset = PFN * sizeof(uint64_t);

    // Seek to the appropriate entry in the file
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to the entry in /proc/kpageflags");
        close(fd);
        exit(1);
    }

    // Read the entry from the file
    if (read(fd, &entry, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("Error reading entry from /proc/kpageflags");
        close(fd);
        exit(1);
    }

    close(fd);

    fd = open("/proc/kpagecount", O_RDONLY);
    if (fd == -1) {
        perror("Error opening /proc/kpagecount");
        exit(1);
    }

    // Seek to the appropriate entry in the file
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to the entry in /proc/kpagecount");
        close(fd);
        exit(1);
    }

    // Read the mapping count from the file
    if (read(fd, &mapping_count, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("Error reading mapping count from /proc/kpagecount");
        close(fd);
        exit(1);
    }

    close(fd);

    // Detailed information
    printf("Frame Number: 0x%lx\n", PFN);
    printf("Mapping Count: %lu\n", mapping_count);
    printf("Flag Values: 0x%lx\n", entry);
    printf("Flags:\n");

    printf("  LOCKED: %s\n", (entry & (1ULL << 0)) ? "Yes" : "No");
    printf("  ERROR: %s\n", (entry & (1ULL << 1)) ? "Yes" : "No");
    printf("  REFERENCED: %s\n", (entry & (1ULL << 2)) ? "Yes" : "No");
    printf("  UPTODATE: %s\n", (entry & (1ULL << 3)) ? "Yes" : "No");
    printf("  DIRTY: %s\n", (entry & (1ULL << 4)) ? "Yes" : "No");
    printf("  LRU: %s\n", (entry & (1ULL << 5)) ? "Yes" : "No");
    printf("  ACTIVE: %s\n", (entry & (1ULL << 6)) ? "Yes" : "No");
    printf("  SLAB: %s\n", (entry & (1ULL << 7)) ? "Yes" : "No");
    printf("  WRITEBACK: %s\n", (entry & (1ULL << 8)) ? "Yes" : "No");
    printf("  RECLAIM: %s\n", (entry & (1ULL << 9)) ? "Yes" : "No");
    printf("  BUDDY: %s\n", (entry & (1ULL << 10)) ? "Yes" : "No");
    printf("  MMAP: %s\n", (entry & (1ULL << 11)) ? "Yes" : "No");
    printf("  ANON: %s\n", (entry & (1ULL << 12)) ? "Yes" : "No");
    printf("  SWAPCACHE: %s\n", (entry & (1ULL << 13)) ? "Yes" : "No");
    printf("  SWAPBACKED: %s\n", (entry & (1ULL << 14)) ? "Yes" : "No");
    printf("  COMPOUND_HEAD: %s\n", (entry & (1ULL << 15)) ? "Yes" : "No");
    printf("  COMPOUND_TAIL: %s\n", (entry & (1ULL << 16)) ? "Yes" : "No");
    printf("  HUGE: %s\n", (entry & (1ULL << 17)) ? "Yes" : "No");
    printf("  UNEVICTABLE: %s\n", (entry & (1ULL << 18)) ? "Yes" : "No");
    printf("  HWPOISON: %s\n", (entry & (1ULL << 19)) ? "Yes" : "No");
    printf("  NOPAGE: %s\n", (entry & (1ULL << 20)) ? "Yes" : "No");
    printf("  KSM: %s\n", (entry & (1ULL << 21)) ? "Yes" : "No");
    printf("  THP: %s\n", (entry & (1ULL << 22)) ? "Yes" : "No");
    printf("  BALLOON: %s\n", (entry & (1ULL << 23)) ? "Yes" : "No");
    printf("  ZERO_PAGE: %s\n", (entry & (1ULL << 24)) ? "Yes" : "No");
    printf("  IDLE: %s\n", (entry & (1ULL << 25)) ? "Yes" : "No");
}

void printMemUsed(unsigned long PID) {
    char filename[100];
    sprintf(filename, "/proc/%lu/maps", PID);
    FILE *mapsFile = fopen(filename, "r");
    if (mapsFile == NULL) {
        perror("Error opening /proc/PID/maps");
        exit(1);
    }

    // Memory usage variables
    unsigned long long totalVirtualMemory = 0;
    unsigned long long physicalMemoryExclusive = 0;
    unsigned long long physicalMemoryAll = 0;

    // Read the maps file line by line
    char line[256];
    while (fgets(line, sizeof(line), mapsFile)) {
        unsigned long long start, end;
        // Extract the start and end virtual addresses of each virtual memory area
        sscanf(line, "%llx-%llx", &start, &end);

        // Calculate the size of the virtual memory area and add it to the total virtual memory usage
        unsigned long long size = end - start;
        totalVirtualMemory += size;

        // Check the mapping count for each page in the virtual memory area
        unsigned long long vaddr;
        for (vaddr = start; vaddr < end; vaddr += PAGE_SIZE) {
            sprintf(filename, "/proc/%lu/pagemap", PID);
            int pagemapFd = open(filename, O_RDONLY);
            if (pagemapFd == -1) {
                perror("Error opening /proc/PID/pagemap");
                exit(1);
            }

            off_t offset = (vaddr / PAGE_SIZE) * sizeof(uint64_t);
            if (lseek(pagemapFd, offset, SEEK_SET) == -1) {
                perror("Error seeking /proc/PID/pagemap");
                exit(1);
            }

            uint64_t pagemapEntry;
            if (read(pagemapFd, &pagemapEntry, sizeof(uint64_t)) == -1) {
                perror("Error reading /proc/PID/pagemap");
                exit(1);
            }

            // Extract the page frame number (PFN) from the pagemap entry
            uint64_t pfn = pagemapEntry & 0x7FFFFFFFFFFFFF;

            // Close /proc/PID/pagemap file
            close(pagemapFd);

            // Open the /proc/kpagecount file
            int kpagecountFd = open("/proc/kpagecount", O_RDONLY);
            if (kpagecountFd == -1) {
                perror("Error opening /proc/kpagecount");
                exit(1);
            }

            // Read the mapping count for the page from /proc/kpagecount
            off_t kpcOffset = pfn * sizeof(uint64_t);
            if (lseek(kpagecountFd, kpcOffset, SEEK_SET) == -1) {
                perror("Error seeking /proc/kpagecount");
                exit(1);
            }

            uint64_t mappingCount;
            if (read(kpagecountFd, &mappingCount, sizeof(uint64_t)) == -1) {
                perror("Error reading /proc/kpagecount");
                exit(1);
            }

            // Close /proc/kpagecount file
            close(kpagecountFd);

            // Check if the mapping count is equal to 1 for exclusive memory
            if (mappingCount == 1) {
                physicalMemoryExclusive += PAGE_SIZE;
            }
            // Check if the mapping count is equal to or greater than 1 for exclusive memory
            if (mappingCount >= 1) {
                physicalMemoryAll += PAGE_SIZE;
            }
        }
    }

    fclose(mapsFile);

    // Print the memory usage in KB
    printf("Total virtual memory used: %llu KB\n", totalVirtualMemory / 1024);
    printf("Physical memory used (exclusively mapped pages): %llu KB\n", physicalMemoryExclusive / 1024);
    printf("Physical memory used (all pages): %llu KB\n", physicalMemoryAll / 1024);
}

int main (int argc, char* argv[]) {
    if (argc < 3) {
            printf("Usage: %s [option] [argument]\n", argv[0]);
        return 1;
    }
    if (!strcmp(argv[1], "-frameinfo")) {
        if (argc < 3) {
            printf("Usage: %s -frameinfo [frame_number]\n", argv[0]);
            return 1;
        }
        unsigned long PFN = strtoul(argv[2], NULL, 0);
        printFrameInfo(PFN);
    }
    if (!strcmp(argv[1], "-memused")) {    
        if (argc < 3) {
            printf("Usage: %s -memused [pid]\n", argv[0]);
            return 1;
        }
        unsigned long PID = strtoul(argv[2], NULL, 0);
        printMemUsed(PID);
    }
    return 0;
}