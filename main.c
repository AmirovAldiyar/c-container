#include <iostream>
#include <stdio.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>

//
// Runs shell binary so that we can run commands
//
void run_shell() {
  char* cmd = "/bin/sh";
  char *_args[] = {cmd, (char *)0 };
  execvp(cmd, _args);
}

//
// Created image with 10 blocks by 100MB and mounts loop device
//
void isolate_mnt() {
  system("dd if=/dev/zero of=loopbackfile.img bs=100M count=10");
  system("losetup -fP loopbackfile.img");
  system("mkfs.ext4 ./loopbackfile.img");
  system("mkdir tmp_mnt");
  system("mount -o loop /dev/loop0 tmp_mnt");

  chdir("tmp_mnt/");
}

//
// Just a printing info about process
// 
void print_info(int is_child) {
  char* process_name = "Parent";
  if (is_child) {
    process_name = "Child";
  }
  printf("%s PID: %d\n", process_name, getpid());

  printf("%s net namespace: \n", process_name);
  system("ip link");
  printf("\n");
  // FFLUSHING SO THAT WE WILL SEE THE INFORMATION BEFORE IT TERMINATES
  fflush(stdout);
}

int child(void *args) {
  // CLEARING ENVIRONMENT
  clearenv();

  // ISOLATING MNT - MOUNT FILE SYSTEM / LOOP DEVICE
  isolate_mnt();

  // SHOW SOME INFORMATION
  print_info(1);

  // RUNNING SHELL
  run_shell();
  
  return EXIT_SUCCESS;  
}

char* alloc_stack() {
  const int stack_size = 131072;
  auto *stack = new char[stack_size];
  // returning pointer to the last element of the array because
  // stack will grow backwards
  return stack+stack_size; 
}

int main() {
  print_info(0);

  // CLONE_NEWPID - Isolation of process tree
  // CLONE_NEWNET - Isolation of net
  // CLONE_NEWNS  - need for mnt isolation
  clone(child, alloc_stack(), CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWNS | SIGCHLD, 0);
  
  // wait - makes parent process wait until child process ends.
  wait(nullptr);
  return EXIT_SUCCESS;
}