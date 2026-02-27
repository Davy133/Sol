<p align="center">
  <img width="200" height="200" alt="Sol operating system symbol" src="https://github.com/user-attachments/assets/33f96a9a-5ac4-4267-979b-c8354acfe7cb"/>
</p>

<h1 align="center">Sol</h1>

<p align="center">
  <i>A personal project used to learn more about operating systems<br>
  and how computers work at a low level.</i>
</p>

<p align="center">
  <img src="https://img.shields.io/github/license/Davy133/Sol?style=for-the-badge" alt="License"/>
</p>

---

### How to Run

To build and run Sol you will need:

- **NASM**
- **Emulator**: QEMU or Bochs
- **Make**
- **Cross-compiler** — see [OSDev GCC Cross-Compiler guide](https://wiki.osdev.org/GCC_Cross-Compiler)

```bash
# 1. Clone the repository
git clone https://github.com/Davy133/Sol.git

# 2. Enter the directory
cd Sol

# 3. Build the bootable OS image
make os-image
