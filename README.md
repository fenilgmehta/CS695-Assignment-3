# CS695-Assignment-3

- Programming Assignment 3 - Build your own container using Linux namespaces and cgroups
- [Problem Statement PDF](./Problem%20Statement%20-%20Programming%20Assignment%203.pdf) ([https://www.cse.iitb.ac.in/~cs695/pa/pa3.html](https://www.cse.iitb.ac.in/~cs695/pa/pa3.html))
- [Demo Video](./CS695-Assignment-3%20-%20Demo%201%20-%202021-03-26%2014-47-00.mp4)

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

       mkdir "${v_NEW_DIR_NAME}_tmp"
       tar --verbose --gzip --extract --file "rootfs.tar.gz" -C "${v_NEW_DIR_NAME}_tmp"  # OR, tar -vzxf "rootfs.tar.gz"
       mv "${v_NEW_DIR_NAME}_tmp/rootfs" "${v_NEW_DIR_NAME}"
       rmdir "${v_NEW_DIR_NAME}_tmp"

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

       # --------------------

       # TESTING
       echo $$ > "/sys/fs/cgroup/cpu/${CGROUP_CPU_NAME}/cgroup.procs"
       echo $$ > "/sys/fs/cgroup/memory/${CGROUP_MEMORY_NAME}/cgroup.procs"

       # Testing of CPU, view the CPU usage via the System Monitor
       while true; do
         a=$((12345*12345))
       done
       while true; do a=$((12345*12345)); done &  # background process

       # Testing of Memory Limits, view the Memory usage via SystemMonitor
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

    6. Network Namespace creation and initialization (OPTIONAL)
       ```sh
       # NOTE: internet access MAY not work (NOT tested)
       # NOTE: if this step is not performed, then local client server program will not work
       NETWORK_NAMESPACE_NAME="cs695netns${v_NUM}"
       v_VETH0="cs695vethA${v_NUM}"
       v_VETH1="cs695vethB${v_NUM}"
       
       # Create a network namespace
       ip netns add "${NETWORK_NAMESPACE_NAME}"
       # Turn the "lo" interface up
       ip netns exec "${NETWORK_NAMESPACE_NAME}" ip link set dev lo up
       # Create virtual ethernet A and B
       ip link add "${v_VETH0}" type veth peer name "${v_VETH1}"
       # veth B to namespace
       ip link set "${v_VETH1}" netns "${NETWORK_NAMESPACE_NAME}"
       
       # Find NEW IP address based on ${v_NUM}
       v_VETH0_IP="$(python3 IPGenerator.py ${v_NUM} a)"
       v_VETH1_IP="$(python3 IPGenerator.py ${v_NUM} b)"

       # IMPORTANT: if below checks give error, then set any unused IP address manually using:
       # v_VETH0_IP="10.0.0.1/24"
       # v_VETH1_IP="10.0.0.2/24"
       if [[ $v_VETH0_IP == ERROR* ]]; then echo -e "WARNING: do NOT proceed\n${v_VETH0_IP}"; fi
       if [[ $v_VETH1_IP == ERROR* ]]; then echo -e "WARNING: do NOT proceed\n${v_VETH1_IP}"; fi

       ifconfig "${v_VETH0}" "${v_VETH0_IP}" up
       ip netns exec "${NETWORK_NAMESPACE_NAME}" ifconfig "${v_VETH1}" "${v_VETH1_IP}" up
       ```

    7. Compile and Execute the container shell
       ```sh
       # CGROUP_CPU_PATH is relative path to "/sys/fs/cgroup/cpu/"
       # CGROUP_MEMORY_PATH is relative path to "/sys/fs/cgroup/memory/"
       make all
       ./MyContainer.out <ROOT_FS_PATH> <HOSTNAME> \
           <CGROUP_CPU_PATH> <CGROUP_MEMORY_PATH> <NETWORK_NAMESPACE_NAME> <MOUNT_DIR_PATH>

       make debug
       ./MyContainer_debug.out \
           "${v_NEW_DIR_NAME}" \
           "hostname-${v_NEW_DIR_NAME}" \
           "${CGROUP_CPU_NAME}"         \
           "${CGROUP_MEMORY_NAME}"      \
           "${NETWORK_NAMESPACE_NAME}"     \
           "custom_mount_folder"
       ```
    
    7. Clean the binaries generated
       ```sh
       make clean
       ```


### References
- https://ericchiang.github.io/post/containers-from-scratch/
- https://lwn.net/Articles/580893/ (Network namespaces)
- https://linuxize.com/post/bash-increment-decrement-variable/
- https://unix.stackexchange.com/questions/11018/how-to-choose-directory-name-during-untarring
- https://unix.stackexchange.com/questions/327226/must-dev-random-and-dev-urandom-be-created-each-boot-or-are-they-static-files
- https://stackoverflow.com/questions/2172352/in-bash-how-can-i-check-if-a-string-begins-with-some-value
- https://developers.redhat.com/blog/2018/10/22/introduction-to-linux-interfaces-for-virtual-networking/
- https://www.redhat.com/sysadmin/7-linux-namespaces

