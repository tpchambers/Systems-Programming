#define _POSIX_C_SOURCE 1 //define macro to execute KILL system call
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#define true 1 
#define max_path 4096
#define DT_DIR 4 //defined for dirent type
#define DT_REG 8 //defined for dirent type

int tree_copy(char *path, char *dest_path);
int filecopy(char*read_path,char*write_path);
int byte_count = 0;

//PID == 0 for child 
//PID > 0 for parent
//Wait() blocks the calling process until child process exits

int main(int agrc, char * argv[]) {

    char line_input[1024];
    
    //MAIN LOOP
    while (true) {
        //grabbing input
        printf("myshell> ");
        fflush(stdout); // force output
        if(!fgets(line_input,1024,stdin)) {break;} //end of file
        char *shell_words[128];
        char*shell_word = strtok(line_input, " \t\n"); //space for next word
        int count = 0;
        //keep going until no words read
        while(shell_word != NULL) {
            shell_words[count] = shell_word;
            shell_word = strtok(0," \t\n");
            count ++;
        }
        shell_words[count] = 0;
        //command now stored in words[0]
        if (count == 0) continue; //prevent seg fault when hitting return

        //LIST 
        //fork current process, in child run custom list command
        if (strcmp(shell_words[0],"list") == 0) {
            pid_t rc = fork();
            if (rc < 0) {
               fprintf(stderr,"myshell : unable to fork: %s\n",strerror(errno));
            }
            else if (rc == 0 ) {
                //in child, first get size of directory, call cwd
                char directory[1024];
                size_t size_d = sizeof(directory);
                if (getcwd(directory,size_d) == NULL) {
                    fprintf(stderr,"my shell: could not access directory: %s\n",strerror(errno));
                    continue;
                }
                //if we grabbed the path, then we stat each file in the directory
                DIR *dir;
                struct dirent *file;
                dir = opendir(directory);
                struct stat file_info;
                //check each files d_type, and print out the corresponding type along with the size from st_size
                while ((file = readdir(dir)) != NULL) {
                    stat(file->d_name,&file_info);
                    int size = file_info.st_size;
                    if (file->d_type == DT_DIR) {
                        printf("Dir: %s, %d bytes\n",file->d_name,size);
                    }
                    else if (file->d_type == DT_REG) {
                        printf("File: %s, %d bytes\n",file->d_name,size); 
                    }
                }
            }
            //wait for child process to finish
           else {wait(NULL);}
        }

        //CHDIR
        else if (strcmp(shell_words[0],"chdir") == 0) {
            if (count <2) {
                printf("myshell: usage: chdir <dir> \n");
                continue;
            }
            //if attempt < 0, there is an error and could not change to the directory
            int attempt = chdir(shell_words[1]);
            if (attempt <0) {
                fprintf(stderr,"myshell: could not change to directory: %s\n",strerror(errno));
                continue;
             }    
        }

        //PWD
        else if (strcmp(shell_words[0],"pwd") == 0) {
            char directory[1024];
            size_t size_d = sizeof(directory);
            //if getcwd returns NULL, there is error, else, works properly
            if (getcwd(directory,size_d) == NULL) {
                fprintf(stderr,"my shell: could not print directory %s: %s\n",shell_words[0],strerror(errno));
                continue;
            }
            else {
                printf("%s\n",directory); 
            }
        }

        //START
        else if (strcmp(shell_words[0],"start") == 0 ) {
            if (count <2) {
               printf("myshell: usage: start <process request>\n");
               continue;
             }
            pid_t rc = fork();
            if (rc < 0 ) {
                fprintf(stderr,"myshell: unable to fork: %s\n",strerror(errno));
               }
            //in child, if we want to print "process started" first, uncomment wait
            //the wait will make the child exec after the printout
            //however, if we don't care, then comment out wait to proceed normally
            //with large processes, the "process started" can print before the child
            //with quick processes, the "process started" can print after the child finishes
            else if (rc == 0 ) {
                //wait(NULL);
                //exec shell_words[1], then command arguments
                execvp(shell_words[1],&shell_words[1]);
                fprintf(stderr,"myshell: execvp did not function: %s\n",strerror(errno));
                continue;
              }
           else {
               printf("myshell: process %d started\n",rc);}  
            }
         
        //WAIT
        //WIFEXITED - if child process returned by wait exited normally, then we print the appropriate status
        //WTERMSIG - if child process did not exit normally, we call WTERMSIG to query termination status
         else if (strcmp(shell_words[0],"wait") == 0) {
                pid_t rc;
                int process;
                rc = wait(&process);
                if (rc >= 0) {
                    if (WIFEXITED(process)) {
                        printf("myshell: process %d exited normally with status %d\n",rc,WEXITSTATUS(process));
                    }
                    else {printf("myshell: process %d exited abnormally with signal %d:\n",rc,WTERMSIG(process));}

                }
                //if <0 then some error
                else {fprintf(stderr,"myshell: could not wait for process: %s\n",strerror(errno));}
     }
            
         //WAITFOR   
         else if (strcmp(shell_words[0],"waitfor") == 0) {
                if (count <2) {
                    printf("myshell: usage: waitfor <child process> (process id number) \n");
                    continue;
                  }
                
                //atoi to grab integer, casting (int) does not return the actual value we want
                pid_t rc = atoi(shell_words[1]);
                pid_t rc_2;
                int kill_t = kill(rc,0); // sig 0 for error checking
                //if signal is 0, error checking is performed, we send NULL signal for this reason
                if (kill_t != 0 ) {
                    printf("myshell: no such process\n");
                    continue;
                   }
                
                //otherwise we call waitpid on the rc
                int process;
                //set rc_2 to value returned from waitpid
                rc_2 = waitpid(rc,&process,0);
                //check for normal wait and print status to console
                if ( rc_2 >= 0) {
                   if (WIFEXITED(process)) {
                       printf("myshell: process %d exited normally with status %d\n",rc,WEXITSTATUS(process));
                   }
                   else {printf("myshell: process %d exited abnormally with signal %d:\n",rc,WTERMSIG(process));}
                }
                // < 0 error return
                else {fprintf(stderr,"myshell: could not wait for process %d, due to %s\n",rc, strerror(errno));}  
         }
            
         //RUN   
         else if (strcmp(shell_words[0],"run") == 0) {
             if (count <2) {
                printf("myshell: usage: run <cmd arg>\n");
                continue;
             }
             pid_t rc = fork();
             if (rc < 0 ) {
                fprintf(stderr,"myshell: unable to fork: %s\n",strerror(errno));
             }
             
             else if (rc == 0) {
                execvp(shell_words[1],&shell_words[1]);
                //only get to here if run failed
                fprintf(stderr,"myshell: unable to execute command argument: %s\n", strerror(errno));
                //prompt user for input again after failure
                continue;
             }
             //in parent, we call waitpid to update status on process passed in
             else { 
                int process;
                waitpid(rc,&process,0);
                if (WIFEXITED(process)) {
                    printf("myshell: process %d exited with status %d\n",rc, WEXITSTATUS(process));
                    }
                else { printf("myshell: process %d exited abnormally with signal %d:\n", rc, WTERMSIG(process));}  
             }
  
         }
        
        //KILL
        else if (strcmp(shell_words[0],"kill") == 0) {
            if (count <2) {
                printf("myshell usage: kill <pid>\n");
                continue;
            }
            pid_t rc = atoi(shell_words[1]);
            int kill_t = kill(rc,SIGKILL);
            if (kill_t == 0) {
                printf("myshell: process %d has been killed\n",rc);
            }
            else {fprintf(stderr,"myshell: unable to kill process %d\n",rc);} 
        }

        else if (strcmp(shell_words[0],"copy") == 0) {
           int bytes_copied= tree_copy(shell_words[1],shell_words[2]);
           if (bytes_copied  == 0) {
              continue;
           }
           printf("myshell: copied %d bytes from %s to %s\n",bytes_copied,shell_words[1],shell_words[2]);
        }
        
        //EXIT/QUIT
        else if ((strcmp(shell_words[0],"exit") == 0) || (strcmp(shell_words[0],"quit") == 0)) {
            kill(0,SIGINT); //kill all processes 
        }
        
        //GIBBERISH
        else {printf("unkown command: %s\n",shell_words[0]);}
   }

}

