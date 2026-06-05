#include <stdio.h>

#define PRINT_LD_1(v1) printf("%s: %ld\n", #v1, (long)(v1));
#define PRINT_LD_2(v1, v2) PRINT_LD_1(v1) PRINT_LD_1(v2)
#define PRINT_LD_3(v1, v2, v3) PRINT_LD_2(v1, v2) PRINT_LD_1(v3)
#define PRINT_LD_4(v1, v2, v3, v4) PRINT_LD_3(v1, v2, v3) PRINT_LD_1(v4)
#define PRINT_LD_5(v1, v2, v3, v4, v5) PRINT_LD_4(v1, v2, v3, v4) PRINT_LD_1(v5)
