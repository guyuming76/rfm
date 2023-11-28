#include <dlfcn.h>
#include <stdbool.h>

int main(int argc, char *argv[]){
	char** selectionList;
	char** (*fileChooser)(char**, bool, char*);

    	void* handle = dlopen("librfm.so", RTLD_LAZY);
	fileChooser = (char** (*)(char**, bool, char*))dlsym(handle,"rfmFileChooser");

	selectionList = fileChooser(argv, 0, "locate rfm.c");
    	dlclose(handle);
}
