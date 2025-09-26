#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm> // std::sort
#include <iostream>
#include <iomanip>
#include <sstream>

std::string mode_to_string(mode_t m) { // convert mode to string like "-rw-r--r--"
    std::string s(10, '-');
    if (S_ISREG(m)) s[0] = '-';
    else if (S_ISDIR(m)) s[0] = 'd';
    else if (S_ISLNK(m)) s[0] = 'l';
    else if (S_ISCHR(m)) s[0] = 'c';
    else if (S_ISBLK(m)) s[0] = 'b';
    else if (S_ISSOCK(m)) s[0] = 's';
    else if (S_ISFIFO(m)) s[0] = 'p';
    const char rwx[] = {'r','w','x'};
    for (int i = 0; i < 9; ++i) {
        if (m & (1 << (8 - i))) s[i+1] = rwx[i%3];
    }
    if (m & S_ISUID) s[3] = (s[3] == 'x') ? 's' : 'S';
    if (m & S_ISGID) s[6] = (s[6] == 'x') ? 's' : 'S';
    if (m & S_ISVTX) s[9] = (s[9] == 'x') ? 't' : 'T';
    return s;
}

std::string fmt_time(time_t t) { // format time_t to "YYYY-MM-DD HH:MM"
    std::tm lt{};
    localtime_r(&t, &lt);
    std::ostringstream oss;
    oss << std::put_time(&lt, "%Y-%m-%d %H:%M");
    return oss.str();
}

std::string join_path(const std::string& dir, const std::string& name) { // join dir and name with '/'
    if (dir == "." || dir.empty()) return name;
    if (dir.back() == '/') return dir + name;
    return dir + "/" + name;
}

int main(int argc, char* argv[]) { // myls [-l] [directory]
    bool long_fmt = false;
    std::string dirpath = ".";

    // parse args: myls [-l] [directory]
    int i = 1;
    if (i < argc && std::string(argv[i]) == "-l") { long_fmt = true; ++i; }
    if (i < argc) dirpath = argv[i];

    DIR* dp = opendir(dirpath.c_str()); // open directory
    if (!dp) { // error
        perror("opendir");
        return 1;
    }

    std::vector<std::string> names; // store entry names
    if (struct dirent* d; true) { 
        while ((d = readdir(dp)) != nullptr) {
            if (std::strcmp(d->d_name, ".") == 0 || std::strcmp(d->d_name, "..") == 0) continue; // skip . and ..
            names.emplace_back(d->d_name); // add name to list
        }
    }
    closedir(dp);

    // sort names to be nice
    std::sort(names.begin(), names.end()); 

    if (!long_fmt) { // jsut names
        for (auto& n : names) std::cout << n << "\n";
        return 0;
    }

    // long format: stat each entry
    for (auto& n : names) {
        std::string full = join_path(dirpath, n); // full path
        struct stat st{};
        if (lstat(full.c_str(), &st) != 0) { // lstat to not follow symlinks
            perror(("lstat: " + full).c_str());
            continue;
        }
        std::string mode = mode_to_string(st.st_mode); // mode string
        struct passwd* pw = getpwuid(st.st_uid); // get user info
        struct group* gr = getgrgid(st.st_gid); // get group info
        std::string owner = pw ? pw->pw_name : std::to_string(st.st_uid); // owner name or uid
        std::string group = gr ? gr->gr_name : std::to_string(st.st_gid); // group name or gid

        std::cout << mode << " "
                  << std::setw(2) << st.st_nlink << " "
                  << std::left << std::setw(8) << owner << " "
                  << std::left << std::setw(8) << group << " "
                  << std::right << std::setw(8) << st.st_size << " "
                  << fmt_time(st.st_mtime) << " "
                  << n;

        // if symlink, show "-> target"
        if (S_ISLNK(st.st_mode)) {
            char target[PATH_MAX];
            ssize_t len = readlink(full.c_str(), target, sizeof(target)-1); // leave space for null terminator
            if (len >= 0) { // success
                target[len] = '\0';
                std::cout << " -> " << target;
            }
        }
        std::cout << "\n";
    }
    return 0;
}

// output:
// kloiekim@csce313:~/CSCE313_Participation3$ ./Q2
// .git
// Q1
// Q1.cpp
// Q2
// Q2.cpp
// Q3
// Q3.cpp
// kloiekim@csce313:~/CSCE313_Participation3$ 