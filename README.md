# 🚀 NetTransmit

A multithreaded **Client-Server-Mirror** file search and transmission system built in C, designed for Unix-like operating systems. `NetTransmit` enables users to issue file-related commands remotely, retrieve `.tar.gz` archives, and even offload requests to a mirror server based on dynamic load-balancing logic.

---

## 🛠️ Components

### 🖥️ `client.c`
- Connects to the **server** or **mirror** depending on server instructions.
- Sends user commands and receives either:
  - File metadata (`filesrch`)
  - Archived file (`.tar.gz`) which can be auto-unzipped (`-u` flag).
- Supports user command parsing and automatic folder extraction.

### 🧭 `server.c`
- Accepts incoming clients and decides whether to:
  - Handle requests directly, or
  - Redirect to a mirror based on active connections (`cnt >= 7 && cnt <= 12` or `cnt > 12 && even`).
- Forks child processes to manage each client.
- Handles all file search and archive generation logic.
- Uses shared memory key for potential extension/IPC.

### 🪞 `mirror.c`
- A parallel **file-handling replica** of the main server.
- Receives clients redirected by `server.c`.
- Handles identical commands and performs the same file operations.
- Supports multiple clients using `fork()`.

---

## 📂 Supported Commands

| Command | Description |
|--------|-------------|
| `fgets file1 file2 ...` | Retrieves up to 4 named files as `.tar.gz` |
| `targzf ext1 ext2 ... [-u]` | Retrieves files by extensions |
| `tarfgetz size1 size2 [-u]` | Retrieves files within a given size range |
| `getdirf yyyy-mm-dd yyyy-mm-dd [-u]` | Retrieves files by creation date range |
| `filesrch filename` | Returns file metadata if found |
| `quit` | Ends client session |

> ✅ `-u` flag automatically unzips the archive into `Unzipped_Folder`.

---

## 🔧 Compilation

Ensure you have a POSIX-compliant system with `gcc` and `tar`.

```bash
gcc client.c -o client
gcc server.c -o server
gcc mirror.c -o mirror
```

## 🚀 Execution
1. Start Mirror (optional):
```bash
./mirror 9090
```
3. Start Server:
```bash
./server 8080
```
5. Start Client:
```bash
./client 8080 9090
```

## 🧪 Sample Usage
```bash
> fgets file1.txt file2.pdf -u
> targzf txt pdf -u
> tarfgetz 100 2048 -u
> getdirf 2023-01-01 2023-12-31 -u
> filesrch report.docx
> quit
```

## 📁 Output

All retrieved files are bundled as: ```temp.tar.gz```

With ```-u```, extracted to: ```Unzipped_Folder```

## 🔒 Dependencies

- POSIX-compatible system (Linux/macOS)

- C compiler (```gcc```)

- Utilities: ```tar```, ```gzip```

- System libraries: ```arpa/inet.h```, ```dirent.h```, ```sys/socket.h```, ```sys/shm.h```, ```zlib.h```, etc.

## 📦 Architecture Overview
```
+-------------+         +-----------+          +-------------+  
|             |  --->   |           |   --->   |             |  
|   Client    |         |   Server  |          |   Mirror    |  
|             |  <---   |           |   <---   |             |  
+-------------+         +-----------+          +-------------+  
       |                     |                       |  
  User Sends Command     Server Decides       Handles Load Offloading  
                        (based on client cnt)   Performs Same Ops
```

                        
## 🧠 Design Highlights

- Efficient client handling using ```fork()```

- Smart redirection based on server load

- Modular command parser and executor

- Easy integration of unzip and tar extraction

- Clean user interface with file validation

## 🤝 Contributing
PRs and suggestions welcome! Fork the repo, add features or improve efficiency, and create a pull request.

## 🙏 Thank You
Thank you for checking out **NetTransmit**!  
We hope this tool helps you learn more about systems programming, client-server architecture, and C networking.  
Feel free to star ⭐ the repository if you found it useful!