//tree copy function
int tree_copy(char *path, char *dest_path) {
    DIR *dir = opendir(path);
    struct dirent *file;
    struct stat file_info;
    stat(path,&file_info);

    if (S_ISDIR(file_info.st_mode)) {
       int attempt_dir = mkdir(dest_path,0777);
       if (attempt_dir < 0) {fprintf(stderr, "myshell: can't create directory %s: %s\n",dest_path,strerror(errno));
       return 0;}
    }
    
    if (S_ISREG(file_info.st_mode)) {
        byte_count += filecopy(path,dest_path);
    }

    if (dir == NULL) {
        return byte_count;
    }

    while ((file = readdir(dir)) != NULL) {
        if (strcmp(file->d_name,".") == 0 || strcmp(file->d_name,"..") == 0) {
            continue;
        }
       
        char diff_base[max_path];
        char new_dest[max_path];
        char comparison[max_path];
        char comparison_2[max_path];

        strcpy(diff_base,path);
        strcat(diff_base,"/");
        strcat(diff_base,file->d_name);

        strcpy(new_dest,dest_path);
        strcat(new_dest,"/");
        strcat(new_dest,file->d_name);

        strcpy(comparison,dest_path);
        strcat(comparison,"/");
        strcat(comparison,dest_path);

        strcat(comparison_2,"/");
        strcat(comparison_2,dest_path);

        if (strcmp(new_dest,comparison) == 0) {
            return byte_count;
            }
        int attempt = stat(path,&file_info);

        if (attempt <0) {closedir(dir); exit(1);}
        
        if (S_ISDIR(file_info.st_mode)) {
            tree_copy(diff_base,new_dest);
            }
    }
    closedir(dir);
    return byte_count;
}
//filecopy used in treecopy
int filecopy (char *src, char *dest_path) {
    #define true 1
    extern int errno;
    int write_file;
    int read_file;
    int attempt;
    char buff[4096];
    int bytes_count = 0;
    read_file = open(src,O_RDONLY);
    write_file = open(dest_path, O_CREAT | O_WRONLY, 0666);   
    while (true) {
        attempt = read(read_file, buff,4096);
        if (attempt <0 ) {
            fprintf(stderr,"myshell: error: %s\n", strerror(errno));
        }
        if (attempt == 0) {
            break;
        }
        int write_attempt = write(write_file, buff, attempt);

        if (write_attempt < 0 ) {
            fprintf(stderr,"myshell: unable to write %s: %s\n", dest_path, strerror(errno));
            break;
        }

        if (write_attempt < attempt) {
            attempt -= write_attempt;
            write_attempt = write (write_file,buff,attempt);
        }
        bytes_count += write_attempt;
    }

    return bytes_count;

}
