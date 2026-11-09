<div align="center">

# 🐚 Shell Implementation

### A Unix-style command-line shell written from scratch in C on GNU/Linux

![Language](https://img.shields.io/badge/language-C-blue?style=for-the-badge&logo=c)
![Platform](https://img.shields.io/badge/platform-GNU%2FLinux-orange?style=for-the-badge&logo=linux&logoColor=white)
![Build](https://img.shields.io/badge/build-GCC%20%2B%20Make-green?style=for-the-badge)
![Readline](https://img.shields.io/badge/input-GNU%20Readline-purple?style=for-the-badge)

*Process creation · Program execution · Built-in commands · I/O redirection · Pipelines · Background jobs · Signal handling*

</div>

---

## 📖 About

This project is a working **Linux shell** built directly on top of POSIX/Linux system calls. It reads commands, parses them, and then runs them the same way a real shell does: by creating processes with `fork()`, loading programs with `execvp()`, wiring up file descriptors with `dup2()` and `pipe()`, and managing children and signals with `waitpid()` and `sigaction()`.

It was built incrementally to connect **operating-system theory** (processes, file descriptors, IPC, signals) with **real system programming**.

---

## ✨ Features at a Glance

| # | Feature | What it does | Key APIs |
|---|---------|--------------|----------|
| 1 | 🎨 **Interactive colored prompt** | Shows the live working directory in color | `getcwd`, ANSI escapes |
| 2 | ⌨️ **Line editing & history** | Arrow-key editing, up/down history, saved across sessions | GNU Readline |
| 3 | ✂️ **Command parser** | Splits input into `argv[]` tokens with an argument limit | `strtok` |
| 4 | 🚀 **External command execution** | Runs any Linux program (`ls`, `gcc`, `grep`, …) | `fork`, `execvp`, `waitpid` |
| 5 | 🧰 **Custom built-in commands** | `mycd`, `myls`, `mystat`, `mycp`, `myhistory`, `exit` | `chdir`, `lstat`, `open`, `read`, `write` |
| 6 | 📂 **I/O redirection** | `<`, `>`, `>>` | `open`, `dup2`, `close` |
| 7 | 🔗 **Multi-stage pipelines** | `cmd1 \| cmd2 \| cmd3 …` (up to 100 commands) | `pipe`, `fork`, `dup2` |
| 8 | 🌙 **Background execution** | `command &` returns the prompt immediately | `setpgid`, `waitpid(WNOHANG)` |
| 9 | 🛡️ **Signal handling** | Ctrl+C never kills the shell; no zombie processes | `sigaction`, `SIGINT`, `SIGCHLD` |
| 10 | ❗ **Error handling** | Invalid syntax, missing files, unknown commands handled safely | `perror`, `errno` |

---

## 🏗️ Architecture

```
                         USER
                           │
                           ▼
              ┌─────────────────────────┐
              │  Prompt / Input Loop    │  readline() + history
              └────────────┬────────────┘
                           ▼
              ┌─────────────────────────┐
              │     Command Parser      │  tokenizes into argv[]
              └────────────┬────────────┘
                           ▼
              ┌─────────────────────────┐
              │   Dispatcher (main.c)   │
              └────────────┬────────────┘
        ┌──────────────┬───┴────────────────────────┬────────────────────────┐
        ▼              ▼                            ▼                        ▼
   contains '&'    contains '|'                   built-in?              external command
        │              │                       (custom_cmds.c)               │
        ▼              ▼                           │                         ▼
  execute_background  execute_pipeline             │                     execute_command
  (jobs.c)            (pipeline.c)                 ▼                     (execute.c)
        │              │                      runs inside the                 │
        │              │                       shell process                fork()
        │              │                                                      │
        └──────────────┴──────────────────────► child: redirection ───────→ execvp()
                                                parent: waitpid / reap jobs

        Signal subsystem (signal.c):  SIGINT  ·  SIGCHLD
```

### Execution order in the main loop

1. Read a line with **Readline** and add it to history
2. **Parse** it into `argv[]`
3. If it contains `&` → run as a **background job**
4. Else if it contains `|` → run as a **pipeline**
5. Else if it is a **built-in** → run it inside the shell process
6. Otherwise → **fork + exec** as an external command

---

## 🔍 Feature Deep-Dive

### 1. 🎨 Interactive Prompt
The prompt is rebuilt every iteration from `getcwd()`, so it always shows where you are, in color:

```
minishell:/home/student/project~$
```

Built on **GNU Readline**, so you get cursor movement, line editing, and history navigation with ↑ / ↓. On `Ctrl+D` (EOF) or typing `exit`, the shell leaves its loop cleanly.

### 2. 📜 Persistent Command History
- History is loaded from `~/.minishell_history` on startup and saved on exit, so it survives between sessions.
- `myhistory` prints every stored command with its history number.

### 3. ✂️ Command Parser
`parse_command()` tokenizes the input line on spaces and fills `argv[]`, terminating it with `NULL` as `execvp()` requires.

```
ls -l /tmp   →   argv[0]="ls"  argv[1]="-l"  argv[2]="/tmp"  argv[3]=NULL
```

Limits are defined in `minishell.h` (`MAX_ARGS`, `MAX_INPUT`).

### 4. 🚀 External Command Execution
`execute_command()` follows the classic model:
- `fork()` creates a child.
- The **child** applies redirection, then calls `execvp()` (and exits with an error message if the command isn't found).
- The **parent** blocks in `waitpid()`, retrying automatically if interrupted by a signal (`EINTR`).

### 5. 🧰 Built-in Commands

Built-ins run **inside the shell process itself** (no fork), which is exactly why `mycd` can change the shell's own working directory.

| Command | Description |
|---------|-------------|
| `mycd [dir]` | Change directory using `chdir()`. With no argument it goes to `$HOME`. Prints an error for invalid paths. |
| `myls` | List non-hidden entries, **color-coded**: 🔵 directories, 🟦 symlinks, 🟢 executables |
| `myls -a` | Include hidden files |
| `myls -i` | Show inode numbers |
| `myls -l` | Long listing: permissions, link count, owner, group, size, timestamp |
| `mystat <file>` | Detailed file info: size, blocks, I/O block, file type, inode, links, permissions, UID/GID, access/modify/change times |
| `mycp <src> <dst>` | Copy a file using `open`/`read`/`write` system calls |
| `myhistory` | Show command history |
| `exit` | Quit the shell and save history |

### 6. 📂 I/O Redirection
| Operator | Meaning | Implementation |
|----------|---------|----------------|
| `<`  | Read stdin from a file | `open(O_RDONLY)` + `dup2(fd, STDIN_FILENO)` |
| `>`  | Write stdout to a file (truncate) | `open(O_WRONLY\|O_CREAT\|O_TRUNC)` + `dup2(fd, STDOUT_FILENO)` |
| `>>` | Append stdout to a file | `open(O_WRONLY\|O_CREAT\|O_APPEND)` + `dup2(fd, STDOUT_FILENO)` |

The redirection operator and filename are removed from `argv[]` before `execvp()`, so the program only sees its real arguments. Multiple redirections in one command are supported (e.g. `sort < in.txt > out.txt`), and missing filenames or back-to-back operators are reported as errors.

### 7. 🔗 Pipelines (N commands)
`execute_pipeline()` generalizes piping to any number of stages:

1. Splits `argv[]` at every `|` into separate commands
2. Validates syntax (rejects leading, trailing, or consecutive `|`)
3. Creates **N−1 pipes** for N commands
4. Forks one child per command and connects stdin/stdout with `dup2()`
5. Closes every unused descriptor (so readers see EOF correctly)
6. Waits for all children

Redirection also works inside pipeline stages.

```bash
cat file.txt | grep Linux | wc -l
```

### 8. 🌙 Background Jobs
Append `&` to run a command without blocking the prompt:

```
minishell:~$ sleep 10 &
[Background PID: 5421]
minishell:~$
```

The child is placed in **its own process group** with `setpgid()` (set by both parent and child to avoid a race), so a terminal Ctrl+C aimed at the foreground doesn't hit it.

### 9. 🛡️ Signal Handling & Zombie Prevention
Handlers are installed with `sigaction()`:

- **`SIGINT` (Ctrl+C)** – The shell survives. At the prompt it clears the line and shows a fresh prompt; while a foreground job runs, only that job is interrupted.
- **`SIGCHLD`** – Sets a flag when a child changes state. A Readline event hook (`check_background_jobs`) then reaps finished children with `waitpid(-1, WNOHANG)` in a loop, prints `[Background process <pid> completed]`, and redraws the prompt. **No zombies accumulate.**

Using flags (`volatile sig_atomic_t`) inside handlers keeps them async-signal-safe; the real work happens in normal code.

---

## 📁 Project Structure

```
shell/
├── main.c          # Shell loop, prompt, history, dispatcher, Readline event hook
├── parser.c        # Tokenizer: input line → argv[]
├── execute.c       # Foreground execution + I/O redirection handler
├── pipeline.c      # N-stage pipeline engine
├── jobs.c          # Background execution + zombie reaping
├── custom_cmds.c   # Built-ins: mycd, myls, mystat, mycp, myhistory
├── signal.c        # SIGINT / SIGCHLD handlers and setup
├── minishell.h     # Shared declarations, limits, extern flags
└── Makefile        # Build rules
```

---

## ⚙️ Getting Started

### Prerequisites
- GNU/Linux (a VM is fine)
- `gcc` and `make`
- GNU Readline development library

```bash
# Debian / Ubuntu
sudo apt install build-essential libreadline-dev

# Fedora / RHEL
sudo dnf install gcc make readline-devel
```

### Build & Run

```bash
git clone <your-repo-url>
cd <your-repo-folder>

make            # compiles all modules and links with -lreadline
./minishell     # start the shell
```

---

## 🧪 Usage Examples

```bash
# Basic commands
minishell:~$ echo Hello Linux
Hello Linux

# Built-ins
minishell:~$ mycd /tmp
minishell:/tmp~$ myls -l
minishell:/tmp~$ mystat files.txt
minishell:/tmp~$ mycp a.txt b.txt

# Redirection
minishell:~$ ls > files.txt
minishell:~$ echo Linux >> log.txt
minishell:~$ cat < files.txt

# Pipelines
minishell:~$ ls -l | grep .c
minishell:~$ cat file.txt | grep Linux | wc -l

# Background job
minishell:~$ sleep 10 &
[Background PID: 5421]

# Error handling
minishell:~$ invalidcommand
execvp: No such file or directory
```

---

## ✅ Test Cases

| ID | Command / Action | Expected Result |
|----|------------------|-----------------|
| T01 | `myls` | Colored listing of current directory |
| T02 | `ls -l` | External command runs, prompt returns |
| T03 | `echo Hello Linux` | Prints `Hello Linux` |
| T04 | `mycd /tmp` | Working directory changes (shown in prompt) |
| T05 | `mycd /invalid` | Error displayed, shell continues |
| T06 | `ls > files.txt` | Output stored in `files.txt` |
| T07 | `echo Linux >> log.txt` | Text appended, old content kept |
| T08 | `cat < files.txt` | `cat` reads `files.txt` as stdin |
| T09 | `ls -l \| grep .c` | `grep` filters `ls` output |
| T10 | `cat file.txt \| grep Linux \| wc -l` | Three-stage pipeline works |
| T11 | `sleep 10 &` | Prompt returns immediately, PID shown |
| T12 | Ctrl+C during foreground `sleep` | Job stops, shell stays alive |
| T13 | `invalidcommand` | Error message, shell continues |
| T14 | Several background jobs | Completion notices shown, no zombies (`ps` to verify) |
| T15 | `exit` | Shell terminates cleanly, history saved |

---

## 🧠 Concepts Demonstrated

- **Process management:** `fork`, `execvp`, `waitpid`, process groups
- **File descriptors:** stdin / stdout / stderr, `open`, `dup2`, `close`
- **Inter-process communication:** anonymous pipes with `pipe()`
- **Signals:** `sigaction`, `SIGINT`, `SIGCHLD`, async-signal-safe flags
- **File system APIs:** `stat`, `lstat`, `opendir`/`readdir`, `getpwuid`, `getgrgid`
- **Modular C design:** multi-file project, shared header, Makefile build

---

## ⚠️ Known Limitations

- Tokenizing is on spaces only: no quoted strings, wildcards, or environment-variable expansion
- No `stderr` redirection (`2>`), `&&`, `||`, or `jobs` / `fg` / `bg`
- Built-ins run only as standalone commands (not inside a pipeline), and `&` is not combined with pipes
- `myls` accepts a single option at a time and lists only the current directory

---

## 🚀 Future Improvements

- [ ] Quoted strings and wildcard expansion
- [ ] `2>` / `2>&1` redirection
- [ ] `jobs`, `fg`, `bg` job control
- [ ] `&&` and `||` operators
- [ ] Environment variables (`export`, `unset`, `$VAR`)
- [ ] Aliases and a configurable prompt

---

## 👤 Author

**Koteswar Rao Golagani**
