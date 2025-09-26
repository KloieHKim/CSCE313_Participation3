#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

static const size_t BLOCK = 4096;

void die(const char* msg) { perror(msg); std::exit(1); }

int main(int argc, char* argv[]) {
    if (argc != 4 || std::string(argv[1]) != "-n") { // expect exactly 3 arguments, CLI Parsing 
        std::cerr << "Usage: " << argv[0] << " -n <lines> <file>\n"; // print usage message
        return 1;
    }
    long n = std::strtol(argv[2], nullptr, 10); // parse number of lines
    if (n < 0) n = 0; // negative means 0
    const char* path = argv[3]; // get file path

    int fd = open(path, O_RDONLY); // open file read-only + stat
    if (fd < 0) die("open");
    struct stat st{}; // get file size
    if (fstat(fd, &st) != 0) die("fstat");
    off_t size = st.st_size;

    if (n == 0 || size == 0) {
        // Print nothing
        close(fd);
        return 0;
    }

    // Find start offset to print last n lines by scanning backwards
    off_t offset = size;
    long lines_found = 0;
    std::vector<char> buf(BLOCK);

    // count line breaks '\n', if file doesn't end with '\n',
    // the last line (trailing segment) still counts as a line.
    bool ends_with_newline = false; // does file end with newline?
    if (size > 0) { // check last byte
        if (lseek(fd, -1, SEEK_END) == -1) die("lseek"); // seek to last byte
        char c;
        if (read(fd, &c, 1) != 1) die("read"); // read last byte
        ends_with_newline = (c == '\n'); // check if it's newline
    }

    while (offset > 0 && lines_found <= n) { // scan backwards until we find n+1 newlines
        // read BLOCK or remaining bytes if less than BLOCK
        size_t to_read = (offset >= (off_t)BLOCK) ? BLOCK : (size_t)offset;
        off_t new_off = offset - to_read;
        if (lseek(fd, new_off, SEEK_SET) == -1) die("lseek");
        ssize_t r = read(fd, buf.data(), to_read); // read bytes
        if (r < 0) die("read"); // error

        // scan backward
        for (ssize_t i = r - 1; i >= 0; --i) {
            if (buf[(size_t)i] == '\n') {
                // don't count the very last newline if the file ends with newline 
                // want the last n complete lines; if the file ends with '\n' 
                // the byte after it starts a "new empty line" that doesn't exist yet ??
                if (!(offset == size && ends_with_newline && (size_t)i == r - 1)) {
                    ++lines_found;
                    if (lines_found == n + 1) {
                        // Start after this newline
                        off_t start = new_off + i + 1;
                        // Print from start to EOF
                        if (lseek(fd, start, SEEK_SET) == -1) die("lseek"); // seek to start
                        std::vector<char> out(8192);
                        ssize_t rr;
                        while ((rr = read(fd, out.data(), out.size())) > 0) { // read chunk
                            if (write(STDOUT_FILENO, out.data(), rr) != rr) die("write"); // write to stdout
                        }
                        close(fd); // close file
                        return 0;
                    }
                }
            }
        }
        offset = new_off;
    }

    // file has fewer than n newlines → print whole file
    if (lseek(fd, 0, SEEK_SET) == -1) die("lseek");
    std::vector<char> out(8192);
    ssize_t rr;
    while ((rr = read(fd, out.data(), out.size())) > 0) {
        if (write(STDOUT_FILENO, out.data(), rr) != rr) die("write"); // write to stdout
    }
    close(fd); // close file
    return 0;
}

// output:
// kloiekim@csce313:~/CSCE313_Participation3$ ./Q3 -n 10 Hello.txt
// Meanwhile the sun and the clear pebbles of the rain
// are moving across the landscapes,
// over the prairies and the deep trees,
// the mountains and the rivers.
// Meanwhile the wild geese, high in the clean blue air,
// are heading home again.
// Whoever you are, no matter how lonely,
// the world offers itself to your imagination,
// calls to you like the wild geese, harsh and exciting—
// over and over announcing your place
// in the family of things.kloiekim@csce313:~/CSCE313_Participation3$ 

// that's one of my favorite poems by Mary Oliver, "Wild Geese"