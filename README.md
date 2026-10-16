# \# Bare-Metal STM32 Memory Allocator

# 

# A custom memory allocator implemented in C++ for the STM32F103C8T6 microcontroller.

# 

# This project explores dynamic memory management on a resource-constrained embedded system. Instead of relying on the standard `malloc()` and `free()` functions, it manages allocations within a fixed-size memory pool with support for block reuse, splitting, and coalescing.

# 

# The STM32 linker script was modified to reserve a dedicated region of SRAM for the allocator, allowing explicit control over memory placement. The implementation runs on bare-metal hardware and can be inspected using STM32CubeIDE and an ST-Link debugger.

# 

# \## Features

# 

# \- \*\*Custom allocation and deallocation:\*\* Implements `my\_malloc()` and `my\_free()` without using the standard heap allocator.

# \- \*\*First-fit allocation:\*\* Searches existing free blocks before allocating additional space.

# \- \*\*Block splitting:\*\* Divides larger free blocks when enough space remains for another block.

# \- \*\*Block coalescing:\*\* Merges adjacent free blocks to reduce external fragmentation.

# \- \*\*8-byte alignment:\*\* Rounds allocation sizes to 8-byte boundaries.

# \- \*\*Memory tracking:\*\* Maintains information about allocated and available memory.

# \- \*\*Linker-controlled memory placement:\*\* Reserves a dedicated SRAM region for the allocator through a modified linker script.

# 

# \## Hardware and Development Environment

# 

# | Component | Description |

# |---|---|

# | Microcontroller | STM32F103C8T6 |

# | Development board | STM32 Blue Pill |

# | Programmer / Debugger | ST-Link V2 |

# | IDE | STM32CubeIDE |

# | Programming language | C++ |

# | Memory pool | 1024 bytes |

# | Debugging interface | SWD |

# 

# \## Allocator Design

# 

# The allocator operates on a fixed-size memory pool. Each block contains metadata describing its size and allocation status.

# 

# Block metadata is stored alongside the allocated memory, allowing the allocator to locate free blocks, split larger blocks, and merge adjacent free blocks.

# 

# \### Memory Allocation

# 

# When `my\_malloc()` receives a request:

# 

# 1\. The requested allocation size is aligned to an 8-byte boundary.

# 2\. The allocator searches existing free blocks using a first-fit strategy.

# 3\. If a suitable free block is found, it is reused and split when sufficient space remains.

# 4\. Otherwise, the allocator attempts to reserve memory from the unused portion of the pool.

# 5\. If the request cannot be satisfied, the function returns `nullptr`.

# 

# \### Memory Deallocation

# 

# When `my\_free()` releases an allocated block:

# 

# 1\. The block is marked as free.

# 2\. Adjacent free blocks are merged where possible.

# 3\. The released memory becomes available for subsequent allocations.

# 

# This allows the allocator to reuse memory without requesting additional storage from the system.

# 

# \## Linker Script and Memory Placement

# 

# The project uses a modified STM32 linker script, `STM32F103C8TX\_FLASH.ld`, to reserve a dedicated region of SRAM for the allocator.

# 

# The linker script controls the placement of the memory pool at build time, while the allocator manages individual blocks within that region at runtime.

# 

# This provides explicit control over the allocator's storage rather than relying on the default placement of an ordinary static array.

# 

# \### Memory Layout

# 

# The allocator manages a 1024-byte memory pool.

# 

# ```text

# &#x20;              STM32 SRAM

# +------------------------------------------+

# |                                          |

# |          Application Memory              |

# |                                          |

# +------------------------------------------+

# |                                          |

# |       Dedicated Allocator Region         |

# |               (1024 B)                   |

# |                                          |

# |  +------------------------------------+  |

# |  | Block Header                       |  |

# |  +------------------------------------+  |

# |  | Allocated Payload                  |  |

# |  +------------------------------------+  |

# |  | Block Header                       |  |

# |  +------------------------------------+  |

# |  | Free Payload                       |  |

# |  +------------------------------------+  |

# |  | Remaining Unallocated Space        |  |

# |  +------------------------------------+  |

# |                                          |

# +------------------------------------------+

# ```

# 

# Each block header contains information required to manage the corresponding allocation. Header storage and alignment padding contribute to the total memory consumed within the pool.

# 

# The allocator does not use the standard C library heap for its managed allocations.

# 

# \## Debugger Demonstration

# 

# The allocator's runtime behavior is demonstrated on the STM32F103C8T6 using STM32CubeIDE and an ST-Link V2 debugger.

# 

# The demonstration shows allocation, deallocation, block reuse, splitting, and coalescing through direct inspection of the memory pool and block metadata.

# 

# !\[STM32 Memory Allocator Demonstration](docs/assets/allocator-demo.gif)

# 

# \## Building and Running

# 

# \### Requirements

# 

# \- STM32F103C8T6 Blue Pill development board

# \- ST-Link V2 programmer/debugger

# \- STM32CubeIDE

# \- SWD connection between the debugger and microcontroller

# 

# \### Instructions

# 

# 1\. Clone the repository:

# 

# &#x20;  ```bash

# &#x20;  git clone https://github.com/Koushik-65/Bare-Metal-STM32-Memory-Allocator.git

# &#x20;  ```

# 

# 2\. Open STM32CubeIDE.

# 3\. Import the repository as an existing STM32CubeIDE project.

# 4\. Build the firmware using the included project configuration and linker script.

# 5\. Connect the ST-Link V2 debugger to the STM32 Blue Pill using SWD.

# 6\. Flash the firmware and start a debugging session.

# 7\. Use the debugger's memory and expressions views to inspect the allocator's internal state.

# 

# No UART connection or external display is required.

# 

# \## Repository Structure

# 

# ```text

# Bare-Metal-STM32-Memory-Allocator/

# ├── Src/

# │   ├── main.cpp

# │   ├── syscalls.c

# │   └── sysmem.c

# ├── Startup/

# │   └── startup\_stm32f103c8tx.s

# ├── STM32F103C8TX\_FLASH.ld

# ├── .project

# ├── .cproject

# ├── .gitignore

# └── README.md

# ```

# 

# | File | Purpose |

# |---|---|

# | `Src/main.cpp` | Memory allocator implementation and test code |

# | `Src/syscalls.c` | System call support |

# | `Src/sysmem.c` | Runtime memory support |

# | `Startup/startup\_stm32f103c8tx.s` | Microcontroller startup assembly |

# | `STM32F103C8TX\_FLASH.ld` | Linker configuration and SRAM placement |

# | `.project` / `.cproject` | STM32CubeIDE project configuration |

# 

# \## Project Status

# 

# \*\*Implementation complete.\*\*

# 

# The allocator implements custom memory allocation and deallocation, first-fit block selection, block splitting, block coalescing, and memory usage tracking.

# 

# The project also incorporates linker-controlled SRAM placement and on-target debugging through ST-Link.

