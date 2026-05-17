#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>


int files_count;
int dir_count;
int last[1024];

int NumberOfDirectories(DIR *dir){
    int idx=0;
    struct dirent *file;
    while( (file = readdir(dir)) != NULL ) {
        if (file->d_name[0] == '.') {
            continue;
        }
        idx+=1;
    }
    return idx;
}

void rtree(char *path,int depth){
    DIR *dir = opendir(path);
    int num_of_dir = NumberOfDirectories(dir);
    rewinddir(dir);  
    struct dirent *file;
    int idx = 0;

    while( (file = readdir(dir)) != NULL ) {
        if (file->d_name[0] == '.') {
            continue;
        }

        idx+=1;

        for (int i = 0; i < depth; i++) {
            if(last[i])printf("     ");
            else printf("|    ");
        }

        int isLast = (idx == num_of_dir);

        if(isLast)printf("`-- %s\n", file->d_name);
        else printf("|-- %s\n", file->d_name);

        last[depth]=isLast;

        char str[1024];
        strcpy(str, path );
        strcat(str, "/");
        strcat(str, file->d_name );

        struct stat st;
        if (lstat(str, &st) == 0 &&  S_ISDIR(st.st_mode)) {
            dir_count++;
            rtree(str,depth+1);
        }
        else{
            files_count++;
        }
        
    }
    
    closedir(dir);
}

void tree(char *path){
    files_count=0;
    dir_count=0;

    printf("%s",path);
    printf("\n");

    rtree(path,0);

    printf("\n");

    printf("%d directories, %d files\n",dir_count+1,files_count);
}

int main(int argc, char *argv[]) {
    char *path = (argc > 1) ? argv[1] : ".";
    tree(path);
    
    return 0;
}