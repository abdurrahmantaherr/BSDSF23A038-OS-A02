### Q1: Difference between stat() and lstat()
`stat()` follows symbolic links and returns the metadata of the file it points to. `lstat()` returns the metadata of the link itself (showing the link type). In `ls -l`, we use `lstat()` so that symbolic links are reported as links (with their own file type) rather than as their target files.

### Q2: Extracting file type and permission bits
The `st_mode` field in `struct stat` contains both file type and permission bits. Use bitwise operators with POSIX macros:
- `S_ISDIR(st.st_mode)` checks whether it's a directory.
- `st.st_mode & S_IRUSR` checks owner read bit.
- `st.st_mode & S_IXGRP` checks group execute bit.

Bitwise AND `&` isolates bits; macros like `S_IFDIR` and `S_IRUSR` are masks provided by `<sys/stat.h>`. Using these, we build the `rwx` string and detect special bits (setuid/setgid/sticky).
