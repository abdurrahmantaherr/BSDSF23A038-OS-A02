<<<<<<< HEAD
### Q1: Difference between stat() and lstat()
`stat()` follows symbolic links and returns the metadata of the file it points to. `lstat()` returns the metadata of the link itself (showing the link type). In `ls -l`, we use `lstat()` so that symbolic links are reported as links (with their own file type) rather than as their target files.

### Q2: Extracting file type and permission bits
The `st_mode` field in `struct stat` contains both file type and permission bits. Use bitwise operators with POSIX macros:
- `S_ISDIR(st.st_mode)` checks whether it's a directory.
- `st.st_mode & S_IRUSR` checks owner read bit.
- `st.st_mode & S_IXGRP` checks group execute bit.

Bitwise AND `&` isolates bits; macros like `S_IFDIR` and `S_IRUSR` are masks provided by `<sys/stat.h>`. Using these, we build the `rwx` string and detect special bits (setuid/setgid/sticky).
=======
### Q1: Difference between stat() and lstat()
`stat()` follows symbolic links and returns the metadata of the file it points to. `lstat()` returns the metadata of the link itself (showing the link type). In `ls -l`, we use `lstat()` so that symbolic links are reported as links (with their own file type) rather than as their target files.

### Q2: Extracting file type and permission bits
The `st_mode` field in `struct stat` contains both file type and permission bits. Use bitwise operators with POSIX macros:
- `S_ISDIR(st.st_mode)` checks whether it's a directory.
- `st.st_mode & S_IRUSR` checks owner read bit.
- `st.st_mode & S_IXGRP` checks group execute bit.

Bitwise AND `&` isolates bits; macros like `S_IFDIR` and `S_IRUSR` are masks provided by `<sys/stat.h>`. Using these, we build the `rwx` string and detect special bits (setuid/setgid/sticky).

## Feature 3: Column Display (Down Then Across)

### Version
v1.2.0

### Description
In this version, the `ls` program was updated to print filenames in **multiple columns**, filling **down the first column and then across**, similar to the default GNU `ls` behavior.  
The display automatically adjusts based on the terminal width, which is detected using the `ioctl()` system call.

---

### Q1. Explain the general logic for printing items in a “down then across” format
To display files in a “down then across” order:
1. All filenames are first stored in an array.
2. The total number of rows and columns is calculated using:
3. Each row is printed in order, filling one column at a time vertically before moving to the next column horizontally.
This ensures filenames fill down the screen first, then move across, just like the real `ls`.

---

### Q2. What is the purpose of the `ioctl()` system call?
The `ioctl()` system call with the argument `TIOCGWINSZ` retrieves the **current terminal width** (in columns).  
This allows the program to dynamically calculate how many filenames can fit on one line.  
If the terminal is resized, the layout automatically adapts to fit the new width.

---

### Example Output

---

### Commit and Tag
- **Commit message:** `feat: implement column display (down then across) using ioctl for terminal width`
- **Tag:** `v1.2.0`
- **Release Title:** `Version 1.2.0 – Column Display (Down Then Across)`
>>>>>>> 8266cb1 (docs: added Feature 3 answers to REPORT.md)
