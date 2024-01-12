#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <bits/mman-linux.h>
#include <sys/mman.h>
#include <bits/semaphore.h>

#define SHM_SIZE sizeof(int)
#define SEM_NAME "shared_semaphore"

int main() {
   // Create a shared memory object
   int shm_fd = shm_open(SEM_NAME, O_RDWR | O_CREAT, 0666);
   if (shm_fd < 0) {
       perror("Error creating shared memory");
       exit(EXIT_FAILURE);
   }
   ftruncate(shm_fd, SHM_SIZE);

   // Map the shared memory object into the address space of the process
   int *shared_memory = (int *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
   if (shared_memory == MAP_FAILED) {
       perror("Error mapping shared memory");
       close(shm_fd);
       exit(EXIT_FAILURE);
   }

   // Create a semaphore
   sem_t *semaphore = sem_open(SEM_NAME, O_CREAT, 0666, 1); // 1 is the initial value of the semaphore
   if (semaphore == SEM_FAILED) {
       perror("Error creating semaphore");
       munmap(shared_memory, SHM_SIZE);
       close(shm_fd);
       exit(EXIT_FAILURE);
   }

   // Create a child process
   pid_t pid = fork();

   if (pid == -1) {
       sem_close(semaphore);
       munmap(shared_memory, SHM_SIZE);
       close(shm_fd);
       perror("Error creating child process");
       exit(EXIT_FAILURE);
   } else if (pid == 0) {
       // Child process
       for (int i = 1; i <= 1000; ++i) {
           sem_wait(semaphore); // Wait for the semaphore

           // Randomly decide whether to write the next number
           if (rand() % 2 == 0) {
               *shared_memory = i;
               printf("Child wrote: %d\n", i);
           }

           sem_post(semaphore); // Release the semaphore
       }

       exit(EXIT_SUCCESS);
   } else {
       // Parent process
       for (int i = 1; i <= 1000; ++i) {
           sem_wait(semaphore); // Wait for the semaphore

           // Randomly decide whether to write the next number
           if (rand() % 2 == 0) {
               *shared_memory = i;
               printf("Parent wrote: %d\n", i);
           }

           sem_post(semaphore); // Release the semaphore
       }

       wait(NULL); // Wait for the child process to finish
       sem_close(semaphore); // Destroy the semaphore
       shm_unlink(SEM_NAME); // Unlink the shared memory object
       munmap(shared_memory, SHM_SIZE); // Unmap the shared memory

       close(shm_fd); // Close the shared memory object
       exit(EXIT_SUCCESS);
   }

   return 0;
}