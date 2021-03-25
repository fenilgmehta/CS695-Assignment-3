# CS695-Assignment-3

- CS 695 - Assignment 3


### Solution Details and Steps

- Prerequisites
    
    1. Root filesystem `rootfs.tar.gz` from [https://github.com/ericchiang/containers-from-scratch/releases/download/v0.1.0/rootfs.tar.gz](https://github.com/ericchiang/containers-from-scratch/releases/download/v0.1.0/rootfs.tar.gz)
       ```sh
       cd src
       wget https://github.com/ericchiang/containers-from-scratch/releases/download/v0.1.0/rootfs.tar.gz
       ```

- Steps to run/test the program
    
    1. **NOTE**: all the below commands need to be executed using **`root`** privileges 
    
    2. Change the working directory
       ```sh
       cd src
       ```
    
    3. Counter and Root filesystem name generating functions
       ```sh
       # Variable: v_NUM
       function f_update_counter() { 
           if [[ -f 'CONTAINER_NUMBER.txt' ]]; then
               v_NUM=$(cat 'CONTAINER_NUMBER.txt')
           else
               v_NUM=0
           fi

           # REFER: https://linuxize.com/post/bash-increment-decrement-variable/
           v_NUM=$(($v_NUM + 1))
           echo $v_NUM > 'CONTAINER_NUMBER.txt'       
           echo "v_NUM = ${v_NUM}"
       }

       # Variable: v_NEW_DIR_NAME
       function f_get_new_dir_name() {
           while true; do
               f_update_counter
               
               v_NEW_DIR_NAME="rootfs_${v_NUM}"
               if [[ -d "${v_NEW_DIR_NAME}" ]]; then
                   echo "WARNING: Directory already exists: '${v_NEW_DIR_NAME}'"
               else
                   echo "Using Directory Name: '${v_NEW_DIR_NAME}'"
                   break
               fi       
           done
       }
       ```
    
    4. Extract the root filesystem `rootfs.tar.gz`
       ```sh
       f_get_new_dir_name

       # REFER: https://unix.stackexchange.com/questions/11018/how-to-choose-directory-name-during-untarring
       tar --verbose --gzip --extract --file "rootfs.tar.gz"  # OR, tar -vzxf "rootfs.tar.gz"
       mv 'rootfs' "${v_NEW_DIR_NAME}"

       # REFER: https://unix.stackexchange.com/questions/327226/must-dev-random-and-dev-urandom-be-created-each-boot-or-are-they-static-files
       # The below lines will create "random" and "urandom" files inside the "${v_NEW_DIR_NAME}/dev"
       # directory. This is necessary because python inside this root filesystem needs these files.
       mknod -m 0666 "${v_NEW_DIR_NAME}/dev/random" c 1 8
       mknod -m 0666 "${v_NEW_DIR_NAME}/dev/urandom" c 1 9
       chown root:root "${v_NEW_DIR_NAME}/dev/random" "${v_NEW_DIR_NAME}/dev/urandom"
       ```

    5. Create CPU and Memory cgroup (OPTIONAL)
       ```sh
       # NOTE: can view the CGROUP a process is in: 
       #       System Monitor > Processes > Right Click on Process Name > Scroll Down to see "Control Group"

       # REFER: https://man7.org/linux/man-pages/man7/cgroups.7.html

       # REFER: https://www.kernel.org/doc/Documentation/scheduler/sched-bwc.txt
       CGROUP_CPU_NAME="cs695cpu"
       mkdir "/sys/fs/cgroup/cpu/${CGROUP_CPU_NAME}"
       # With 1 second period, 0.5 second quota will be equivalent to 50% of 1 CPU
       echo "1000000" > "/sys/fs/cgroup/cpu/${CGROUP_CPU_NAME}/cpu.cfs_period_us"
       echo "500000" > "/sys/fs/cgroup/cpu/${CGROUP_CPU_NAME}/cpu.cfs_quota_us"

       # REFER: https://www.kernel.org/doc/Documentation/cgroup-v1/memory.txt
       CGROUP_MEMORY_NAME="cs695mem"
       mkdir "/sys/fs/cgroup/memory/${CGROUP_MEMORY_NAME}"
       # set limit of memory usage      = 16 MB = 16 * 1024 * 1024 = 16777216
       # set limit of memory+Swap usage = 24 MB = 24 * 1024 * 1024 = 25165824
       echo "16777216" > "/sys/fs/cgroup/memory/${CGROUP_MEMORY_NAME}/memory.limit_in_bytes"
       echo "25165824" > "/sys/fs/cgroup/memory/${CGROUP_MEMORY_NAME}/memory.memsw.limit_in_bytes"

       echo $$ > "/sys/fs/cgroup/cpu/${CGROUP_CPU_NAME}/cgroup.procs"
       echo $$ > "/sys/fs/cgroup/memory/${CGROUP_MEMORY_NAME}/cgroup.procs"

       # Testing of CPU, view the CPU usage via the System Monitor
       while true; do
         a=$((12345*12345))
       done
       while true; do a=$((12345*12345)); done &  # background process

       # Testing Memory Limits, view the Memory usage via SystemMonitor
       # This will abort the process in the 13th iteration for 16 MB (RAM), 24 MB (RAM + SWAP)
       # 12th iteration = ~10 MB, 13th iteration = ~28 MB, 14th iteration = ~83 MB, 15th iteration = ~247 MB
       # REFER: https://stackoverflow.com/questions/40888164/c-program-crashes-with-exit-code-9-sigkill
       a="123"
       for i in {1..12}; do
         echo $i
         a="${a}${a}${a}"
         sleep 1
       done
       a="${a}${a}"
       ```

    6. Compile and Execute the container shell
       ```sh
       # CGROUP_CPU_PATH is relative path to "/sys/fs/cgroup/cpu/"
       # CGROUP_MEMORY_PATH is relative path to "/sys/fs/cgroup/memory/"
       make all
       ./MyContainer.out <ROOT_FS_PATH> <HOSTNAME> <CGROUP_CPU_PATH> <CGROUP_MEMORY_PATH>

       make debug
       ./MyContainer_debug.out "${v_NEW_DIR_NAME}" "hostname-${v_NEW_DIR_NAME}" "${CGROUP_CPU_NAME}" "${CGROUP_MEMORY_NAME}"
       ```
    
    7. Clean the binaries generated using: `make clean`


<!-- 
### Useful Commands
```sh

```
-->

### References
- https://ericchiang.github.io/post/containers-from-scratch/

