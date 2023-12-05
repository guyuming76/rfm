#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>

void* handle = NULL;

int main(int argc, char *argv[]){
	char** selectionList = NULL;
	char** (*fileChooser)(int, char*, bool, char**, void(*)(char**));// startwithVT, search_cmd, async, selectionList, callbackfunction
	void (*fileChooserResultPrint)(char**);
    	handle = dlopen("librfm.so", RTLD_LAZY);
	fileChooser = (char** (*)(int, char*, bool, char**, void(*)(char**)))dlsym(handle,"rfmFileChooser");
	fileChooserResultPrint = (void (*)(char**))dlsym(handle,"fileChooserCallback");

        if (argc>4){
		selectionList = calloc(argc-4+1, sizeof(char*));
		for(int i=4; i<argc; i++)
			selectionList[i-4]=strdup(argv[i]);
		selectionList[argc]=NULL;
		fileChooserResultPrint(fileChooser(atoi(argv[1]), argv[2] , atoi(argv[3]), selectionList, NULL));
	}else if (argc>3)
		fileChooserResultPrint(fileChooser(atoi(argv[1]), argv[2] , atoi(argv[3]), selectionList, NULL));
	else if (argc>2)
		fileChooserResultPrint(fileChooser(atoi(argv[1]), argv[2] , 0, selectionList, NULL));
	else if (argc>1)
		fileChooserResultPrint(fileChooser(atoi(argv[1]), "" , 0, selectionList, NULL));
	else
		fileChooserResultPrint(fileChooser(0, "locate rfm.c", 0, selectionList, NULL));

        dlclose(handle);
}
