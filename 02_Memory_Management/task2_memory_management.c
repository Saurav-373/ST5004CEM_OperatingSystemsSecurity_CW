#include <stdio.h>

#define MAX_REFERENCES 100
#define MAX_FRAMES 20
#define EMPTY -1

void initializeFrames(int frames[], int frameCount) {
    for (int i = 0; i < frameCount; i++) {
        frames[i] = EMPTY;
    }
}

int searchPage(int frames[], int frameCount, int page) {
    for (int i = 0; i < frameCount; i++) {
        if (frames[i] == page) {
            return i;
        }
    }

    return -1;
}

void displayFrames(int frames[], int frameCount) {
    printf("[ ");

    for (int i = 0; i < frameCount; i++) {
        if (frames[i] == EMPTY) {
            printf("- ");
        } else {
            printf("%d ", frames[i]);
        }
    }

    printf("]");
}

void printResult(int hits, int faults, int totalReferences) {
    float hitRatio;
    float missRatio;

    hitRatio = ((float) hits / totalReferences) * 100;
    missRatio = ((float) faults / totalReferences) * 100;

    printf("\nTotal References : %d\n", totalReferences);
    printf("Total Hits       : %d\n", hits);
    printf("Total Faults     : %d\n", faults);
    printf("Hit Ratio        : %.2f%%\n", hitRatio);
    printf("Miss Ratio       : %.2f%%\n", missRatio);
}

void fifoPageReplacement(int pages[], int referenceCount, int frameCount) {
    int frames[MAX_FRAMES];
    int nextReplace = 0;
    int hits = 0;
    int faults = 0;

    initializeFrames(frames, frameCount);

    printf("\n=====================================\n");
    printf("FIFO Page Replacement\n");
    printf("=====================================\n");

    for (int i = 0; i < referenceCount; i++) {
        int currentPage = pages[i];

        printf("Page %2d -> ", currentPage);

        if (searchPage(frames, frameCount, currentPage) != -1) {
            hits++;
            printf("Hit   ");
        } else {
            faults++;
            frames[nextReplace] = currentPage;
            nextReplace = (nextReplace + 1) % frameCount;
            printf("Fault ");
        }

        displayFrames(frames, frameCount);
        printf("\n");
    }

    printResult(hits, faults, referenceCount);
}

int findLeastRecentlyUsed(int lastUsed[], int frameCount) {
    int smallestIndex = 0;

    for (int i = 1; i < frameCount; i++) {
        if (lastUsed[i] < lastUsed[smallestIndex]) {
            smallestIndex = i;
        }
    }

    return smallestIndex;
}

int findEmptyFrame(int frames[], int frameCount) {
    for (int i = 0; i < frameCount; i++) {
        if (frames[i] == EMPTY) {
            return i;
        }
    }

    return -1;
}

void lruPageReplacement(int pages[], int referenceCount, int frameCount) {
    int frames[MAX_FRAMES];
    int lastUsed[MAX_FRAMES];
    int hits = 0;
    int faults = 0;

    initializeFrames(frames, frameCount);

    for (int i = 0; i < frameCount; i++) {
        lastUsed[i] = -1;
    }

    printf("\n=====================================\n");
    printf("LRU Page Replacement\n");
    printf("=====================================\n");

    for (int i = 0; i < referenceCount; i++) {
        int currentPage = pages[i];
        int pageIndex = searchPage(frames, frameCount, currentPage);

        printf("Page %2d -> ", currentPage);

        if (pageIndex != -1) {
            hits++;
            lastUsed[pageIndex] = i;
            printf("Hit   ");
        } else {
            int replaceIndex;

            faults++;

            replaceIndex = findEmptyFrame(frames, frameCount);

            if (replaceIndex == -1) {
                replaceIndex = findLeastRecentlyUsed(lastUsed, frameCount);
            }

            frames[replaceIndex] = currentPage;
            lastUsed[replaceIndex] = i;

            printf("Fault ");
        }

        displayFrames(frames, frameCount);
        printf("\n");
    }

    printResult(hits, faults, referenceCount);
}

void convertAddressesToPages(
    int addresses[],
    int pages[],
    int referenceCount,
    int pageSizeKB
) {
    int pageSizeBytes = pageSizeKB * 1024;

    printf("\nAddress to Page Number Conversion\n");
    printf("---------------------------------\n");

    for (int i = 0; i < referenceCount; i++) {
        pages[i] = addresses[i] / pageSizeBytes;

        printf(
            "Address %d -> Page %d\n",
            addresses[i],
            pages[i]
        );
    }
}

int main() {
    int pageSizeKB;
    int frameCount;
    int referenceCount;
    int inputChoice;
    int rawInput[MAX_REFERENCES];
    int pages[MAX_REFERENCES];

    printf("Virtual Memory Management Simulator\n");
    printf("FIFO and LRU Page Replacement in C\n");
    printf("==================================\n");

    printf("Enter page size in KB: ");
    scanf("%d", &pageSizeKB);

    if (pageSizeKB <= 0) {
        printf("Invalid page size.\n");
        return 1;
    }

    printf("Enter number of frames: ");
    scanf("%d", &frameCount);

    if (frameCount <= 0 || frameCount > MAX_FRAMES) {
        printf("Frame count must be between 1 and %d.\n", MAX_FRAMES);
        return 1;
    }

    printf("Enter number of references: ");
    scanf("%d", &referenceCount);

    if (referenceCount <= 0 || referenceCount > MAX_REFERENCES) {
        printf("Reference count must be between 1 and %d.\n", MAX_REFERENCES);
        return 1;
    }

    printf("\nChoose input type:\n");
    printf("1. Page numbers\n");
    printf("2. Logical addresses\n");
    printf("Enter choice: ");
    scanf("%d", &inputChoice);

    if (inputChoice != 1 && inputChoice != 2) {
        printf("Invalid input choice.\n");
        return 1;
    }

    printf("\nEnter the reference string:\n");

    for (int i = 0; i < referenceCount; i++) {
        scanf("%d", &rawInput[i]);

        if (rawInput[i] < 0) {
            printf("Negative values are not allowed.\n");
            return 1;
        }

        if (inputChoice == 1) {
            pages[i] = rawInput[i];
        }
    }

    if (inputChoice == 2) {
        convertAddressesToPages(rawInput, pages, referenceCount, pageSizeKB);
    }

    fifoPageReplacement(pages, referenceCount, frameCount);
    lruPageReplacement(pages, referenceCount, frameCount);

    return 0;
}
