#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int compare_i64(const void *left_, const void *right_) {
  int64_t left = *(int64_t *)left_;
  int64_t right = *(int64_t *)right_;
  if (left < right) return -1;
  if (left > right) return 1;
  return 0;
}

void seq_sort(int64_t *arr, size_t begin, size_t end) {
  size_t num_elements = end - begin;
  qsort(arr + begin, num_elements, sizeof(int64_t), compare_i64);
}

// Merge the elements in the sorted ranges [begin, mid) and [mid, end),
// copying the result into temparr.
void merge(int64_t *arr, size_t begin, size_t mid, size_t end, int64_t *temparr) {
  int64_t *endl = arr + mid;
  int64_t *endr = arr + end;
  int64_t *left = arr + begin, *right = arr + mid, *dst = temparr;

  for (;;) {
    int at_end_l = left >= endl;
    int at_end_r = right >= endr;

    if (at_end_l && at_end_r) break;

    if (at_end_l)
      *dst++ = *right++;
    else if (at_end_r)
      *dst++ = *left++;
    else {
      int cmp = compare_i64(left, right);
      if (cmp <= 0)
        *dst++ = *left++;
      else
        *dst++ = *right++;
    }
  }
}

void fatal(const char *msg) __attribute__ ((noreturn));

void fatal(const char *msg) {
  fprintf(stderr, "Error: %s\n", msg);
  exit(1);
}

void merge_sort(int64_t *arr, size_t begin, size_t end, size_t threshold) {
  assert(end >= begin);
  size_t size = end - begin;

  if (size <= threshold) {
    seq_sort(arr, begin, end);
    return;
  }

  // recursively sort halves in parallel

  size_t mid = begin + size/2;

  // parallelize the recursive sorting
  // sort left half in a new process
  pid_t left_pid = fork();
  if (left_pid == -1) {
    fprintf(stderr, "Error: left fork child pid failed to start a new process!");
    exit(1);
  } else if (left_pid == 0) {
    merge_sort(arr, begin, mid, threshold);
    exit(0);
  }

  // sort right half in a new process
  pid_t right_pid = fork();
  if (right_pid == -1) {
    fprintf(stderr, "Error: right fork child pid failed to start a new process!");
    exit(1);
  } else if (right_pid == 0) {
    merge_sort(arr, mid, end, threshold);
    exit(0);
  }

  // blocks until the process indentified by pid_to_wait_for completes (left_pid)
  int wstatus_left;
  pid_t actual_pid_left = waitpid(left_pid, &wstatus_left, 0);

  if (actual_pid_left == -1) {
    fprintf(stderr, "Error: waitpid failure! (left)");
    exit(1);
  }

  if (!WIFEXITED(wstatus_left)) {
    fprintf(stderr, "Error: subprocess crashed, was interruped, or did not exit normally! (left)");
    exit(1);
  }
  if (WEXITSTATUS(wstatus_left) != 0) {
    fprintf(stderr, "Error: subprocess returned a non-zero exit code! (left)");
    exit(1);
  }

  // blocks until the process indentified by pid_to_wait_for completes (right_pid)
  int wstatus_right;
  int actual_pid_right = waitpid(right_pid, &wstatus_right, 0);

  if (actual_pid_right == -1) {
    fprintf(stderr, "Error: waitpid failure! (right)");
    exit(1);
  }

  if (!WIFEXITED(wstatus_right)) {
    fprintf(stderr, "Error: subprocess crashed, was interruped, or did not exit normally! (right)");
    exit(1);
  }

  if (WEXITSTATUS(wstatus_right) != 0) {
    fprintf(stderr, "Error: subprocess returned a non-zero exit code! (right)");
    exit(1);
  }  


  // allocate temp array now, so we can avoid unnecessary work
  // if the malloc fails
  int64_t *temp_arr = (int64_t *) malloc(size * sizeof(int64_t));
  if (temp_arr == NULL)
    fatal("malloc() failed");

  // child processes completed successfully, so in theory
  // we should be able to merge their results
  merge(arr, begin, mid, end, temp_arr);

  // copy data back to main array
  for (size_t i = 0; i < size; i++)
    arr[begin + i] = temp_arr[i];

  // now we can free the temp array
  free(temp_arr);

  // success!
}

int main(int argc, char **argv) {
  // check for correct number of command line arguments
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <filename> <sequential threshold>\n", argv[0]);
    return 1;
  }

  // process command line arguments
  const char *filename = argv[1];
  char *end;
  size_t threshold = (size_t) strtoul(argv[2], &end, 10);
  if (end != argv[2] + strlen(argv[2])) {
    // report an error (threshold value is invalid)
    fprintf(stderr, "Error: Invalid threshold value!");
    return 1;
  }

  // open the file
  int fd = open(filename, O_RDWR);
  if (fd < 0) {
    // file couldn't be opened: handle error and exit
    fprintf(stderr, "Error: file could not be opened!");
    return 1;
  }

  // use fstat to determine the size of the file
  struct stat statbuf;
  int rc = fstat(fd, &statbuf);
  if (rc != 0) {
    // handle fstat error and exit
    fprintf(stderr, "Error: fstat error!");
    return 1;
  }
  size_t file_size_in_bytes = statbuf.st_size;

  // map the file into memory using mmap
  int64_t *data = mmap(NULL, file_size_in_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  // Immediately close the file descriptor as it's no longer needed
  int close_status = close(fd);
  if (close_status != 0) {
    fprintf(stderr, "Error: file cannot be closed properly!");
    return 1;
  }
  // handle mmap error and exit
  if (data == MAP_FAILED) {
    fprintf(stderr, "Error: Could not map the into the memory!");
    return 1;
  }

  // sort the data
  merge_sort(data, 0, file_size_in_bytes / sizeof(int64_t), threshold);

  // unmap and close the file
  int munmap_status = munmap(data, file_size_in_bytes);
  if (munmap_status == -1) {
    fprintf(stderr, "Error: Could not unmap the memory!");
    return 1;
  }

  // exit with a 0 exit code if sort was successful
  return 0;
}
