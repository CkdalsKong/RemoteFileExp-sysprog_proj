# **Remote File Explorer**



Remote File Explorer is a Linux-based file navigation program designed to help users easily manage local or remote file systems. It allows users to access and download files from remote servers without the need for complex remote file transfer tools, as well as providing functionality for local file systems. The program supports system administrators, developers, and users to streamline and quickly handle file management tasks.



## Getting Started

### Prerequisites

- System Requirements and Supported Operating Systems
  : macOS, Linux 20.04

- Necessary libraries
  : curses library 

  ```sh
  sudo apt-get update
  sudo apt-get install libncurses5-dev libncursesw5-dev
  ```

### Installation

1. **Clone the GitHub repository.**

   ```sh
   git clone https://github.com/CkdalsKong/RemoteFileExp-sysprog_proj.git
   ```

   

2. **Navigate to the project directory and Build the project.**

   with 'make' in termianl.

3. **Run the project.**

   ```sh
   ./RemoteFileExp
   ```



## **Usage**

### **1. Local Key Controls**

#### **Navigation**

- **Up & Down arrow**
  : move cursor up & down.
- **Enter**
  : Show files and directories in selected folder.
- **Backspace**
  : Back to previous directory.

#### **File Operations**

- **c** : Duplicate a file in current row. Aborts if the name exist or fails
- **d** : Delete file in the current row.
- **m** : Create new directory.

#### **Sorting**

- **s** : Sort in ascending order. (A to Z)
- **S** : Sort in descending order. (A to Z)

#### **Other**

- **h** : Opens manual page.
- **Q** : Exit the program.

### **2. Remote Key Controls**

#### **Navigation**

- **Up & Down arrow**
  : move cursor up & down.
- **Enter**
  : Show files and directories in selected folder.
- **Backspace**
  : Back to previous directory.

#### **File Operations**

- **d** : Download file from the remote system.

#### **Sorting**

- **s** : Sort in descending order.
- **S** : Sort in ascending order.

#### **Other**

- **Q** : Exit the program.



## ✉️ Contact and Links

#### Lee Chang-Min : lchm1106@icloud.com

#### Lee Sang-Jun : gumoning9010@naver.com

#### Geum Kyeong-Hun : kghun81@naver.com

