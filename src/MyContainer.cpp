#include <iostream>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include "MyDebugger.hpp"

using namespace std;


void my_shell() {
    string cmd;
    while (true) {
        cout << "\033[31m" << "MyShell> " << "\033[39m";

        // REFER: https://www.geeksforgeeks.org/getline-string-c/
        getline(cin, cmd);
        if (!cin) break;  // REFER: https://stackoverflow.com/questions/27003967/how-to-check-if-cin-is-int-in-c
        if (cmd == "exit") break;

        const auto res = system(cmd.c_str());
        if (res != 0) {
            log_warning(string() + "system(...) returned with = " + to_string(res));
        }

        // DEBUG
        // for(int32_t i = 0; i < cmd.size()+3; ++i) db(static_cast<int32_t>(cmd.c_str()[i]));
    }
}


// REFER: https://stackoverflow.com/questions/8518743/get-directory-from-file-path-c
std::string last_dir_name(const std::string &fname) {
    const bool ends_with_separator = (fname[fname.size() - 1] == '/' || fname[fname.size() - 1] == '\\');
    // db(ends_with_separator);
    const std::size_t fname_len_new = fname.size() - (ends_with_separator ? 1 : 0);
    // db(fname.size(), fname_len_new);
    const std::size_t pos = fname.find_last_of(
            "\\/",
            fname_len_new - 1
    );
    // db(pos, pos == std::string::npos);
    return (pos == std::string::npos) ? fname.substr(0, fname_len_new) : fname.substr(pos + 1, fname_len_new - pos - 1);
}


