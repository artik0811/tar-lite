#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int add_files_to_archive(char* archive_name, char** file_names, int file_count,int create_flag) 
{
    if(create_flag==1)
    {
        FILE* archive_file = fopen(archive_name, "wb");
        if (!archive_file) 
        {
            printf("Error: Could not create archive file.\n");
            return 1;
        }
        // Write the number of files in the archive
        fwrite(&file_count, sizeof(int), 1, archive_file);
        printf("Number of files: %d\n",file_count);
        fclose(archive_file);
    }
    else if(create_flag==0)
    {
        FILE* archive_files_count = fopen(archive_name, "r+b");
        if (!archive_files_count) 
        {
            printf("Error: Could not open archive file.\n");
            return 1;
        }
        // Changing number of files in archive
        char *count_str = calloc(5,sizeof(char));
        int current_file_count;
        fread(&current_file_count, sizeof(int), 1, archive_files_count);
        current_file_count +=file_count;
        sprintf(count_str,"%d",current_file_count);
        fseek(archive_files_count,0,SEEK_SET);
        fwrite(&current_file_count, sizeof(int), 1, archive_files_count);
        fclose(archive_files_count);
        free(count_str);
    }
    // Write each file to the archive
    for (int i = 0; i < file_count; i++) 
    {
        struct stat buf;
        stat(file_names[i],&buf);
        if(S_ISDIR(buf.st_mode)) // Check file is a directory
        {
            FILE* archive_file = fopen(archive_name, "ab");
            if (!archive_file) 
            {
                printf("Error: Could not open archive file.\n");
                return 1;
            }
             // Write the filename length and filename
            int filename_length = strlen(file_names[i]);
            fwrite(&filename_length, sizeof(int), 1, archive_file);
            fwrite(file_names[i], sizeof(char), filename_length, archive_file);
            
            // Write type of file(directory/file)
            fputc('d',archive_file);

            DIR *dir;
            dir = opendir(file_names[i]);
            if(!dir)
            {
                printf("Can't open a '%s' directory",file_names[i]);
                continue;
            }
            int dcount = 0;
            struct dirent *entry;

            // Counting number of files in directory
            while((entry=readdir(dir))!=NULL) 
                if(strcmp(entry->d_name,".")!=0 && strcmp(entry->d_name,"..")!=0)
                    dcount++;

            // Write count of files in directory
            fwrite(&dcount,sizeof(int),1,archive_file);
            char **dir_files = calloc(dcount+2,sizeof(char));
            rewinddir(dir);
            int j=0;
            while((entry=readdir(dir))!=NULL)
            {
                if(strcmp(entry->d_name,".")!=0 && strcmp(entry->d_name,"..")!=0)
                {
                    dir_files[j] = calloc(strlen(entry->d_name)+strlen(file_names[i]) + 2,sizeof(char));
                    strcpy(dir_files[j],file_names[i]);
                    strcat(dir_files[j],"/");
                    strcat(dir_files[j],entry->d_name);
                    j++;
                }
            }
            fclose(archive_file);
            closedir(dir);// -------- Throws corrupted size vs. prev_size
            add_files_to_archive(archive_name,dir_files,dcount,0);
            for(int k=0;k<dcount;k++)
                free(dir_files[k]); //---------- Throws double free or corruption (out)
            free(dir_files);
            
        }
        else
        {
            FILE* archive_file = fopen(archive_name, "ab");
            if (!archive_file) 
            {
                printf("Error: Could not open archive file.\n");
                return 1;
            }
            
            FILE* file = fopen(file_names[i], "rb");
            if (!file) 
            {
                printf("Error: Could not open file '%s'.\n", file_names[i]);
                fclose(archive_file);
                FILE* archive_files_count = fopen(archive_name, "r+b");
                if (!archive_files_count) 
                {
                    printf("Error: Could not open archive file.\n");
                    return 1;
                }
                // Changing number of files in archive to -1
                char *count_str = calloc(5,sizeof(char));
                int current_file_count;
                fread(&current_file_count, sizeof(int), 1, archive_files_count);
                current_file_count--;
                sprintf(count_str,"%d",current_file_count);
                fseek(archive_files_count,0,SEEK_SET);
                fwrite(&current_file_count, sizeof(int), 1, archive_files_count);
                fclose(archive_files_count);
                free(count_str);
                continue;
            }
            // Get the file size
            fseek(file, 0L, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0L, SEEK_SET);

            // Write the filename length and filename
            int filename_length = strlen(file_names[i]);
            fwrite(&filename_length, sizeof(int), 1, archive_file);
            fwrite(file_names[i], sizeof(char), filename_length, archive_file);

            // Write type of file(directory/file)
            fputc('f',archive_file);

            // Write the file size and contents
            fwrite(&file_size, sizeof(long), 1, archive_file);
            char *buffer = calloc(1024,sizeof(char));
            while (file_size > 0) 
            {
                int bytes_to_read = (file_size < 1024) ? file_size : 1024;
                fread(buffer, sizeof(char), bytes_to_read, file);
                fwrite(buffer, sizeof(char), bytes_to_read, archive_file);
                file_size -= bytes_to_read;
            }
            free(buffer);
            fclose(file);
            fclose(archive_file);
        }
    }
    return 0;
}

