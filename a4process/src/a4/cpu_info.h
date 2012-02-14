// Based on code at
// http://stackoverflow.com/a/3082553
// "Programatically detect number of physical processors/cores or if 
//  hyper-threading is active on Windows, Mac and Linux"
// jcoffland's response dated 21 Jun 2010

#include <string>

struct cpuinfo_t {
    unsigned int physical_cores,
                 logical_cores;
    bool hyperthreading;
    std::string vendor;
};

void cpuid(unsigned i, unsigned regs[4]) {
#ifdef _WIN32
  __cpuid((int *)regs, (int)i);

#else
  asm volatile
    ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
     : "a" (i), "c" (0));
  // ECX is set to zero for cpuid function 4
#endif
}

struct cpuinfo_t get_cpuinfo() {
    struct cpuinfo_t cpuinfo;
    
    // Take content in registers and interpret them as a string
    union {
        char vendor[12];
        unsigned regs[4];
    };
    cpuid(0, regs);
    cpuinfo.vendor = std::string(vendor, 12);

    // Get CPU features
    cpuid(1, regs);
    unsigned cpuFeatures = regs[3]; // EDX

    // Logical core count per CPU
    cpuid(1, regs);
    cpuinfo.logical_cores = (regs[1] >> 16) & 0xff; // EBX[23:16]
    cpuinfo.physical_cores = cpuinfo.logical_cores;

    if (cpuinfo.vendor == "GenuineIntel") {
        // Get DCP cache info
        cpuid(4, regs);
        cpuinfo.physical_cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1
    } else if (cpuinfo.vendor == "AuthenticAMD") {
        // Get NC: Number of CPU cores - 1
        cpuid(0x80000008, regs);
        cpuinfo.physical_cores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
    } else {
        cpuinfo.physical_cores = 1;
        cpuinfo.logical_cores = 1;
        cpuinfo.hyperthreading = 0;
        return cpuinfo; // unknown processor
    }

    // Detect hyper-threads  
    cpuinfo.hyperthreading = cpuFeatures & (1 << 28) && 
                             cpuinfo.physical_cores < cpuinfo.logical_cores;
    return cpuinfo;
}