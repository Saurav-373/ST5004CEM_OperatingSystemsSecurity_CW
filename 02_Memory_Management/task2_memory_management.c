#include <stdio.h>

#define MAX_REFERENCES 100
#define MAX_FRAMES 20
#define EMPTY_FRAME -1

void initializeFrames(int frames[], int frameCount)
{
    for (int i = 0; i < frameCount; i++) {
        frames[i] = EMPTY_FRAME;
    }
}

int findPage(int frames[], int frameCount, int pageNumber)
{
    for (int i = 0; i < frameCount; i++) {
        if (frames[i] == pageNumber) {
            return i;
        }
    }

    return -1;
}

void displayFrames(int frames[], int frameCount)
{
    printf("[ ");

    for (int i = 0; i < frameCount; i++) {
        if (frames[i] == EMPTY_FRAME) {
            printf("- ");
        } else {
            printf("%d ", frames[i]);
        }
    }

    printf("]");
}

void displaySummary(int hits, int faults, int totalReferences)
{
    float hitRatio = ((float) hits / totalReferences) * 100;
    float missRatio = ((float) faults / totalReferences) * 100;

    printf("\nTotal references : %d\n", totalReferences);
    printf("Page hits        : %d\n", hits);
    printf("Page faults      : %d\n", faults);
    printf("Hit ratio        : %.2f%%\n", hitRatio);
    printf("Miss ratio       : %.2f%%\n", missRatio);
}

void runFIFO(int pages[], int referenceCount, int frameCount)
{
    int frames[MAX_FRAMES];
    int nextFrame = 0;
    int hits = 0;
    int faults = 0;

    initializeFrames(frames, frameCount);

    printf("\n========================================\n");
    printf("FIFO PAGE REPLACEMENT\n");
    printf("========================================\n");

    for (int i = 0; i < referenceCount; i++) {
        int currentPage = pages[i];

        printf("Page %2d -> ", currentPage);

        if (findPage(frames, frameCount, currentPage) != -1) {
            hits++;
            printf("Hit   ");
        } else {
            faults++;

            frames[nextFrame] = currentPage;
            nextFrame = (nextFrame + 1) % frameCount;

            printf("Fault ");
        }

        displayFrames(frames, frameCount);
        printf("\n");
    }

    displaySummary(hits, faults, referenceCount);
}

int findLeastRecentlyUsedFrame(int lastUsed[], int frameCount)
{
    int leastRecentlyUsedIndex = 0;

    for (int i = 1; i < frameCount; i++) {
        if (lastUsed[i] < lastUsed[leastRecentlyUsedIndex]) {
            leastRecentlyUsedIndex = i;
        }
    }

    return leastRecentlyUsedIndex;
}

void runLRU(int pages[], int referenceCount, int frameCount)
{
    int frames[MAX_FRAMES];
    int lastUsed[MAX_FRAMES];
    int hits = 0;
    int faults = 0;

    initializeFrames(frames, frameCount);

    for (int i = 0; i < frameCount; i++) {
        lastUsed[i] = -1;
    }

    printf("\n========================================\n");
    printf("LRU PAGE REPLACEMENT\n");
    printf("========================================\n");

    for (int i = 0; i < referenceCount; i++) {
        int currentPage = pages[i];
        int pagePosition = findPage(frames, frameCount, currentPage);

        printf("Page %2d -> ", currentPage);

        if (pagePosition != -1) {
            hits++;
            lastUsed[pagePosition] = i;

            printf("Hit   ");
        } else {
            int replacementIndex = -1;

            faults++;

            /* Use an empty frame before replacing an existing page. */
            for (int j = 0; j < frameCount; j++) {
                if (frames[j] == EMPTY_FRAME) {
                    replacementIndex = j;
                    break;
                }
            }

            if (replacementIndex == -1) {
                replacementIndex =
                    findLeastRecentlyUsedFrame(lastUsed, frameCount);
            }

            frames[replacementIndex] = currentPage;
            lastUsed[replacementIndex] = i;

            printf("Fault ");
        }

        displayFrames(frames, frameCount);
        printf("\n");
    }

    displaySummary(hits, faults, referenceCount);
}

int main(void)
{
    int pageSizeKB;
    int pageSizeBytes;
    int frameCount;
    int referenceCount;

    int logicalAddresses[MAX_REFERENCES];
    int pageNumbers[MAX_REFERENCES];

    printf("========================================\n");
    printf("VIRTUAL MEMORY MANAGEMENT SIMULATOR\n");
    printf("========================================\n");

    printf("Enter page size in KB: ");

    if (scanf("%d", &pageSizeKB) != 1 || pageSizeKB <= 0) {
        printf("Error: Page size must be a positive number.\n");
        return 1;
    }

    printf("Enter number of memory frames: ");

    if (scanf("%d", &frameCount) != 1 ||
        frameCount <= 0 ||
        frameCount > MAX_FRAMES) {
        printf(
            "Error: Frame count must be between 1 and %d.\n",
            MAX_FRAMES
        );
        return 1;
    }

    printf("Enter number of logical addresses: ");

    if (scanf("%d", &referenceCount) != 1 ||
        referenceCount <= 0 ||
        referenceCount > MAX_REFERENCES) {
        printf(
            "Error: Reference count must be between 1 and %d.\n",
            MAX_REFERENCES
        );
        return 1;
    }

    pageSizeBytes = pageSizeKB * 1024;

    printf(
        "Enter %d logical addresses separated by spaces:\n",
        referenceCount
    );

    for (int i = 0; i < referenceCount; i++) {
        if (scanf("%d", &logicalAddresses[i]) != 1) {
            printf("Error: Invalid address entered.\n");
            return 1;
        }

        if (logicalAddresses[i] < 0) {
            printf("Error: Logical addresses cannot be negative.\n");
            return 1;
        }

        pageNumbers[i] = logicalAddresses[i] / pageSizeBytes;
    }

    printf("\nAddress to page conversion\n");
    printf("--------------------------\n");

    for (int i = 0; i < referenceCount; i++) {
        printf(
            "Address %6d -> Page %d\n",
            logicalAddresses[i],
            pageNumbers[i]
        );
    }

    runFIFO(pageNumbers, referenceCount, frameCount);
    runLRU(pageNumbers, referenceCount, frameCount);

    return 0;
}