void list_archive_contents(char* archive_name) 
{
    FILE* archive_file = fopen(archive_name, "rb");
    if (!archive_file) 
    {
        printf("Error: Could not open archive file.\n");
        return;
    }

    // Read the number of files in the archive
    int file_count;
    fread(&file_count, sizeof(int), 1, archive_file);
    if(feof(archive_file)!=0) file_count =0 ;
    // Read each file from the archive
    printf("%d files in archive:\n",file_count);
    int dflag = 0;
    int cnt_flind = 0;

    char *dir_name = malloc(1);
    char *new_fldir = malloc(1);
    for (int i = 0; i < file_count; i++) 
    {
        // Read the filename length and filename
        int filename_length;
        fread(&filename_length, sizeof(int), 1, archive_file);
        char *filename = calloc(filename_length+1,sizeof(char));
        fread(filename, sizeof(char), filename_length, archive_file);
        char *type = calloc(2,sizeof(char));
        fread(type,1,1,archive_file);
        int flag = 0;
        if(dflag>0) // Check if file in directory
        {
            int we = 0;
            new_fldir = realloc(new_fldir,strlen(filename));
            for(int kw = 0;kw<strlen(filename);kw++)
            {
                if(filename[kw]==dir_name[kw]) continue;
                new_fldir[we] = filename[kw];
                we++;
            }
            if(strcmp(type,"d")==0)
            {
                new_fldir[we] = '/';
                new_fldir[we+1] = '\0';
            }
            else
                new_fldir[we] = '\0';
            printf("%s\n",new_fldir);
            flag = 1;
        }
        if(strcmp(type,"d")==0) // Check is file a directory
        {
            dir_name = realloc(dir_name,strlen(filename)+1);
            strcpy(dir_name,filename);
            if(flag == 0)
                printf("%s/\n",filename);
            fread(&cnt_flind,sizeof(int),1,archive_file);
            dflag++;
        }
        else
        {
            if(dflag==0 && flag==0)
                printf("%s\n",filename);
            //Skip the file size and content
            long file_size;
            
            fread(&file_size, sizeof(long), 1, archive_file);
            fseek(archive_file, file_size, SEEK_CUR);
            if(cnt_flind == 0 && dflag != 0) dflag--;
            if(cnt_flind == 0) dflag = 0;
        }
        for(int tmp=0;tmp<dflag;tmp++)
             printf("|   ");
        if(cnt_flind>0) 
            cnt_flind--;
        free(type);
        free(filename);
        flag = 0;
    }
    free(dir_name);
    free(new_fldir);
    fclose(archive_file);
}

void see_archive_file(char* archive_name, char* file_name) 
{
    FILE* archive_file = fopen(archive_name, "rb");
    if (!archive_file) 
    {
        printf("Error: Could not open archive file.\n");
        return;
    }

    // Read the number of files in the archive
    int file_count;
    fread(&file_count, sizeof(int), 1, archive_file);

    char *filename = calloc(1,sizeof(char));
    char *type = calloc(2,1);
    // Search for the file in the archive
    int found = 0;
    for (int i = 0; i < file_count; i++) 
    {
        // Read the filename length and filename
        int filename_length;
        fread(&filename_length, sizeof(int), 1, archive_file);
        filename = realloc(filename,filename_length+1);
        fread(filename, sizeof(char), filename_length, archive_file);
        filename[filename_length+1] = '\0';
        fread(type,1,1,archive_file);
        // Check if this is the file we're looking for
        if (strcmp(filename, file_name) == 0) 
        {
            if(strcmp(type,"d")==0) 
            {
                found = 1;
                int cfd = 0;
                fread(&cfd,sizeof(int),1,archive_file);
                printf("'%s' is a directory:\n",filename);
                for(int k=0;k<cfd;k++)
                {
                    int _tempfilename_length;
                    fread(&_tempfilename_length, sizeof(int), 1, archive_file);
                    char *_tempfilename = calloc(_tempfilename_length,sizeof(char));
                    fread(_tempfilename, sizeof(char), _tempfilename_length, archive_file);
                    _tempfilename[_tempfilename_length] = '\0';
                    printf("    %s\n",_tempfilename);

                    fseek(archive_file, 1, SEEK_CUR); // Skip type of a file
                    long file_size;
                    fread(&file_size, sizeof(long), 1, archive_file);
                    fseek(archive_file, file_size, SEEK_CUR);
                    free(_tempfilename);
                }                
                break;
            }
            found = 1;
            // Read the file size and contents
            long file_size;
            fread(&file_size, sizeof(long), 1, archive_file);
            char *buffer = malloc(1024);
            while (file_size > 0) 
            {
                int bytes_to_read = (file_size < sizeof(buffer)) ? file_size : sizeof(buffer);
                fread(buffer, sizeof(char), bytes_to_read, archive_file);
                fwrite(buffer, sizeof(char), bytes_to_read, stdout);
                file_size -= bytes_to_read;
            }
            free(buffer);
            break;
        } 
        else 
        {
            if(strcmp(type,"d")==0) 
            {
                fseek(archive_file,sizeof(int),SEEK_CUR); // Skip count of files in directory
                continue;
            }
            // Skip over this file's contents
            long file_size;
            fread(&file_size, sizeof(long), 1, archive_file);
            fseek(archive_file, file_size, SEEK_CUR);
        }
    }
    free(type);
    free(filename);
    if (!found) 
    {
        printf("Error: File %s not found in archive.\n", file_name);
    }
    fclose(archive_file);
}

