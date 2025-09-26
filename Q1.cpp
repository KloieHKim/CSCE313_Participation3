#include <sys/stat.h>  // stat(), lstat(), struct stat, S_ISDIR, ...
#include <pwd.h>       // getpwuid()
#include <grp.h>       // getgrgid()
#include <unistd.h>    // POSIX (not strictly needed here, but common)
#include <ctime>       // time_t, localtime_r
#include <iostream>    // C++ streams
#include <iomanip>     // std::put_time, std::setw

std::string mode_to_string(mode_t m) { // convert mode to string like "-rw-r--r--"
    std::string s(10, '-');
    // file type
    if (S_ISREG(m)) s[0] = '-'; // regular file
    else if (S_ISDIR(m)) s[0] = 'd'; // directory
    else if (S_ISLNK(m)) s[0] = 'l'; // symlink
    else if (S_ISCHR(m)) s[0] = 'c'; // char device
    else if (S_ISBLK(m)) s[0] = 'b'; // block device
    else if (S_ISSOCK(m)) s[0] = 's'; // socket
    else if (S_ISFIFO(m)) s[0] = 'p'; // fifo

    // permissions
    const char rwx[] = {'r','w','x'}; // read, write, execute
    for (int i = 0; i < 9; ++i) { // user, group, others
        if (m & (1 << (8 - i))) s[i+1] = rwx[i%3]; // set rwx
    }
    // suid/sgid/sticky
    if (m & S_ISUID) s[3] = (s[3] == 'x') ? 's' : 'S'; // user execute
    if (m & S_ISGID) s[6] = (s[6] == 'x') ? 's' : 'S'; // group execute
    if (m & S_ISVTX) s[9] = (s[9] == 'x') ? 't' : 'T'; // sticky bit, sticky bit means only owner can delete/rename file in dir
    return s;
}

std::string fmt_time(time_t t) { // format time_t to "YYYY-MM-DD HH:MM:SS"
    std::tm lt{}; // local time
    localtime_r(&t, &lt); // thread-safe version of localtime
    std::ostringstream oss; //
    oss << std::put_time(&lt, "%Y-%m-%d %H:%M:%S"); 
    return oss.str(); // return formatted string
}

int main(int argc, char* argv[]) {
    if (argc != 2) { // expect exactly one argument
        std::cerr << "Usage: " << argv[0] << " <path>\n"; // print usage message
        return 1; // exit with error code
    }
    const char* path = argv[1]; // get path from command line
    struct stat st{}; // stat structure to hold file info
    if (lstat(path, &st) != 0) { // use lstat to not follow symlinks
        perror("lstat"); // error message
        return 1;
    }

    struct passwd* pw = getpwuid(st.st_uid); // get user info
    struct group* gr = getgrgid(st.st_gid); // get group info

    std::cout << "  Path: " << path << "\n"; // print file path
    std::cout << "  Size: " << st.st_size << " bytes\n"; // print file size
    std::cout << "Blocks: " << st.st_blocks << "\n"; // print number of blocks
    std::cout << "  Inode: " << st.st_ino << "\n"; // print inode number
    std::cout << " Links: " << st.st_nlink << "\n"; // print number of links
    std::cout << "  Mode: " << mode_to_string(st.st_mode) // print mode string
              << " (" << std::oct << std::showbase << (st.st_mode & 07777) << std::dec << ")\n"; //octal
    std::cout << " Owner: " << (pw ? pw->pw_name : std::to_string(st.st_uid)) // print owner name or uid
              << " (uid " << st.st_uid << ")\n"; // print uid
    std::cout << " Group: " << (gr ? gr->gr_name : std::to_string(st.st_gid)) // print group name or gid
              << " (gid " << st.st_gid << ")\n"; // print gid
    std::cout << "Access: " << fmt_time(st.st_atime) << "\n"; // print access time
    std::cout << "Modify: " << fmt_time(st.st_mtime) << "\n"; // print modification time
    std::cout << "Change: " << fmt_time(st.st_ctime) << "\n"; // print change time

    return 0;
}


// output:
// kloiekim@csce313:~/CSCE313_Participation3$ ./Q1 .
//   Path: .
//   Size: 4096 bytes
// Blocks: 8
//   Inode: 526767
//  Links: 3
//   Mode: drwxrwxr-x (0775)
//  Owner: kloiekim (uid 1000)
//  Group: kloiekim (gid 1000)
// Access: 2025-09-26 19:34:18
// Modify: 2025-09-26 19:34:17
// Change: 2025-09-26 19:34:17
// kloiekim@csce313:~/CSCE313_Participation3$ 