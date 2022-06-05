# include <stdio.h>
# include <stdlib.h>
# include <fcntl.h>
# include <unistd.h>
# include <string.h>

int main() {
  int fd1 = 0, fd2 = 0, fd3 = 0; 
  fd1 = open("./dup2_example_data", O_WRONLY | O_CREAT | O_TRUNC);

  printf("fcntl(F_DUPFD) TEST\n");
  fd2 = fcntl(fd1, F_DUPFD, 20);        // fcntl() 로 30 이상의 fd를 할당하여 fd2, fd3에 반환
  printf("fd2: %d\n", fd2);
  fd3 = fcntl(fd1, F_DUPFD, 30);
  printf("fd3: %d\n", fd3);

  close(fd2);
  close(fd3);
ㅣㄴ

  printf("dup2() TEST\n");
  fd2 = dup2(fd1, 30);                    // Dup2() 로 30의 fd를 할당하여 fd2, fd3에 반환
  printf("fd2: %d\n", fd2);
  fd3 = dup2(fd1, 30);  
  printf("fd3: %d\n", fd3);

  close(fd2);
  close(fd3);
  
  close(fd1);
  return 0;
}