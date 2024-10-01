# C++ Static Server.

Server for sharing static files written on C++. Made by Alexander Kudryavtsev as coursework.

### Workflow

Build app:

```bash
docker build -t cppstaticserver .
```

Run app:

```bash
docker run -p 8080:8080 cppstaticserver
```