# SWYNEX-Final-C-Project
# TaskFlow

**A small, dependable task manager built with modern C++.** TaskFlow runs in the terminal, keeps task data on your computer, and makes it easy to see what needs attention next.

## Highlights

- Create, edit, remove, search, and filter tasks.
- Track category, due date, priority, status, and notes.
- View a progress summary and open high-priority work.
- Save automatically to a readable local TSV file after changes.
- Handle malformed saved rows without losing the remaining task list.
- No third-party libraries required.

## Build

Requires a C++17 compiler and CMake 3.16 or newer.

```sh
cmake -S . -B build
cmake --build build --config Release
```

Run `./build/taskflow` on Linux or macOS, or `build\Release\taskflow.exe` with a Visual Studio CMake build on Windows. With a single-configuration generator on Windows, run `build\taskflow.exe`. The first run creates `tasks.tsv` when you save a task.

You can choose a different data file by passing its path:

```sh
taskflow ./my-project-tasks.tsv
```

## Data format

TaskFlow stores one task per line in `tasks.tsv`. Tabs, newlines, and backslashes inside text are escaped, so notes and titles remain intact. The file is plain text and can be backed up or moved with the project.

## Menu

| Option | Action |
|---|---|
| 1 | View all tasks or filter by status |
| 2 | Add a task |
| 3 | Update task status |
| 4 | Edit task details |
| 5 | Remove a task after confirmation |
| 6 | Search titles, categories, and notes |
| 7 | See task and completion summary |
| 0 | Save and exit |

See [SAMPLE_RUNS.md](SAMPLE_RUNS.md) for example sessions.

## Design notes

The app is dependency-free. `Task` is the data model, `TaskRepository` handles local file storage, and `TaskManager` coordinates the collection and menu actions. STL vectors hold tasks, and standard algorithms support filtering, searching, and sorting. Each task receives a stable integer ID; records are loaded independently so one damaged row does not prevent other rows from loading. Dates use the `YYYY-MM-DD` format.
