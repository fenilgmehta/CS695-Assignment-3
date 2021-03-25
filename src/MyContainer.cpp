#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include <sys/syscall.h>
#include <cstring>
// #include <sched.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <sys/syscall.h>
// #include <sys/mount.h>
// #include <sys/stat.h>
// #include <limits.h>
// #include <sys/mman.h>

#include "MyDebugger.hpp"

using namespace std;

// REFER: https://man7.org/linux/man-pages/man2/pivot_root.2.html
static int pivot_root(const char *new_root, const char *put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

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

// REFER: https://www.geeksforgeeks.org/command-line-arguments-in-c-cpp/
int main(int argc, char **argv) {
    log_info("Command line arguments", false, true);
    db(argc);
    for (int32_t i = 0; i < argc; ++i) {
        db(i, argv[i]);
    }
    log_info("----------------------------------------");

    if (argc != 5) {
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


    const auto parent_pid = getpid();
    {
        // 6. CGROUPS - limit CPU usage
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


    // THE MOST IMPORTANT THING
    // REFER: https://man7.org/linux/man-pages/man7/namespaces.7.html
    // REFER: https://man7.org/linux/man-pages/man2/unshare.2.html
    // REFER: https://stackoverflow.com/questions/34872453/unshare-does-not-work-as-expected-in-c-api
    // 1. CLONE_NEWNS   --->  New Mount Namespace
    // 2. CLONE_NEWPID  --->  New PID Namespace
    //                           -> Fork is used because PID Namespace changes are in effect from child process onwards)
    // 3. CLONE_NEWUTS  --->  New UTS namespace
    //                           -> It provide isolation of system identifiers: (a) hostname, and (b) NIS domain name
    //                           -> REFER: https://man7.org/linux/man-pages/man7/uts_namespaces.7.html
    // 6. CGROUP        --->  New CPU and Memory CGROUP
    // 7. Show prompt using /bin/bash
    unshare(CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWCGROUP);

    const auto child_pid = fork();
    if (child_pid == -1) {
        log_error("fork() failed, errno = " + to_string(errno));
        log_error(strerror(errno));
        // ENOMEM == 12
        exit(1);
    }
    if (child_pid != 0) {
        // PARENT will execute from here
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

    // CHILD will execute from here
    // NOTE: "cin" will not work in below code because the child process runs in background
    //       and the console/TTY input is mapped to the parent process's STDIN


    {
        // 1. MOUNT Namespace
        // REFER: https://www.redhat.com/sysadmin/mount-namespaces
        // REFER: https://linuxlink.timesys.com/docs/classic/change_root
        // system("pwd");  // DEBUG
        // Move to the new root filesystem directory
        if (chdir(param_rootfs_path) == -1) {
            log_error(
                    string() + "Could NOT change to dir = \"" + param_rootfs_path + "\", errno = " + to_string(errno));
            log_error(strerror(errno));
            exit(1);
        } else {
            log_success(string() + "chdir(\"" + param_rootfs_path + "\")");
        }
        // system("pwd");  // DEBUG
        if (chroot(".") == -1) {
            log_error(string() + R"(Could NOT "chroot" to path = ")" + param_rootfs_path +
                      "\", errno = " + to_string(errno), false, true);
            log_error(strerror(errno));
            exit(1);
        } else {
            log_success("chroot(\".\")");
        }
        // system("pwd");  // DEBUG
        // REFER: https://man7.org/linux/man-pages/man2/pivot_root.2.html#NOTES:~:text=.-,pivot_root(%22.%22%2C%20%22.%22)
        // REFER: https://tiebing.blogspot.com/2014/02/linux-switchroot-vs-pivotroot-vs-chroot.html
        // REFER: https://superuser.com/questions/1575316/usage-of-chroot-after-pivot-root
        // if (pivot_root(".", ".") == -1) {
        //     log_error(string() + "Could NOT \"pivot_root\", errno = " + to_string(errno), false, true);
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


    {
        // 2. PID Namespace
        // NOTE: the below umount and mount is necessary because of New MOUNT Namespace
        // REFER: https://man7.org/linux/man-pages/man2/mount.2.html

        // // un-mount proc
        // if (mount("none", "/proc", nullptr, MS_PRIVATE | MS_REC, nullptr) == -1) {
        //     log_error(string() + "Could NOT umount proc, errno = " + to_string(errno), false, true);
        //     exit(1);
        // } else {
        //     log_success("umount proc");
        // }

        // mount new /proc
        if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr) == -1) {
            log_error(string() + "Could NOT mount proc, errno = " + to_string(errno), false, true);
            log_error(strerror(errno));
            exit(1);
        } else {
            log_success("mount proc");
        }

        cout << "INFO: Child's NEW PID after unshare is " << getpid() << endl;
    }


    {
        // 3. HOSTNAME - will change hostname in UTS namespace of child
        // See the below link for example on how to change hostname in a UTS namespace
        // REFER: https://man7.org/linux/man-pages/man2/clone.2.html#EXAMPLES:~:text=Change%20hostname%20in%20UTS%20namespace%20of%20child
        if (sethostname(param_new_hostname, strlen(param_new_hostname)) == -1) {
            perror("sethostname");
        }
    }


    // 7. Show Shell
    // my_shell();
    // system("/bin/bash");
    const auto bash_pid = fork();
    if (bash_pid == -1) {
        log_error("fork() to launch /bin/bash failed");
        log_error(strerror(errno));
        exit(1);
    } else if (bash_pid == 0) {
        // Process of "/bin/bash"
        // REFER: https://stackoverflow.com/questions/12596839/how-to-call-execl-in-c-with-the-proper-arguments
        execl("/bin/bash", "/bin/bash", nullptr);
        return 0;
    } else {
        int bash_status;
        waitpid(bash_pid, &bash_status, 0);
        db(bash_status)
    }

    return 0;

    // /* And pivot the root filesystem. */
    // auto new_root = param_rootfs_path;
    // if (pivot_root(new_root, ".") == -1)
    //     printf("pivot_root");
    //
    // /* Switch the current working directory to "/". */
    //
    // if (chdir("/") == -1)
    //     printf("chdir");
    //
    // /* Unmount old root and remove mount point. */
    // if (umount2("..", MNT_DETACH) == -1)
    //     perror("umount2");
    // if (rmdir("..") == -1)
    //     perror("rmdir");


    // clone();
    // setns()

}
