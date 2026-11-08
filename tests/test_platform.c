#include <string.h>

#include "snake_puzzle/platform.h"
#include "snake_puzzle/common.h"
#include "test.h"

int test_platform_run(void)
{
    char output[SP_ATOM_PATH];

    SP_CHECK(sp_resolve_atoms_path(SP_DEFAULT_ATOMS_DIR, true, output, sizeof(output)));
    SP_CHECK(strcmp(output, SP_DEFAULT_ATOMS_DIR) == 0);
    SP_CHECK(sp_executable_dir(output, sizeof(output)));
    SP_CHECK(output[0] != '\0');
    return 0;
}
