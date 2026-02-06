#include <cbuild.h>
#include <stdlib.h>
#include <wait.h>

int main(int argc, char **argv) {
    cbuild_recompile_myself(__FILE__,
                            argv,
                            "-Llib/cbuild",
                            "-lcbuild",
                            "-Ilib/cbuild/include",
                            NULL);

    cbuild_obj_t *least_o = cbuild_obj_create("least.c", "-Wall", "-Wextra", NULL);
    if (least_o == NULL)
        return 1;

    cbuild_target_t *least = cbuild_create_executable("least", least_o, NULL);
    if (least == NULL)
        return 1;

    pid_t cpid = cbuild_target_compile(least);
    if (cpid < 0)
        return 1;

    int ws;
    waitpid(cpid, &ws, 0);
    if (!WIFEXITED(ws))
        return 1;
    return WEXITSTATUS(ws);
}
