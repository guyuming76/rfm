#include <dlfcn.h>
#include <stdbool.h>

int main(int argc, char *argv[]){
	char** selectionList;
	char** (*fileChooser)(char**, bool);

    	void* handle = dlopen("librfm.so", RTLD_LAZY);
	fileChooser = (char** (*)(char**, bool))dlsym(handle,"rfmFileChooser");

	selectionList = fileChooser(argv, 0);
    	dlclose(handle);
}
