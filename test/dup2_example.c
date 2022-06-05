# include <stdio.h>
# include <stdlib.h>
# include <fcntl.h>
# include <unistd.h>
# include <string.h>
# include <sys/stat.h>
# include <sys/types.h>


int main() {
  int fd = open("./dup2_example_data", O_CREAT | O_WRONLY, 0755);
  if(dup2(fd, STDOUT_FILENO) == -1)
  {
    perror("failed dup2");
    return 1;
  }

  printf("Hello World!\n");
  close(fd);
  return 0;
}