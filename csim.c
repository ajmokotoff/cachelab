#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

/**
 * Name: Andrew Jack Mokotoff
 * Username: ajmokotoff
 */

typedef struct {
	int validBit;
	unsigned long tagBit;
	int priorCall;
} line;

typedef struct {
	line *lines;
} set;

struct {
	set *sets;
} theCache;
	
typedef unsigned long long int memoryAddress;

struct parameters {
	int setIndexBit;	// s
	int lines;		// E
	int blockBit;		// b
	int setSize;		// S
	int blockSize;		// B
	int operationNum;	// How many operations have run
	int hitCount;
	int missCount;
	int evictionCount;
	int verbose;
	int option;
	memoryAddress lastCalled;
} cacheParams;



void printHelp() {
	printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
	printf("Options: \n");
	printf(" -h\tPrint this help message.\n");
	printf(" -v\tOptional verbose flag.\n");
	printf(" -s <num>\tNumber of set index bits.\n");
	printf(" -E <num>\tNumber of lines per set.\n");
	printf(" -b <num>\tNumber of block offset bits.\n");
	printf(" -t <file>\tTrace file.\n\n");

	printf("Examples:\n");
	printf(" linux> ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
	printf(" linux> ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");	
}


void buildCache() {
	line cacheLine;
	
	cacheParams.setSize = 1 << cacheParams.setIndexBit;
	cacheParams.blockSize = 1 << cacheParams.blockBit;

	theCache.sets = malloc(sizeof(set) * cacheParams.setSize);

	for(int i = 0; i < cacheParams.setSize; i++) {
		theCache.sets[i].lines = malloc(sizeof(line) * cacheParams.lines);

		for(int j = 0; j < cacheParams.lines; j++) {
			cacheLine.validBit = 0;
			cacheLine.tagBit = 0;
			cacheLine.priorCall = 0;
			theCache.sets[i].lines[j] = cacheLine;	
		}
	}
}

void runOperation(char op, memoryAddress address, int size) {
	cacheParams.operationNum++;

	memoryAddress setIndex = (address << (64 - cacheParams.setIndexBit - cacheParams.blockBit));
	setIndex = setIndex >> (64 - cacheParams.setIndexBit);
	memoryAddress currentTag = address >> (cacheParams.setIndexBit + cacheParams.blockBit);

	set currentSet = theCache.sets[setIndex];

	for(int i=0; i<cacheParams.lines; i++) {
		if(currentSet.lines[i].validBit == 1 && currentSet.lines[i].tagBit == currentTag) {
			cacheParams.hitCount++;
			currentSet.lines[i].priorCall = cacheParams.operationNum;
			if(cacheParams.verbose == 1) {
				if (op == 'M' && cacheParams.lastCalled == address) {
					printf(" hit\n");
				} else if (op == 'M') {
					printf("%c %llx,%d hit", op, address, size);
				} else {
					printf("%c %llx,%d hit\n", op, address, size);
				}
			}
			cacheParams.lastCalled = address;
			return;
		}
	}
	
	cacheParams.missCount++;

	for(int i=0; i<cacheParams.lines; i++) {
		if(currentSet.lines[i].validBit == 0) {
			currentSet.lines[i].validBit = 1;
			currentSet.lines[i].tagBit = currentTag;
			currentSet.lines[i].priorCall = cacheParams.operationNum;
			
			if(cacheParams.verbose == 1) {
				if (op == 'M') {
					printf("%c %llx,%d miss", op, address, size);
				} else {
					printf("%c %llx,%d miss\n", op, address, size);
				}
			}
			cacheParams.lastCalled = address;
			return;
		}

	}

	int minIndex = 0;
	int minAge = currentSet.lines[0].priorCall;
	cacheParams.evictionCount++;

	for(int i=0; i<cacheParams.lines; i++) {
		if (currentSet.lines[i].priorCall < minAge) {
			minIndex = i;
			minAge = currentSet.lines[i].priorCall;
		}
	}

	currentSet.lines[minIndex].tagBit = currentTag;
	currentSet.lines[minIndex].priorCall = cacheParams.operationNum;
	if(cacheParams.verbose == 1) {
		printf("%c %llx,%d miss eviction\n", op, address, size);
	}
}

int main(int argc, char* argv[]) {
	char *fileName;
	int option = 0;

	cacheParams.hitCount = 0;
	cacheParams.missCount = 0;
	cacheParams.evictionCount = 0;
	cacheParams.option = 0;
	cacheParams.operationNum = 0;
	cacheParams.verbose = 0;

	while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
		switch (option) {
			case 'h' : printHelp();
				break;
			case 'v' : cacheParams.verbose = 1;
				break;
			case 's' : cacheParams.setIndexBit = atoi(optarg);
				break;
			case 'E' : cacheParams.lines = atoi(optarg);
				break;
			case 'b' : cacheParams.blockBit = atoi(optarg);
				break;
			case 't' : fileName = optarg;
				break;
			default: printHelp();
				exit(EXIT_FAILURE);
				break;
		}
	}

	// Build Cache here based on parameters
	buildCache();


	
	// Start traversing trace file
	FILE* file = fopen(fileName, "r");
	char first, second;
	memoryAddress address;
	int size;
	char line[60];

	while(fscanf(file, "%c %c %llx,%d", &first, &second, &address, &size) != EOF) {
		fgets(line, 60, file);

		if (first != 'I') {
			switch (second) {
				case 'S':
					//continue
				case 'L':
					runOperation(second, address, size);
					break;
				case 'M':
					runOperation(second, address, size);
					runOperation(second, address, size);
					break;
			}
		}
	}

	printf("\n");
	printSummary(cacheParams.hitCount, cacheParams.missCount, cacheParams.evictionCount);
	return 0;
}
