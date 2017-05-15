# GZIP extension (for sparce files)

Usage:

```
gcc pr.c -o pr
./pr output_unzipped_sparse_file
```

or script for testing

```
bash script.sh
```

Script builds `pr.c`, generates sparse file `file_sparse`, compress this file (`.gz`), unzip (`file_unzipped`) and executes program with result `file_unzipped_sparse`