void unarchive(char* archive_name)
{
    FILE *archive_file = fopen(archive_name,"rb");
    if(!archive_file)
    {
        printf("Can't find archive '%s'\n",archive_name);
        return;
    }
    // Read the number of files in the archive
    int file_count;
    fread(&file_count, sizeof(int), 1, archive_file);
    for(int i =0;i<file_count;i++)
    {
        // Read the filename length and filename
        int filename_length;
        fread(&filename_length, sizeof(int), 1, archive_file);
        char *filename = calloc(filename_length+1,sizeof(char));
        fread(filename, sizeof(char), filename_length, archive_file);
        // Read type of a file
        char type = fgetc(archive_file);
        if(type == 'd')
        {
            struct stat st = {0};
            if (stat(filename, &st) == -1) 
            {
                mkdir(filename, 0700);
            }
            fseek(archive_file,sizeof(int),SEEK_CUR);
        }
        else
        {
            FILE *file = fopen(filename,"wb");
            if(!file)
            {
                printf("Can't find '%s' in archive\n",filename);
                continue;
            }
            char *buffer = calloc(1024,sizeof(char));
            long long file_size;
            fread(&file_size, sizeof(long), 1, archive_file);
            while(file_size>0)
            {
                int bytes_to_read = (file_size < 1024) ? file_size : 1024;
                fread(buffer, sizeof(char), bytes_to_read, archive_file);
                fwrite(buffer, sizeof(char), bytes_to_read, file);
                file_size -= bytes_to_read;   
            }
            fclose(file);
            free(buffer);
        }
        free(filename);
    }
    printf("'%s' has succesfully unzip\n",archive_name);
    fclose(archive_file);
}

int main(int argc, char** argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s <flag> <archive_name> [file1] [file2] ... [fileN]\n", argv[0]);
        printf("Flags:\n");
        printf("    -c: Create archive and add files to it.\n");
        printf("    -r: Add files to existing archive.\n");
        printf("    -l: List all files in archive.\n");
        printf("    -u: Unzip archive\n");
        printf("    --see: Display contents of a file in the archive.\n");
        printf("    --help: Show this help message.\n");
        printf("Examples:\n");
        printf("    ./tar-lite -c foo.tar-lite 123.txt Makefile foo.c\n");
        printf("    ./tar-lite -r foo.tar-lite foo2.c Sun.mp3\n");
        printf("    ./tar-lite -l foo.tar-lite\n");
        printf("    ./tar-lite --see foo.tar-lite 123.txt\n");
        return 0;
    }

    int err;
    char* flag = argv[1];
    char* archive_name = argv[2];

    if (strcmp(flag, "-c") == 0) 
    {
        char** file_names = calloc(argc+1,sizeof(char*));
        int file_count = argc - 3;
        for (int i = 0; i < file_count; i++) 
        {
            file_names[i] = calloc(strlen(argv[i + 3]),sizeof(char));
            strcpy(file_names[i],argv[i+3]);
            printf("File name:%s\n",file_names[i]);
        }

        err = add_files_to_archive(archive_name, file_names, file_count,1);

        if(err == 0)
            printf("Archive created successfully.\n");
        else
            printf("Failed to create archive.\n");

        for(int i=0;i<file_count;i++)
        {
            free(file_names[i]);
        }
        free(file_names);
    } 
    else if (strcmp(flag, "-r") == 0) 
    {
        char** file_names = calloc(argc+1,sizeof(char*));
        int file_count = argc - 3;
        for (int i = 0; i < file_count; i++) 
        {
            file_names[i] = calloc(strlen(argv[i + 3]),sizeof(char));
            strcpy(file_names[i],argv[i+3]);
            printf("File name:%s\n",file_names[i]);
        }
        
        err = add_files_to_archive(archive_name, file_names, file_count,0);

        if(err == 0)
            printf("Files added to archive successfully.\n");
        else   
            printf("Failed to add files to archive\n.");

        for(int i=0;i<file_count;i++)
            free(file_names[i]);
        free(file_names);
    } 
    else if (strcmp(flag, "-l") == 0) 
    {
        list_archive_contents(archive_name);
    } 
    else if (strcmp(flag, "--see") == 0) 
    {
        if (argc < 4) 
        {
            printf("Error: Missing filename argument.\n");
            return 0;
        }

        char* file_name = argv[3];
        see_archive_file(archive_name, file_name);
    } 
    else if(strcmp(flag,"-u")==0)
    {
        unarchive(archive_name);
    }
    else 
    {
        printf("Error: Invalid flag.\n");
    }

    return 0;
}