// REFER: https://www.geeksforgeeks.org/command-line-arguments-in-c-cpp/
int main(int argc, char **argv) {
    // REFER: https://man7.org/linux/man-pages/man3/system.3.html
    if (system(nullptr) == 0) {
        log_error("system(NULL) returned 0 => no shell is available");
        return 1;
    }

    log_info("Command line arguments");
    db(argc);
    for (int32_t i = 0; i < argc; ++i) {
        db(i, argv[i]);
    }
    log_info("----------------------------------------");

    if (argc != 7) {
        log_error("Invalid command line arguments");
        return 1;
    }

    // Path to the new root filesystem
    const char *param_rootfs_path = argv[1];
    // New hostname to be used for the container shell
    const char *param_new_hostname = argv[2];
    // Only the C-GROUP name is to be passed,
    // eg: cs695group ---> /sys/fs/cgroup/cpu/cs695group
    // Pass empty string ("") if the container is not
    // to be added to any CPU cgroup
    const char *param_cgroup_cpu = argv[3];
    const char *param_cgroup_memory = argv[4];
    const char *param_network_namespace = argv[5];
    const char *param_dir_mount = argv[6];


    // THE MOST IMPORTANT THING
    // REFER: https://man7.org/linux/man-pages/man7/namespaces.7.html
    // REFER: https://man7.org/linux/man-pages/man2/unshare.2.html
    // REFER: https://stackoverflow.com/questions/34872453/unshare-does-not-work-as-expected-in-c-api
    // 1. CGROUP        --->  New CPU and Memory CGROUP
    // 2. Join a Network Namespace if given
    // 3. Mount custom directory inside the root filesystem of the container
    // 4. unshare
    //    4a. CLONE_NEWNS  ---> New Mount Namespace
    //    4b. CLONE_NEWPID ---> New PID Namespace
    //                            -> Fork is used because PID Namespace changes are in effect from the child
    //                               process onwards
    //    4c. CLONE_NEWUTS ---> New UTS namespace
    //                            -> It provide isolation of system identifiers: (a) hostname, and (b) NIS domain name
    //                            -> REFER: https://man7.org/linux/man-pages/man7/uts_namespaces.7.html
    // 5. Show prompt using /bin/bash


    const auto parent_pid = getpid();
    // 1. CGROUPS - limit CPU usage
    {
        // REFER: https://man7.org/linux/man-pages/man7/cgroups.7.html
        //            -> https://www.kernel.org/doc/Documentation/scheduler/sched-bwc.txt
        //            -> https://www.kernel.org/doc/Documentation/cgroup-v1/memory.txt
        // NOTE: CGROUPS version 1 is used

        // NOTE: it is important to do these CGROUP operations before the "unshare"
        //       system call because after "unshare", new namespaces and new cgroups
        //       are created which interfere with this

        if (param_cgroup_cpu[0] == '\0') {
            log_info("NO CPU cgroup passed as a command line argument");
        } else {
            log_info(string() + "Using CPU cgroup = '" + param_cgroup_cpu + "'");
            log_info((string("echo ") + to_string(parent_pid) +
                      " > '/sys/fs/cgroup/cpu/" + param_cgroup_cpu + "/tasks'").c_str());
            log_info((string("echo ") + to_string(parent_pid) +
                      " > '/sys/fs/cgroup/cpu/" + param_cgroup_cpu + "/cgroup.procs'").c_str());
            const auto res = system((string("echo ") + to_string(parent_pid) +
                                     " > '/sys/fs/cgroup/cpu/" + param_cgroup_cpu + "/cgroup.procs'").c_str());
            if (res != 0) {
                log_warning(string() + "system(...) returned with = " + to_string(res));
            }
        }

        if (param_cgroup_memory[0] == '\0') {
            log_info("NO MEMORY cgroup passed as a command line argument");
        } else {
            log_info(string() + "Using MEMORY cgroup = '" + param_cgroup_memory + "'");
            log_info((string("echo ") + to_string(parent_pid) +
                      " > '/sys/fs/cgroup/memory/" + param_cgroup_memory + "/tasks'").c_str());
            log_info((string("echo ") + to_string(parent_pid) +
                      " > '/sys/fs/cgroup/memory/" + param_cgroup_memory + "/cgroup.procs'").c_str());
            const auto res = system((string("echo ") + to_string(parent_pid) +
                                     " > '/sys/fs/cgroup/memory/" + param_cgroup_memory + "/cgroup.procs'").c_str());
            if (res != 0) {
                log_warning(string() + "system(...) returned with = " + to_string(res));
            }
        }
    }


    // 2. Join a Network Namespace if given
    {
        // REFER: https://man7.org/linux/man-pages/man2/setns.2.html
        log_info("Network Namespace");
        auto netns_fd = open(("/var/run/netns/" + string(param_network_namespace)).c_str(), O_RDONLY);
        if (netns_fd == -1) {
            log_error("open(Network Namespace), errno = " + to_string(errno));
            log_error(strerror(errno));
            log_warning("Not setting any Network Namespace");
        } else {
            log_success("open(Network Namespace)");
            if (setns(netns_fd, CLONE_NEWNET) == -1) {
                log_error("setns(...) for Network Namespace failed, errno = " + to_string(errno));
                log_error(strerror(errno));
            } else {
                log_success("setns(...)");
            }
        }
    }


    // 3. Mount custom directory inside the root filesystem of the container
    if (param_dir_mount[0] == '\0') {
        // Do NOT mount anything
        log_info("NO custom director to mount");
    } else {
        // Mount if folder with same name does not exist in rootfs/mnt
        const auto rootfs_str_len = strlen(param_rootfs_path);
        db(rootfs_str_len);
        const bool has_slash_at_end = param_rootfs_path[rootfs_str_len - 1] == '/';
        db(has_slash_at_end);
        const string str_param_dir_mount(param_dir_mount);
        const string mnt_folder_name = last_dir_name(str_param_dir_mount);

        // REFER: https://askubuntu.com/questions/557733/what-is-the-difference-between-ln-s-and-mount-bind
        // REFER: https://askubuntu.com/questions/205841/how-do-i-mount-a-folder-from-another-partition
        log_info((string() + "mkdir '" + param_rootfs_path + (has_slash_at_end ? "" : "/") +
                  "mnt/" + mnt_folder_name + "'").c_str());
        const auto res1 = system((string() + "mkdir '" + param_rootfs_path + (has_slash_at_end ? "" : "/") +
                                  "mnt/" + mnt_folder_name + "'").c_str());
        if (res1 != 0) {
            log_warning("mkdir for custom directory mount failed with res1 = " + to_string(res1));
        } else {
            log_info((string() + "mount --bind '" + param_dir_mount + "' '"
                      + param_rootfs_path + (has_slash_at_end ? "" : "/") + "mnt/" + mnt_folder_name + "'").c_str());
            const auto res2 = system((string() + "mount --bind '" + param_dir_mount + "' '"
                                      + param_rootfs_path + (has_slash_at_end ? "" : "/") +
                                      "mnt/" + mnt_folder_name + "'").c_str());
            if (res2 != 0) {
                log_warning("Custom directory mount failed with res2 = " + to_string(res2));
            } else {
                log_success("Mounted \"" + str_param_dir_mount + R"(" under "/mnt")");
            }
        }
    }


    // 4. unshare the indicated namespaces from the parent process
    unshare(CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWCGROUP);

    const auto child_pid = fork();
    if (child_pid == -1) {
        log_error("fork() failed, errno = " + to_string(errno));
        log_error(strerror(errno));
        // ENOMEM == 12
        exit(1);
    }
    if (child_pid != 0) {
        // A. PARENT will execute from here
        log_info(string() + "Parent PID = " + to_string(parent_pid));
        log_info(string() + "Child PID = " + to_string(child_pid));

        int status;
        waitpid(-1, &status, 0);
        db(status);
        if (status == 9) {
            log_error(
                    "READ More at https://stackoverflow.com/questions/40888164/c-program-crashes-with-exit-code-9-sigkill"
            );
        }
        return status;
    }

    // B. CHILD will execute from here
    // NOTE: "cin" will not work in below code because the child process runs in background
    //       and the console/TTY input is mapped to the parent process's STDIN


    // 4a. MOUNT Namespace
    {
        // REFER: https://www.redhat.com/sysadmin/mount-namespaces
        // REFER: https://linuxlink.timesys.com/docs/classic/change_root
        // system("pwd");  // DEBUG
        // Move to the new root filesystem directory
        if (chdir(param_rootfs_path) == -1) {
            log_error(string() + "Could NOT change to dir = \"" + param_rootfs_path +
                      "\", errno = " + to_string(errno));
            log_error(strerror(errno));
            exit(1);
        } else {
            log_success(string() + "chdir(\"" + param_rootfs_path + "\")");
            // system("pwd");  // DEBUG
            if (chroot(".") == -1) {
                log_error(string() + R"(Could NOT "chroot" to path = ")" + param_rootfs_path +
                          "\", errno = " + to_string(errno));
                log_error(strerror(errno));
                exit(1);
            } else {
                log_success("chroot(\".\")");
                log_success("Mount Namespace");
            }
        }
        // system("pwd");  // DEBUG
        // REFER: https://unix.stackexchange.com/questions/456620/how-to-perform-chroot-with-linux-namespaces
        // REFER: https://man7.org/linux/man-pages/man2/pivot_root.2.html#NOTES:~:text=.-,pivot_root(%22.%22%2C%20%22.%22)
        // REFER: https://tiebing.blogspot.com/2014/02/linux-switchroot-vs-pivotroot-vs-chroot.html
        // REFER: https://superuser.com/questions/1575316/usage-of-chroot-after-pivot-root
        // if (pivot_root(".", ".") == -1) {
        //     log_error(string() + "Could NOT \"pivot_root\", errno = " + to_string(errno));
        //     exit(1);
        // } else {
        //     log_success(R"(pivot_root(".", "."))");
        // }
        // if (umount2(".", MNT_DETACH) == -1) {
        //     log_error(string() + "umount2(\".\", MNT_DETACH), errno = " + to_string(errno));
        //     exit(1);
        // } else {
        //     log_success("umount2(\".\", MNT_DETACH)");
        // }
    }


    // 4b. PID Namespace
    {
        // NOTE: the below umount and mount is necessary because of New MOUNT Namespace
        // REFER: https://man7.org/linux/man-pages/man2/mount.2.html

        // // un-mount proc
        // if (mount("none", "/proc", nullptr, MS_PRIVATE | MS_REC, nullptr) == -1) {
        //     log_error(string() + "Could NOT umount proc, errno = " + to_string(errno));
        //     exit(1);
        // } else {
        //     log_success("umount proc");
        // }

        // mount new /proc
        if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr) == -1) {
            log_error(string() + "Could NOT mount proc, errno = " + to_string(errno));
            log_error(strerror(errno));
            exit(1);
        } else {
            log_success("mount proc (PID Namespace)");
        }

        log_info("Child's NEW PID after unshare = " + to_string(getpid()));
    }


    // 4c. HOSTNAME - will change hostname in UTS namespace of CHILD (i.e. B)
    {
        // See the below link for example on how to change hostname in a UTS namespace
        // REFER: https://man7.org/linux/man-pages/man2/clone.2.html#EXAMPLES:~:text=Change%20hostname%20in%20UTS%20namespace%20of%20child
        if (sethostname(param_new_hostname, strlen(param_new_hostname)) == -1) {
            perror("sethostname");
        } else {
            log_success("Hostname successfully set (UTS Namespace)");
        }
    }


    // 5. Show Shell
    const auto bash_pid = fork();
    if (bash_pid == -1) {
        log_error("fork() to launch /bin/bash failed");
        log_error(strerror(errno));
        log_info("Launching custom shell");
        my_shell();
    } else if (bash_pid == 0) {
        // C. /bin/bash
        // REFER: https://stackoverflow.com/questions/12596839/how-to-call-execl-in-c-with-the-proper-arguments
        execl("/bin/bash", "/bin/bash", nullptr);
        return 0;
    } else {
        // B. CHILD
        int bash_status;
        waitpid(bash_pid, &bash_status, 0);
        db(bash_status);

        // REFER: https://www.howtogeek.com/414634/how-to-mount-and-unmount-storage-devices-from-the-linux-terminal/
        // If umount is not done, then it remains mounted
        const auto res = system("umount /proc");
        if (res == 0) {
            log_success("umount /proc");
        } else {
            log_error("system(\"umount /proc\") failed with result = " + to_string(res));
        }

        if (param_dir_mount[0] == '\0') {
            // Do NOT mount anything
            log_info("NO custom directory to umount");
        } else {
            // Mount if folder with same name does not exist in rootfs/mnt
            const auto rootfs_str_len = strlen(param_rootfs_path);
            const string str_param_dir_mount(param_dir_mount);
            const string mnt_folder_name = last_dir_name(str_param_dir_mount);

            // REFER: https://askubuntu.com/questions/557733/what-is-the-difference-between-ln-s-and-mount-bind
            // REFER: https://askubuntu.com/questions/205841/how-do-i-mount-a-folder-from-another-partition
            log_info((string() + "umount '/mnt/" + mnt_folder_name + "'").c_str());
            if (system((string() + "umount '/mnt/" + mnt_folder_name + "'").c_str()) == 0) {
                log_success(string() + "umount '/mnt/" + mnt_folder_name + "'");
            } else {
                log_error("umount '/mnt/" + mnt_folder_name + "', errno = " + to_string(errno));
                log_error(strerror(errno));
            }

            log_info((string() + "rmdir '/mnt/" + mnt_folder_name + "'").c_str());
            if (system((string() + "rmdir '/mnt/" + mnt_folder_name + "'").c_str()) == 0) {
                log_success(string() + "rmdir '/mnt/" + mnt_folder_name + "'");
            } else {
                log_error("rmdir '/mnt/" + mnt_folder_name + "', errno = " + to_string(errno));
                log_error(strerror(errno));
            }
        }

        log_info("Exiting...");
    }

    return 0;
}
