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

## Feature 4: `ls -x` Horizontal Display

### Version
v1.3.0

### Description
Added support for the `-x` option to display filenames **across then down**, like the real `ls -x`.  
This uses the same column logic but changes the index formula to print horizontally first.

---

### Q1. Difference between “down then across” and “across then down”
- **Down then across (default):** Fills one column top-to-bottom, then moves to the next.
- **Across then down (-x):** Fills rows left-to-right first, then continues on the next row.

---

### Q2. How is “across then down” implemented?
The indexing logic is changed from:
to:
This reorders filenames horizontally before moving to the next line.

---

### Commit and Tag
- **Commit message:** `feat: implement ls -x for horizontal (across-then-down) display`
- **Tag:** `v1.3.0`
- **Release:** `Version 1.3.0 – ls -x (Horizontal Display)`
### **Feature 5 – Alphabetical Sorting (v1.4.0)**
**Objective:**  
Ensure all files are displayed in alphabetical order for all modes (`default`, `-x`, and `-l`).

**Implementation:**  
- Used the standard C function `qsort()` for sorting strings.  
- Added comparator `cmpstr()` for lexicographic comparison.  
- Sorting applied to all modes before display.  

**Example Output:**

**Version Tag:** `v1.4.0`  
**Branch:** `feature-sort-v1.4.0`  
**Release Name:** *Version 1.4.0 – Alphabetical Sorting*

---

## 🧩 Feature 6 – Show Hidden Files (`-a`)

### 🎯 Objective
Enhance the `ls` implementation to display *hidden files* (those whose names begin with a dot `.`) when the `-a` option is provided.  
By default, hidden files remain excluded—matching the standard behaviour of GNU `ls`.

---

### ⚙️ Implementation Details
1. **Added a new command-line flag** `-a` in `main()` using `getopt(argc, argv, "lxa")`.
2. Introduced a global boolean `show_all` variable.  
3. Updated all directory-reading loops (`simple_list_vertical`, `simple_list_horizontal`, and `long_listing`) so that  
   ```c
   if (!show_all && de->d_name[0] == '.') continue;
---

## 🧩 Feature 7 – Recursive Listing (`-R`)

### 🎯 Objective
Enable the `ls` command to traverse subdirectories recursively when `-R` is specified.

### ⚙️ Implementation
- Added a new flag `-R` in the command-line parser.
- Implemented a new helper function `list_recursive()` that:
  - Prints the current directory.
  - Lists its contents using existing display modes.
  - Recursively calls itself for subdirectories (excluding `.` and `..`).

### 🧪 Testing
| Command | Expected Result |
|----------|----------------|
| `./bin/ls -R` | Lists current dir and all subdirs |
| `./bin/ls -lR` | Long listing recursively |
| `./bin/ls -aR` | Includes hidden files recursively |

### 🏷️ Version
**Tag:** `v1.6.0`  
**Commit Message:** `feat: added -R option for recursive directory listing`

---

